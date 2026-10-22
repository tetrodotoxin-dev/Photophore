// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Description {

// VertexInput describes one R32 scalar or vector in an interleaved vertex
// record. location selects the shader input, offset measures bytes from the
// record start, and every entry shares the stride of binding zero.
// ShaderProgram maps one through four components to Vulkan formats. Pipelines
// admits the two entries matching its position and texture coordinate quad.
struct VertexInput {
  Count location = 0;
  Count components = 0;
  Count offset = 0;
  Count stride = 0;
};

}  // namespace Photophore::Description
