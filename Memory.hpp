#pragma once
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <new>

namespace rics {
template <typename SizeType, typename T>
concept Allocator = noexcept(T::allocate(SizeType{}))&& noexcept(T::deallocate((void*){})) && requires(SizeType size, void* ptr) {
    { T::allocate(size) } -> ::std::same_as<void*>;
    { T::deallocate(ptr) } -> ::std::same_as<void>;
};
template <::std::integral T = ::std::size_t>
struct DefaultAllocator {
    static inline void* allocate(T size) noexcept { return ::operator new(size, ::std::nothrow_t{}); }
    static inline void  deallocate(void* ptr) noexcept { ::operator delete(ptr, ::std::nothrow_t{}); }
};
} // namespace rics
