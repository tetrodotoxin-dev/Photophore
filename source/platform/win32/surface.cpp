// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/perimortem.h"

#ifdef PERI_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "perimortem/core/null_terminated.hpp"

#include "photophore/vulkan.hpp"

using namespace Photophore;
using namespace Perimortem::Core;

auto Vulkan::surface_extension(Platform::Presentation::Kind kind) -> const
    char* {
  return kind == Platform::Presentation::Kind::Win32
             ? VK_KHR_WIN32_SURFACE_EXTENSION_NAME
             : nullptr;
}

auto Vulkan::create_surface(
    VkInstance instance,
    Platform::Presentation presentation,
    VkSurfaceKHR* surface) -> VkResult {
  // Context selects this adapter for a Win32 Presentation. The instance and
  // window handles remain owned by Window, which must survive the Vulkan
  // surface created from them.
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
}

#endif
