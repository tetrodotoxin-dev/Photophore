// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "photophore/description/resource.hpp"

namespace Photophore::Description {

// DescriptorBinding describes one resource position in a compiled shader.
// Frame::Batch supplies Texture2D values in this same sequence, so Pipelines
// can bind a draw by index while set and slot preserve the shader declaration.
// name remains borrowed reflection data for diagnostics and inspection.
//
// The current Vulkan realization admits sampled textures at slot zero and
// gives each texture a distinct descriptor set. That profile keeps every
// Texture in one independently allocated set while Program preserves the full
// shader coordinates independently of that realization profile.
struct DescriptorBinding {
  Perimortem::Core::View::Bytes name;
  Count set = 0;
  Count slot = 0;
  Resource resource = Resource::SampledTexture2D;
};

}  // namespace Photophore::Description
