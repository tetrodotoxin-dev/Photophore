// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "toolchain/toolchain.hpp"

namespace Photophore::Input {

// Pointer records one window relative position together with the motion and
// scroll accumulated since the previous Snapshot. An inactive value marks an
// interval without pointer focus. The next focused Snapshot starts with zero
// motion because no earlier position belongs to that interval.
struct Pointer {
  R32 x = 0;
  R32 y = 0;
  R32 delta_x = 0;
  R32 delta_y = 0;
  R32 scroll_x = 0;
  R32 scroll_y = 0;
  Bool active = False;
};

}  // namespace Photophore::Input
