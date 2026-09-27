// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/perimortem.h"

#ifdef PERI_WASM
#include "photophore/vulkan.hpp"

using namespace Photophore;
using namespace Perimortem::Core;

// The browser host does not expose a native Vulkan surface. Reporting absence
// at the adapter lets the ordinary Context acquisition path decline rendering
// without changing the library's exported operations.
auto Vulkan::surface_extension(Platform::Presentation::Kind) -> const char* {
  return nullptr;
}
auto Vulkan::create_surface(VkInstance, Platform::Presentation, VkSurfaceKHR*)
    -> VkResult {
  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

#endif
