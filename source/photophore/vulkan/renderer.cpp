// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/vulkan/renderer.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "implementation/vulkan/dispatch.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Vulkan;

static auto require_success(VkResult result, View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Diagnostics::Log::fatal(message);
  }
}

static auto allocate_frames(const Context& context, auto& frames) -> void {
  // A frame slot groups reusable command storage, image acquisition and the
  // fence generation that Pipelines records for texture retirement.
  VkCommandBufferAllocateInfo allocation = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocation.commandPool = context.get_command_pool();
  allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocation.commandBufferCount = 1;
  for (Count index = 0; index < frames.get_size(); index++) {
    auto& frame = frames[index];
    require_success(
        Photophore::Vulkan::vkAllocateCommandBuffers(
            context.get_device(), &allocation, &frame.command_buffer),
        "Photophore: Failed to allocate a frame command buffer."_view);
    VkSemaphoreCreateInfo semaphore = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    require_success(
        Photophore::Vulkan::vkCreateSemaphore(
            context.get_device(), &semaphore, nullptr, &frame.image_available),
        "Photophore: Failed to create an acquisition semaphore."_view);
    VkFenceCreateInfo fence = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    // A signaled initial fence makes the first slot use follow the same wait
    // and reset sequence as every completed submission that follows it.
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    require_success(
        Photophore::Vulkan::vkCreateFence(
            context.get_device(), &fence, nullptr, &frame.fence),
        "Photophore: Failed to create a submission fence."_view);
  }
}

static auto destroy_frames(const Context& context, auto& frames) -> void {
  for (Count index = 0; index < frames.get_size(); index++) {
    auto& frame = frames[index];
    if (frame.fence) {
      Photophore::Vulkan::vkDestroyFence(
          context.get_device(), frame.fence, nullptr);
    }
    if (frame.image_available) {
      Photophore::Vulkan::vkDestroySemaphore(
          context.get_device(), frame.image_available, nullptr);
    }
    frame.fence = VK_NULL_HANDLE;
    frame.image_available = VK_NULL_HANDLE;
    frame.command_buffer = VK_NULL_HANDLE;
  }
}

static auto allocate_presentations(
    const Context& context,
    Count image_count,
    auto& presentations) -> void {
  // Presentation state follows swapchain image identity because acquisition
  // order is independent of the rotating command frame slots.
  for (Count index = 0; index < image_count; index++) {
    auto& presentation = presentations[index];
    VkSemaphoreCreateInfo semaphore = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    require_success(
        Photophore::Vulkan::vkCreateSemaphore(
            context.get_device(), &semaphore, nullptr,
            &presentation.render_finished),
        "Photophore: Failed to create a presentation semaphore."_view);
    VkFenceCreateInfo fence = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    require_success(
        Photophore::Vulkan::vkCreateFence(
            context.get_device(), &fence, nullptr, &presentation.completion),
        "Photophore: Failed to create a presentation fence."_view);
  }
}

static auto wait_for_presentation(const Context& context, auto& presentation)
    -> void {
  if (!presentation.pending) {
    return;
  }
  require_success(
      Photophore::Vulkan::vkWaitForFences(
          context.get_device(), 1, &presentation.completion, VK_TRUE,
          UINT64_MAX),
      "Photophore: Failed to wait for presentation."_view);
  presentation.pending = False;
}

static auto destroy_presentations(const Context& context, auto& presentations)
    -> void {
  for (Count index = 0; index < presentations.get_size(); index++) {
    auto& presentation = presentations[index];
    if (presentation.render_finished) {
      Photophore::Vulkan::vkDestroySemaphore(
          context.get_device(), presentation.render_finished, nullptr);
    }
    if (presentation.completion) {
      Photophore::Vulkan::vkDestroyFence(
          context.get_device(), presentation.completion, nullptr);
    }
    presentation.render_finished = VK_NULL_HANDLE;
    presentation.completion = VK_NULL_HANDLE;
    presentation.pending = False;
  }
}

static auto begin_rendering(
    const Swapchain& swapchain,
    const Renderer::Frame& frame) -> void {
  VkCommandBufferBeginInfo begin_info = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  require_success(
      Photophore::Vulkan::vkBeginCommandBuffer(
          frame.command_buffer, &begin_info),
      "Photophore::Vulkan::Renderer: Failed to begin command buffer."_view);

  // Every frame clears its color image, making undefined the correct source
  // layout. The acquire semaphore orders this transition after the presentation
  // engine has made the image available again.
  VkImageMemoryBarrier2 to_attachment = {
    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  to_attachment.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  to_attachment.srcAccessMask = VkAccessFlags2(0);
  to_attachment.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  to_attachment.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  to_attachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  to_attachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  to_attachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to_attachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to_attachment.image = swapchain.get_image(frame.image_index);
  to_attachment.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  VkDependencyInfo attachment_dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  attachment_dependency.imageMemoryBarrierCount = 1;
  attachment_dependency.pImageMemoryBarriers = &to_attachment;
  Photophore::Vulkan::vkCmdPipelineBarrier2(
      frame.command_buffer, &attachment_dependency);

  VkRenderingAttachmentInfo color_attachment = {
    VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
  color_attachment.imageView = swapchain.get_image_view(frame.image_index);
  color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

  const VkExtent2D extent = {frame.width, frame.height};
  VkRenderingInfo rendering = {VK_STRUCTURE_TYPE_RENDERING_INFO};
  rendering.renderArea = {{0, 0}, extent};
  rendering.layerCount = 1;
  rendering.colorAttachmentCount = 1;
  rendering.pColorAttachments = &color_attachment;
  Photophore::Vulkan::vkCmdBeginRendering(frame.command_buffer, &rendering);

  const VkViewport viewport = {0.0f, 0.0f, R32(frame.width), R32(frame.height),
                               0.0f, 1.0f};
  Photophore::Vulkan::vkCmdSetViewport(frame.command_buffer, 0, 1, &viewport);
  const VkRect2D scissor = {{0, 0}, extent};
  Photophore::Vulkan::vkCmdSetScissor(frame.command_buffer, 0, 1, &scissor);
}

static auto finish_rendering(
    const Swapchain& swapchain,
    const Renderer::Frame& frame) -> void {
  // The final barrier publishes color writes to presentation. The command
  // buffer then contains one complete acquired image transaction.
  Photophore::Vulkan::vkCmdEndRendering(frame.command_buffer);

  VkImageMemoryBarrier2 to_present = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  to_present.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  to_present.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  to_present.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
  to_present.dstAccessMask = VkAccessFlags2(0);
  to_present.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  to_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to_present.image = swapchain.get_image(frame.image_index);
  to_present.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  VkDependencyInfo present_dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  present_dependency.imageMemoryBarrierCount = 1;
  present_dependency.pImageMemoryBarriers = &to_present;
  Photophore::Vulkan::vkCmdPipelineBarrier2(
      frame.command_buffer, &present_dependency);
  require_success(
      Photophore::Vulkan::vkEndCommandBuffer(frame.command_buffer),
      "Photophore::Vulkan::Renderer: Failed to end command buffer."_view);
}

Renderer::Renderer(Platform::Presentation presentation, U32 width, U32 height)
    : context(Context::create(presentation)),
      swapchain(Swapchain::create(context, width, height)) {
  if (!context.get_device()) {
    return;
  }
  suspended = Bool(!width || !height);
  allocate_frames(context, frames);
  allocate_presentations(context, swapchain.get_image_count(), presentations);
}

Renderer::~Renderer() {
  wait_idle();
  destroy_presentations(context, presentations);
  destroy_frames(context, frames);
}

auto Renderer::begin_frame(Frame& frame) -> Bool {
  if (suspended || !context.get_device() || !swapchain.get_swapchain()) {
    return False;
  }
  auto& state = frames[current_frame];
  require_success(
      Photophore::Vulkan::vkWaitForFences(
          context.get_device(), 1, &state.fence, VK_TRUE, UINT64_MAX),
      "Photophore: Failed to wait for a frame slot."_view);
  const U32 image_index = swapchain.acquire_next_image(state.image_available);
  if (image_index == UINT32_MAX) {
    return False;
  }

  // The submission fence protects the rotating command slot. Presentation
  // state follows the independently acquired image, whose completion fence
  // proves its semaphore and swapchain references are ready for reuse.
  wait_for_presentation(context, presentations[image_index]);
  // Successful acquisition commits this slot to a submission, so its signaled
  // fence can then be reset. Advancing the generation first preserves proof of
  // the completed use recorded by texture cache entries.
  ++state.generation;
  require_success(
      Photophore::Vulkan::vkResetFences(context.get_device(), 1, &state.fence),
      "Photophore: Failed to reset a submission fence."_view);
  require_success(
      Photophore::Vulkan::vkResetCommandBuffer(state.command_buffer, 0),
      "Photophore: Failed to reset a command buffer."_view);
  frame.command_buffer = state.command_buffer;
  frame.completion = state.fence;
  frame.completion_generation = &state.generation;
  frame.width = swapchain.get_extent().width;
  frame.height = swapchain.get_extent().height;
  frame.frame_index = current_frame;
  frame.image_index = image_index;
  begin_rendering(swapchain, frame);
  return True;
}

auto Renderer::end_frame(const Frame& frame) -> void {
  // Submission waits for acquisition, records completion on the rotating frame
  // fence and signals the semaphore dedicated to this swapchain image.
  finish_rendering(swapchain, frame);
  auto& state = frames[frame.frame_index];
  auto& presentation = presentations[frame.image_index];
  constexpr VkPipelineStageFlags wait_stage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = &state.image_available;
  submit.pWaitDstStageMask = &wait_stage;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &state.command_buffer;
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = &presentation.render_finished;
  require_success(
      Photophore::Vulkan::vkQueueSubmit(
          context.get_graphics_queue(), 1, &submit, state.fence),
      "Photophore: Failed to submit a frame."_view);

  require_success(
      Photophore::Vulkan::vkResetFences(
          context.get_device(), 1, &presentation.completion),
      "Photophore: Failed to reset a presentation fence."_view);
  VkSwapchainPresentFenceInfoKHR completion = {
    VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_KHR};
  completion.swapchainCount = 1;
  completion.pFences = &presentation.completion;
  const VkSwapchainKHR swapchain_handle = swapchain.get_swapchain();
  VkPresentInfoKHR present = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present.pNext = &completion;
  present.waitSemaphoreCount = 1;
  present.pWaitSemaphores = &presentation.render_finished;
  present.swapchainCount = 1;
  present.pSwapchains = &swapchain_handle;
  present.pImageIndices = &frame.image_index;
  const VkResult result = Photophore::Vulkan::vkQueuePresentKHR(
      context.get_graphics_queue(), &present);
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR &&
      result != VK_ERROR_OUT_OF_DATE_KHR) {
    Diagnostics::Log::fatal("Photophore: Failed to present a frame."_view);
  }

  // OUT_OF_DATE still enqueues presentation wait operations. The pending fence
  // carries that work into resize, which observes completion before retirement.
  presentation.pending = True;
  current_frame = (current_frame + 1) % frames_in_flight;
}

auto Renderer::wait_idle() -> void {
  if (!context.get_device()) {
    return;
  }
  require_success(
      Photophore::Vulkan::vkDeviceWaitIdle(context.get_device()),
      "Photophore: Failed to wait for rendering."_view);
  // Device idle completes queue submissions. Presentation fences extend that
  // proof through the presentation engine's payload references, making every
  // application semaphore and swapchain object ready for release.
  for (Count index = 0; index < presentations.get_size(); index++) {
    auto& presentation = presentations[index];
    wait_for_presentation(context, presentation);
  }
}

auto Renderer::resize(U32 width, U32 height) -> Bool {
  if (!context.get_device()) {
    return False;
  }
  suspended = Bool(!width || !height);
  if (suspended) {
    return False;
  }
  wait_idle();
  // Presentation slots have one entry per old swapchain image. Rebuild them
  // around the replacement image inventory and report the pipeline format edge.
  const VkFormat old_format = swapchain.get_format();
  destroy_presentations(context, presentations);
  swapchain.recreate(context, width, height);
  allocate_presentations(context, swapchain.get_image_count(), presentations);
  return swapchain.get_format() != old_format;
}

auto Renderer::get_width() const -> U32 {
  return swapchain.get_extent().width;
}
auto Renderer::get_height() const -> U32 {
  return swapchain.get_extent().height;
}
auto Renderer::get_context() const -> const Context& {
  return context;
}
auto Renderer::get_swapchain() const -> const Swapchain& {
  return swapchain;
}
