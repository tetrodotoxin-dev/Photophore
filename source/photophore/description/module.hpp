// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "photophore/description/stage.hpp"

namespace Photophore::Description {

// Module references one SPIR V binary and its shader stage. U32 words preserve
// the alignment required by Vulkan and give module creation an exact byte
// extent. `entry` names the exported shader function, with an empty View
// selecting `main`. Both Views remain accessible for the containing Program
// access interval and are consumed synchronously by ShaderProgram creation.
struct Module {
  Stage stage = Stage::Vertex;
  Perimortem::Core::View::Vector<U32> words;
  Perimortem::Core::View::Bytes entry;
};

}  // namespace Photophore::Description
