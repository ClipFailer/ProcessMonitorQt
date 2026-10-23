#include "CollectorWorker.hpp"

namespace pm::worker {

CollectorWorker::CollectorWorker(QObject* parent)
    : QObject(parent)
    , m_collector(pm::core::createSystemCollector())
    , m_timer(new QTimer(this))
{
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &CollectorWorker::poll);
}

void CollectorWorker::start(int intervalMs) {
    if (intervalMs > 0) {
        m_timer->start(intervalMs);
    }
    // Выполняем первый опрос сразу после запуска
    poll();
}

void CollectorWorker::stop() {
    m_timer->stop();
}

void CollectorWorker::setInterval(int intervalMs) {
    if (intervalMs > 0) {
        m_timer->setInterval(intervalMs);
    }
}

void CollectorWorker::poll() {
    if (!m_collector) {
        return;
    }

    // 1. Опрос системных показателей
    pm::core::SystemSnapshot sysSnapshot = m_collector->collectSystemSnapshot();
    emit systemSnapshotReady(sysSnapshot);

    // 2. Опрос списка процессов и передача через move-семантику
    std::vector<pm::core::ProcessSnapshot> processes = m_collector->collectProcesses();
    emit processesReady(std::move(processes));
}

void CollectorWorker::terminateProcess(pm::core::ProcessId pid) {
    if (!m_collector) return;
    const bool ok = m_collector->terminateProcess(pid);
    emit operationCompleted(ok, ok ? tr("Процесс %1 успешно завершен").arg(pid)
                                  : tr("Не удалось завершить процесс %1 (отказано в доступе)").arg(pid));
    // Сразу инициируем опрос для быстрого обновления UI
    poll();
}

void CollectorWorker::suspendProcess(pm::core::ProcessId pid) {
    if (!m_collector) return;
    const bool ok = m_collector->suspendProcess(pid);
    emit operationCompleted(ok, ok ? tr("Процесс %1 приостановлен").arg(pid)
                                  : tr("Не удалось приостановить процесс %1").arg(pid));
}

void CollectorWorker::resumeProcess(pm::core::ProcessId pid) {
    if (!m_collector) return;
    const bool ok = m_collector->resumeProcess(pid);
    emit operationCompleted(ok, ok ? tr("Процесс %1 возобновлен").arg(pid)
                                  : tr("Не удалось возобновить процесс %1").arg(pid));
}

void CollectorWorker::setPriority(pm::core::ProcessId pid, pm::core::ProcessPriority priority) {
    if (!m_collector) return;
    const bool ok = m_collector->setProcessPriority(pid, priority);
    emit operationCompleted(ok, ok ? tr("Приоритет процесса %1 изменен").arg(pid)
                                  : tr("Не удалось изменить приоритет процесса %1").arg(pid));
}

void CollectorWorker::inspectProcess(pm::core::ProcessId pid) {
    if (!m_collector) return;
    auto threads = m_collector->collectProcessThreads(pid);
    auto modules = m_collector->collectProcessModules(pid);
    auto handles = m_collector->collectProcessHandles(pid);
    emit processDetailsReady(pid, std::move(threads), std::move(modules), std::move(handles));
}

} // namespace pm::worker
