// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "photophore/description/stage.hpp"

namespace Photophore::Description {

// A borrowed SPIR V module selected for one Vulkan pipeline stage. Keeping the
// compiled module as 32 bit words preserves its alignment and supplies one
// unambiguous byte count. An empty entry name selects `main`.
struct Module {
  Stage stage = Stage::Vertex;
  Perimortem::Core::View::Vector<U32> words;
  Perimortem::Core::View::Bytes entry;
};

}  // namespace Photophore::Description
