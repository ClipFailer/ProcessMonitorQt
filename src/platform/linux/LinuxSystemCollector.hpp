#pragma once

#if defined(__linux__)

#include "core/ISystemCollector.hpp"
#include <unordered_map>
#include <chrono>

namespace pm::platform::linux_os {

/**
 * @brief Предыдущие временные показатели процесса для расчета CPU дельты в Linux.
 */
struct LinuxProcessTimeRecord {
    std::uint64_t utime{0};
    std::uint64_t stime{0};
};

/**
 * @brief Высокопроизводительная реализация сборщика метрик для Linux.
 * @details Парсит виртуальные файловые системы /proc и /sys с минимальным количеством
 * промежуточных динамических аллокаций.
 */
class LinuxSystemCollector final : public core::ISystemCollector {
public:
    LinuxSystemCollector();
    ~LinuxSystemCollector() override = default;

    LinuxSystemCollector(const LinuxSystemCollector&) = delete;
    LinuxSystemCollector& operator=(const LinuxSystemCollector&) = delete;
    LinuxSystemCollector(LinuxSystemCollector&&) = delete;
    LinuxSystemCollector& operator=(LinuxSystemCollector&&) = delete;

    [[nodiscard]] core::SystemSnapshot collectSystemSnapshot() override;
    [[nodiscard]] std::vector<core::ProcessSnapshot> collectProcesses() override;
    [[nodiscard]] std::vector<core::ThreadSnapshot> collectProcessThreads(core::ProcessId pid) override;
    [[nodiscard]] std::vector<core::ModuleSnapshot> collectProcessModules(core::ProcessId pid) override;
    [[nodiscard]] std::vector<core::HandleSnapshot> collectProcessHandles(core::ProcessId pid) override;

    bool terminateProcess(core::ProcessId pid, int exitCode = 0) override;
    bool suspendProcess(core::ProcessId pid) override;
    bool resumeProcess(core::ProcessId pid) override;
    bool setProcessPriority(core::ProcessId pid, core::ProcessPriority priority) override;
    [[nodiscard]] bool isRunningAsAdministrator() const noexcept override;

private:
    std::uint64_t m_prevTotalCpuTime{0};
    std::uint64_t m_prevIdleCpuTime{0};
    std::chrono::steady_clock::time_point m_prevSampleTime;

    std::uint64_t m_prevNetRx{0};
    std::uint64_t m_prevNetTx{0};
    std::chrono::steady_clock::time_point m_prevNetTime;

    std::unordered_map<core::ProcessId, LinuxProcessTimeRecord> m_processTimes;
    std::unordered_map<core::ProcessId, std::pair<uint64_t, uint64_t>> m_processIo;
    std::unordered_map<core::ProcessId, std::uint8_t> m_newProcessCounters;

    long m_clockTicksPerSec{100};
    long m_pageSizeBytes{4096};
    std::uint32_t m_logicalCores{1};
};

} // namespace pm::platform::linux_os

#endif // __linux__
