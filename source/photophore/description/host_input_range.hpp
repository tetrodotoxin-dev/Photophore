// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "photophore/description/stage.hpp"

namespace Photophore::Description {

// HostInputRange declares the portion of Program's host block visible to a set
// of shader stages. ShaderProgram copies offset, size and stages into one
// Vulkan push constant range, preserving the compiled agreement when commands
// later supply bytes for a draw. Vulkan measures both offset and size in four
// byte units and requires a positive size.
struct HostInputRange {
  Count offset = 0;
  Count size = 0;
  Perimortem::Core::View::Vector<Stage> stages;
};

}  // namespace Photophore::Description
