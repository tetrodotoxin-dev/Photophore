// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Description {

// Each entry identifies an R32 scalar or vector in one interleaved vertex
// buffer. location is the shader input location, offset is measured from the
// vertex start, and stride is shared by all entries on binding zero.
// ShaderProgram accepts one through four components. Pipelines further checks
// that this layout matches the unit quad buffer it supplies.
struct VertexInput {
  Count location = 0;
  Count components = 0;
  Count offset = 0;
  Count stride = 0;
};

}  // namespace Photophore::Description
