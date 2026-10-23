#pragma once

#include "core/Types.hpp"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>

namespace pm::gui {

class SparklineWidget;

/**
 * @brief Верхняя панель дашборда с карточками общесистемной статистики в реальном времени.
 * 
 * Отображает:
 * 1. Карточку CPU (процент, частота/ядра, мини-график истории).
 * 2. Карточку RAM (занято/всего, процент, мини-график).
 * 3. Карточку Диска (скорость чтения и записи).
 * 4. Карточку Сети (скорость приема и передачи).
 */
class SystemSummaryBar : public QWidget {
    Q_OBJECT

public:
    explicit SystemSummaryBar(QWidget* parent = nullptr);
    ~SystemSummaryBar() override = default;

public slots:
    /**
     * @brief Обновить карточки новыми данными телеметрии.
     * @param[in] snapshot Общесистемный снимок.
     */
    void updateMetrics(const pm::core::SystemSnapshot& snapshot);

private:
    [[nodiscard]] QWidget* createCard(
        const QString& title,
        QLabel*& mainLabel,
        QLabel*& subLabel,
        SparklineWidget*& sparkline,
        const QColor& accentColor
    );

    QLabel* m_cpuMainLabel{nullptr};
    QLabel* m_cpuSubLabel{nullptr};
    SparklineWidget* m_cpuSparkline{nullptr};

    QLabel* m_ramMainLabel{nullptr};
    QLabel* m_ramSubLabel{nullptr};
    SparklineWidget* m_ramSparkline{nullptr};

    QLabel* m_diskMainLabel{nullptr};
    QLabel* m_diskSubLabel{nullptr};
    SparklineWidget* m_diskSparkline{nullptr};

    QLabel* m_netMainLabel{nullptr};
    QLabel* m_netSubLabel{nullptr};
    SparklineWidget* m_netSparkline{nullptr};
};

} // namespace pm::gui
