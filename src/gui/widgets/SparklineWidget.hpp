#pragma once

#include "core/RingBuffer.hpp"
#include <QtWidgets/QWidget>
#include <QtGui/QColor>

namespace pm::gui {

/**
 * @brief Легковесный векторный мини-график нагрузки (Sparkline) на базе QPainter.
 * 
 * Особенности:
 * - Внутренний фиксированный кольцевой буфер без аллокаций.
 * - Сглаженные линии (Antialiasing) и полупрозрачная градиентная заливка под графиком.
 * - Настраиваемый диапазон шкалы (автоматический или фиксированный 0..100%).
 */
class SparklineWidget : public QWidget {
    Q_OBJECT

public:
    static constexpr std::size_t DefaultCapacity = 60;

    explicit SparklineWidget(
        QColor lineColor = QColor(56, 189, 248),
        QWidget* parent = nullptr
    );
    ~SparklineWidget() override = default;

    /**
     * @brief Добавить новую точку данных на график.
     * @param[in] value Числовое значение.
     */
    void addPoint(double value);

    /**
     * @brief Очистить накопленную историю.
     */
    void clear();

    /**
     * @brief Задать фиксированный максимум шкалы (например 100.0 для CPU).
     * @param[in] maxVal Максимальное значение (0.0 для автомасштабирования).
     */
    void setFixedMax(double maxVal);

    /**
     * @brief Задать основной цвет линии.
     */
    void setLineColor(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    pm::core::RingBuffer<double, DefaultCapacity> m_history;
    QColor m_lineColor;
    double m_fixedMax{0.0};
};

} // namespace pm::gui
