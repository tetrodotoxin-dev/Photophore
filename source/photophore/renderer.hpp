// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/static/vector.hpp"

#include "photophore/context.hpp"
#include "photophore/swapchain.hpp"

namespace Photophore {

// Renderer coordinates command recording and presentation for one native
// window. It owns the Context and Swapchain, plus two rotating command buffers
// and their synchronization objects. Pipelines supplies the actual draws
// between begin_frame and end_frame, keeping program and resource selection
// outside the window presentation loop.
//
// A successful begin_frame must be paired with end_frame for that exact Frame.
// The frame borrows a command buffer and image until submission, and it cannot
// be retained for a later iteration. begin_frame returns False when
// presentation is unavailable, suspended or needs its swapchain recreated.
// Resize and teardown wait for rendering and presentation separately. A queue
// submission fence cannot prove that the presentation engine has released its
// semaphore or swapchain references. Keep Renderer alive while Pipelines
// borrows its Context and completion fences. Frame recording and submission
// use one calling thread so the borrowed completion observations stay ordered.
class Renderer {
 public:
  // The frame slot selects command storage, while image_index selects the
  // image acquired from the swapchain. Their orders are independent.
  // completion is reset before this frame is published and signaled by its
  // submission. Pipelines borrows it to retire resources after their last use.
  // The generation advances only after waiting for that slot, distinguishing
  // completed work from a later submission that reuses the same fence.
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

}  // namespace Photophore
