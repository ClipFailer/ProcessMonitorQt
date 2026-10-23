#include "ProcessItemDelegate.hpp"
#include "models/ProcessTreeModel.hpp"
#include "theme/Theme.hpp"
#include <QtGui/QPainter>
#include <algorithm>

namespace pm::gui {

ProcessItemDelegate::ProcessItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QSize ProcessItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(std::max(size.height(), 24));
    return size;
}

void ProcessItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 1. Отрисовка подсветки жизненного цикла строки (зеленый/красный тинт)
    const int lifecycle = index.data(ProcessTreeModel::LifecycleRole).toInt();
    const int counter = index.data(ProcessTreeModel::LifecycleCounterRole).toInt();

    if (lifecycle == static_cast<int>(pm::core::ProcessLifecycleState::New)) {
        const int alpha = std::clamp(20 + counter * 20, 20, 80);
        painter->fillRect(option.rect, QColor(16, 185, 129, alpha));
    } else if (lifecycle == static_cast<int>(pm::core::ProcessLifecycleState::Terminating)) {
        painter->fillRect(option.rect, QColor(239, 68, 68, 60));
    } else if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, theme::BgSelected);
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect, theme::BgHover);
    }

    // 2. Отрисовка колонки ЦП с мини-баром заполненности
    if (index.column() == ProcessTreeModel::ColCpu) {
        const double cpuPercent = index.data(ProcessTreeModel::RawValueRole).toDouble();
        if (cpuPercent > 0.05) {
            const double factor = std::clamp(cpuPercent / 100.0, 0.0, 1.0);
            QRect barRect = option.rect.adjusted(4, 4, -4, -4);
            barRect.setWidth(static_cast<int>(barRect.width() * factor));

            // Плавный градиент для бара CPU
            QLinearGradient grad(barRect.topLeft(), barRect.topRight());
            if (cpuPercent > 50.0) {
                grad.setColorAt(0.0, QColor(245, 158, 11, 140)); // Amber
                grad.setColorAt(1.0, QColor(239, 68, 68, 180));  // Red
            } else {
                grad.setColorAt(0.0, QColor(56, 189, 248, 100)); // Sky Blue
                grad.setColorAt(1.0, QColor(56, 189, 248, 180));
            }

            painter->setPen(Qt::NoPen);
            painter->setBrush(grad);
            painter->drawRoundedRect(barRect, 3, 3);
        }

        // Текст поверх бара
        const QString text = index.data(Qt::DisplayRole).toString();
        painter->setPen(theme::TextPrimary);
        painter->drawText(option.rect.adjusted(6, 0, -6, 0), Qt::AlignRight | Qt::AlignVCenter, text);
        painter->restore();
        return;
    }

    // 3. Отрисовка стандартных колонок с правильным выравниванием
    const QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen(theme::TextPrimary);

    Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter;
    if (index.column() == ProcessTreeModel::ColPid ||
        index.column() == ProcessTreeModel::ColWorkingSet ||
        index.column() == ProcessTreeModel::ColPrivateBytes ||
        index.column() == ProcessTreeModel::ColThreads ||
        index.column() == ProcessTreeModel::ColHandles ||
        index.column() == ProcessTreeModel::ColIoRead ||
        index.column() == ProcessTreeModel::ColIoWrite) {
        align = Qt::AlignRight | Qt::AlignVCenter;
    }

    painter->drawText(option.rect.adjusted(6, 0, -6, 0), static_cast<int>(align), text);
    painter->restore();
}

} // namespace pm::gui
