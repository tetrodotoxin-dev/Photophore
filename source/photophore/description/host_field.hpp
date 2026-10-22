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
// name preserves borrowed reflection data for diagnostics. Pipelines fills the
// block from role, offset and size, giving compiled layout facts authority over
// a producer's naming convention.
struct HostField {
  Perimortem::Core::View::Bytes name;
  Count offset = 0;
  Count size = 0;
  HostRole role = HostRole::Parameter;
};

}  // namespace Photophore::Description
