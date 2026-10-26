// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/bytes.hpp"

namespace Photophore::Vulkan {

// These declarations form Photophore's process wide function table for the
// native Vulkan loader. The first Context acquisition opens the library and
// resolves every required function under a lock. Each live Context holds one
// reservation, and the final release clears the table before closing the
// library. Recording uses the immutable table directly after acquisition.
//
// The pointers come from loader exports and serve every Context, including
// windows backed by different devices. Presentation providers receive
// `vkGetInstanceProcAddr` from this table and create their own platform
// surface.
class Dispatch {
 public:
  static auto acquire() -> Bool;
  static auto release() -> void;
  static auto symbol(Perimortem::Core::View::Bytes name) -> void*;

#define PHOTOPHORE_VULKAN(name) static PFN_##name name;

#include "implementation/vulkan/functions.hpp"
#undef PHOTOPHORE_VULKAN
};

}  // namespace Photophore::Vulkan
