#pragma once

#ifdef _WIN32

#include "core/ISystemCollector.hpp"
#include "WinUtils.hpp"
#include <unordered_map>
#include <chrono>

namespace pm::platform::win {

/**
 * @brief Предыдущие временные показатели процесса для расчета мгновенных дельт CPU.
 */
struct ProcessTimeRecord {
    std::uint64_t kernelTime{0};
    std::uint64_t userTime{0};
};

/**
 * @brief Предыдущие показатели дискового ввода-вывода для расчета скорости (Bytes/sec).
 */
struct ProcessIoRecord {
    std::uint64_t readBytes{0};
    std::uint64_t writeBytes{0};
};

/**
 * @brief Высокопроизводительная реализация сборщика метрик для Windows.
 * 
 * Использует Win32 Toolhelp32 API, PSAPI, IP Helper API, а также Native API (ntdll.dll)
 * для получения телеметрии, заморозки/возобновления процессов и чтения аргументов командной строки.
 */
class WinSystemCollector final : public core::ISystemCollector {
public:
    /**
     * @brief Конструктор сборщика для Windows.
     * @details Активирует привилегию SeDebugPrivilege и загружает указатели на Native API ntdll.
     */
    WinSystemCollector();

    /**
     * @brief Деструктор.
     */
    ~WinSystemCollector() override = default;

    // Запрет копирования и перемещения для безопасности ресурсов
    WinSystemCollector(const WinSystemCollector&) = delete;
    WinSystemCollector& operator=(const WinSystemCollector&) = delete;
    WinSystemCollector(WinSystemCollector&&) = delete;
    WinSystemCollector& operator=(WinSystemCollector&&) = delete;

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
    /**
     * @brief Инициализация динамических указателей на недокументированные вызовы ntdll.dll.
     */
    void initNtApis() noexcept;

    /**
     * @brief Извлечь аргументы командной строки процесса через NtQueryInformationProcess или PEB.
     * @param[in] hProcess Дескриптор открытого процесса.
     * @return std::string Строка аргументов.
     */
    [[nodiscard]] std::string readCommandLine(HANDLE hProcess) const;

    /**
     * @brief Преобразование Windows Priority Class в платформенно-независимый enum.
     * @param[in] winPriority Значение GetPriorityClass.
     * @return core::ProcessPriority.
     */
    [[nodiscard]] static core::ProcessPriority toProcessPriority(DWORD winPriority) noexcept;

    /**
     * @brief Преобразование core::ProcessPriority в Win32 DWORD флаг приоритета.
     * @param[in] priority Значение перечисления.
     * @return DWORD флаг для SetPriorityClass.
     */
    [[nodiscard]] static DWORD fromProcessPriority(core::ProcessPriority priority) noexcept;

    // --- Предыдущее состояние системы ---
    std::uint64_t m_prevSysIdle{0};
    std::uint64_t m_prevSysKernel{0};
    std::uint64_t m_prevSysUser{0};
    std::chrono::steady_clock::time_point m_prevSysSampleTime;

    std::uint64_t m_prevNetRxBytes{0};
    std::uint64_t m_prevNetTxBytes{0};
    std::chrono::steady_clock::time_point m_prevNetSampleTime;

    // --- Предыдущие состояния процессов для расчета дельт ---
    std::unordered_map<core::ProcessId, ProcessTimeRecord> m_processTimes;
    std::unordered_map<core::ProcessId, ProcessIoRecord> m_processIo;
    std::unordered_map<core::ProcessId, std::uint8_t> m_newProcessCounters;

    std::uint32_t m_logicalCores{1};
    bool m_isAdmin{false};

    // --- Указатели на функции ntdll.dll ---
    using NtSuspendProcessFn = LONG(NTAPI*)(HANDLE ProcessHandle);
    using NtResumeProcessFn = LONG(NTAPI*)(HANDLE ProcessHandle);

    NtSuspendProcessFn m_pfnNtSuspendProcess{nullptr};
    NtResumeProcessFn m_pfnNtResumeProcess{nullptr};
};

} // namespace pm::platform::win

#endif // _WIN32
