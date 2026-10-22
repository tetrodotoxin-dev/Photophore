// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Frame {

// Program carries the address identity of one Description::Program into a
// Frame::Batch. Pipelines compares that token with its borrowed inventory and
// uses the matching description to realize the draw. The caller keeps the
// locator stable for every inventory and batch that shares it.
//
// The pointed byte serves only as identity. Description admission establishes
// the shader and layout agreement associated with that address.
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
