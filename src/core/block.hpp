#pragma once

#include <core/types.hpp>
#include <type_traits>

template <typename T>
concept ClassOrVoid = std::is_class_v<T> || std::is_void_v<T>;

template <typename I, typename O>
  requires(ClassOrVoid<I> && ClassOrVoid<O>)
class Block {
public:
  using Input = I;

  using Output = O;

  explicit Block(const u32 blockId) : blockId(blockId) {}
  virtual ~Block() = default;

  virtual O apply(I input) = 0;

  const u32 blockId;
};

template <typename O>
  requires(ClassOrVoid<O>)
class Block<void, O> {
public:
  using Input = void;

  using Output = O;

  explicit Block(const u32 blockId) : blockId(blockId) {}
  virtual ~Block() = default;

  virtual O apply() = 0;

  const u32 blockId;
};
