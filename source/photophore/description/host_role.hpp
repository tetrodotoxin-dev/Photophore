// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Description {

// Host roles distinguish bytes copied from a batch from values supplied by
// its current placement. TransformX and TransformY each hold one affine row
// scaled by the quad size, followed by its translation and framebuffer extent.
// That four R32 convention lets shaders map unit geometry into the current
// view.
enum class HostRole : U8 {
  Parameter,
  TransformX,
  TransformY,
};

}  // namespace Photophore::Description
