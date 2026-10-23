#pragma once

#include "core/Types.hpp"
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtCore/QThread>
#include <memory>

namespace pm::worker {
class CollectorWorker;
}

namespace pm::gui {

class ProcessTreeModel;
class ProcessSortFilterProxyModel;
class ProcessItemDelegate;
class SystemSummaryBar;
class ProcessInspectDialog;

/**
 * @brief Главное окно монитора процессов.
 * 
 * Интегрирует:
 * - Управление фоновым потоком сбора метрик (QThread + CollectorWorker).
 * - Верхнюю статус-панель дашборда (SystemSummaryBar).
 * - Дерево процессов с поиском, контекстными действиями и числовой сортировкой.
 * - Диалог углубленной инспекции (модули, потоки, дескрипторы).
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onSearchTextChanged(const QString& text);
    void onIntervalChanged(int index);
    void onCustomContextMenuRequested(const QPoint& pos);
    void onTerminateSelected();
    void onSuspendSelected();
    void onResumeSelected();
    void onPrioritySelected(pm::core::ProcessPriority priority);
    void onInspectSelected();
    void onOpenFileLocation();
    void onCopyPid();
    void onCopyPath();
    void onCopyCommandLine();
    void onOperationCompleted(bool success, const QString& message);

private:
    void setupUi();
    void setupThread();
    [[nodiscard]] const pm::core::ProcessSnapshot* getSelectedProcess() const;

    // --- Модели и представления ---
    ProcessTreeModel* m_treeModel{nullptr};
    ProcessSortFilterProxyModel* m_proxyModel{nullptr};
    ProcessItemDelegate* m_itemDelegate{nullptr};
    QTreeView* m_treeView{nullptr};

    // --- UI элементы ---
    SystemSummaryBar* m_summaryBar{nullptr};
    QLineEdit* m_searchEdit{nullptr};
    QComboBox* m_intervalCombo{nullptr};
    QLabel* m_statusLabel{nullptr};
    QLabel* m_adminBadge{nullptr};

    // --- Фоновый поток ---
    QThread* m_workerThread{nullptr};
    pm::worker::CollectorWorker* m_worker{nullptr};

    // --- Инспектор ---
    ProcessInspectDialog* m_activeInspectDialog{nullptr};
    pm::core::ProcessId m_inspectingPid{0};
};

} // namespace pm::gui
