// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "toolchain/toolchain.hpp"

namespace Photophore::Description {

// HostRole selects how Pipelines fills one HostField. Parameter copies authored
// bytes from Frame::Batch. TransformX and TransformY each publish one affine
// row scaled by the quad size, followed by translation and framebuffer extent.
// Those four R32 values map unit geometry into the current presentation area.
enum class HostRole : U8 {
  Parameter,
  TransformX,
  TransformY,
};

}  // namespace Photophore::Description
