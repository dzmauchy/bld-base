#pragma once

template <typename T>
struct RemoveReference {
  using type = T;
};

template <typename T>
struct RemoveReference<T&> {
  using type = T;
};

template <typename T>
struct RemoveReference<T&&> {
  using type = T;
};

template <typename T>
using remove_reference_t = typename RemoveReference<T>::type;

template <typename T>
constexpr auto move(T& value) noexcept -> remove_reference_t<T>&& {
  return static_cast<remove_reference_t<T>&&>(value);
}
