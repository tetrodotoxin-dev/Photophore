// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan_core.h>

#include "toolchain/toolchain.hpp"

namespace Photophore::Platform {

// Presentation lets a Window provider create the Vulkan surface for its native
// presentation system. The provider supplies one context with the exact
// operations that interpret it. Vulkan calls those operations without
// recovering Wayland, Win32 or another provider's state.
//
// `get_instance_extension` names the platform extension required before
// `VkInstance` creation and keeps that null terminated name accessible through
// creation. `create_surface` uses the resulting instance and its loader to
// publish one `VkSurfaceKHR`. Both calls are synchronous. Calls use the context
// supplied with this exact operation table. The provider keeps both accessible
// through every call and remains live until the resulting Vulkan surface is
// destroyed. Copies preserve that same access interval.
class Presentation {
 public:
  struct Operations {
    const char* (*get_instance_extension)(void* context) = nullptr;
    VkResult (*create_surface)(
        void* context,
        VkInstance instance,
        PFN_vkGetInstanceProcAddr load,
        VkSurfaceKHR* surface) = nullptr;
  };

  static constexpr auto create(void* context, const Operations& operations)
      -> Presentation {
    return Presentation(context, &operations);
  }

  constexpr Presentation() = default;

  // Reports that the context and both required operations are present. Surface
  // creation reports the provider's current host availability through
  // VkResult.
  constexpr auto is_valid() const -> Bool {
    return Bool(
        context && operations && operations->get_instance_extension &&
        operations->create_surface);
  }

  constexpr auto get_instance_extension() const -> const char* {
    return is_valid() ? operations->get_instance_extension(context) : nullptr;
  }

  constexpr auto create_surface(
      VkInstance instance,
      PFN_vkGetInstanceProcAddr load,
      VkSurfaceKHR* surface) const -> VkResult {
    if (!is_valid() || !instance || !load || !surface) {
      return VK_ERROR_INITIALIZATION_FAILED;
    }

    return operations->create_surface(context, instance, load, surface);
  }

 private:
  constexpr Presentation(void* context, const Operations* operations)
      : context(context), operations(operations) {}

  void* context = nullptr;
  const Operations* operations = nullptr;
};

static_assert(sizeof(Presentation) == sizeof(void*) * 2);
static_assert(__is_trivially_copyable(Presentation));
static_assert(__is_standard_layout(Presentation));

}  // namespace Photophore::Platform
