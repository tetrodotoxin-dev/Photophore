// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "photophore/description/host_role.hpp"

namespace Photophore::Description {

// HostField locates one value in Program's complete host input block. Parameter
// selects bytes copied from Frame::Batch, while TransformX and TransformY
// select the affine rows derived from its placement and framebuffer extent.
// Each transform row occupies four R32 values.
//
// The `name` View follows the containing Program access interval and is used
// for diagnostics. Pipelines fills the block from `role`, `offset` and `size`,
// so the compiled layout determines placement independently of authored names.
struct HostField {
  Perimortem::Core::View::Bytes name;
  Count offset = 0;
  Count size = 0;
  HostRole role = HostRole::Parameter;
};

}  // namespace Photophore::Description
