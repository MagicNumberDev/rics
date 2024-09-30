#pragma once
#include "Memory.hpp"
#include <cstdlib>
#include <limits>
#include <tuple>
#include <utility>


namespace rics {

template <typename T, typename SizeType = ::std::size_t, typename AllocatorType = ::rics::DefaultAllocator<SizeType>>
    requires ::rics::Allocator<SizeType, AllocatorType>
struct Vector {
private:
    T*                  data_     = nullptr;
    SizeType            size_     = 0;
    SizeType            capacity_ = 0;
    constexpr static T* allocate(SizeType count) {
        if consteval {
            return new T[count];
        } else {
            auto mem = AllocatorType::allocate(count * sizeof(T));
            return new (mem) T[count];
        }
    }
    constexpr static void deallocate(T* ptr) {
        if consteval {
            delete[] ptr;
        } else {
            AllocatorType::deallocate(ptr);
        }
    }

public:
    using value_type = T;
    using size_type  = SizeType;
    constexpr auto&       data() { return data_; }
    constexpr const auto& data() const { return data_; }
    constexpr auto        size() const { return size_; }
    constexpr Vector() {
        data_     = nullptr;
        size_     = 0;
        capacity_ = 0;
    }
    constexpr Vector(const Vector& o) {
        if (o.size_ == 0 || o.data_ == nullptr) return;
        delete_if_has_data();
        new_length(o.size_);
        for (size_type i = 0; i < o.size_; i++) data_[i] = o.data_[i];
    }
    constexpr Vector(Vector&& o) {
        size_ = o.size_;
        data_ = o.data_;
        o.delete_if_has_data();
    }
    template <typename... Ts>
    constexpr Vector(const Ts&&... args) {
        new_length(sizeof...(args));
        set_all(::std::make_index_sequence<sizeof...(args)>{}, args...);
    }
    constexpr Vector(SizeType size) { new_length(size); }
    constexpr ~Vector() { delete_if_has_data(); }
    constexpr auto&       operator[](size_type i) { return data_[i]; }
    constexpr const auto& operator[](size_type i) const { return data_[i]; }
    constexpr auto        begin() { return &data_[0]; }
    constexpr const auto  begin() const { return &data_[0]; }
    constexpr auto        end() { return &data_[size_]; }
    constexpr const auto  end() const { return &data_[size_]; }
    constexpr auto&       push_back(T&& v) {
        new_length<true>(size_ + 1);
        data_[size_ - 1] = std::move(v);
        return data_[size_ - 1];
    }
    constexpr auto pop_back() {
        auto t = std::move(data_[size_ - 1]);
        new_length<true>(size_ - 1);
        return t;
    }
    constexpr void resize(size_type s) { new_length<true>(s); }
    constexpr auto max_size() { return ::std::numeric_limits<size_type>::max(); }
    constexpr auto capacity() { return capacity_; }

private:
    constexpr void delete_if_has_data() {
        if (data_ != nullptr && size_ != 0) deallocate(data_);
        data_ = nullptr;
        size_ = 0;
    }
    template <bool CopyOldData = false>
    constexpr void new_length(size_type l) {
        if (capacity_ == 0) {
            data_     = allocate(2);
            capacity_ = 2;
        }
        if (l < capacity_) {
            size_ = l;
            return;
        }
        auto temp_data = data_;
        data_          = nullptr;
        if (capacity_ + capacity_ / 2 > max_size()) {
            data_     = allocate(max_size());
            capacity_ = max_size();
        } else {
            data_     = allocate(capacity_ + capacity_ / 2);
            capacity_ = capacity_ + capacity_ / 2;
        }
        if constexpr (CopyOldData) {
            if (temp_data != nullptr)
                for (size_type i = 0; i < std::min(size_, l); i++) {
                    data_[i] = std::move(temp_data[i]);
                }
        }
        deallocate(temp_data);
        size_ = l;
    }
    template <auto I>
    constexpr auto set(const auto& args) {
        data_[I] = ::std::get<I>(args);
    }
    template <auto... I>
    constexpr auto set_all(const ::std::index_sequence<I...>&, const auto&... args) {
        (set<I>(::std::forward_as_tuple(args...)), ...);
    }
};
template <typename T, typename... Ts>
Vector(T first, Ts... rest) -> Vector<::std::remove_cvref_t<T>>;
} // namespace rics
