#pragma once

#include <QtGui/QColor>
#include <QtCore/QString>

namespace pm::gui::theme {

// Цветовые токены современной темной минималистичной темы (Slate Dark)
inline constexpr QColor BgPrimary{0x12, 0x12, 0x14};     // Основной фон (#121214)
inline constexpr QColor BgCard{0x18, 0x18, 0x1c};        // Карточки и дашборд (#18181c)
inline constexpr QColor BgHeader{0x1e, 0x1e, 0x24};      // Заголовки таблиц (#1e1e24)
inline constexpr QColor BgInput{0x22, 0x22, 0x2a};       // Поля ввода (#22222a)
inline constexpr QColor BgHover{0x2a, 0x2a, 0x36};       // Ховер строк (#2a2a36)
inline constexpr QColor BgSelected{0x25, 0x63, 0xeb, 0x50}; // Выделенная строка (полупрозрачный синий)

inline constexpr QColor BorderSubtle{0x2c, 0x2c, 0x36};  // Тонкие рамки (#2c2c36)

inline constexpr QColor TextPrimary{0xf4, 0xf4, 0xf5};   // Основной яркий текст (#f4f4f5)
inline constexpr QColor TextMuted{0xa1, 0xa1, 0xaa};     // Вторичный серый текст (#a1a1aa)

inline constexpr QColor AccentCpu{0x38, 0xbd, 0xf8};     // Небесно-голубой (#38bdf8) для CPU
inline constexpr QColor AccentRam{0x34, 0xd3, 0x99};     // Изумрудно-зеленый (#34d399) для RAM
inline constexpr QColor AccentDisk{0xa7, 0x8b, 0xfa};    // Мягкий фиолетовый (#a78bfa) для Диска
inline constexpr QColor AccentNet{0xfb, 0x92, 0x3c};     // Теплый оранжевый (#fb923c) для Сети

inline constexpr QColor StateNew{0x10, 0xb9, 0x81};      // Зеленый для новых процессов
inline constexpr QColor StateTerminating{0xef, 0x44, 0x44}; // Красный для завершающихся

/**
 * @brief Генерация глобального QSS стиля приложения в минималистичном темном дизайне.
 */
inline QString getDarkStyleSheet() {
    return QString::fromUtf8(R"qss(
        QWidget {
            background-color: #121214;
            color: #f4f4f5;
            font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;
            font-size: 13px;
            selection-background-color: #3b82f6;
            selection-color: #ffffff;
        }

        QMainWindow {
            background-color: #121214;
        }

        QTreeView {
            background-color: #151518;
            border: 1px solid #27272f;
            border-radius: 6px;
            outline: none;
            show-decoration-selected: 1;
        }

        QTreeView::item {
            padding: 4px 6px;
            border-bottom: 1px solid #1a1a20;
        }

        QTreeView::item:hover {
            background-color: #22222b;
        }

        QTreeView::item:selected {
            background-color: #1e3a8a;
            color: #ffffff;
        }

        QHeaderView::section {
            background-color: #1a1a20;
            color: #a1a1aa;
            padding: 6px 8px;
            border: none;
            border-right: 1px solid #24242e;
            border-bottom: 1px solid #27272f;
            font-weight: 600;
            font-size: 12px;
        }

        QHeaderView::section:hover {
            background-color: #24242e;
            color: #f4f4f5;
        }

        QLineEdit {
            background-color: #1b1b22;
            color: #f4f4f5;
            border: 1px solid #2c2c38;
            border-radius: 6px;
            padding: 5px 12px;
            selection-background-color: #3b82f6;
        }

        QLineEdit:focus {
            border: 1px solid #38bdf8;
            background-color: #202029;
        }

        QComboBox {
            background-color: #1b1b22;
            color: #f4f4f5;
            border: 1px solid #2c2c38;
            border-radius: 6px;
            padding: 5px 10px;
        }

        QComboBox:hover {
            border: 1px solid #38bdf8;
        }

        QComboBox QAbstractItemView {
            background-color: #1c1c24;
            border: 1px solid #2e2e3a;
            selection-background-color: #2563eb;
            color: #f4f4f5;
        }

        QPushButton {
            background-color: #22222c;
            color: #f4f4f5;
            border: 1px solid #2e2e3a;
            border-radius: 6px;
            padding: 5px 14px;
            font-weight: 500;
        }

        QPushButton:hover {
            background-color: #2d2d3a;
            border: 1px solid #38bdf8;
        }

        QPushButton:pressed {
            background-color: #1b1b24;
        }

        QScrollBar:vertical {
            background: #141418;
            width: 8px;
            margin: 0;
            border-radius: 4px;
        }

        QScrollBar::handle:vertical {
            background: #333342;
            min-height: 20px;
            border-radius: 4px;
        }

        QScrollBar::handle:vertical:hover {
            background: #4b4b5e;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
            background: none;
        }

        QScrollBar:horizontal {
            background: #141418;
            height: 8px;
            margin: 0;
            border-radius: 4px;
        }

        QScrollBar::handle:horizontal {
            background: #333342;
            min-width: 20px;
            border-radius: 4px;
        }

        QScrollBar::handle:horizontal:hover {
            background: #4b4b5e;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
            background: none;
        }

        QMenu {
            background-color: #1c1c24;
            color: #f4f4f5;
            border: 1px solid #2e2e3a;
            border-radius: 6px;
            padding: 4px;
        }

        QMenu::item {
            padding: 6px 20px;
            border-radius: 4px;
        }

        QMenu::item:selected {
            background-color: #2563eb;
            color: #ffffff;
        }

        QMenu::separator {
            height: 1px;
            background-color: #2c2c38;
            margin: 4px 6px;
        }

        QTabWidget::pane {
            border: 1px solid #27272f;
            border-radius: 6px;
            background-color: #151518;
        }

        QTabBar::tab {
            background-color: #18181c;
            color: #a1a1aa;
            padding: 8px 16px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            margin-right: 2px;
        }

        QTabBar::tab:selected {
            background-color: #22222b;
            color: #38bdf8;
            font-weight: 600;
        }

        QStatusBar {
            background-color: #141418;
            color: #a1a1aa;
            border-top: 1px solid #24242e;
        }
    )qss");
}

} // namespace pm::gui::theme
