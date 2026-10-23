#pragma once

#include "core/Types.hpp"
#include <QtWidgets/QDialog>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QLabel>

namespace pm::gui {

/**
 * @brief Диалоговое окно углубленной инспекции процесса (Profiler / Deep View).
 * 
 * Вкладки:
 * 1. Загруженные модули/DLL: базовые виртуальные адреса, размеры в памяти, полные пути.
 * 2. Потоки выполнения: Thread ID, приоритет, время в User/Kernel режимах, состояние.
 * 3. Параметры запуска: аргументы командной строки, путь, разрядность, дескрипторы.
 */
class ProcessInspectDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProcessInspectDialog(
        const pm::core::ProcessSnapshot& snapshot,
        QWidget* parent = nullptr
    );
    ~ProcessInspectDialog() override = default;

public slots:
    /**
     * @brief Заполнить таблицы инспекции детальными данными.
     */
    void setProcessDetails(
        std::vector<pm::core::ThreadSnapshot> threads,
        std::vector<pm::core::ModuleSnapshot> modules,
        std::vector<pm::core::HandleSnapshot> handles
    );

private:
    void setupGeneralTab(const pm::core::ProcessSnapshot& s);
    void setupModulesTab();
    void setupThreadsTab();

    QTabWidget* m_tabWidget{nullptr};
    QTableWidget* m_modulesTable{nullptr};
    QTableWidget* m_threadsTable{nullptr};
};

} // namespace pm::gui
