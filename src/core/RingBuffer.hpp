#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vector>
#include <stdexcept>
#include <type_traits>

namespace pm::core {

/**
 * @brief Высокопроизводительный циклический буфер фиксированной емкости.
 * 
 * Буфер не производит динамических аллокаций памяти в куче при добавлении элементов,
 * размещая данные в непрерывном массиве фиксированного размера на стеке или внутри объекта.
 * При заполнении старые элементы автоматически перезаписываются новыми (FIFO semantics).
 * 
 * @tparam T Тип хранимого значения (должен быть перемещаемым или копируемым).
 * @tparam Capacity Максимальная фиксированная емкость буфера.
 */
template <typename T, std::size_t Capacity>
class RingBuffer {
    static_assert(Capacity > 0, "Capacity кольцевого буфера должна быть больше 0");

public:
    /**
     * @brief Конструктор по умолчанию.
     */
    constexpr RingBuffer() noexcept = default;

    /**
     * @brief Добавить новый элемент в конец буфера.
     * @details Если буфер заполнен, старейший элемент перезаписывается. Сложность: O(1).
     * @param[in] value Добавляемое значение (lvalue).
     */
    void push(const T& value) {
        m_buffer[m_head] = value;
        advance();
    }

    /**
     * @brief Добавить новый элемент в конец буфера с использованием move-семантики.
     * @details Если буфер заполнен, старейший элемент перезаписывается. Сложность: O(1).
     * @param[in,out] value Добавляемое значение (rvalue).
     */
    void push(T&& value) {
        m_buffer[m_head] = std::move(value);
        advance();
    }

    /**
     * @brief Сконструировать элемент прямо на месте последнего слота.
     * @tparam Args Типы аргументов конструктора типа T.
     * @param[in] args Аргументы конструктора.
     */
    template <typename... Args>
    void emplace(Args&&... args) {
        m_buffer[m_head] = T(std::forward<Args>(args)...);
        advance();
    }

    /**
     * @brief Очистить содержимое буфера.
     */
    void clear() noexcept {
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

    /**
     * @brief Текущее количество элементов в буфере.
     * @return std::size_t Количество активных элементов (от 0 до Capacity).
     */
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return m_size;
    }

    /**
     * @brief Максимальная емкость буфера.
     * @return constexpr std::size_t Значение Capacity.
     */
    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return Capacity;
    }

    /**
     * @brief Проверка буфера на пустоту.
     * @return true если буфер пуст, иначе false.
     */
    [[nodiscard]] constexpr bool empty() const noexcept {
        return m_size == 0;
    }

    /**
     * @brief Проверка буфера на полное заполнение.
     * @return true если количество элементов равно Capacity, иначе false.
     */
    [[nodiscard]] constexpr bool full() const noexcept {
        return m_size == Capacity;
    }

    /**
     * @brief Доступ к элементу по логическому индексу от старейшего к новейшему.
     * @note Индекс 0 соответствует старейшему доступному элементу, индекс (size() - 1) — новейшему.
     * @param[in] index Логический индекс (0 <= index < size()).
     * @return Константная ссылка на элемент.
     * @throws std::out_of_range При выходе индекса за границы size().
     */
    [[nodiscard]] const T& at(std::size_t index) const {
        if (index >= m_size) {
            throw std::out_of_range("RingBuffer::at: индекс выходит за пределы текущего размера");
        }
        return m_buffer[(m_tail + index) % Capacity];
    }

    /**
     * @brief Доступ к элементу по логическому индексу без проверки границ.
     * @param[in] index Логический индекс (0 <= index < size()).
     * @return Константная ссылка на элемент.
     */
    [[nodiscard]] const T& operator[](std::size_t index) const noexcept {
        return m_buffer[(m_tail + index) % Capacity];
    }

    /**
     * @brief Получить самый свежий (последний добавленный) элемент.
     * @return Ссылка на новейший элемент.
     */
    [[nodiscard]] const T& back() const {
        if (empty()) {
            throw std::out_of_range("RingBuffer::back: буфер пуст");
        }
        return operator[](m_size - 1);
    }

    /**
     * @brief Преобразовать хронологическую последовательность в вектор (от старых к новым).
     * @return std::vector<T> Вектор с упорядоченными данными для построения графиков.
     */
    [[nodiscard]] std::vector<T> toVector() const {
        std::vector<T> result;
        result.reserve(m_size);
        for (std::size_t i = 0; i < m_size; ++i) {
            result.push_back(operator[](i));
        }
        return result;
    }

private:
    /**
     * @brief Продвинуть указатель головы вперед при добавлении элемента.
     */
    void advance() noexcept {
        m_head = (m_head + 1) % Capacity;
        if (m_size < Capacity) {
            ++m_size;
        } else {
            // Если буфер уже был полон, хвост сдвигается вместе с головой
            m_tail = (m_tail + 1) % Capacity;
        }
    }

    std::array<T, Capacity> m_buffer{}; ///< Фиксированное хранилище данных
    std::size_t m_head{0};              ///< Индекс для записи следующего элемента
    std::size_t m_tail{0};              ///< Индекс старейшего существующего элемента
    std::size_t m_size{0};              ///< Текущее количество элементов в буфере
};

} // namespace pm::core
