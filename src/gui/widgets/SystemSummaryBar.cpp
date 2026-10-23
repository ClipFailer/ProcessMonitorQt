#include "SystemSummaryBar.hpp"
#include "SparklineWidget.hpp"
#include "theme/Theme.hpp"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QFrame>

namespace pm::gui {

namespace {

QString formatBytesSec(std::uint64_t bytes) {
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;

    const auto d = static_cast<double>(bytes);
    if (d >= GB) return QString::asprintf("%.1f GB/s", d / GB);
    if (d >= MB) return QString::asprintf("%.1f MB/s", d / MB);
    if (d >= KB) return QString::asprintf("%.0f KB/s", d / KB);
    return QString::asprintf("%llu B/s", static_cast<unsigned long long>(bytes));
}

QString formatGigabytes(std::uint64_t bytes) {
    constexpr double GB = 1024.0 * 1024.0 * 1024.0;
    return QString::asprintf("%.1f GB", static_cast<double>(bytes) / GB);
}

} // namespace

SystemSummaryBar::SystemSummaryBar(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // 1. Карточка CPU
    layout->addWidget(createCard(
        tr("ПРОЦЕССОР"),
        m_cpuMainLabel,
        m_cpuSubLabel,
        m_cpuSparkline,
        theme::AccentCpu
    ));
    m_cpuSparkline->setFixedMax(100.0);

    // 2. Карточка RAM
    layout->addWidget(createCard(
        tr("ПАМЯТЬ"),
        m_ramMainLabel,
        m_ramSubLabel,
        m_ramSparkline,
        theme::AccentRam
    ));
    m_ramSparkline->setFixedMax(100.0);

    // 3. Карточка Disk
    layout->addWidget(createCard(
        tr("ДИСК"),
        m_diskMainLabel,
        m_diskSubLabel,
        m_diskSparkline,
        theme::AccentDisk
    ));

    // 4. Карточка Network
    layout->addWidget(createCard(
        tr("СЕТЬ"),
        m_netMainLabel,
        m_netSubLabel,
        m_netSparkline,
        theme::AccentNet
    ));
}

QWidget* SystemSummaryBar::createCard(
    const QString& title,
    QLabel*& mainLabel,
    QLabel*& subLabel,
    SparklineWidget*& sparkline,
    const QColor& accentColor)
{
    auto* card = new QFrame(this);
    card->setObjectName("SummaryCard");
    card->setStyleSheet(
        "QFrame#SummaryCard {"
        "   background-color: #17171c;"
        "   border: 1px solid #272733;"
        "   border-radius: 8px;"
        "   padding: 4px;"
        "}"
    );

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 8, 12, 8);
    cardLayout->setSpacing(4);

    // Заголовок
    auto* titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet(QString("color: %1; font-weight: 700; font-size: 11px; letter-spacing: 0.5px;")
                              .arg(accentColor.name()));

    // Основное крупное значение
    mainLabel = new QLabel("--", card);
    mainLabel->setStyleSheet("font-size: 18px; font-weight: 700; color: #f4f4f5;");

    // Вторичные детали
    subLabel = new QLabel("--", card);
    subLabel->setStyleSheet("font-size: 11px; color: #a1a1aa;");

    // Мини-график
    sparkline = new SparklineWidget(accentColor, card);
    sparkline->setFixedHeight(32);

    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(mainLabel);
    cardLayout->addWidget(subLabel);
    cardLayout->addWidget(sparkline);

    return card;
}

void SystemSummaryBar::updateMetrics(const pm::core::SystemSnapshot& snapshot) {
    // 1. CPU
    m_cpuMainLabel->setText(QString::asprintf("%.1f%%", snapshot.totalCpuPercent));
    m_cpuSubLabel->setText(tr("Ядер: %1 | User: %.0f%% | Kernel: %.0f%%")
                           .arg(snapshot.logicalCoresCount)
                           .arg(snapshot.userCpuPercent)
                           .arg(snapshot.kernelCpuPercent));
    m_cpuSparkline->addPoint(snapshot.totalCpuPercent);

    // 2. RAM
    const double ramPercent = snapshot.totalPhysicalRam > 0
        ? (static_cast<double>(snapshot.usedPhysicalRam) * 100.0 / static_cast<double>(snapshot.totalPhysicalRam))
        : 0.0;
    m_ramMainLabel->setText(QString::asprintf("%.1f%%", ramPercent));
    m_ramSubLabel->setText(tr("%1 / %2 (Swap: %3)")
                           .arg(formatGigabytes(snapshot.usedPhysicalRam))
                           .arg(formatGigabytes(snapshot.totalPhysicalRam))
                           .arg(formatGigabytes(snapshot.usedPagefile)));
    m_ramSparkline->addPoint(ramPercent);

    // 3. Диск
    const uint64_t totalDiskIo = snapshot.diskReadBytesPerSec + snapshot.diskWriteBytesPerSec;
    m_diskMainLabel->setText(formatBytesSec(totalDiskIo));
    m_diskSubLabel->setText(tr("R: %1 | W: %2")
                            .arg(formatBytesSec(snapshot.diskReadBytesPerSec))
                            .arg(formatBytesSec(snapshot.diskWriteBytesPerSec)));
    m_diskSparkline->addPoint(static_cast<double>(totalDiskIo));

    // 4. Сеть
    const uint64_t totalNetIo = snapshot.netRxBytesPerSec + snapshot.netTxBytesPerSec;
    m_netMainLabel->setText(formatBytesSec(totalNetIo));
    m_netSubLabel->setText(tr("↓ %1 | ↑ %2")
                           .arg(formatBytesSec(snapshot.netRxBytesPerSec))
                           .arg(formatBytesSec(snapshot.netTxBytesPerSec)));
    m_netSparkline->addPoint(static_cast<double>(totalNetIo));
}

} // namespace pm::gui
