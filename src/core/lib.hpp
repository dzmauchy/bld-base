#pragma once

#if defined(__wasm__)
#include <wasm.hpp>
#else
#include <new>
#endif

namespace core {

using size_t = decltype(sizeof(0));

namespace detail {
template <typename T, typename... Args>
inline constexpr bool constructible = __is_constructible(T, Args...);
template <typename T> struct remove_reference {
  using type = T;
};
template <typename T> struct remove_reference<T &> {
  using type = T;
};
template <typename T> struct remove_reference<T &&> {
  using type = T;
};
template <typename T> struct remove_cv {
  using type = T;
};
template <typename T> struct remove_cv<const T> {
  using type = T;
};
template <typename T> struct remove_cv<volatile T> {
  using type = T;
};
template <typename T> struct remove_cv<const volatile T> {
  using type = T;
};
template <typename T>
using value_type = typename remove_cv<typename remove_reference<T>::type>::type;
template <typename T> struct decay {
  using type = value_type<T>;
};
template <typename T, size_t N> struct decay<T[N]> {
  using type = T *;
};
template <typename R, typename... Args> struct decay<R(Args...)> {
  using type = R (*)(Args...);
};
template <typename R, typename... Args> struct decay<R(Args...) noexcept> {
  using type = R (*)(Args...) noexcept;
};
template <typename T> using decay_t = typename decay<typename remove_reference<T>::type>::type;
template <typename A, typename B> inline constexpr bool same = false;
template <typename T> inline constexpr bool             same<T, T> = true;
template <typename T> inline constexpr bool             pointer = false;
template <typename T> inline constexpr bool             pointer<T *> = true;
template <typename T> T                               &&declval();
template <typename T> void                              accept(T);

template <typename T> constexpr T &&forward(typename remove_reference<T>::type &value) noexcept {
  return static_cast<T &&>(value);
}
template <typename T> constexpr typename remove_reference<T>::type &&move(T &&value) noexcept {
  return static_cast<typename remove_reference<T>::type &&>(value);
}

template <typename R, typename F, typename... Args>
concept callable = (same<R, void> && requires(F &f) { f(declval<Args>()...); }) ||
                   requires(F &f) { accept<R>(f(declval<Args>()...)); };
} // namespace detail

// Owns a copyable callable. Copies have independent captured values; references
// and pointers still borrow their targets. Empty invocation traps. No RTTI or
// exceptions are required, and allocation uses the target's ordinary new/delete.
template <typename Signature> class function;

template <typename R, typename... Args> class function<R(Args...)> {
  struct operations {
    R (*invoke)(void *,
                Args...);
    void *(*copy)(const void *);
    void (*destroy)(void *);
  };

  template <typename F>
  inline static constexpr operations ops_for{
      [](void *object, Args... args) -> R {
        if constexpr (detail::same<R, void>)
          (*static_cast<F *>(object))(detail::forward<Args>(args)...);
        else
          return (*static_cast<F *>(object))(detail::forward<Args>(args)...);
      },
      [](const void *object) -> void * { return new F(*static_cast<const F *>(object)); },
      [](void *object) { delete static_cast<F *>(object); }};

  void             *object_ = nullptr;
  const operations *ops_ = nullptr;

public:
  function() noexcept = default;
  function(decltype(nullptr)) noexcept {}

  template <typename F,
            typename Stored = detail::decay_t<F>>
    requires(!detail::same<Stored,
                           function> &&
             detail::constructible<Stored,
                                   F &&> &&
             detail::constructible<Stored,
                                   const Stored &> &&
             detail::callable<R,
                              Stored,
                              Args...>)
  function(F &&callable) {
    if constexpr (detail::pointer<Stored>) {
      if (callable == nullptr)
        return;
    }
    object_ = new Stored(detail::forward<F>(callable));
    ops_ = &ops_for<Stored>;
  }

  function(const function &other)
      : object_(other.ops_ ? other.ops_->copy(other.object_) : nullptr),
        ops_(other.ops_) {}
  function(function &&other) noexcept { swap(other); }
  ~function() { reset(); }

  function &operator=(const function &other) {
    if (this == &other)
      return *this;
    function copy(other);
    swap(copy);
    return *this;
  }
  function &operator=(function &&other) noexcept {
    if (this == &other)
      return *this;
    function moved(detail::move(other));
    swap(moved);
    return *this;
  }
  function &operator=(decltype(nullptr)) noexcept {
    reset();
    return *this;
  }
  template <typename F>
    requires requires(F &&f) { function(detail::forward<F>(f)); }
  function &operator=(F &&callable) {
    function replacement(detail::forward<F>(callable));
    swap(replacement);
    return *this;
  }

  explicit operator bool() const noexcept { return ops_ != nullptr; }
  R        operator()(Args... args) const {
    if (!ops_)
      __builtin_trap();
    return ops_->invoke(object_, detail::forward<Args>(args)...);
  }
  void reset() noexcept {
    auto *object = object_;
    auto *ops = ops_;
    object_ = nullptr;
    ops_ = nullptr;
    if (ops)
      ops->destroy(object);
  }
  void swap(function &other) noexcept {
    auto *object = object_;
    auto *ops = ops_;
    object_ = other.object_;
    ops_ = other.ops_;
    other.object_ = object;
    other.ops_ = ops;
  }
};

// Owns contiguous storage with a size chosen at construction. There are no
// growth or resize operations. Copies own separate storage; moves transfer the
// buffer and empty the source. Invalid checked indices and size overflow trap.
template <typename T> class array {
  T     *data_ = nullptr;
  size_t size_ = 0;

  template <typename Construct>
  void initialize(size_t    count,
                  Construct construct) {
    if (count > max_size())
      __builtin_trap();
    array replacement;
    if (count)
      replacement.data_ =
          static_cast<T *>(::operator new(count * sizeof(T), std::align_val_t{alignof(T)}));
    // The temporary owns every completed element if construction throws.
    while (replacement.size_ < count) {
      construct(replacement.data_ + replacement.size_, replacement.size_);
      ++replacement.size_;
    }
    swap(replacement);
  }

public:
  using value_type = T;
  using size_type = size_t;
  using iterator = T *;
  using const_iterator = const T *;

  array() noexcept = default;
  explicit array(size_t count) {
    initialize(count, [](T *slot, size_t) { ::new (static_cast<void *>(slot)) T(); });
  }
  array(size_t   count,
        const T &value) {
    initialize(count, [&value](T *slot, size_t) { ::new (static_cast<void *>(slot)) T(value); });
  }
  array(const T *first,
        const T *last) {
    initialize(first == last ? 0 : static_cast<size_t>(last - first),
               [first](T *slot, size_t i) { ::new (static_cast<void *>(slot)) T(first[i]); });
  }
  template <typename Range>
    requires(!detail::same<detail::value_type<Range>,
                           array> &&
             requires(const Range &values) {
               detail::accept<const T *>(values.data());
               values.size();
             })
  array(const Range &values)
      : array(values.data(),
              values.size() ? values.data() + values.size() : values.data()) {}
  template <size_t N>
  array(const T (&values)[N])
      : array(values,
              values + N) {}
  array(const array &other)
    requires(detail::constructible<T,
                                   const T &>)
      : array(other.begin(),
              other.end()) {}
  array(array &&other) noexcept { swap(other); }
  ~array() {
    while (size_)
      data_[--size_].~T();
    if (data_)
      ::operator delete(data_, std::align_val_t{alignof(T)});
  }

  array &operator=(const array &other)
    requires(detail::constructible<T,
                                   const T &>)
  {
    if (this != &other) {
      array copy(other);
      swap(copy);
    }
    return *this;
  }
  array &operator=(array &&other) noexcept {
    if (this != &other) {
      array moved(detail::move(other));
      swap(moved);
    }
    return *this;
  }
  void swap(array &other) noexcept {
    T     *data = data_;
    size_t size = size_;
    data_ = other.data_;
    size_ = other.size_;
    other.data_ = data;
    other.size_ = size;
  }

  static constexpr size_t max_size() noexcept { return __PTRDIFF_MAX__ / sizeof(T); }
  size_t                  size() const noexcept { return size_; }
  bool                    empty() const noexcept { return size_ == 0; }
  T                      *data() noexcept { return data_; }
  const T                *data() const noexcept { return data_; }
  T                      *begin() noexcept { return data_; }
  const T                *begin() const noexcept { return data_; }
  T                      *end() noexcept { return size_ ? data_ + size_ : data_; }
  const T                *end() const noexcept { return size_ ? data_ + size_ : data_; }
  T                      &operator[](size_t index) noexcept { return data_[index]; }
  const T                &operator[](size_t index) const noexcept { return data_[index]; }
  T                      &at(size_t index) {
    if (index >= size_)
      __builtin_trap();
    return data_[index];
  }
  const T &at(size_t index) const {
    if (index >= size_)
      __builtin_trap();
    return data_[index];
  }
  T       &front() { return at(0); }
  const T &front() const { return at(0); }
  T       &back() { return at(size_ - 1); }
  const T &back() const { return at(size_ - 1); }
  void     fill(const T &value) {
    for (auto &element : *this)
      element = value;
  }
};

template <typename T,
          size_t N>
array(const T (&)[N]) -> array<T>;

// A borrowed contiguous view. Owners and pointer targets must outlive the view.
template <typename T> class span {
  T     *data_ = nullptr;
  size_t size_ = 0;

public:
  span() noexcept = default;
  span(T     *data,
       size_t count) noexcept
      : data_(data),
        size_(count) {}
  template <typename U,
            size_t N>
    requires requires(U *data) { detail::accept<T *>(data); }
  span(U (&values)[N]) noexcept
      : data_(values),
        size_(N) {}
  template <typename Range>
    requires requires(Range &values) {
      detail::accept<T *>(values.data());
      values.size();
    }
  span(Range &&values) noexcept
      : data_(values.data()),
        size_(values.size()) {}
  size_t size() const noexcept { return size_; }
  bool   empty() const noexcept { return size_ == 0; }
  T     *data() const noexcept { return data_; }
  T     *begin() const noexcept { return data_; }
  T     *end() const noexcept { return size_ ? data_ + size_ : data_; }
  T     &operator[](size_t index) const noexcept { return data_[index]; }
  span   first(size_t count) const noexcept {
    if (count > size_)
      __builtin_trap();
    return {data_, count};
  }
};

// Single-threaded shared ownership keeps block state stable across callable copies.
template <typename T> class shared_ptr {
  struct storage {
    size_t owners = 1;
    T      value;
    template <typename... Args> storage(Args &&...args) : value(detail::forward<Args>(args)...) {}
  };
  storage *storage_ = nullptr;

public:
  template <typename... Args> static shared_ptr make(Args &&...args) {
    shared_ptr result;
    result.storage_ = new storage(detail::forward<Args>(args)...);
    return result;
  }
  shared_ptr() noexcept = default;
  shared_ptr(const shared_ptr &other) noexcept : storage_(other.storage_) {
    if (storage_)
      ++storage_->owners;
  }
  shared_ptr(shared_ptr &&other) noexcept : storage_(other.storage_) { other.storage_ = nullptr; }
  ~shared_ptr() { reset(); }
  shared_ptr &operator=(const shared_ptr &other) noexcept {
    shared_ptr copy(other);
    swap(copy);
    return *this;
  }
  shared_ptr &operator=(shared_ptr &&other) noexcept {
    shared_ptr moved(detail::move(other));
    swap(moved);
    return *this;
  }
  void reset() noexcept {
    auto *storage = storage_;
    storage_ = nullptr;
    if (storage && --storage->owners == 0)
      delete storage;
  }
  void swap(shared_ptr &other) noexcept {
    auto *storage = storage_;
    storage_ = other.storage_;
    other.storage_ = storage;
  }
  T       *get() const noexcept { return storage_ ? &storage_->value : nullptr; }
  T       &operator*() const noexcept { return *get(); }
  T       *operator->() const noexcept { return get(); }
  explicit operator bool() const noexcept { return storage_ != nullptr; }
};

template <typename T,
          typename... Args>
shared_ptr<T> make_shared(Args &&...args) {
  return shared_ptr<T>::make(detail::forward<Args>(args)...);
}

} // namespace core
