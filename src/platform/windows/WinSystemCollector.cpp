#include "WinSystemCollector.hpp"

#ifdef _WIN32

#include <psapi.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <algorithm>
#include <thread>
#include <unordered_set>

namespace pm::platform::win {

namespace {

// Неэкспортируемый класс информации об аргументах в NTAPI
constexpr ULONG ProcessCommandLineInformationClass = 60;

typedef struct _UNICODE_STRING_DATA {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING_DATA;

using NtQueryInformationProcessFn = NTSTATUS(NTAPI*)(
    HANDLE ProcessHandle,
    ULONG ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
);

} // namespace

WinSystemCollector::WinSystemCollector() {
    m_logicalCores = std::max(1u, std::thread::hardware_concurrency());
    m_isAdmin = isUserAdmin();

    // Активируем привилегии отладки для доступа к процессам других пользователей и служб
    enableDebugPrivilege();

    initNtApis();

    // Инициализация базовых системных тиков
    FILETIME idleTime{}, kernelTime{}, userTime{};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        m_prevSysIdle = fileTimeToUint64(idleTime);
        m_prevSysKernel = fileTimeToUint64(kernelTime);
        m_prevSysUser = fileTimeToUint64(userTime);
    }
    m_prevSysSampleTime = std::chrono::steady_clock::now();
    m_prevNetSampleTime = m_prevSysSampleTime;
}

void WinSystemCollector::initNtApis() noexcept {
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        // Загрузка недокументированных вызовов заморозки/возобновления
        // Подавление предупреждения -Wcast-function-type при приведении FARPROC
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
        m_pfnNtSuspendProcess = reinterpret_cast<NtSuspendProcessFn>(
            GetProcAddress(hNtdll, "NtSuspendProcess")
        );
        m_pfnNtResumeProcess = reinterpret_cast<NtResumeProcessFn>(
            GetProcAddress(hNtdll, "NtResumeProcess")
        );
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    }
}

bool WinSystemCollector::isRunningAsAdministrator() const noexcept {
    return m_isAdmin;
}

std::string WinSystemCollector::readCommandLine(HANDLE hProcess) const {
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) {
        return {};
    }

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
    auto pfnQuery = reinterpret_cast<NtQueryInformationProcessFn>(
        GetProcAddress(hNtdll, "NtQueryInformationProcess")
    );
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    if (!pfnQuery) {
        return {};
    }

    // Буфер для структуры UNICODE_STRING и данных строки
    std::vector<BYTE> buffer(4096);
    ULONG returnLength = 0;

    NTSTATUS status = pfnQuery(
        hProcess,
        ProcessCommandLineInformationClass,
        buffer.data(),
        static_cast<ULONG>(buffer.size()),
        &returnLength
    );

    // Если размер больше начального буфера, перевыделяем
    if (status == static_cast<NTSTATUS>(0xC0000004L) /* STATUS_INFO_LENGTH_MISMATCH */ && returnLength > buffer.size()) {
        buffer.resize(returnLength);
        status = pfnQuery(
            hProcess,
            ProcessCommandLineInformationClass,
            buffer.data(),
            static_cast<ULONG>(buffer.size()),
            &returnLength
        );
    }

    if (status >= 0 && returnLength >= sizeof(UNICODE_STRING_DATA)) {
        auto* ustr = reinterpret_cast<UNICODE_STRING_DATA*>(buffer.data());
        if (ustr->Buffer && ustr->Length > 0) {
            std::wstring cmdW(ustr->Buffer, ustr->Length / sizeof(WCHAR));
            return wideToUtf8(cmdW);
        }
    }

    return {};
}

core::ProcessPriority WinSystemCollector::toProcessPriority(DWORD winPriority) noexcept {
    switch (winPriority) {
        case IDLE_PRIORITY_CLASS:         return core::ProcessPriority::Idle;
        case BELOW_NORMAL_PRIORITY_CLASS: return core::ProcessPriority::BelowNormal;
        case NORMAL_PRIORITY_CLASS:       return core::ProcessPriority::Normal;
        case ABOVE_NORMAL_PRIORITY_CLASS: return core::ProcessPriority::AboveNormal;
        case HIGH_PRIORITY_CLASS:         return core::ProcessPriority::High;
        case REALTIME_PRIORITY_CLASS:     return core::ProcessPriority::RealTime;
        default:                          return core::ProcessPriority::Unknown;
    }
}

DWORD WinSystemCollector::fromProcessPriority(core::ProcessPriority priority) noexcept {
    switch (priority) {
        case core::ProcessPriority::Idle:        return IDLE_PRIORITY_CLASS;
        case core::ProcessPriority::BelowNormal: return BELOW_NORMAL_PRIORITY_CLASS;
        case core::ProcessPriority::Normal:      return NORMAL_PRIORITY_CLASS;
        case core::ProcessPriority::AboveNormal: return ABOVE_NORMAL_PRIORITY_CLASS;
        case core::ProcessPriority::High:        return HIGH_PRIORITY_CLASS;
        case core::ProcessPriority::RealTime:    return REALTIME_PRIORITY_CLASS;
        default:                                 return NORMAL_PRIORITY_CLASS;
    }
}

core::SystemSnapshot WinSystemCollector::collectSystemSnapshot() {
    core::SystemSnapshot snapshot;
    snapshot.logicalCoresCount = m_logicalCores;
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.uptimeSeconds = GetTickCount64() / 1000;

    const auto now = std::chrono::steady_clock::now();

    // 1. Расчет общесистемной нагрузки CPU
    FILETIME ftIdle{}, ftKernel{}, ftUser{};
    if (GetSystemTimes(&ftIdle, &ftKernel, &ftUser)) {
        const uint64_t currIdle = fileTimeToUint64(ftIdle);
        const uint64_t currKernel = fileTimeToUint64(ftKernel);
        const uint64_t currUser = fileTimeToUint64(ftUser);

        const uint64_t deltaIdle = currIdle - m_prevSysIdle;
        const uint64_t deltaKernelTotal = currKernel - m_prevSysKernel;
        const uint64_t deltaUser = currUser - m_prevSysUser;

        // В Windows GetSystemTimes включает idleTime внутри kernelTime!
        const uint64_t deltaKernelPure = (deltaKernelTotal >= deltaIdle) ? (deltaKernelTotal - deltaIdle) : 0;
        const uint64_t deltaTotal = deltaKernelTotal + deltaUser;

        if (deltaTotal > 0) {
            snapshot.totalCpuPercent = std::clamp(
                static_cast<double>(deltaTotal - deltaIdle) * 100.0 / static_cast<double>(deltaTotal),
                0.0, 100.0
            );
            snapshot.userCpuPercent = std::clamp(
                static_cast<double>(deltaUser) * 100.0 / static_cast<double>(deltaTotal),
                0.0, 100.0
            );
            snapshot.kernelCpuPercent = std::clamp(
                static_cast<double>(deltaKernelPure) * 100.0 / static_cast<double>(deltaTotal),
                0.0, 100.0
            );
            snapshot.idleCpuPercent = std::clamp(
                static_cast<double>(deltaIdle) * 100.0 / static_cast<double>(deltaTotal),
                0.0, 100.0
            );
        }

        m_prevSysIdle = currIdle;
        m_prevSysKernel = currKernel;
        m_prevSysUser = currUser;
        m_prevSysSampleTime = now;
    }

    // 2. Использование оперативной памяти (RAM) и файла подкачки
    MEMORYSTATUSEX memStatus{};
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        snapshot.totalPhysicalRam = memStatus.ullTotalPhys;
        snapshot.freePhysicalRam = memStatus.ullAvailPhys;
        snapshot.usedPhysicalRam = memStatus.ullTotalPhys - memStatus.ullAvailPhys;
        snapshot.totalPagefile = memStatus.ullTotalPageFile;
        snapshot.usedPagefile = memStatus.ullTotalPageFile - memStatus.ullAvailPageFile;
    }

    // 3. Сетевой трафик через GetIfTable
    ULONG ifTableSize = 0;
    if (GetIfTable(nullptr, &ifTableSize, FALSE) == ERROR_INSUFFICIENT_BUFFER && ifTableSize > 0) {
        std::vector<BYTE> ifBuffer(ifTableSize);
        auto* pIfTable = reinterpret_cast<MIB_IFTABLE*>(ifBuffer.data());

        if (GetIfTable(pIfTable, &ifTableSize, FALSE) == NO_ERROR) {
            uint64_t totalRx = 0;
            uint64_t totalTx = 0;

            for (DWORD i = 0; i < pIfTable->dwNumEntries; ++i) {
                const auto& row = pIfTable->table[i];
                if (row.dwType != MIB_IF_TYPE_LOOPBACK) {
                    totalRx += row.dwInOctets;
                    totalTx += row.dwOutOctets;
                }
            }

            const auto elapsedSec = std::chrono::duration<double>(now - m_prevNetSampleTime).count();
            if (elapsedSec > 0.05 && m_prevNetRxBytes > 0) {
                if (totalRx >= m_prevNetRxBytes) {
                    snapshot.netRxBytesPerSec = static_cast<uint64_t>(
                        static_cast<double>(totalRx - m_prevNetRxBytes) / elapsedSec
                    );
                }
                if (totalTx >= m_prevNetTxBytes) {
                    snapshot.netTxBytesPerSec = static_cast<uint64_t>(
                        static_cast<double>(totalTx - m_prevNetTxBytes) / elapsedSec
                    );
                }
            }

            m_prevNetRxBytes = totalRx;
            m_prevNetTxBytes = totalTx;
            m_prevNetSampleTime = now;
        }
    }

    // Общая статистика процессов/потоков
    PERFORMANCE_INFORMATION perfInfo{};
    perfInfo.cb = sizeof(perfInfo);
    if (GetPerformanceInfo(&perfInfo, sizeof(perfInfo))) {
        snapshot.processCount = static_cast<uint32_t>(perfInfo.ProcessCount);
        snapshot.threadCount = static_cast<uint32_t>(perfInfo.ThreadCount);
        snapshot.handleCount = static_cast<uint32_t>(perfInfo.HandleCount);
    }

    return snapshot;
}

std::vector<core::ProcessSnapshot> WinSystemCollector::collectProcesses() {
    std::vector<core::ProcessSnapshot> processes;
    processes.reserve(512);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    std::unordered_set<core::ProcessId> activePids;
    activePids.reserve(512);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            const core::ProcessId pid = pe.th32ProcessID;
            activePids.insert(pid);

            core::ProcessSnapshot proc;
            proc.pid = pid;
            proc.parentPid = pe.th32ParentProcessID;
            proc.threadCount = pe.cntThreads;
            proc.name = wideToUtf8(pe.szExeFile);

            // Обработка специального процесса System Idle Process (PID 0)
            if (pid == 0) {
                proc.name = "System Idle Process";
                proc.is64Bit = true;
                processes.push_back(std::move(proc));
                continue;
            }

            // Пытаемся открыть дескриптор процесса с максимальными доступными правами
            HANDLE hProcess = OpenProcess(
                PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                FALSE,
                pid
            );
            if (!hProcess) {
                hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            }

            if (hProcess) {
                // 1. Полный путь к исполняемому файлу
                WCHAR pathBuf[MAX_PATH * 2]{};
                DWORD pathLen = static_cast<DWORD>(std::size(pathBuf));
                if (QueryFullProcessImageNameW(hProcess, 0, pathBuf, &pathLen)) {
                    proc.exePath = wideToUtf8(pathBuf);
                } else {
                    proc.exePath = proc.name;
                }

                // 2. Определение разрядности (32/64 bit)
                BOOL isWow64 = FALSE;
                if (IsWow64Process(hProcess, &isWow64)) {
                    proc.is64Bit = (isWow64 == FALSE);
                }

                // 3. Расчет дельты нагрузки процессора (GetProcessTimes)
                FILETIME ftCreation{}, ftExit{}, ftKernel{}, ftUser{};
                if (GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
                    const uint64_t procKernel100ns = fileTimeToUint64(ftKernel);
                    const uint64_t procUser100ns = fileTimeToUint64(ftUser);

                    proc.userTimeMs = procUser100ns / 10000;
                    proc.kernelTimeMs = procKernel100ns / 10000;
                    proc.creationTimestamp = static_cast<int64_t>(
                        (fileTimeToUint64(ftCreation) - 116444736000000000ULL) / 10000
                    );

                    auto itPrev = m_processTimes.find(pid);
                    if (itPrev != m_processTimes.end()) {
                        const uint64_t deltaKernel = procKernel100ns >= itPrev->second.kernelTime
                            ? (procKernel100ns - itPrev->second.kernelTime) : 0;
                        const uint64_t deltaUser = procUser100ns >= itPrev->second.userTime
                            ? (procUser100ns - itPrev->second.userTime) : 0;
                        const uint64_t deltaProcTotal100ns = deltaKernel + deltaUser;

                        // Дельта общего времени системы за 1 такт в 100-нс интервалах
                        const uint64_t deltaSysTotal = (m_prevSysKernel + m_prevSysUser);
                        if (deltaSysTotal > 0 && m_logicalCores > 0) {
                            // Нормализация дельты на общее время и число логических ядер
                            const double rawPercent = (static_cast<double>(deltaProcTotal100ns) * 100.0) /
                                (static_cast<double>(10000000ULL) * static_cast<double>(m_logicalCores));
                            proc.cpuUsagePercent = std::clamp(rawPercent, 0.0, 100.0);
                        }

                        itPrev->second = {procKernel100ns, procUser100ns};
                        proc.lifecycleState = core::ProcessLifecycleState::Active;
                    } else {
                        // Новый обнаруженный процесс
                        m_processTimes[pid] = {procKernel100ns, procUser100ns};
                        proc.lifecycleState = core::ProcessLifecycleState::New;
                        m_newProcessCounters[pid] = 3; // Подсветка зеленым на 3 такта
                    }
                }

                // 4. Потребление памяти (Working Set / Private Commit)
                PROCESS_MEMORY_COUNTERS_EX pmc{};
                if (GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
                    proc.workingSetBytes = pmc.WorkingSetSize;
                    proc.peakWorkingSetBytes = pmc.PeakWorkingSetSize;
                    proc.privateBytes = pmc.PrivateUsage;
                }

                // 5. Дисковый ввод-вывод (I/O Counters)
                IO_COUNTERS ioCounters{};
                if (GetProcessIoCounters(hProcess, &ioCounters)) {
                    proc.readBytesTotal = ioCounters.ReadTransferCount;
                    proc.writeBytesTotal = ioCounters.WriteTransferCount;

                    auto itIo = m_processIo.find(pid);
                    if (itIo != m_processIo.end()) {
                        if (ioCounters.ReadTransferCount >= itIo->second.readBytes) {
                            proc.readBytesPerSec = ioCounters.ReadTransferCount - itIo->second.readBytes;
                        }
                        if (ioCounters.WriteTransferCount >= itIo->second.writeBytes) {
                            proc.writeBytesPerSec = ioCounters.WriteTransferCount - itIo->second.writeBytes;
                        }
                        itIo->second = {ioCounters.ReadTransferCount, ioCounters.WriteTransferCount};
                    } else {
                        m_processIo[pid] = {ioCounters.ReadTransferCount, ioCounters.WriteTransferCount};
                    }
                }

                // 6. Количество открытых дескрипторов
                DWORD handleCount = 0;
                if (GetProcessHandleCount(hProcess, &handleCount)) {
                    proc.handleCount = handleCount;
                }

                // 7. Базовый приоритет
                const DWORD priorityClass = GetPriorityClass(hProcess);
                proc.priority = toProcessPriority(priorityClass);

                // 8. Аргументы командной строки
                proc.commandLine = readCommandLine(hProcess);
                if (proc.commandLine.empty()) {
                    proc.commandLine = proc.exePath;
                }

                CloseHandle(hProcess);
            } else {
                // Если не удалось открыть дескриптор (защищенный процесс или отказано в доступе)
                proc.exePath = proc.name;
                proc.commandLine = proc.name;
            }

            // Обработка счетчика анимации подсветки новых процессов
            auto itNew = m_newProcessCounters.find(pid);
            if (itNew != m_newProcessCounters.end()) {
                proc.lifecycleState = core::ProcessLifecycleState::New;
                proc.lifecycleCounter = itNew->second;
                if (itNew->second > 0) {
                    itNew->second--;
                } else {
                    m_newProcessCounters.erase(itNew);
                }
            }

            processes.push_back(std::move(proc));
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);

    // Удаляем из кэшей завершившиеся процессы
    std::erase_if(m_processTimes, [&activePids](const auto& pair) {
        return !activePids.contains(pair.first);
    });
    std::erase_if(m_processIo, [&activePids](const auto& pair) {
        return !activePids.contains(pair.first);
    });
    std::erase_if(m_newProcessCounters, [&activePids](const auto& pair) {
        return !activePids.contains(pair.first);
    });

    return processes;
}

std::vector<core::ThreadSnapshot> WinSystemCollector::collectProcessThreads(core::ProcessId pid) {
    std::vector<core::ThreadSnapshot> threads;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return threads;
    }

    THREADENTRY32 te{};
    te.dwSize = sizeof(te);

    if (Thread32First(hSnapshot, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) {
                core::ThreadSnapshot ts;
                ts.tid = te.th32ThreadID;
                ts.priority = te.tpBasePri;
                ts.state = "Active";

                HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, te.th32ThreadID);
                if (hThread) {
                    FILETIME ftCreate{}, ftExit{}, ftKernel{}, ftUser{};
                    if (GetThreadTimes(hThread, &ftCreate, &ftExit, &ftKernel, &ftUser)) {
                        ts.kernelTimeMs = fileTimeToUint64(ftKernel) / 10000;
                        ts.userTimeMs = fileTimeToUint64(ftUser) / 10000;
                    }
                    CloseHandle(hThread);
                }

                threads.push_back(std::move(ts));
            }
        } while (Thread32Next(hSnapshot, &te));
    }

    CloseHandle(hSnapshot);
    return threads;
}

std::vector<core::ModuleSnapshot> WinSystemCollector::collectProcessModules(core::ProcessId pid) {
    std::vector<core::ModuleSnapshot> modules;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
        pid
    );
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return modules;
    }

    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);

    if (Module32FirstW(hSnapshot, &me)) {
        do {
            core::ModuleSnapshot mod;
            mod.baseAddress = reinterpret_cast<uint64_t>(me.modBaseAddr);
            mod.sizeBytes = me.modBaseSize;
            mod.name = wideToUtf8(me.szModule);
            mod.path = wideToUtf8(me.szExePath);
            modules.push_back(std::move(mod));
        } while (Module32NextW(hSnapshot, &me));
    }

    CloseHandle(hSnapshot);
    return modules;
}

std::vector<core::HandleSnapshot> WinSystemCollector::collectProcessHandles(core::ProcessId /*pid*/) {
    // Получение полного списка именованных объектов требует NtQuerySystemInformation(SystemExtendedHandleInformation)
    // и дублирования дескрипторов (DuplicateHandle). Возвращаем пустой вектор как безопасный фоллбэк при отсутствии спецправ.
    return {};
}

bool WinSystemCollector::terminateProcess(core::ProcessId pid, int exitCode) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) {
        return false;
    }
    const BOOL res = TerminateProcess(hProcess, static_cast<UINT>(exitCode));
    CloseHandle(hProcess);
    return res != FALSE;
}

bool WinSystemCollector::suspendProcess(core::ProcessId pid) {
    if (!m_pfnNtSuspendProcess) {
        return false;
    }
    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (!hProcess) {
        return false;
    }
    const NTSTATUS status = m_pfnNtSuspendProcess(hProcess);
    CloseHandle(hProcess);
    return status >= 0;
}

bool WinSystemCollector::resumeProcess(core::ProcessId pid) {
    if (!m_pfnNtResumeProcess) {
        return false;
    }
    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (!hProcess) {
        return false;
    }
    const NTSTATUS status = m_pfnNtResumeProcess(hProcess);
    CloseHandle(hProcess);
    return status >= 0;
}

bool WinSystemCollector::setProcessPriority(core::ProcessId pid, core::ProcessPriority priority) {
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
    if (!hProcess) {
        return false;
    }
    const DWORD winPriority = fromProcessPriority(priority);
    const BOOL res = SetPriorityClass(hProcess, winPriority);
    CloseHandle(hProcess);
    return res != FALSE;
}

} // namespace pm::platform::win

#endif // _WIN32
