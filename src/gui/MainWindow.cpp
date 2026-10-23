#include "MainWindow.hpp"
#include "models/ProcessTreeModel.hpp"
#include "models/ProcessSortFilterProxyModel.hpp"
#include "delegates/ProcessItemDelegate.hpp"
#include "widgets/SystemSummaryBar.hpp"
#include "dialogs/ProcessInspectDialog.hpp"
#include "theme/Theme.hpp"
#include "worker/CollectorWorker.hpp"

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>

namespace pm::gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Process Monitor — Монитор процессов"));
    resize(1200, 780);
    setStyleSheet(theme::getDarkStyleSheet());

    setupUi();
    setupThread();
}

MainWindow::~MainWindow() {
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(3000);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(2000);
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // 1. Верхняя панель управления и поиска
    auto* topBar = new QHBoxLayout();
    topBar->setSpacing(8);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("🔍 Поиск процесса по названию или PID..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMinimumWidth(280);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    topBar->addWidget(m_searchEdit);

    topBar->addSpacing(10);

    auto* intervalLabel = new QLabel(tr("Частота:"), this);
    intervalLabel->setStyleSheet("color: #a1a1aa; font-weight: 500;");
    topBar->addWidget(intervalLabel);

    m_intervalCombo = new QComboBox(this);
    m_intervalCombo->addItems({
        tr("250 мс (Высокая)"),
        tr("500 мс"),
        tr("1000 мс (Обычная)"),
        tr("2000 мс (Экономная)"),
        tr("Пауза")
    });
    m_intervalCombo->setCurrentIndex(2); // 1000 мс по умолчанию
    connect(m_intervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onIntervalChanged);
    topBar->addWidget(m_intervalCombo);

    topBar->addStretch();

    // Бейдж прав администратора
    m_adminBadge = new QLabel(this);
    m_adminBadge->setStyleSheet(
        "padding: 4px 10px; border-radius: 4px; font-weight: 600; font-size: 11px;"
    );
    topBar->addWidget(m_adminBadge);

    auto* inspectBtn = new QPushButton(tr("Инспектор..."), this);
    connect(inspectBtn, &QPushButton::clicked, this, &MainWindow::onInspectSelected);
    topBar->addWidget(inspectBtn);

    auto* terminateBtn = new QPushButton(tr("Завершить процесс"), this);
    terminateBtn->setStyleSheet(
        "QPushButton { background-color: #7f1d1d; border: 1px solid #991b1b; color: #fecaca; }"
        "QPushButton:hover { background-color: #991b1b; border: 1px solid #ef4444; }"
    );
    connect(terminateBtn, &QPushButton::clicked, this, &MainWindow::onTerminateSelected);
    topBar->addWidget(terminateBtn);

    mainLayout->addLayout(topBar);

    // 2. Верхняя карточная панель метрик системы
    m_summaryBar = new SystemSummaryBar(this);
    mainLayout->addWidget(m_summaryBar);

    // 3. Дерево процессов
    m_treeModel = new ProcessTreeModel(this);
    m_proxyModel = new ProcessSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_treeModel);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_proxyModel);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(ProcessTreeModel::ColCpu, Qt::DescendingOrder);
    m_treeView->setAnimated(true);
    m_treeView->setAllColumnsShowFocus(true);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::onCustomContextMenuRequested);
    connect(m_treeView, &QTreeView::doubleClicked,
            this, &MainWindow::onInspectSelected);

    m_itemDelegate = new ProcessItemDelegate(this);
    m_treeView->setItemDelegate(m_itemDelegate);

    // Настройка колонок и ширины
    auto* header = m_treeView->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(ProcessTreeModel::ColName, QHeaderView::Interactive);
    header->resizeSection(ProcessTreeModel::ColName, 260);
    header->resizeSection(ProcessTreeModel::ColPid, 75);
    header->resizeSection(ProcessTreeModel::ColCpu, 90);
    header->resizeSection(ProcessTreeModel::ColWorkingSet, 110);
    header->resizeSection(ProcessTreeModel::ColPrivateBytes, 110);
    header->resizeSection(ProcessTreeModel::ColThreads, 70);
    header->resizeSection(ProcessTreeModel::ColHandles, 85);
    header->resizeSection(ProcessTreeModel::ColIoRead, 95);
    header->resizeSection(ProcessTreeModel::ColIoWrite, 95);
    header->resizeSection(ProcessTreeModel::ColPriority, 110);
    header->setSectionResizeMode(ProcessTreeModel::ColPath, QHeaderView::Stretch);

    mainLayout->addWidget(m_treeView, 1);

    // Статус-бар
    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel);
}

void MainWindow::setupThread() {
    m_workerThread = new QThread(this);
    m_worker = new pm::worker::CollectorWorker();
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, [this]() {
        m_worker->start(1000);
    });
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // Передача системных метрик в дашборд
    connect(m_worker, &pm::worker::CollectorWorker::systemSnapshotReady,
            this, [this](const pm::core::SystemSnapshot& snapshot) {
        m_summaryBar->updateMetrics(snapshot);

        const QString adminText = m_adminBadge->text();
        if (adminText.isEmpty()) {
            // Проверка прав администратора
            // Базовый статус бейджа
            m_adminBadge->setText(tr("● Пользователь"));
            m_adminBadge->setStyleSheet("color: #a1a1aa; background-color: #272733;");
        }

        m_statusLabel->setText(tr("Процессов: %1 | Потоков: %2 | Дескрипторов: %3 | Аптайм: %4 ч %5 мин | Обновлено: %6")
            .arg(snapshot.processCount)
            .arg(snapshot.threadCount)
            .arg(snapshot.handleCount)
            .arg(snapshot.uptimeSeconds / 3600)
            .arg((snapshot.uptimeSeconds % 3600) / 60)
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    });

    // Точечное обновление дерева процессов
    connect(m_worker, &pm::worker::CollectorWorker::processesReady,
            m_treeModel, &ProcessTreeModel::updateProcesses);

    // Инспекция процесса
    connect(m_worker, &pm::worker::CollectorWorker::processDetailsReady,
            this, [this](pm::core::ProcessId pid,
                         std::vector<pm::core::ThreadSnapshot> threads,
                         std::vector<pm::core::ModuleSnapshot> modules,
                         std::vector<pm::core::HandleSnapshot> handles) {
        if (m_activeInspectDialog && m_inspectingPid == pid) {
            m_activeInspectDialog->setProcessDetails(std::move(threads), std::move(modules), std::move(handles));
        }
    });

    // Уведомления об операциях
    connect(m_worker, &pm::worker::CollectorWorker::operationCompleted,
            this, &MainWindow::onOperationCompleted);

    m_workerThread->start();
}

const pm::core::ProcessSnapshot* MainWindow::getSelectedProcess() const {
    const QModelIndex proxyIndex = m_treeView->currentIndex();
    if (!proxyIndex.isValid()) {
        return nullptr;
    }
    const QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    return m_treeModel->processAt(sourceIndex);
}

void MainWindow::onSearchTextChanged(const QString& text) {
    m_proxyModel->setSearchPattern(text);
    if (!text.isEmpty()) {
        m_treeView->expandAll();
    }
}

void MainWindow::onIntervalChanged(int index) {
    if (!m_worker) return;

    switch (index) {
        case 0: QMetaObject::invokeMethod(m_worker, "start", Q_ARG(int, 250)); break;
        case 1: QMetaObject::invokeMethod(m_worker, "start", Q_ARG(int, 500)); break;
        case 2: QMetaObject::invokeMethod(m_worker, "start", Q_ARG(int, 1000)); break;
        case 3: QMetaObject::invokeMethod(m_worker, "start", Q_ARG(int, 2000)); break;
        case 4: QMetaObject::invokeMethod(m_worker, "stop"); break;
        default: break;
    }
}

void MainWindow::onCustomContextMenuRequested(const QPoint& pos) {
    const auto* proc = getSelectedProcess();
    if (!proc) {
        return;
    }

    QMenu menu(this);

    auto* inspectAct = menu.addAction(tr("🔍 Инспектор процесса..."));
    connect(inspectAct, &QAction::triggered, this, &MainWindow::onInspectSelected);

    menu.addSeparator();

    auto* terminateAct = menu.addAction(tr("🛑 Завершить процесс"));
    connect(terminateAct, &QAction::triggered, this, &MainWindow::onTerminateSelected);

    auto* suspendAct = menu.addAction(tr("⏸ Заморозить (Suspend)"));
    connect(suspendAct, &QAction::triggered, this, &MainWindow::onSuspendSelected);

    auto* resumeAct = menu.addAction(tr("▶ Возобновить (Resume)"));
    connect(resumeAct, &QAction::triggered, this, &MainWindow::onResumeSelected);

    // Подменю приоритетов
    auto* priorityMenu = menu.addMenu(tr("⚡ Приоритет"));
    auto addPriorityItem = [this, priorityMenu, proc](const QString& name, pm::core::ProcessPriority prio) {
        auto* act = priorityMenu->addAction(name);
        act->setCheckable(true);
        if (proc->priority == prio) {
            act->setChecked(true);
        }
        connect(act, &QAction::triggered, this, [this, prio]() {
            onPrioritySelected(prio);
        });
    };

    addPriorityItem(tr("Низкий (Idle)"), pm::core::ProcessPriority::Idle);
    addPriorityItem(tr("Ниже обычного"), pm::core::ProcessPriority::BelowNormal);
    addPriorityItem(tr("Обычный"), pm::core::ProcessPriority::Normal);
    addPriorityItem(tr("Выше обычного"), pm::core::ProcessPriority::AboveNormal);
    addPriorityItem(tr("Высокий"), pm::core::ProcessPriority::High);
    addPriorityItem(tr("Реального времени"), pm::core::ProcessPriority::RealTime);

    menu.addSeparator();

    auto* openLocAct = menu.addAction(tr("📁 Открыть расположение файла"));
    connect(openLocAct, &QAction::triggered, this, &MainWindow::onOpenFileLocation);

    auto* copyMenu = menu.addMenu(tr("📋 Копировать"));
    auto* copyPidAct = copyMenu->addAction(tr("PID"));
    connect(copyPidAct, &QAction::triggered, this, &MainWindow::onCopyPid);

    auto* copyPathAct = copyMenu->addAction(tr("Путь к файлу"));
    connect(copyPathAct, &QAction::triggered, this, &MainWindow::onCopyPath);

    auto* copyCmdAct = copyMenu->addAction(tr("Командную строку"));
    connect(copyCmdAct, &QAction::triggered, this, &MainWindow::onCopyCommandLine);

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void MainWindow::onTerminateSelected() {
    const auto* proc = getSelectedProcess();
    if (!proc) return;

    const auto reply = QMessageBox::question(
        this,
        tr("Подтверждение завершения"),
        tr("Вы действительно хотите принудительно завершить процесс \"%1\" (PID: %2)?\n"
           "Несохраненные данные процесса могут быть утеряны.")
            .arg(QString::fromStdString(proc->name))
            .arg(proc->pid),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        QMetaObject::invokeMethod(
            m_worker,
            "terminateProcess",
            Q_ARG(pm::core::ProcessId, proc->pid)
        );
    }
}

void MainWindow::onSuspendSelected() {
    const auto* proc = getSelectedProcess();
    if (!proc) return;

    QMetaObject::invokeMethod(
        m_worker,
        "suspendProcess",
        Q_ARG(pm::core::ProcessId, proc->pid)
    );
}

void MainWindow::onResumeSelected() {
    const auto* proc = getSelectedProcess();
    if (!proc) return;

    QMetaObject::invokeMethod(
        m_worker,
        "resumeProcess",
        Q_ARG(pm::core::ProcessId, proc->pid)
    );
}

void MainWindow::onPrioritySelected(pm::core::ProcessPriority priority) {
    const auto* proc = getSelectedProcess();
    if (!proc) return;

    QMetaObject::invokeMethod(
        m_worker,
        "setPriority",
        Q_ARG(pm::core::ProcessId, proc->pid),
        Q_ARG(pm::core::ProcessPriority, priority)
    );
}

void MainWindow::onInspectSelected() {
    const auto* proc = getSelectedProcess();
    if (!proc) return;

    m_inspectingPid = proc->pid;
    auto* dialog = new ProcessInspectDialog(*proc, this);
    m_activeInspectDialog = dialog;

    // Запрашиваем детали у фонового коллектора
    QMetaObject::invokeMethod(
        m_worker,
        "inspectProcess",
        Q_ARG(pm::core::ProcessId, proc->pid)
    );

    dialog->exec();
    m_activeInspectDialog = nullptr;
    dialog->deleteLater();
}

void MainWindow::onOpenFileLocation() {
    const auto* proc = getSelectedProcess();
    if (!proc || proc->exePath.empty()) return;

    const QFileInfo fileInfo(QString::fromStdString(proc->exePath));
    if (fileInfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
    }
}

void MainWindow::onCopyPid() {
    const auto* proc = getSelectedProcess();
    if (!proc) return;
    QGuiApplication::clipboard()->setText(QString::number(proc->pid));
}

void MainWindow::onCopyPath() {
    const auto* proc = getSelectedProcess();
    if (!proc) return;
    QGuiApplication::clipboard()->setText(QString::fromStdString(proc->exePath));
}

void MainWindow::onCopyCommandLine() {
    const auto* proc = getSelectedProcess();
    if (!proc) return;
    QGuiApplication::clipboard()->setText(QString::fromStdString(proc->commandLine));
}

void MainWindow::onOperationCompleted(bool success, const QString& message) {
    statusBar()->showMessage(message, 3000);
    if (!success) {
        QMessageBox::warning(this, tr("Ошибка операции"), message);
    }
}

} // namespace pm::gui
