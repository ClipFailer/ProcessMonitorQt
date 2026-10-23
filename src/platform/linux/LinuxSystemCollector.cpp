#include "LinuxSystemCollector.hpp"

#if defined(__linux__)

#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <charconv>
#include <algorithm>
#include <thread>
#include <unordered_set>

namespace pm::platform::linux_os {

LinuxSystemCollector::LinuxSystemCollector() {
    m_clockTicksPerSec = sysconf(_SC_CLK_TCK);
    if (m_clockTicksPerSec <= 0) m_clockTicksPerSec = 100;

    m_pageSizeBytes = sysconf(_SC_PAGESIZE);
    if (m_pageSizeBytes <= 0) m_pageSizeBytes = 4096;

    m_logicalCores = std::max(1u, std::thread::hardware_concurrency());
    m_prevSampleTime = std::chrono::steady_clock::now();
    m_prevNetTime = m_prevSampleTime;
}

bool LinuxSystemCollector::isRunningAsAdministrator() const noexcept {
    return geteuid() == 0;
}

core::SystemSnapshot LinuxSystemCollector::collectSystemSnapshot() {
    core::SystemSnapshot snapshot;
    snapshot.logicalCoresCount = m_logicalCores;
    snapshot.timestamp = std::chrono::system_clock::now();

    struct sysinfo si{};
    if (sysinfo(&si) == 0) {
        snapshot.uptimeSeconds = static_cast<uint64_t>(si.uptime);
        snapshot.totalPhysicalRam = static_cast<uint64_t>(si.totalram) * si.mem_unit;
        snapshot.freePhysicalRam = static_cast<uint64_t>(si.freeram) * si.mem_unit;
        snapshot.usedPhysicalRam = snapshot.totalPhysicalRam - snapshot.freePhysicalRam;
        snapshot.totalPagefile = static_cast<uint64_t>(si.totalswap) * si.mem_unit;
        snapshot.usedPagefile = (static_cast<uint64_t>(si.totalswap) - static_cast<uint64_t>(si.freeswap)) * si.mem_unit;
        snapshot.processCount = static_cast<uint32_t>(si.procs);
    }

    // Чтение /proc/stat для расчета суммарного CPU
    std::ifstream statFile("/proc/stat");
    if (statFile.is_open()) {
        std::string line;
        if (std::getline(statFile, line) && line.rfind("cpu ", 0) == 0) {
            std::istringstream iss(line.substr(4));
            uint64_t user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
            iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

            const uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
            const uint64_t totalIdle = idle + iowait;

            if (m_prevTotalCpuTime > 0 && total > m_prevTotalCpuTime) {
                const uint64_t deltaTotal = total - m_prevTotalCpuTime;
                const uint64_t deltaIdle = (totalIdle >= m_prevIdleCpuTime) ? (totalIdle - m_prevIdleCpuTime) : 0;
                const double busy = static_cast<double>(deltaTotal - deltaIdle);

                snapshot.totalCpuPercent = std::clamp((busy * 100.0) / static_cast<double>(deltaTotal), 0.0, 100.0);
                snapshot.idleCpuPercent = std::clamp((static_cast<double>(deltaIdle) * 100.0) / static_cast<double>(deltaTotal), 0.0, 100.0);
            }

            m_prevTotalCpuTime = total;
            m_prevIdleCpuTime = totalIdle;
        }
    }

    return snapshot;
}

std::vector<core::ProcessSnapshot> LinuxSystemCollector::collectProcesses() {
    std::vector<core::ProcessSnapshot> processes;
    DIR* procDir = opendir("/proc");
    if (!procDir) return processes;

    struct dirent* entry = nullptr;
    std::unordered_set<core::ProcessId> activePids;

    while ((entry = readdir(procDir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;
        const char* name = entry->d_name;
        if (*name < '0' || *name > '9') continue;

        core::ProcessId pid = 0;
        const auto [ptr, ec] = std::from_chars(name, name + strlen(name), pid);
        if (ec != std::errc() || pid == 0) continue;

        activePids.insert(pid);

        // Чтение /proc/[pid]/stat
        const std::string statPath = std::string("/proc/") + name + "/stat";
        std::ifstream statStream(statPath);
        if (!statStream.is_open()) continue;

        std::string statContent;
        std::getline(statStream, statContent);
        if (statContent.empty()) continue;

        const auto openParen = statContent.find('(');
        const auto closeParen = statContent.rfind(')');
        if (openParen == std::string::npos || closeParen == std::string::npos) continue;

        core::ProcessSnapshot proc;
        proc.pid = pid;
        proc.name = statContent.substr(openParen + 1, closeParen - openParen - 1);

        std::istringstream rest(statContent.substr(closeParen + 2));
        char stateChar = 'R';
        int ppid = 0, pgrp = 0, session = 0, tty = 0, tpgid = 0;
        unsigned int flags = 0;
        uint64_t minflt = 0, cminflt = 0, majflt = 0, cmajflt = 0;
        uint64_t utime = 0, stime = 0;
        int64_t cutime = 0, cstime = 0, priority = 0, nice = 0, numThreads = 1;
        uint64_t itrealvalue = 0, starttime = 0, vsize = 0;
        int64_t rss = 0;

        rest >> stateChar >> ppid >> pgrp >> session >> tty >> tpgid >> flags
             >> minflt >> cminflt >> majflt >> cmajflt
             >> utime >> stime >> cutime >> cstime
             >> priority >> nice >> numThreads >> itrealvalue >> starttime >> vsize >> rss;

        proc.parentPid = static_cast<core::ProcessId>(ppid);
        proc.threadCount = static_cast<uint32_t>(numThreads);
        proc.workingSetBytes = static_cast<uint64_t>(std::max(int64_t(0), rss)) * static_cast<uint64_t>(m_pageSizeBytes);
        proc.privateBytes = vsize;

        // CPU дельта
        auto itPrev = m_processTimes.find(pid);
        if (itPrev != m_processTimes.end()) {
            const uint64_t deltaU = utime >= itPrev->second.utime ? (utime - itPrev->second.utime) : 0;
            const uint64_t deltaS = stime >= itPrev->second.stime ? (stime - itPrev->second.stime) : 0;
            const double totalDeltaTicks = static_cast<double>(deltaU + deltaS);
            proc.cpuUsagePercent = std::clamp((totalDeltaTicks * 100.0) / (static_cast<double>(m_clockTicksPerSec) * static_cast<double>(m_logicalCores)), 0.0, 100.0);
            itPrev->second = {utime, stime};
        } else {
            m_processTimes[pid] = {utime, stime};
            proc.lifecycleState = core::ProcessLifecycleState::New;
        }

        // Чтение exe link
        char exeBuf[PATH_MAX]{};
        const std::string exePath = std::string("/proc/") + name + "/exe";
        const ssize_t len = readlink(exePath.c_str(), exeBuf, sizeof(exeBuf) - 1);
        if (len > 0) {
            exeBuf[len] = '\0';
            proc.exePath = exeBuf;
        } else {
            proc.exePath = proc.name;
        }

        processes.push_back(std::move(proc));
    }

    closedir(procDir);

    std::erase_if(m_processTimes, [&activePids](const auto& pair) {
        return !activePids.contains(pair.first);
    });

    return processes;
}

std::vector<core::ThreadSnapshot> LinuxSystemCollector::collectProcessThreads(core::ProcessId /*pid*/) {
    return {};
}

std::vector<core::ModuleSnapshot> LinuxSystemCollector::collectProcessModules(core::ProcessId /*pid*/) {
    return {};
}

std::vector<core::HandleSnapshot> LinuxSystemCollector::collectProcessHandles(core::ProcessId /*pid*/) {
    return {};
}

bool LinuxSystemCollector::terminateProcess(core::ProcessId pid, int /*exitCode*/) {
    return kill(static_cast<pid_t>(pid), SIGKILL) == 0;
}

bool LinuxSystemCollector::suspendProcess(core::ProcessId pid) {
    return kill(static_cast<pid_t>(pid), SIGSTOP) == 0;
}

bool LinuxSystemCollector::resumeProcess(core::ProcessId pid) {
    return kill(static_cast<pid_t>(pid), SIGCONT) == 0;
}

bool LinuxSystemCollector::setProcessPriority(core::ProcessId pid, core::ProcessPriority priority) {
    int niceVal = 0;
    switch (priority) {
        case core::ProcessPriority::Idle: niceVal = 19; break;
        case core::ProcessPriority::BelowNormal: niceVal = 10; break;
        case core::ProcessPriority::Normal: niceVal = 0; break;
        case core::ProcessPriority::AboveNormal: niceVal = -5; break;
        case core::ProcessPriority::High: niceVal = -15; break;
        case core::ProcessPriority::RealTime: niceVal = -20; break;
        default: niceVal = 0; break;
    }
    return setpriority(PRIO_PROCESS, static_cast<id_t>(pid), niceVal) == 0;
}

} // namespace pm::platform::linux_os

#endif // __linux__
