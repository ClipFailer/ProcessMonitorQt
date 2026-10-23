#pragma once

#include "core/ISystemCollector.hpp"
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <memory>

namespace pm::worker {

/**
 * @brief Асинхронный воркер периодического сбора системной телеметрии.
 * 
 * Выполняется в выделенном рабочем потоке (QThread), опрашивает платформенный ISystemCollector
 * по высокоточному таймеру и отправляет готовые снимки в главный UI-поток через сигналы
 * со строгой move-семантикой без взаимных блокировок интерфейса.
 */
class CollectorWorker : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Конструктор воркера сбора.
     * @param[in] parent Родительский QObject.
     */
    explicit CollectorWorker(QObject* parent = nullptr);

    /**
     * @brief Деструктор.
     */
    ~CollectorWorker() override = default;

public slots:
    /**
     * @brief Запустить периодический опрос системы.
     * @param[in] intervalMs Интервал обновления в миллисекундах (по умолчанию 1000 мс).
     */
    void start(int intervalMs = 1000);

    /**
     * @brief Остановить таймер опроса.
     */
    void stop();

    /**
     * @brief Изменить интервал обновления.
     * @param[in] intervalMs Новый интервал в миллисекундах.
     */
    void setInterval(int intervalMs);

    /**
     * @brief Принудительно выполнить единичный такт сбора метрик.
     */
    void poll();

    /**
     * @brief Запросить завершение процесса.
     * @param[in] pid Идентификатор процесса.
     */
    void terminateProcess(pm::core::ProcessId pid);

    /**
     * @brief Запросить заморозку процесса.
     * @param[in] pid Идентификатор процесса.
     */
    void suspendProcess(pm::core::ProcessId pid);

    /**
     * @brief Запросить возобновление работы процесса.
     * @param[in] pid Идентификатор процесса.
     */
    void resumeProcess(pm::core::ProcessId pid);

    /**
     * @brief Изменить приоритет процесса.
     * @param[in] pid Идентификатор процесса.
     * @param[in] priority Желаемый класс приоритета.
     */
    void setPriority(pm::core::ProcessId pid, pm::core::ProcessPriority priority);

    /**
     * @brief Запросить глубокую инспекцию процесса (потоки, библиотеки).
     * @param[in] pid Идентификатор процесса.
     */
    void inspectProcess(pm::core::ProcessId pid);

signals:
    /**
     * @brief Сигнал готовности снимка общесистемных показателей.
     * @param[out] snapshot Снимок ресурсов операционной системы.
     */
    void systemSnapshotReady(const pm::core::SystemSnapshot& snapshot);

    /**
     * @brief Сигнал готовности полного снимка процессов.
     * @param[out] processes Вектор процессов для отображения в дереве.
     */
    void processesReady(std::vector<pm::core::ProcessSnapshot> processes);

    /**
     * @brief Сигнал готовности детальной информации по инспектируемому процессу.
     * @param[out] pid Идентификатор процесса.
     * @param[out] threads Список потоков.
     * @param[out] modules Список DLL/модулей.
     * @param[out] handles Список дескрипторов.
     */
    void processDetailsReady(
        pm::core::ProcessId pid,
        std::vector<pm::core::ThreadSnapshot> threads,
        std::vector<pm::core::ModuleSnapshot> modules,
        std::vector<pm::core::HandleSnapshot> handles
    );

    /**
     * @brief Сигнал результата выполнения команды управления процессом.
     * @param[out] success true если операция выполнена успешно, иначе false.
     * @param[out] message Текстовое описание результата или ошибки.
     */
    void operationCompleted(bool success, const QString& message);

private:
    std::unique_ptr<pm::core::ISystemCollector> m_collector; ///< Платформенный коллектор
    QTimer* m_timer{nullptr};                                ///< Высокоточный таймер тиков
};

} // namespace pm::worker
