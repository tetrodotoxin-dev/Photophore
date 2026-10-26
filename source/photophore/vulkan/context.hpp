// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "toolchain/toolchain.hpp"

#include "photophore/platform/presentation.hpp"

namespace Photophore::Vulkan {

// Context is the Vulkan device capability used by renderer resources. A
// successful value contains the instance, presentation surface, selected
// physical and logical devices, graphics queue and upload command pool.
// Destruction releases that chain in the reverse order required by Vulkan.
//
// Presentation's Window keeps its native handles accessible through surface
// creation and Context use. Renderer, Pipelines, Swapchain, TextureImage and
// Texture store handles from the selected device. Their queued work completes
// and their handles are destroyed before this Context.
//
// `create` requires Vulkan 1.3, dynamic rendering, synchronization2 and either
// KHR or EXT swapchain maintenance presentation fences. Success has a nonnull
// `get_device`. Missing requirements return an empty Context after releasing
// every native handle created during discovery.
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
  auto operator=(const Context&) -> Context& = delete;

  EXPORTED(PHOTOPHORE) auto get_instance() const -> VkInstance;
  EXPORTED(PHOTOPHORE) auto get_physical_device() const -> VkPhysicalDevice;
  EXPORTED(PHOTOPHORE) auto get_device() const -> VkDevice;
  EXPORTED(PHOTOPHORE) auto get_graphics_queue() const -> VkQueue;
  EXPORTED(PHOTOPHORE) auto get_graphics_queue_family() const -> U32;
  EXPORTED(PHOTOPHORE) auto get_command_pool() const -> VkCommandPool;
  EXPORTED(PHOTOPHORE) auto get_surface() const -> VkSurfaceKHR;
  EXPORTED(PHOTOPHORE) auto supports_float64() const -> Bool;

  // Selects the first memory type present in both the device type filter and
  // requested property flags. U32(-1) reports an empty intersection.
  EXPORTED(PHOTOPHORE) auto find_memory_type(
      U32 type_filter,
      VkMemoryPropertyFlags properties) const -> U32;

  // `begin_immediate_commands` allocates one transient primary command buffer.
  // `submit_immediate_commands` ends it, submits it, waits for the graphics
  // queue and frees it. Returning from submission proves that upload staging
  // storage is no longer referenced, with one synchronous queue wait per call.
  EXPORTED(PHOTOPHORE) auto begin_immediate_commands() const -> VkCommandBuffer;
  EXPORTED(PHOTOPHORE) auto submit_immediate_commands(
      VkCommandBuffer command_buffer) const -> void;

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
