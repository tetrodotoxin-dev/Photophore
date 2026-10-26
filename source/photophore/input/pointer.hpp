// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Input {

// Pointer is one surface-local pointer observation. Position names the latest
// focused location. Motion and scroll accumulate between Snapshots, giving a
// consumer one stable value independently of the number of native events.
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
