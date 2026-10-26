// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "toolchain/toolchain.hpp"

namespace Photophore::Description {

// Stage identifies the shader execution stage of a Module and the stages that
// can read a HostInputRange. Vertex and Pixel remain independent of Vulkan
// numeric flags. ShaderProgram converts each value when creating a pipeline.
enum class Stage : U32 {
  Vertex,
  Pixel,
};

}  // namespace Photophore::Description
