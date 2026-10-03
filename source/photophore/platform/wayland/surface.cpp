// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/perimortem.h"

#ifdef PERI_LINUX
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#include "perimortem/core/null_terminated.hpp"

#include "photophore/vulkan.hpp"

using namespace Photophore;
using namespace Perimortem::Core;

auto Vulkan::surface_extension(Platform::Presentation::Kind kind) -> const
    char* {
  return kind == Platform::Presentation::Kind::Wayland
             ? VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
             : nullptr;
}

auto Vulkan::create_surface(
    VkInstance instance,
    Platform::Presentation presentation,
    VkSurfaceKHR* surface) -> VkResult {
  // Context selects this adapter before acquiring its surface. The two
  // borrowed handles can therefore be interpreted as Wayland objects here,
  // without exposing those native types in the common Presentation header.
  auto create = reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(
      symbol("vkCreateWaylandSurfaceKHR"_view));
  if (!create) {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
  VkWaylandSurfaceCreateInfoKHR info = {
    VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
  info.display = static_cast<wl_display*>(presentation.get_host());
  info.surface = static_cast<wl_surface*>(presentation.get_surface());
  return create(instance, &info, nullptr, surface);
}

#endif
