#pragma once

#include "core/types.hpp"

class Block {
 public:
  explicit Block(const u32 blockId) : blockId(blockId) {}
  virtual ~Block() = default;

  [[nodiscard]] auto id() const { return blockId; }

 protected:
  u32 blockId;
};
