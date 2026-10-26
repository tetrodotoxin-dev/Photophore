// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "photophore/description/resource.hpp"

namespace Photophore::Description {

// DescriptorBinding names one resource position required by a compiled shader.
// Frame::Batch supplies Texture2D values in the same sequence. Pipelines binds
// each value by index while set and slot preserve the shader declaration. The
// name View remains valid for the Description::Program access interval and is
// used only for diagnostics and inspection.
//
// The current Vulkan realization accepts sampled textures at slot zero and
// assigns each texture a distinct descriptor set. Program still records the
// complete shader coordinates so another realization can interpret them.
struct DescriptorBinding {
  Perimortem::Core::View::Bytes name;
  Count set = 0;
  Count slot = 0;
  Resource resource = Resource::SampledTexture2D;
};

}  // namespace Photophore::Description
