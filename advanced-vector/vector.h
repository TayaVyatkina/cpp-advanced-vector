#pragma once
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept
        : buffer_(std::exchange(other.buffer_, nullptr))
        , capacity_(std::exchange(other.capacity_, 0))
    {}

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            buffer_ = std::move(rhs.buffer_);
            capacity_ = std::move(rhs.capacity_);
            rhs.buffer_ = nullptr;
            rhs.capacity_ = 0;
        }

        return *this;
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    // Конструктор по умолчанию.Инициализирует вектор нулевого размера и вместимости.
    // Не выбрасывает исключений.Алгоритмическая сложность : O(1).
    Vector() = default;

    // Конструктор, который создаёт вектор заданного размера.
    // Вместимость созданного вектора равна его размеру, а элементы проинициализированы значением по умолчанию для типа T.
    // Алгоритмическая сложность : O(размер вектора).
    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }

    // Копирующий конструктор.Создаёт копию элементов исходного вектора.
    // Имеет вместимость, равную размеру исходного вектора, то есть выделяет память без запаса.
    // Алгоритмическая сложность : O(размер исходного вектора).
    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    // Деструктор.Разрушает содержащиеся в векторе элементы и освобождает занимаемую ими память.
    // Алгоритмическая сложность : O(размер вектора).
    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    // Перемещающий конструктор. Выполняется за O(1) и не выбрасывает исключений
    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_(std::exchange(other.size_, 0))
    {}

    Vector& operator=(const Vector& other) {

        if (this != &other) {

            if (other.size_ <= data_.Capacity()) {

                if (size_ <= other.size_) {

                    std::copy(other.data_.GetAddress(),
                        other.data_.GetAddress() + size_,
                        data_.GetAddress());

                    std::uninitialized_copy_n(other.data_.GetAddress() + size_,
                        other.size_ - size_,
                        data_.GetAddress() + size_);
                }
                else {

                    std::copy(other.data_.GetAddress(),
                        other.data_.GetAddress() + other.size_,
                        data_.GetAddress());

                    std::destroy_n(data_.GetAddress() + other.size_,
                        size_ - other.size_);
                }

                size_ = other.size_;

            }
            else {
                Vector other_copy(other);
                Swap(other_copy);
            }
        }

        return *this;
    }

    // Оператор перемещающего присваивания. Выполняется за O(1) и не выбрасывает исключений.
    Vector& operator=(Vector&& rhs) noexcept {
        Swap(rhs); return *this;
    }

    // Метод Swap, выполняющий обмен содержимого вектора с другим вектором. 
    // Операция должна иметь сложность O(1) и не выбрасывать исключений.
    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_),
            std::swap(size_, other.size_);
    }

    // Метод void Reserve(size_t capacity).Резервирует достаточно места, чтобы вместить количество элементов, равное capacity.
    // Если новая вместимость не превышает текущую, метод не делает ничего. Алгоритмическая сложность : O(размер вектора).
    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);

        // constexpr оператор if будет вычислен во время компиляции
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (new_size < Size()) {
            std::destroy_n(data_.GetAddress() + new_size, Size() - new_size);
            size_ = new_size;
        }
        else if (new_size > Size()) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + Size(), new_size - Size());
            size_ = new_size;
        }
    }

    // Реализация похожа на PushBack, только вместо копирования или перемещения
    //переданного элемента, он конструируется путём передачи параметров метода конструктору T
    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            std::construct_at(new_data.GetAddress() + Size(), std::forward<Args>(args)...);

            try {
                MoveArgs(data_.GetAddress(), size_, new_data.GetAddress());
            }
            catch (...) {
                std::destroy_at(new_data.GetAddress() + Size());
                throw;
            }

            data_.Swap(new_data);
        }
        else {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return *(data_.GetAddress() + Size() - 1);
    }

    void PushBack(T&& value) {
        this->EmplaceBack(std::move(value));
    }

    void PushBack(const T& value) {
        this->EmplaceBack(value);
    }

    void PopBack() /* noexcept */ {
        assert(Size());
        Resize(Size() - 1);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + Size();
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_.GetAddress() + Size();
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return data_.GetAddress() + Size();
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(pos >= begin() && pos <= end());
        if (pos == end()) {
            return &EmplaceBack(std::forward<Args>(args)...);
        }
        int index_for_insert = pos - begin();
        if (Size() == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            std::construct_at(new_data.GetAddress() + index_for_insert, std::forward<Args>(args)...);

            try {
                MoveArgs(begin(), index_for_insert, new_data.GetAddress());
            }
            catch (...) {
                std::destroy_at(new_data.GetAddress() + index_for_insert);
                throw;
            }

            try {
                MoveArgs(begin() + index_for_insert, Size() - index_for_insert, new_data.GetAddress() + index_for_insert + 1);
            }
            catch (...) {
                std::destroy_n(new_data.GetAddress(), index_for_insert + 1);
                throw;
            }
            data_.Swap(new_data);

        }
        else {
            T new_values(std::forward<Args>(args)...);
            new (end()) T(std::forward<T>(*(end() - 1)));
            std::move_backward(begin() + index_for_insert, end() - 1, end());
            *(begin() + index_for_insert) = std::move(new_values);
        }
        size_++;
        return begin() + index_for_insert;
    }

    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/ {
        assert(pos >= begin() && pos <= end());
        int index_for_erase = pos - begin();
        std::move(begin() + index_for_erase + 1, end(), begin() + index_for_erase);
        this->PopBack();
        return begin() + index_for_erase;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    void MoveArgs(T* from, int size, T* to) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(from, size, to);
        }
        else {
            std::uninitialized_copy_n(from, size, to);
        }
        std::destroy_n(from, size);
    }
    // Вместо этой функции лучше использовать std::destroy_n.
    //// Вызывает деструкторы n объектов массива по адресу buf
    //static void DestroyN(T* buf, size_t n) noexcept {
    //    for (size_t i = 0; i != n; ++i) {
    //        Destroy(buf + i);
    //    }
    //}

    //// Создаёт копию объекта elem в сырой памяти по адресу buf
    //static void CopyConstruct(T* buf, const T& elem) {
    //    new (buf) T(elem);
    //}

    // Вместо этой функции лучше использовать std::destroy_at.
    //// Вызывает деструктор объекта по адресу buf
    //static void Destroy(T* buf) noexcept {
    //    buf->~T();
    //}

};