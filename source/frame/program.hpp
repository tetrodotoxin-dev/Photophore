// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Frame {

// A batch selects a compiled program by an opaque address token. Pipelines
// compares this token with its Description::Program inventory and never reads
// the pointed byte. The producer keeps the identity stable for the lifetime of
// all batches and inventories that use it. This value neither owns the program
// nor establishes agreement between independently loaded modules.
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
