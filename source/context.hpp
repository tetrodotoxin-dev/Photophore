// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/perimortem.hpp"

#include "photophore/platform/presentation.hpp"

namespace Photophore {

// Rendering resources belong to a Vulkan device and must be destroyed while
// that device and its loader are still alive. Context groups that lifetime with
// the presentation surface, one queue for both graphics and presentation, and
// the command pool used by uploads and frame recording. The native window in
// Presentation remains borrowed and must outlive the context.
//
// Creation selects a Vulkan 1.3 device with dynamic rendering,
// synchronization2 and KHR or EXT swapchain maintenance presentation fences.
// Missing host support returns an empty Context after
// releasing partial state. Device children are owned by the renderer, pipelines
// and textures, so those owners must finish their GPU work and release their
// resources before Context is destroyed. Its destructor does not wait for
// outstanding submissions.
class Context {
 public:
  // An unavailable loader, surface or device produces an empty context whose
  // get_device() is null. Native resources acquired during discovery are still
  // released, allowing a host to choose another presenter after failure.
  static auto create(Photophore::Platform::Presentation presentation)
      -> Context;

  Context() = default;
  ~Context();
  Context(Context&&) noexcept;
  auto operator=(Context&&) noexcept -> Context&;
  Context(const Context&) = delete;
  auto operator=(const Context&) = delete;

  auto get_instance() const -> VkInstance;
  auto get_physical_device() const -> VkPhysicalDevice;
  auto get_device() const -> VkDevice;
  auto get_graphics_queue() const -> VkQueue;
  auto get_graphics_queue_family() const -> U32;
  auto get_command_pool() const -> VkCommandPool;
  auto get_surface() const -> VkSurfaceKHR;
  auto supports_float64() const -> Bool;

  // Returns the index of the first memory type that satisfies both the type
  // filter bitmask and the required property flags. Returns UINT32_MAX if
  // no matching type is found.
  auto find_memory_type(U32 type_filter, VkMemoryPropertyFlags properties) const
      -> U32;

  // Uploads use a temporary command buffer from this context's pool. submit
  // waits for the graphics queue and frees that buffer, so staging memory can
  // be released on return. That synchronous wait is paid for each upload.
  auto begin_immediate_commands() const -> VkCommandBuffer;
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

}  // namespace Photophore
