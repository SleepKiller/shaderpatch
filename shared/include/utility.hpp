#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <malloc.h>
#include <vector>

#include <gsl/gsl>

namespace sp {

template<std::size_t alignment>
class Aligned_scratch_buffer {
public:
   Aligned_scratch_buffer() noexcept = default;

   ~Aligned_scratch_buffer()
   {
      if (_buffer) _aligned_free(_buffer);
   }

   Aligned_scratch_buffer(const Aligned_scratch_buffer&) = delete;
   Aligned_scratch_buffer& operator=(const Aligned_scratch_buffer&) = delete;

   Aligned_scratch_buffer(Aligned_scratch_buffer&&) = delete;
   Aligned_scratch_buffer& operator=(Aligned_scratch_buffer&&) = delete;

   void resize(const std::size_t new_size) noexcept
   {
      if (_size == new_size) return;
      if (new_size == 0) {
         _buffer = nullptr;
         _size = 0;

         return;
      }

      _buffer = static_cast<std::byte*>(_aligned_malloc(new_size, alignment));

      if (!_buffer) std::terminate();

      _size = new_size;
   }

   void ensure_min_size(const std::size_t required_size) noexcept
   {
      if (_size >= required_size) return;

      resize(required_size);
   }

   void ensure_within_size(const std::size_t max_size) noexcept
   {
      if (_size <= max_size) return;

      resize(max_size);
   }

   std::size_t size() const noexcept
   {
      return _size;
   }

   std::byte* data() noexcept
   {
      return _buffer;
   }

   const std::byte* data() const noexcept
   {
      return _buffer;
   }

private:
   std::byte* _buffer = nullptr;
   std::size_t _size = 0;
};

template<typename To, typename From>
inline To bit_cast(const From from) noexcept
{
   static_assert(sizeof(From) <= sizeof(To));
   static_assert(std::is_standard_layout_v<To>);
   static_assert(std::is_trivially_copyable_v<From>);

   To to;

   std::memcpy(&to, &from, sizeof(From));

   return to;
}

template<typename To>
inline To bit_cast(const gsl::span<const std::byte> from) noexcept
{
   static_assert(std::is_standard_layout_v<To>);

   Expects(sizeof(To) <= from.size());

   To to;

   std::memcpy(&to, from.data(), sizeof(to));

   return to;
}

template<typename To>
inline To bit_cast(const gsl::span<std::byte> from) noexcept
{
   return bit_cast<To>(gsl::span<const std::byte>{from});
}

template<typename From>
inline auto make_vector(const From& from) noexcept
   -> std::vector<typename From::value_type>
{
   return {std::cbegin(from), std::cend(from)};
}

template<auto multiple>
constexpr auto next_multiple_of(const decltype(multiple) value) noexcept
   -> decltype(multiple)
{
   const auto remainder = value % multiple;

   if (remainder != 0) return value + (multiple - remainder);

   return value;
}

template<auto multiple>
constexpr bool is_multiple_of(const decltype(multiple) value) noexcept
{
   return (value % multiple) == 0;
}

struct Index_iterator {
   using difference_type = std::ptrdiff_t;
   using value_type = const difference_type;
   using pointer = value_type*;
   using reference = value_type&;
   using iterator_category = std::random_access_iterator_tag;

   Index_iterator() = default;
   ~Index_iterator() = default;

   Index_iterator(const Index_iterator&) = default;
   Index_iterator& operator=(const Index_iterator&) = default;
   Index_iterator(Index_iterator&&) = default;
   Index_iterator& operator=(Index_iterator&&) = default;

   auto operator*() const noexcept -> value_type
   {
      return _index;
   }

   auto operator++() noexcept -> Index_iterator&
   {
      ++_index;

      return *this;
   }

   auto operator++(int) const noexcept -> Index_iterator
   {
      auto copy = *this;

      ++copy._index;

      return copy;
   }

   auto operator--() noexcept -> Index_iterator&
   {
      --_index;

      return *this;
   }

   auto operator--(int) const noexcept -> Index_iterator
   {
      auto copy = *this;

      --copy._index;

      return copy;
   }

   auto operator[](const difference_type offset) const noexcept -> value_type
   {
      return _index + offset;
   }

   auto operator+=(const difference_type offset) noexcept -> Index_iterator&
   {
      _index += offset;

      return *this;
   }

   auto operator-=(const difference_type offset) noexcept -> Index_iterator&
   {
      _index -= offset;

      return *this;
   }

   friend bool operator==(const Index_iterator& left, const Index_iterator& right) noexcept;

   friend bool operator!=(const Index_iterator& left, const Index_iterator& right) noexcept;

   friend bool operator<(const Index_iterator& left, const Index_iterator& right) noexcept;

   friend bool operator<=(const Index_iterator& left, const Index_iterator& right) noexcept;

   friend bool operator>(const Index_iterator& left, const Index_iterator& right) noexcept;

   friend bool operator>=(const Index_iterator& left, const Index_iterator& right) noexcept;

private:
   difference_type _index{};
};

inline auto operator+(const Index_iterator& iter,
                      const Index_iterator::difference_type offset) noexcept -> Index_iterator
{
   auto copy = iter;

   return copy += offset;
}

inline auto operator+(const Index_iterator::difference_type offset,
                      const Index_iterator& iter) noexcept -> Index_iterator
{
   return iter + offset;
}

inline auto operator-(const Index_iterator& iter,
                      const Index_iterator::difference_type offset) noexcept -> Index_iterator
{
   auto copy = iter;

   return copy -= offset;
}

inline auto operator-(const Index_iterator::difference_type offset,
                      const Index_iterator& iter) noexcept -> Index_iterator
{
   return iter - offset;
}

inline auto operator-(const Index_iterator& left, const Index_iterator& right) noexcept
   -> Index_iterator::difference_type
{
   return *left - *right;
}

inline bool operator==(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left == *right;
}

inline bool operator!=(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left != *right;
}

inline bool operator<(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left < *right;
}

inline bool operator<=(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left <= *right;
}

inline bool operator>(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left > *right;
}

inline bool operator>=(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left >= *right;
}

template<typename Type, typename Alignment>
struct Aligned_allocator {
   static_assert(Alignment::value >= alignof(Type));

   using value_type = Type;
   using is_always_equal = std::true_type;

   Aligned_allocator() noexcept = default;

   template<typename Other>
   Aligned_allocator(const Aligned_allocator<Other, Alignment>&) noexcept
   {
   }

   [[nodiscard]] auto allocate(const std::size_t count) const noexcept -> Type*
   {
      auto* const result = _aligned_malloc(sizeof(Type) * count, Alignment::value);

      if (!result) std::terminate();

      return static_cast<Type*>(result);
   }

   void deallocate(Type* const mem, const std::size_t) const noexcept
   {
      _aligned_free(mem);
   }
};

template<typename L, typename La, typename R, typename Ra>
constexpr bool operator==(const Aligned_allocator<L, La>,
                          const Aligned_allocator<R, Ra>) noexcept
{
   return true;
}

template<typename L, typename La, typename R, typename Ra>
constexpr bool operator!=(const Aligned_allocator<L, La>,
                          const Aligned_allocator<R, Ra>) noexcept
{
   return false;
}

template<typename Type, std::size_t alignment>
using Aligned_allocator_for =
   Aligned_allocator<Type, std::integral_constant<std::size_t, alignment>>;

template<typename Type, std::size_t alignment>
using Aligned_vector = std::vector<Type, Aligned_allocator_for<Type, alignment>>;

template<std::size_t alignment, typename From>
inline auto make_aligned_vector(const From& from) noexcept
   -> Aligned_vector<typename From::value_type, alignment>
{
   return {std::cbegin(from), std::cend(from)};
}

}
