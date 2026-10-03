// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "photophore/description/host_role.hpp"

namespace Photophore::Description {

// Host fields locate values inside Program::host_size. A Parameter field must
// lie inside the batch parameter region, which is copied as already laid out
// bytes. Transform roles are filled from the batch's placement and framebuffer
// dimensions and occupy four R32 values. The borrowed name is reflection data,
// so filling the block does not depend on spelling or a source language Type.
struct HostField {
  Perimortem::Core::View::Bytes name;
  Count offset = 0;
  Count size = 0;
  HostRole role = HostRole::Parameter;
};

}  // namespace Photophore::Description
