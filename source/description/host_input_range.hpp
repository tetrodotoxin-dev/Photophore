// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "photophore/description/stage.hpp"

namespace Photophore::Description {

// A shader stage can read only the push constant ranges declared by its
// pipeline layout. This record supplies a byte offset, size and borrowed stage
// list for that declaration. Vulkan requires nonzero size and four byte units.
// Parameter values arrive later through command recording, independently of
// these layout records.
struct HostInputRange {
  Count offset = 0;
  Count size = 0;
  Perimortem::Core::View::Vector<Stage> stages;
};

}  // namespace Photophore::Description
