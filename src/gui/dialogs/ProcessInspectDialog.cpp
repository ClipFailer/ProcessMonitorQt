#include "ProcessInspectDialog.hpp"
#include "theme/Theme.hpp"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>

namespace pm::gui {

ProcessInspectDialog::ProcessInspectDialog(const pm::core::ProcessSnapshot& snapshot, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Инспектор процесса: %1 (PID %2)")
                   .arg(QString::fromStdString(snapshot.name))
                   .arg(snapshot.pid));
    resize(780, 520);
    setStyleSheet(theme::getDarkStyleSheet());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    m_tabWidget = new QTabWidget(this);

    // 1. Вкладка "Общее"
    setupGeneralTab(snapshot);

    // 2. Вкладка "Модули / DLL"
    setupModulesTab();

    // 3. Вкладка "Потоки"
    setupThreadsTab();

    mainLayout->addWidget(m_tabWidget);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* closeBtn = new QPushButton(tr("Закрыть"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    mainLayout->addLayout(btnLayout);
}

void ProcessInspectDialog::setupGeneralTab(const pm::core::ProcessSnapshot& s) {
    auto* generalWidget = new QWidget(this);
    auto* formLayout = new QFormLayout(generalWidget);
    formLayout->setContentsMargins(16, 16, 16, 16);
    formLayout->setSpacing(10);

    auto addField = [formLayout](const QString& label, const QString& val) {
        auto* lineEdit = new QLineEdit(val);
        lineEdit->setReadOnly(true);
        formLayout->addRow(label, lineEdit);
    };

    addField(tr("Имя процесса:"), QString::fromStdString(s.name));
    addField(tr("PID / Parent PID:"), QString("%1 / %2").arg(s.pid).arg(s.parentPid));
    addField(tr("Разрядность:"), s.is64Bit ? tr("64-бит (x64)") : tr("32-бит (x86 / WOW64)"));
    addField(tr("Исполняемый файл:"), QString::fromStdString(s.exePath));

    auto* cmdEdit = new QTextEdit(generalWidget);
    cmdEdit->setReadOnly(true);
    cmdEdit->setPlainText(QString::fromStdString(s.commandLine));
    cmdEdit->setMaximumHeight(70);
    formLayout->addRow(tr("Командная строка:"), cmdEdit);

    addField(tr("Количество потоков:"), QString::number(s.threadCount));
    addField(tr("Количество дескрипторов:"), QString::number(s.handleCount));
    addField(tr("Рабочий набор (RAM):"), QString::asprintf("%.1f MB", static_cast<double>(s.workingSetBytes) / (1024.0 * 1024.0)));

    m_tabWidget->addTab(generalWidget, tr("Свойства"));
}

void ProcessInspectDialog::setupModulesTab() {
    m_modulesTable = new QTableWidget(this);
    m_modulesTable->setColumnCount(4);
    m_modulesTable->setHorizontalHeaderLabels({
        tr("Базовый адрес"),
        tr("Размер (КБ)"),
        tr("Имя модуля"),
        tr("Путь на диске")
    });
    m_modulesTable->horizontalHeader()->setStretchLastSection(true);
    m_modulesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    m_modulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_modulesTable->verticalHeader()->setVisible(false);

    m_tabWidget->addTab(m_modulesTable, tr("Модули (DLL)"));
}

void ProcessInspectDialog::setupThreadsTab() {
    m_threadsTable = new QTableWidget(this);
    m_threadsTable->setColumnCount(5);
    m_threadsTable->setHorizontalHeaderLabels({
        tr("TID"),
        tr("Приоритет"),
        tr("User Время (мс)"),
        tr("Kernel Время (мс)"),
        tr("Состояние")
    });
    m_threadsTable->horizontalHeader()->setStretchLastSection(true);
    m_threadsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    m_threadsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_threadsTable->verticalHeader()->setVisible(false);

    m_tabWidget->addTab(m_threadsTable, tr("Потоки"));
}

void ProcessInspectDialog::setProcessDetails(
    std::vector<pm::core::ThreadSnapshot> threads,
    std::vector<pm::core::ModuleSnapshot> modules,
    std::vector<pm::core::HandleSnapshot> /*handles*/)
{
    // Заполнение модулей
    m_modulesTable->setRowCount(static_cast<int>(modules.size()));
    for (std::size_t i = 0; i < modules.size(); ++i) {
        const auto& mod = modules[i];
        const int row = static_cast<int>(i);

        m_modulesTable->setItem(row, 0, new QTableWidgetItem(QString::asprintf("0x%016llX", static_cast<unsigned long long>(mod.baseAddress))));
        m_modulesTable->setItem(row, 1, new QTableWidgetItem(QString::number(mod.sizeBytes / 1024)));
        m_modulesTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(mod.name)));
        m_modulesTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(mod.path)));
    }

    // Заполнение потоков
    m_threadsTable->setRowCount(static_cast<int>(threads.size()));
    for (std::size_t i = 0; i < threads.size(); ++i) {
        const auto& th = threads[i];
        const int row = static_cast<int>(i);

        m_threadsTable->setItem(row, 0, new QTableWidgetItem(QString::number(th.tid)));
        m_threadsTable->setItem(row, 1, new QTableWidgetItem(QString::number(th.priority)));
        m_threadsTable->setItem(row, 2, new QTableWidgetItem(QString::number(th.userTimeMs)));
        m_threadsTable->setItem(row, 3, new QTableWidgetItem(QString::number(th.kernelTimeMs)));
        m_threadsTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(th.state)));
    }
}

} // namespace pm::gui
