#pragma once

#include "Types.hpp"
#include <memory>
#include <vector>

namespace pm::core {

/**
 * @brief Абстрактный интерфейс платформенно-независимого сборщика системной телеметрии.
 * 
 * Предоставляет унифицированный API для сбора статистики операционной системы,
 * древовидного снимка процессов, глубокой инспекции (потоки, библиотеки, дескрипторы)
 * и базового управления жизненным циклом процессов.
 */
class ISystemCollector {
public:
    virtual ~ISystemCollector() = default;

    /**
     * @brief Собрать моментальный снимок общесистемных метрик (CPU, RAM, Диск, Сеть, Аптайм).
     * @return SystemSnapshot Структура с текущими системными показателями.
     */
    [[nodiscard]] virtual SystemSnapshot collectSystemSnapshot() = 0;

    /**
     * @brief Собрать моментальный список всех активных процессов в операционной системе.
     * @details Метод рассчитывает дельты CPU и I/O относительно предыдущего вызова.
     * @return std::vector<ProcessSnapshot> Плоский список снимков процессов.
     */
    [[nodiscard]] virtual std::vector<ProcessSnapshot> collectProcesses() = 0;

    /**
     * @brief Получить список активных потоков выполнения целевого процесса.
     * @param[in] pid Идентификатор процесса.
     * @return std::vector<ThreadSnapshot> Список потоков процесса.
     */
    [[nodiscard]] virtual std::vector<ThreadSnapshot> collectProcessThreads(ProcessId pid) = 0;

    /**
     * @brief Получить список динамически загруженных библиотек/модулей процесса.
     * @param[in] pid Идентификатор процесса.
     * @return std::vector<ModuleSnapshot> Список библиотек с адресами и путями.
     */
    [[nodiscard]] virtual std::vector<ModuleSnapshot> collectProcessModules(ProcessId pid) = 0;

    /**
     * @brief Получить список открытых дескрипторов/файлов процесса.
     * @note Может возвращать неполный список или требовать повышенных привилегий (Admin/Root).
     * @param[in] pid Идентификатор процесса.
     * @return std::vector<HandleSnapshot> Список открытых дескрипторов.
     */
    [[nodiscard]] virtual std::vector<HandleSnapshot> collectProcessHandles(ProcessId pid) = 0;

    /**
     * @brief Принудительно завершить процесс.
     * @param[in] pid Идентификатор процесса.
     * @param[in] exitCode Код завершения (по умолчанию 0).
     * @return true если запрос на завершение успешно отправлен ОС, иначе false.
     */
    virtual bool terminateProcess(ProcessId pid, int exitCode = 0) = 0;

    /**
     * @brief Приостановить выполнение всех потоков процесса (Freeze/Suspend).
     * @param[in] pid Идентификатор процесса.
     * @return true если процесс успешно заморожен, иначе false.
     */
    virtual bool suspendProcess(ProcessId pid) = 0;

    /**
     * @brief Возобновить выполнение приостановленного процесса (Thaw/Resume).
     * @param[in] pid Идентификатор процесса.
     * @return true если процесс успешно возобновлен, иначе false.
     */
    virtual bool resumeProcess(ProcessId pid) = 0;

    /**
     * @brief Изменить класс приоритета процесса.
     * @param[in] pid Идентификатор процесса.
     * @param[in] priority Желаемый уровень приоритета.
     * @return true если приоритет успешно изменен, иначе false.
     */
    virtual bool setProcessPriority(ProcessId pid, ProcessPriority priority) = 0;

    /**
     * @brief Проверить, запущено ли приложение с правами суперпользователя (Administrator/root).
     * @return true если приложение обладает повышенными правами, иначе false.
     */
    [[nodiscard]] virtual bool isRunningAsAdministrator() const noexcept = 0;
};

/**
 * @brief Фабричная функция для создания платформозависимого экземпляра сборщика метрик.
 * @return std::unique_ptr<ISystemCollector> Экземпляр коллектора под текущую целевую ОС.
 */
std::unique_ptr<ISystemCollector> createSystemCollector();

} // namespace pm::core
