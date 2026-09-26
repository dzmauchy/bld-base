#pragma once

/**
 * RemoveReference
 * @brief Obtains the underlying type after removing a reference.
 * @image type.svg
 */
template <typename T>
struct RemoveReference {
  /**
   * type
   * @brief The underlying value type.
   * @image type.svg
   */
  using type = T;
};

/**
 * RemoveReference
 * @brief Obtains the underlying type after removing a reference.
 * @image type.svg
 */
template <typename T>
struct RemoveReference<T&> {
  /**
   * type
   * @brief The underlying value type.
   * @image type.svg
   */
  using type = T;
};

/**
 * RemoveReference
 * @brief Obtains the underlying type after removing a reference.
 * @image type.svg
 */
template <typename T>
struct RemoveReference<T&&> {
  /**
   * type
   * @brief The underlying value type.
   * @image type.svg
   */
  using type = T;
};

/**
 * remove_reference_t
 * @brief The underlying type with its reference removed.
 * @image type.svg
 */
template <typename T>
using remove_reference_t = RemoveReference<T>::type;

template <typename T>
constexpr auto move(T& value) noexcept -> remove_reference_t<T>&& {
  return static_cast<remove_reference_t<T>&&>(value);
}
