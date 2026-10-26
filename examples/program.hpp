// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "photophore/description/program.hpp"
#include "photophore/frame/program.hpp"

namespace Photophore::Examples {

// SpriteProgram publishes the complete process lifetime shader description
// used by the demo. The frame identity and Vulkan description identify the same
// product while keeping authored shader inputs separate from command recording.
class SpriteProgram {
 public:
  static auto get_program() -> Frame::Program;
  static auto get_description() -> Description::Program;
};

}  // namespace Photophore::Examples
