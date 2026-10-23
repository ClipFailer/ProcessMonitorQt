#pragma once

#include "core/Types.hpp"
#include <memory>
#include <vector>

namespace pm::gui {

/**
 * @brief Узел дерева процессов в модели ProcessTreeModel.
 * 
 * Управляет связями предок-потомок и хранит актуальный снимок процесса.
 * Дочерние элементы хранятся в векторе умных указателей `std::unique_ptr<ProcessTreeItem>`,
 * обеспечивая автоматическое освобождение памяти без утечек (RAII).
 */
class ProcessTreeItem {
public:
    /**
     * @brief Конструктор узла дерева процессов.
     * @param[in] snapshot Снимок телеметрии процесса.
     * @param[in] parent Указатель на родительский узел (nullptr для фиктивного корня).
     */
    explicit ProcessTreeItem(pm::core::ProcessSnapshot snapshot, ProcessTreeItem* parent = nullptr);

    ~ProcessTreeItem() = default;

    ProcessTreeItem(const ProcessTreeItem&) = delete;
    ProcessTreeItem& operator=(const ProcessTreeItem&) = delete;
    ProcessTreeItem(ProcessTreeItem&&) noexcept = default;
    ProcessTreeItem& operator=(ProcessTreeItem&&) noexcept = default;

    /**
     * @brief Добавить дочерний узел в конец списка детей.
     * @param[in] child Умный указатель на перемещаемый дочерний узел.
     */
    void appendChild(std::unique_ptr<ProcessTreeItem> child);

    /**
     * @brief Извлечь и удалить дочерний узел по индексу строки.
     * @param[in] row Индекс удаляемой строки.
     * @return std::unique_ptr<ProcessTreeItem> Удаленный узел.
     */
    std::unique_ptr<ProcessTreeItem> removeChild(int row);

    /**
     * @brief Получить дочерний элемент по индексу.
     * @param[in] row Индекс дочерней строки.
     * @return ProcessTreeItem* Указатель на узел или nullptr.
     */
    [[nodiscard]] ProcessTreeItem* child(int row) const noexcept;

    /**
     * @brief Количество прямых потомков узла.
     */
    [[nodiscard]] int childCount() const noexcept;

    /**
     * @brief Получить родительский узел.
     */
    [[nodiscard]] ProcessTreeItem* parentItem() const noexcept;

    /**
     * @brief Получить индекс текущего узла среди детей родителя (номер строки).
     */
    [[nodiscard]] int row() const noexcept;

    /**
     * @brief Получить доступ к снимку процесса.
     */
    [[nodiscard]] const pm::core::ProcessSnapshot& snapshot() const noexcept;

    /**
     * @brief Обновить снимок процесса.
     * @param[in] newSnapshot Новые данные телеметрии.
     */
    void updateSnapshot(pm::core::ProcessSnapshot newSnapshot) noexcept;

private:
    pm::core::ProcessSnapshot m_snapshot;
    ProcessTreeItem* m_parentItem{nullptr};
    std::vector<std::unique_ptr<ProcessTreeItem>> m_childItems;
};

} // namespace pm::gui
