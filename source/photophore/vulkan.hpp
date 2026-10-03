// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/bytes.hpp"

#include "photophore/platform/presentation.hpp"

namespace Photophore {

// Photophore can be loaded on a host without a Vulkan driver. Context acquires
// the native loader before requesting any Vulkan objects and releases its
// reservation after destroying them. The first acquisition fills this table
// under a lock, and the last release clears it before unloading the library.
// Live contexts keep the table fixed, so drawing needs no loader lock.
//
// These pointers come from the loader's exported dispatch functions. They are
// shared across contexts rather than replaced by one device's entry points,
// which allows windows on different contexts to coexist. Surface creation is
// implemented by each host and accepts only its own Presentation kind. Wasm
// reports Vulkan as unavailable through the same acquisition path.
class Vulkan {
 public:
  static EXPORTED(PHOTOPHORE) auto acquire() -> Bool;
  static EXPORTED(PHOTOPHORE) auto release() -> void;
  static EXPORTED(PHOTOPHORE) auto symbol(Perimortem::Core::View::Bytes name)
      -> void*;
  static EXPORTED(PHOTOPHORE) auto surface_extension(
      Platform::Presentation::Kind kind) -> const char*;
  static EXPORTED(PHOTOPHORE) auto create_surface(
      VkInstance instance,
      Platform::Presentation presentation,
      VkSurfaceKHR* surface) -> VkResult;

#define PHOTOPHORE_VULKAN(name) static EXPORTED(PHOTOPHORE) PFN_##name name;
#include "photophore/vulkan_functions.hpp"
#undef PHOTOPHORE_VULKAN
};

}  // namespace Photophore
