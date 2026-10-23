#pragma once

#include "ProcessTreeItem.hpp"
#include <QtCore/QAbstractItemModel>
#include <unordered_map>
#include <memory>
#include <vector>

namespace pm::gui {

/**
 * @brief Иерархическая модель данных дерева процессов с поддержкой Smart Diff обновления.
 * 
 * Предоставляет древовидную структуру отношений Parent PID -> Child PID.
 * При каждом тике обновления модель не сбрасывает дерево через beginResetModel(),
 * а точечно обновляет значения изменившихся ячеек через dataChanged(), вставляет
 * новые узлы через beginInsertRows() и удаляет завершившиеся процессы через beginRemoveRows(),
 * сохраняя фокус, состояние раскрытия веток и позицию скролла в представлении.
 */
class ProcessTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    /**
     * @brief Перечисление столбцов модели дерева процессов.
     */
    enum Column : int {
        ColName = 0,        ///< Имя процесса
        ColPid = 1,         ///< Идентификатор процесса (PID)
        ColCpu = 2,         ///< Мгновенная нагрузка на CPU (%)
        ColWorkingSet = 3,  ///< Рабочий набор физической памяти (RSS)
        ColPrivateBytes = 4,///< Выделенная приватная память (Commit)
        ColThreads = 5,     ///< Количество активных потоков
        ColHandles = 6,     ///< Количество открытых дескрипторов
        ColIoRead = 7,      ///< Скорость чтения с диска (B/s)
        ColIoWrite = 8,     ///< Скорость записи на диск (B/s)
        ColPriority = 9,    ///< Класс приоритета
        ColPath = 10,       ///< Полный путь к файлу
        ColumnCount = 11    ///< Общее число столбцов
    };

    /**
     * @brief Пользовательские роли модели.
     */
    enum ItemRole : int {
        RawValueRole = Qt::UserRole + 1,       ///< Сырое числовое значение (для правильной сортировки и делегатов)
        LifecycleRole = Qt::UserRole + 2,      ///< Состояние жизненного цикла (Active, New, Terminating)
        LifecycleCounterRole = Qt::UserRole + 3,///< Счетчик для анимации затухания подсветки
        PidRole = Qt::UserRole + 4             ///< Числовой PID процесса
    };

    explicit ProcessTreeModel(QObject* parent = nullptr);
    ~ProcessTreeModel() override = default;

    // --- Методы QAbstractItemModel ---
    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;

    /**
     * @brief Получить снимок процесса по индексу модели.
     * @param[in] index Индекс в модели.
     * @return const pm::core::ProcessSnapshot* Указатель на снимок или nullptr.
     */
    [[nodiscard]] const pm::core::ProcessSnapshot* processAt(const QModelIndex& index) const noexcept;

    /**
     * @brief Найти индекс процесса по его PID.
     * @param[in] pid Идентификатор процесса.
     * @return QModelIndex Индекс узла или недопустимый индекс, если процесс не найден.
     */
    [[nodiscard]] QModelIndex indexForPid(pm::core::ProcessId pid) const;

public slots:
    /**
     * @brief Точечное обновление снимка процессов (Smart Diff).
     * @param[in] processes Вектор актуальных снимков процессов из фонового воркера.
     */
    void updateProcesses(std::vector<pm::core::ProcessSnapshot> processes);

private:
    /**
     * @brief Получить указатель на ProcessTreeItem из QModelIndex.
     */
    [[nodiscard]] ProcessTreeItem* itemFromIndex(const QModelIndex& index) const noexcept;

    /**
     * @brief Сформировать индекс для узла.
     */
    [[nodiscard]] QModelIndex indexForItem(ProcessTreeItem* item, int column = 0) const;

    /**
     * @brief Форматирование размера в байтах в человекочитаемый вид (Б, КБ, МБ, ГБ).
     */
    [[nodiscard]] static QString formatBytes(std::uint64_t bytes);

    /**
     * @brief Преобразование перечисления приоритета в локализованный текст.
     */
    [[nodiscard]] static QString formatPriority(pm::core::ProcessPriority priority);

    std::unique_ptr<ProcessTreeItem> m_rootItem;                          ///< Фиктивный корень дерева
    std::unordered_map<pm::core::ProcessId, ProcessTreeItem*> m_itemMap;  ///< Быстрый индекс узлов по PID
};

} // namespace pm::gui
