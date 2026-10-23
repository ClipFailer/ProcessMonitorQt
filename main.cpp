#include "core/MetaTypes.hpp"
#include "gui/MainWindow.hpp"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[]) {
    // 1. Инициализация приложения Qt
    QApplication app(argc, argv);
    app.setApplicationName("ProcessMonitor");
    app.setApplicationDisplayName("Process Monitor");
    app.setOrganizationName("ProcessMonitor");

    // 2. Регистрация пользовательских структур в Qt MetaObject System
    pm::core::registerMetaTypes();

    // 3. Создание и отображение главного окна
    pm::gui::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
