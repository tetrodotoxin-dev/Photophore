// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/static/vector.hpp"

#include "photophore/vulkan/context.hpp"
#include "photophore/vulkan/swapchain.hpp"

namespace Photophore::Vulkan {

// Renderer owns the Context, Swapchain and synchronization state used to
// present one native Window. Two rotating frame slots provide command buffers,
// acquisition semaphores and submission fences. A separate presentation slot
// follows each swapchain image because acquisition order and frame order are
// independent.
//
// A successful begin_frame lends one Frame for immediate command recording.
// end_frame submits and presents that exact value. Pipelines records draws
// between those calls and borrows the Context plus frame completion data from
// Renderer. All three operations run on one calling thread, preserving the
// order of fence generations and borrowed command state.
//
// False from begin_frame reports suspension, unavailable presentation or a
// swapchain recreation request before command recording begins. resize and
// wait_idle settle both queue submissions and presentation fences. The second
// fence set proves when the presentation engine has released its semaphore and
// swapchain references.
class Renderer {
 public:
  // Frame identifies both rotating command storage and the independently
  // acquired swapchain image. completion is signaled by this submission, and
  // completion_generation advances after the previous signal has been
  // observed. Pipelines records both values with each used image so fence reuse
  // remains distinguishable from completion of the earlier frame.
  struct Frame {
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkFence completion = VK_NULL_HANDLE;
    const Count* completion_generation = nullptr;
    U32 width = 0;
    U32 height = 0;
    Count frame_index = 0;
    U32 image_index = 0;
  };

  EXPORTED(PHOTOPHORE)
  Renderer(
      Photophore::Platform::Presentation presentation,
      U32 width,
      U32 height);
  EXPORTED(PHOTOPHORE) ~Renderer();
  Renderer(const Renderer&) = delete;
  auto operator=(const Renderer&) -> Renderer& = delete;

  // Rebuilds the swapchain for a new physical extent and reports whether its
  // color format changed, which requires Pipelines to be rebuilt.
  // A zero extent suspends acquisition until the window becomes drawable.
  EXPORTED(PHOTOPHORE) auto resize(U32 width, U32 height) -> Bool;
  EXPORTED(PHOTOPHORE) auto begin_frame(Frame& frame) -> Bool;
  EXPORTED(PHOTOPHORE) auto end_frame(const Frame& frame) -> void;
  EXPORTED(PHOTOPHORE) auto wait_idle() -> void;

  EXPORTED(PHOTOPHORE) auto get_width() const -> U32;
  EXPORTED(PHOTOPHORE) auto get_height() const -> U32;
  EXPORTED(PHOTOPHORE) auto get_context() const -> const Context&;
  EXPORTED(PHOTOPHORE) auto get_swapchain() const -> const Swapchain&;

 private:
  static constexpr Count frames_in_flight = 2;

  struct FrameState {
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkSemaphore image_available = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    Count generation = 0;
  };

  struct PresentationState {
    VkSemaphore render_finished = VK_NULL_HANDLE;
    VkFence completion = VK_NULL_HANDLE;
    Bool pending = False;
  };

  Context context;
  Swapchain swapchain;
  Perimortem::Core::Static::Vector<FrameState, frames_in_flight> frames;
  Perimortem::Core::Static::Vector<PresentationState, Swapchain::max_images>
      presentations;
  Count current_frame = 0;
  Bool suspended = False;
};

}  // namespace Photophore::Vulkan
