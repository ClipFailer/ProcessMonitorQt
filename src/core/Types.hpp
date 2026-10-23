#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <QtCore/QMetaType>
#include <QtCore/QString>

namespace pm::core {

/**
 * @brief Тип уникального системного идентификатора процесса (PID).
 */
using ProcessId = std::uint32_t;

/**
 * @brief Тип уникального системного идентификатора потока (TID).
 */
using ThreadId = std::uint32_t;

/**
 * @brief Классы базового приоритета выполнения процесса в операционной системе.
 */
enum class ProcessPriority : std::uint8_t {
    Idle = 0,         ///< Фоновый/минимальный приоритет (Low/Idle)
    BelowNormal = 1,  ///< Ниже обычного
    Normal = 2,       ///< Стандартный приоритет
    AboveNormal = 3,  ///< Выше обычного
    High = 4,         ///< Высокий приоритет
    RealTime = 5,     ///< Приоритет реального времени (требует прав администратора)
    Unknown = 255     ///< Приоритет не определен или недоступен
};

/**
 * @brief Жизненный цикл процесса для UI-индикации активности.
 */
enum class ProcessLifecycleState : std::uint8_t {
    Active,       ///< Стабильно работающий процесс
    New,          ///< Недавно обнаруженный процесс (индикация зеленым)
    Terminating   ///< Завершающийся процесс перед удалением из модели (индикация красным)
};

/**
 * @brief Снимок метрик и параметров отдельного потока выполнения.
 */
struct ThreadSnapshot {
    ThreadId tid{0};                  ///< Идентификатор потока
    int priority{0};                  ///< Числовой приоритет потока
    std::uint64_t userTimeMs{0};      ///< Время работы в пространстве пользователя (мс)
    std::uint64_t kernelTimeMs{0};    ///< Время работы в режиме ядра (мс)
    std::string state;                ///< Текстовое описание состояния потока (Running, Waiting, etc.)
};

/**
 * @brief Снимок динамически загруженной библиотеки (DLL / Shared Object).
 */
struct ModuleSnapshot {
    std::uint64_t baseAddress{0};     ///< Базовый виртуальный адрес загрузки в память
    std::uint64_t sizeBytes{0};       ///< Размер модуля в байтах
    std::string name;                 ///< Короткое имя модуля (например, ntdll.dll)
    std::string path;                 ///< Полный путь к файлу модуля на диске
    std::string version;              ///< Версия файла библиотеки (если доступна)
};

/**
 * @brief Снимок открытого дескриптора или системного ресурса процесса.
 */
struct HandleSnapshot {
    std::uint64_t handleValue{0};     ///< Числовое значение дескриптора
    std::string typeName;             ///< Тип ресурса (File, Directory, Key, Mutant, Section, Socket)
    std::string objectName;           ///< Имя открытого ресурса (путь к файлу, имя мьютекса и т.д.)
};

/**
 * @brief Полный моментальный снимок телеметрии конкретного процесса.
 * @note Структура легковесна, содержит минимально необходимые поля и поддерживает быструю move-семантику.
 */
struct ProcessSnapshot {
    // --- Идентификация ---
    ProcessId pid{0};                     ///< Идентификатор процесса
    ProcessId parentPid{0};               ///< Идентификатор родительского процесса
    std::string name;                     ///< Имя процесса / исполняемого файла
    std::string exePath;                  ///< Абсолютный путь к исполняемому файлу
    std::string commandLine;              ///< Полная командная строка запуска с аргументами
    bool is64Bit{true};                   ///< Признак 64-битной разрядности процесса

    // --- Нагрузка на процессор ---
    double cpuUsagePercent{0.0};          ///< Мгновенная загрузка CPU (0.0% – 100.0%)
    std::uint64_t userTimeMs{0};          ///< Время выполнения в User Space (мс)
    std::uint64_t kernelTimeMs{0};        ///< Время выполнения в Kernel Space (мс)
    std::int64_t creationTimestamp{0};    ///< Время запуска (Unix epoch в миллисекундах)

    // --- Использование оперативной памяти ---
    std::uint64_t workingSetBytes{0};     ///< Resident Set Size / Рабочий набор памяти (байты)
    std::uint64_t privateBytes{0};        ///< Commit Charge / Выделенная приватная память (байты)
    std::uint64_t peakWorkingSetBytes{0}; ///< Пиковый рабочий набор памяти (байты)

    // --- Дисковые и файловые операции (I/O) ---
    std::uint64_t readBytesTotal{0};      ///< Всего прочитано байт за все время жизни
    std::uint64_t writeBytesTotal{0};     ///< Всего записано байт за все время жизни
    std::uint64_t readBytesPerSec{0};     ///< Текущая скорость чтения (байт/сек)
    std::uint64_t writeBytesPerSec{0};    ///< Текущая скорость записи (байт/сек)

    // --- Системные ресурсы ---
    std::uint32_t threadCount{0};         ///< Количество активных потоков
    std::uint32_t handleCount{0};         ///< Количество открытых дескрипторов/файлов
    ProcessPriority priority{ProcessPriority::Normal}; ///< Класс приоритета

    // --- Состояние для UI ---
    ProcessLifecycleState lifecycleState{ProcessLifecycleState::Active}; ///< Статус подсветки
    std::uint8_t lifecycleCounter{0};     ///< Счетчик тактов для анимации затухания подсветки
};

/**
 * @brief Снимок общесистемных ресурсов и телеметрии всей ОС.
 */
struct SystemSnapshot {
    // --- Загрузка CPU ---
    double totalCpuPercent{0.0};          ///< Общая загрузка CPU (0.0% – 100.0%)
    double userCpuPercent{0.0};           ///< Доля времени в режиме пользователя
    double kernelCpuPercent{0.0};         ///< Доля времени в режиме ядра (system/kernel)
    double idleCpuPercent{100.0};         ///< Доля времени простоя (idle)
    std::uint32_t logicalCoresCount{1};   ///< Количество логических ядер процессора

    // --- Оперативная память (RAM) ---
    std::uint64_t totalPhysicalRam{0};    ///< Полный объем физической памяти (байты)
    std::uint64_t usedPhysicalRam{0};     ///< Занятая физическая память (байты)
    std::uint64_t freePhysicalRam{0};     ///< Свободная физическая память (байты)

    // --- Файл подкачки (Pagefile / Swap) ---
    std::uint64_t totalPagefile{0};       ///< Полный объем виртуальной памяти/подкачки
    std::uint64_t usedPagefile{0};        ///< Занятый объем виртуальной памяти/подкачки

    // --- Суммарный ввод-вывод дисков (I/O) ---
    std::uint64_t diskReadBytesPerSec{0}; ///< Текущая скорость чтения со всех дисков (байт/сек)
    std::uint64_t diskWriteBytesPerSec{0};///< Текущая скорость записи на все диски (байт/сек)

    // --- Сетевой трафик ---
    std::uint64_t netRxBytesPerSec{0};    ///< Текущая скорость приема пакетов сетью (байт/сек)
    std::uint64_t netTxBytesPerSec{0};    ///< Текущая скорость передачи пакетов сетью (байт/сек)

    // --- Общая статистика системы ---
    std::uint32_t processCount{0};        ///< Суммарное число запущенных процессов
    std::uint32_t threadCount{0};         ///< Суммарное число запущенных потоков
    std::uint32_t handleCount{0};         ///< Суммарное число дескрипторов
    std::uint64_t uptimeSeconds{0};       ///< Время непрерывной работы системы (секунды)
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()}; ///< Метка времени
};

} // namespace pm::core

// Регистрация структур в системе метатипов Qt для безопасной передачи через queued connections
Q_DECLARE_METATYPE(pm::core::ProcessSnapshot)
Q_DECLARE_METATYPE(std::vector<pm::core::ProcessSnapshot>)
Q_DECLARE_METATYPE(pm::core::SystemSnapshot)
Q_DECLARE_METATYPE(pm::core::ThreadSnapshot)
Q_DECLARE_METATYPE(std::vector<pm::core::ThreadSnapshot>)
Q_DECLARE_METATYPE(pm::core::ModuleSnapshot)
Q_DECLARE_METATYPE(std::vector<pm::core::ModuleSnapshot>)
Q_DECLARE_METATYPE(pm::core::HandleSnapshot)
Q_DECLARE_METATYPE(std::vector<pm::core::HandleSnapshot>)
