#pragma once

#include <core/types.hpp>
#include <type_traits>

/**
 * Block
 * @brief A block with one input struct and an output struct or void.
 * @image block.svg
 */
template <typename I, typename O>
  requires(std::is_class_v<I> && (std::is_class_v<O> || std::is_void_v<O>))
class Block {
 public:
  /**
   * Input
   * @brief The struct whose fields define the input ports.
   * @image input.svg
   */
  using Input = I;
  /**
   * Output
   * @brief The struct whose fields define the output ports, or void.
   * @image output.svg
   */
  using Output = O;

  explicit Block(const u32 blockId) : blockId(blockId) {}
  virtual ~Block() = default;

  virtual O apply(I input) = 0;

  [[nodiscard]] auto id() const { return blockId; }

 protected:
  u32 blockId;
};
