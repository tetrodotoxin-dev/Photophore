// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Description {

// Stage identifies where a Module executes and which shaders can observe a
// HostInputRange. ShaderProgram performs the Vulkan flag conversion while this
// description stays independent of native numeric values.
enum class Stage : U32 {
  Vertex,
  Pixel,
};

}  // namespace Photophore::Description
