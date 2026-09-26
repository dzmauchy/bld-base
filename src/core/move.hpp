#pragma once

/**
 * RemoveReference
 * @brief Obtains the underlying type after removing a reference.
 * @image type.svg
 */
template <typename T> struct RemoveReference {
  using type = T;
};

/**
 * RemoveReference
 * @brief Obtains the underlying type after removing a reference.
 * @image type.svg
 */
template <typename T> struct RemoveReference<T &> {
  using type = T;
};

/**
 * RemoveReference
 * @brief Obtains the underlying type after removing a reference.
 * @image type.svg
 */
template <typename T> struct RemoveReference<T &&> {
  using type = T;
};

template <typename T> using remove_reference_t = RemoveReference<T>::type;

template <typename T> constexpr auto move(T &value) noexcept -> remove_reference_t<T> && {
  return static_cast<remove_reference_t<T> &&>(value);
}
