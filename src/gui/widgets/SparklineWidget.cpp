#include "SparklineWidget.hpp"
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QLinearGradient>
#include <algorithm>

namespace pm::gui {

SparklineWidget::SparklineWidget(QColor lineColor, QWidget* parent)
    : QWidget(parent)
    , m_lineColor(lineColor)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(80, 24);
}

void SparklineWidget::addPoint(double value) {
    m_history.push(value);
    update();
}

void SparklineWidget::clear() {
    m_history.clear();
    update();
}

void SparklineWidget::setFixedMax(double maxVal) {
    m_fixedMax = maxVal;
    update();
}

void SparklineWidget::setLineColor(const QColor& color) {
    m_lineColor = color;
    update();
}

void SparklineWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int w = width();
    const int h = height();
    if (w <= 2 || h <= 2) {
        return;
    }

    const std::size_t count = m_history.size();
    if (count < 2) {
        return;
    }

    // Определяем максимальное значение для масштабирования
    double maxVal = m_fixedMax > 0.0 ? m_fixedMax : 1.0;
    if (m_fixedMax <= 0.0) {
        for (std::size_t i = 0; i < count; ++i) {
            maxVal = std::max(maxVal, m_history[i]);
        }
    }
    if (maxVal <= 0.0001) {
        maxVal = 1.0;
    }

    // Построение пути
    const double stepX = static_cast<double>(w) / static_cast<double>(SparklineWidget::DefaultCapacity - 1);
    const double startX = static_cast<double>(w) - (static_cast<double>(count - 1) * stepX);

    QPainterPath path;
    const double firstY = static_cast<double>(h) - (std::clamp(m_history[0] / maxVal, 0.0, 1.0) * static_cast<double>(h - 4)) - 2.0;
    path.moveTo(startX, firstY);

    for (std::size_t i = 1; i < count; ++i) {
        const double x = startX + static_cast<double>(i) * stepX;
        const double y = static_cast<double>(h) - (std::clamp(m_history[i] / maxVal, 0.0, 1.0) * static_cast<double>(h - 4)) - 2.0;
        path.lineTo(x, y);
    }

    // Полупрозрачная градиентная заливка под графиком
    QPainterPath fillPath = path;
    const double lastX = startX + static_cast<double>(count - 1) * stepX;
    fillPath.lineTo(lastX, static_cast<double>(h));
    fillPath.lineTo(startX, static_cast<double>(h));
    fillPath.closeSubpath();

    QLinearGradient fillGrad(0, 0, 0, h);
    QColor fillTop = m_lineColor;
    fillTop.setAlpha(60);
    QColor fillBottom = m_lineColor;
    fillBottom.setAlpha(5);
    fillGrad.setColorAt(0.0, fillTop);
    fillGrad.setColorAt(1.0, fillBottom);

    painter.setPen(Qt::NoPen);
    painter.setBrush(fillGrad);
    painter.drawPath(fillPath);

    // Отрисовка основной линии
    QPen linePen(m_lineColor, 1.5);
    linePen.setCapStyle(Qt::RoundCap);
    linePen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(linePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

} // namespace pm::gui
