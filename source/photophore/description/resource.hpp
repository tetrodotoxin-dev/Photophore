// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Description {

// Resource names the Vulkan descriptor representation required by a compiled
// shader. SampledTexture2D selects a combined image and sampler sourced from a
// Frame::Batch Texture2D. Each enumerator pairs with one concrete
// Pipelines realization and its validation rules.
enum class Resource : U8 {
  SampledTexture2D,
};

}  // namespace Photophore::Description
