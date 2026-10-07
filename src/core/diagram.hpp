#pragma once

#include <core/types.hpp>

namespace core {

namespace detail {

template <size_t Index, typename First, typename... Rest>
struct parameter_at : parameter_at<Index - 1, Rest...> {};
template <typename First, typename... Rest> struct parameter_at<0, First, Rest...> {
  using type = value_type<First>;
};
template <typename Factory> struct factory_parameters;
template <typename R, typename Id, typename... Parameters>
struct factory_parameters<R (*)(Id, Parameters...)> {
  template <size_t Index> using parameter = typename parameter_at<Index, Parameters...>::type;
};
template <typename R, typename Id, typename... Parameters>
struct factory_parameters<R (*)(Id, Parameters...) noexcept>
    : factory_parameters<R (*)(Id, Parameters...)> {};

template <typename Parameter> struct configuration {
  template <typename Value> static Parameter make(Value value) {
    return static_cast<Parameter>(move(value));
  }
};
template <typename T> struct configuration<array<T>> {
  template <typename... Values> static array<T> make(Values... values) {
    if constexpr (sizeof...(Values) == 0) {
      return {};
    } else {
      const T items[]{static_cast<T>(values)...};
      return array<T>{items};
    }
  }
};

struct empty_ports {};
template <typename Callable> struct block_binding;
template <typename R, typename I> struct block_binding<function<R(I)>> {
  static I    inputs() { return {}; }
  static auto bind(function<R(I)> &block,
                   I               input) {
    if constexpr (same<R, void>) {
      block(move(input));
      return empty_ports{};
    } else {
      return block(move(input));
    }
  }
};
template <typename R> struct block_binding<function<R()>> {
  static empty_ports inputs() { return {}; }
  static auto        bind(function<R()> &block,
                          empty_ports) {
    if constexpr (same<R, void>) {
      block();
      return empty_ports{};
    } else {
      return block();
    }
  }
};

template <typename Port, size_t Width> class output_storage {
public:
  explicit output_storage(Port port) : port_(move(port)) {
    if (port_.size() < Width)
      __builtin_trap();
  }
  decltype(auto) at(size_t index) {
    if (index >= Width)
      __builtin_trap();
    return port_[index];
  }

private:
  Port port_;
};
template <typename Range, typename Count, size_t Width>
class output_storage<function<Range(Count)>, Width> {
  static_assert(static_cast<size_t>(static_cast<Count>(Width)) == Width,
                "Output channel count does not fit the port's count type");

public:
  explicit output_storage(function<Range(Count)> port)
      : owner_(move(port)),
        channels_(owner_(static_cast<Count>(Width))) {
    if (channels_.size() < Width)
      __builtin_trap();
  }
  decltype(auto) at(size_t index) {
    if (index >= Width)
      __builtin_trap();
    return channels_[index];
  }

private:
  // Keep the callable and its captured consumer storage alive with the view.
  function<Range(Count)> owner_;
  Range                  channels_;
};

} // namespace detail

/** Converts JSON configuration values using the selected factory parameter's actual C++ type. */
template <size_t Index,
          typename Factory,
          typename... Values>
auto config_arg(Factory factory,
                Values... values) {
  (void)factory;
  using Parameter = typename detail::factory_parameters<Factory>::template parameter<Index>;
  return detail::configuration<Parameter>::make(detail::move(values)...);
}

/** Creates the complete input struct without exposing the block's callable signature. */
template <typename Callable> auto block_inputs(const Callable &) {
  return detail::block_binding<detail::value_type<Callable>>::inputs();
}

/** Wires a block and returns its output struct; void inputs/outputs use an empty placeholder. */
template <typename Callable,
          typename Input>
auto bind_block(Callable &block,
                Input     input) {
  return detail::block_binding<detail::value_type<Callable>>::bind(block, detail::move(input));
}

/** Scalar and vectorized outputs share at(index); the metadata flag selects their behavior. */
template <typename Port, bool Vectorized, size_t Width> class OutputChannels {
  static_assert(Width <= 1,
                "Scalar outputs have at most one channel");

public:
  explicit OutputChannels(Port port) : port_(detail::move(port)) {}
  Port at(size_t index) const {
    if (index >= Width)
      __builtin_trap();
    return port_;
  }

private:
  Port port_;
};
template <typename Port, size_t Width> class OutputChannels<Port, true, Width> {
public:
  explicit OutputChannels(Port port) : storage_(detail::move(port)) {}
  decltype(auto) at(size_t index) { return storage_.at(index); }

private:
  detail::output_storage<Port, Width> storage_;
};
template <bool   Vectorized,
          size_t Width,
          typename Port>
auto output_channels(Port port) {
  return OutputChannels<Port, Vectorized, Width>{detail::move(port)};
}

/** Owns temporary connection lists. Keep it alive until bind_block has copied its view. */
template <typename Port, bool Vectorized, size_t Connections, size_t Width> class InputConnections {
  static_assert(Connections <= 1,
                "Scalar inputs accept one connection");
  static_assert(Width <= 1,
                "Scalar inputs have at most one channel");

public:
  void connect(size_t index,
               Port   value) {
    if (index >= Width || connected_ || Connections == 0)
      __builtin_trap();
    value_ = detail::move(value);
    connected_ = true;
  }
  Port view() & { return value_; }

private:
  Port value_{};
  bool connected_ = false;
};
template <typename Element, size_t Connections, size_t Width>
class InputConnections<span<Element>, true, Connections, Width> {
  using Value = detail::value_type<Element>;

public:
  void connect(size_t index,
               Value  value) {
    if (index >= Width || size_ >= Connections)
      __builtin_trap();
    values_[size_++] = value;
  }
  span<Element> view() & { return {values_, size_}; }

private:
  Value  values_[Connections ? Connections : 1]{};
  size_t size_ = 0;
};
template <typename Port, size_t Connections, size_t Width>
class InputConnections<array<Port>, true, Connections, Width> {
public:
  template <typename Value>
  void connect(size_t index,
               Value  value) {
    if (index >= Width)
      __builtin_trap();
    groups_[index].connect(0, detail::move(value));
  }
  array<Port> view() & {
    array<Port> ports(Width);
    for (size_t index = 0; index < Width; ++index)
      ports[index] = groups_[index].view();
    return ports;
  }

private:
  InputConnections<Port, true, Connections, 1> groups_[Width ? Width : 1];
};
template <typename T, size_t Connections, size_t Width>
class InputConnections<array<T *>, true, Connections, Width>
    : public InputConnections<span<T *const>, true, Connections, Width> {
public:
  array<T *> view() & {
    return array<T *>{InputConnections<span<T *const>, true, Connections, Width>::view()};
  }
};
template <bool   Vectorized,
          size_t Connections,
          size_t Width,
          typename Port>
auto input_connections(const Port &) {
  static_assert(Vectorized || (Connections <= 1 && Width <= 1),
                "Scalar input metadata permits one connection on channel zero");
  return InputConnections<Port, Vectorized, Connections, Width>{};
}

} // namespace core
