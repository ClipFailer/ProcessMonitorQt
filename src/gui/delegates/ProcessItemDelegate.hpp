#pragma once

#include <QtWidgets/QStyledItemDelegate>

namespace pm::gui {

/**
 * @brief Пользовательский делегат отрисовки ячеек дерева процессов.
 * 
 * Отвечает за:
 * 1. Индикацию загрузки CPU (интерактивный мини progress-bar внутри ячейки).
 * 2. Цветовую подсветку состояния жизненного цикла (зеленый для новых процессов, красный для завершающихся).
 * 3. Форматированное выравнивание чисел и текста.
 */
class ProcessItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit ProcessItemDelegate(QObject* parent = nullptr);
    ~ProcessItemDelegate() override = default;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

} // namespace pm::gui
