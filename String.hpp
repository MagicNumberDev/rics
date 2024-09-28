#pragma once
#include "Memory.hpp"
#include <concepts>
#include <type_traits>

namespace rics {
template <typename T>
concept CharT = ::std::is_same_v<T, char> || ::std::is_same_v<T, char16_t> || ::std::is_same_v<T, char32_t> || ::std::is_same_v<T, wchar_t>;
template <CharT T = char, ::std::integral SizeType = ::std::size_t, typename AllocatorType = ::rics::DefaultAllocator<SizeType>>
    requires ::rics::Allocator<SizeType, AllocatorType>
class String {
    constexpr static SizeType small_str_max_size = sizeof(T*) / sizeof(T); // NOLINT(bugprone-sizeof-expression)
    union {
        T* large;
        T  small[small_str_max_size]{0};
    } data;
    SizeType            size;
    constexpr bool      is_small() const { return size <= small_str_max_size; }
    constexpr static T* allocate(SizeType count) {
        if consteval {
            return new T[count];
        } else {
            return reinterpret_cast<T*>(AllocatorType::allocate(sizeof(T) * count));
        }
    }
    constexpr static void deallocate(T* ptr) {
        if consteval {
            delete[] ptr;
        } else {
            AllocatorType::deallocate(ptr);
        }
    }
    constexpr String(SizeType size) : size(size) {
        if (size > small_str_max_size) {
            data.large           = allocate(size);
            data.large[size - 1] = 0;
        }
    }

public:
    constexpr static SizeType npos = -1;
    struct StringIterator {
        String*                   str   = nullptr;
        SizeType                  index = npos;
        constexpr StringIterator& operator++() {
            index++;
            return *this;
        }
        constexpr StringIterator& operator--() {
            index--;
            return *this;
        }
        constexpr StringIterator operator++(int) {
            auto old = *this;
            operator++();
            return old;
        }
        constexpr StringIterator operator--(int) {
            auto old = *this;
            operator--();
            return old;
        }
        constexpr          operator T*() { return str->at(index); }
        constexpr T&       operator*() { return *(str->at(index)); }
        constexpr SizeType position_of() { return index; }
        constexpr bool     valid() { return str != nullptr && str->size > index; }
    };
    constexpr T&       operator[](SizeType i) { return (is_small() ? data.small : data.large)[i]; }
    constexpr const T& operator[](SizeType i) const { return (is_small() ? data.small : data.large)[i]; }
    constexpr T*       at(SizeType i) {
        if (i >= size) return nullptr;
        return (is_small() ? data.small : data.large) + i;
    }
    constexpr const T* at(SizeType i) const {
        if (i >= size) return nullptr;
        return (is_small() ? data.small : data.large) + i;
    }
    constexpr SizeType length() const { return size; }
    template <auto N>
    constexpr String(const T (&str)[N]) : size(N) {
        if (!is_small()) data.large = allocate(size);
        for (SizeType i = 0; i < size; i++) (is_small() ? data.small : data.large)[i] = str[i];
    }

    constexpr String(const String& o) : size(o.size) {
        if (!is_small()) data.large = allocate(size);
        for (SizeType i = 0; i < size; i++) (is_small() ? data.small : data.large)[i] = (o.is_small() ? o.data.small : o.data.large)[i];
    }
    constexpr String(String&& o) {
        ::std::swap(data, o.data);
        ::std::swap(size, o.size);
    }
    constexpr String(const T* str) {
        for (size = 1; str[size - 1]; size++)
            ;
        if (!is_small()) data.large = allocate(size);
        for (SizeType i = 0; i < size; i++) (is_small() ? data.small : data.large)[i] = str[i];
    }
    constexpr String() : size(0) {}
    constexpr String& operator=(const String& o) {
        if (!is_small()) data.large = allocate(size);
        for (SizeType i = 0; i < size; i++) (is_small() ? data.small : data.large)[i] = (o.is_small() ? o.data.small : o.data.large)[i];
        return *this;
    }
    constexpr String& operator=(String&& o) {
        ::std::swap(data, o.data);
        ::std::swap(size, o.size);
        return *this;
    }
    constexpr ~String() {
        if (!is_small()) deallocate(data.large);
    }
    constexpr const T* begin() { return is_small() ? data.small : data.large; }
    constexpr const T* end() { return (is_small() ? data.small : data.large) + size; }
    constexpr const T* begin() const { return is_small() ? data.small : data.large; }
    constexpr const T* end() const { return (is_small() ? data.small : data.large) + size; }
    constexpr const T* c_str() const { return is_small() ? data.small : data.large; }
    constexpr void     resize(SizeType ns) {
        if (ns > small_str_max_size) {
            auto nd = allocate(ns);
            for (SizeType i = 0; i < (ns > size ? size : ns); i++) nd[i] = is_small() ? data.small[i] : data.large[i];
            if (!is_small()) deallocate(data.large);
            data.large = nd;
            size       = ns;
        } else {
            if (is_small()) {
                size = ns;
            } else {
                auto c = data.large;
                for (SizeType i = 0; i < ns; i++) data.small[i] = c[i];
                size = ns;
                deallocate(c);
            }
        }
        (is_small() ? data.small : data.large)[size - 1] = 0;
    }
    constexpr String substr(const T* begin, const T* end = nullptr) const {
        String res((end ? end : this->end()) - begin + 1);
        for (SizeType i = 0; i < (end ? end : this->end()) - begin; i++) {
            res[i] = begin[i];
        }
        return res;
    }
    constexpr String substr(SizeType begin, SizeType end = npos) const {
        if (end == npos) end = size;
        return substr(is_small() ? data.small + begin : data.large + begin, is_small() ? data.small + end : data.large + end);
    }
    constexpr StringIterator find(T c, const T* begin) { return find(c, begin - (is_small() ? data.small : data.large)); }
    constexpr StringIterator find(const String& str, const T* begin) { return find(str, begin - (is_small() ? data.small : data.large)); }
    constexpr StringIterator find(T c, SizeType begin = 0) {
        for (SizeType i = begin; i < size; i++)
            if ((is_small() ? data.small : data.large)[i] == c) {
                return {this, i};
            }
        return {this, npos};
    }
    constexpr StringIterator find(const String& str, SizeType begin = 0) {
        if (size < str.size || str.size == 0) return {this, npos};
        SizeType current = 0;
        for (SizeType i = begin; i < size - str.size + 1; i++) {
            bool matched = true;
            for (SizeType j = 0; j < str.size - 1; j++) {
                if ((is_small() ? data.small : data.large)[i + j] != (str.is_small() ? str.data.small : str.data.large)[j]) {
                    matched = false;
                    break;
                }
            }
            if (matched) return {this, i};
        }
        return {this, npos};
    }
    constexpr bool begin_with(const String& str) const {
        if (str.size > size) return false;
        return substr(0, str.size - 1) == str;
    }
    constexpr bool end_with(const String& str) const {
        if (str.size > size) return false;
        return substr(size - str.size, size - 1) == str;
    }
    friend constexpr ::rics::String<T, SizeType, AllocatorType>
    operator+(const ::rics::String<T, SizeType, AllocatorType>& a, const ::rics::String<T, SizeType, AllocatorType>& b) {
        ::rics::String<T, SizeType, AllocatorType> res(a.length() + b.length() - 1);
        for (SizeType i = 0; i < a.length() - 1; i++) res[i] = a[i];
        for (SizeType i = a.length() - 1; i < a.length() + b.length() - 1; i++) res[i] = b[i - (a.length() - 1)];
        return res;
    }
    constexpr ::rics::String<T, SizeType, AllocatorType>& operator+=(const ::rics::String<T, SizeType, AllocatorType>& o) {
        auto old = size - 1;
        resize(size + o.size - 1);
        for (auto i = old; i < size; i++) (is_small() ? data.small : data.large)[i] = (o.is_small() ? o.data.small : o.data.large)[i - old];
        (is_small() ? data.small : data.large)[size - 1] = 0;
        return *this;
    }
    friend constexpr bool
    operator==(const ::rics::String<T, SizeType, AllocatorType>& a, const ::rics::String<T, SizeType, AllocatorType>& b) {
        if (a.size != b.size) return false;
        for (SizeType i = 0; i < a.size; i++)
            if (a[i] != b[i]) return false;
        return true;
    }
};

} // namespace rics
