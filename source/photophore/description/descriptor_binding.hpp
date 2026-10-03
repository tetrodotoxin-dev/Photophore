// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "photophore/description/resource.hpp"

namespace Photophore::Description {

// A batch's resources follow this list in order. set and slot locate each
// resource in the shader interface, while name preserves the producer's label
// as borrowed reflection data. The current Pipelines path accepts one sampled
// texture at slot zero per distinct set and rejects other arrangements.
struct DescriptorBinding {
  Perimortem::Core::View::Bytes name;
  Count set = 0;
  Count slot = 0;
  Resource resource = Resource::SampledTexture2D;
};

}  // namespace Photophore::Description
