// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Description {

// Resource selects the descriptor representation required by the compiled
// shader. The current renderer realizes SampledTexture2D as a combined image
// sampler. Additional resource kinds need a corresponding realization before
// Pipelines can accept descriptions that use them.
enum class Resource : U8 {
  SampledTexture2D,
};

}  // namespace Photophore::Description
