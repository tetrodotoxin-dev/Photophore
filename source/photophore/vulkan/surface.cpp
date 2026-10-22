// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/perimortem.h"

#ifdef PERI_LINUX
#define VK_USE_PLATFORM_WAYLAND_KHR
#elifdef PERI_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

#include "perimortem/core/null_terminated.hpp"

#include "implementation/vulkan/dispatch.hpp"

using namespace Perimortem::Core;

auto Photophore::Vulkan::surface_extension(Platform::Presentation::Kind kind)
    -> const char* {
#ifdef PERI_LINUX

  return kind == Platform::Presentation::Kind::Wayland
             ? VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
             : nullptr;

#elifdef PERI_WINDOWS

  return kind == Platform::Presentation::Kind::Win32
             ? VK_KHR_WIN32_SURFACE_EXTENSION_NAME
             : nullptr;

#else

  return nullptr;

#endif
}

auto Photophore::Vulkan::create_surface(
    VkInstance instance,
    Platform::Presentation presentation,
    VkSurfaceKHR* surface) -> VkResult {
#ifdef PERI_LINUX

  // Context selects this adapter before acquiring its surface. Native types
  // remain inside this host branch while Presentation stays platform neutral.
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

#elifdef PERI_WINDOWS

  auto create = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
      symbol("vkCreateWin32SurfaceKHR"_view));
  if (!create) {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
  VkWin32SurfaceCreateInfoKHR info = {
    VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
  info.hinstance = static_cast<HINSTANCE>(presentation.get_host());
  info.hwnd = static_cast<HWND>(presentation.get_surface());
  return create(instance, &info, nullptr, surface);

#else

  return VK_ERROR_EXTENSION_NOT_PRESENT;

#endif
}
