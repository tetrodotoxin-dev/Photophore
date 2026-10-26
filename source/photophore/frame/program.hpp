// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "toolchain/toolchain.hpp"

namespace Photophore::Frame {

// Program carries the address token of one Description::Program into a
// Frame::Batch. Pipelines compares that token with its description inventory
// and uses the matching record to realize the draw. The caller keeps the token
// storage at a stable address while the inventory and its batches are used.
//
// The pointed byte is never read. Description validation establishes the shader
// and layout agreement associated with the address.
class Program {
 public:
  constexpr Program() = default;
  explicit constexpr Program(const U8* locator) : locator(locator) {}

  constexpr auto is_valid() const -> Bool { return locator != nullptr; }
  constexpr auto get_locator() const -> const U8* { return locator; }
  constexpr auto operator==(const Program& rhs) const -> Bool {
    return locator == rhs.locator;
  }

 private:
  const U8* locator = nullptr;
};

}  // namespace Photophore::Frame
