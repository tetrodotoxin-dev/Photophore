// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/perimortem.hpp"

#include "photophore/platform/presentation.hpp"

namespace Photophore::Vulkan {

// Context owns the Vulkan instance, presentation surface, physical and logical
// device selection, graphics queue and upload command pool for one renderer.
// Keeping this chain together gives every child resource one device lifetime
// and gives destruction the reverse native order required by Vulkan.
//
// Presentation lends the native window handles used to create the surface, so
// its Window remains live through the Context lifetime. Renderer, Pipelines,
// Swapchain, TextureImage and Texture borrow the selected device. Their native
// resources finish GPU use and end before Context destruction.
//
// create selects Vulkan 1.3 with dynamic rendering, synchronization2 and KHR
// or EXT swapchain maintenance presentation fences. A complete selection has a
// nonnull get_device. An unavailable requirement yields an empty Context after
// releasing every partial native handle acquired during discovery.
class Context {
 public:
  // An empty result reports unavailable loader, surface or device support
  // through a null get_device and remains safe to destroy.
  static EXPORTED(PHOTOPHORE) auto create(
      Photophore::Platform::Presentation presentation) -> Context;

  Context() = default;
  EXPORTED(PHOTOPHORE) ~Context();
  EXPORTED(PHOTOPHORE) Context(Context&&) noexcept;
  EXPORTED(PHOTOPHORE) auto operator=(Context&&) noexcept -> Context&;
  Context(const Context&) = delete;
  auto operator=(const Context&) = delete;

  EXPORTED(PHOTOPHORE) auto get_instance() const -> VkInstance;
  EXPORTED(PHOTOPHORE) auto get_physical_device() const -> VkPhysicalDevice;
  EXPORTED(PHOTOPHORE) auto get_device() const -> VkDevice;
  EXPORTED(PHOTOPHORE) auto get_graphics_queue() const -> VkQueue;
  EXPORTED(PHOTOPHORE) auto get_graphics_queue_family() const -> U32;
  EXPORTED(PHOTOPHORE) auto get_command_pool() const -> VkCommandPool;
  EXPORTED(PHOTOPHORE) auto get_surface() const -> VkSurfaceKHR;
  EXPORTED(PHOTOPHORE) auto supports_float64() const -> Bool;

  // Selects the first memory type admitted by the type filter and property
  // flags. UINT32_MAX is the explicit result for an empty intersection.
  EXPORTED(PHOTOPHORE)
  auto find_memory_type(U32 type_filter, VkMemoryPropertyFlags properties) const
      -> U32;

  // begin_immediate_commands allocates one transient primary command buffer.
  // submit_immediate_commands ends it, submits it, waits for the graphics queue
  // and frees it. The completed return lets an upload release staging memory
  // immediately, with one synchronous queue wait per call.
  EXPORTED(PHOTOPHORE) auto begin_immediate_commands() const -> VkCommandBuffer;
  EXPORTED(PHOTOPHORE)
  auto submit_immediate_commands(VkCommandBuffer command_buffer) const -> void;

 private:
  VkInstance instance = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphics_queue = VK_NULL_HANDLE;
  U32 graphics_queue_family = 0;
  VkCommandPool command_pool = VK_NULL_HANDLE;
  Bool float64 = False;
  Bool owns_loader = False;
};

}  // namespace Photophore::Vulkan
