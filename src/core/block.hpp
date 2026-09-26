#pragma once

#include <core/types.hpp>
#include <type_traits>

template <typename I, typename O>
  requires(std::is_class_v<I> && (std::is_class_v<O> || std::is_void_v<O>))
class Block {
 public:
  using Input = I;

  using Output = O;

  explicit Block(const u32 blockId) : blockId(blockId) {}
  virtual ~Block() = default;

  virtual O apply(I input) = 0;

  const u32 blockId;
};
