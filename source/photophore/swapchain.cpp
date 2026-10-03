// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/swapchain.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/scalar.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "photophore/vulkan.hpp"

using namespace Perimortem::Core;
using namespace Photophore;
using namespace Perimortem;

static auto require_success(VkResult result, View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Diagnostics::Log::fatal(message);
  }
}

static auto require_enumeration_read(VkResult result, View::Bytes message)
    -> void {
  if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
    Diagnostics::Log::fatal(message);
  }
}

static auto choose_surface_format(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface) -> VkSurfaceFormatKHR {
  U32 surface_format_count = 0;
  require_success(
      Vulkan::vkGetPhysicalDeviceSurfaceFormatsKHR(
          physical_device, surface, &surface_format_count, nullptr),
      "Vulkan: Failed to query swapchain surface formats."_view);
  if (surface_format_count == 0) {
    Diagnostics::Log::fatal("Vulkan: No swapchain surface formats found."_view);
  }

  Perimortem::Memory::Dynamic::Vector<VkSurfaceFormatKHR> surface_formats;
  surface_formats.forgetful_resize(surface_format_count);
  U32 read_surface_format_count = surface_format_count;
  require_enumeration_read(
      Vulkan::vkGetPhysicalDeviceSurfaceFormatsKHR(
          physical_device, surface, &read_surface_format_count,
          surface_formats.get_data()),
      "Vulkan: Failed to read swapchain surface formats."_view);
  if (read_surface_format_count == 0) {
    Diagnostics::Log::fatal("Vulkan: No swapchain surface formats found."_view);
  }

  const VkSurfaceFormatKHR preferred_surface_format = {
    VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  if (read_surface_format_count == 1 &&
      surface_formats[0].format == VK_FORMAT_UNDEFINED) {
    return preferred_surface_format;
  }

  for (U32 i = 0; i < read_surface_format_count; i++) {
    if (surface_formats[i].format == preferred_surface_format.format &&
        surface_formats[i].colorSpace == preferred_surface_format.colorSpace) {
      return surface_formats[i];
    }
  }

  return surface_formats[0];
}

static auto choose_present_mode(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface) -> VkPresentModeKHR {
  U32 present_mode_count = 0;
  require_success(
      Vulkan::vkGetPhysicalDeviceSurfacePresentModesKHR(
          physical_device, surface, &present_mode_count, nullptr),
      "Vulkan: Failed to query swapchain present modes."_view);
  if (present_mode_count == 0) {
    Diagnostics::Log::fatal("Vulkan: No swapchain present modes found."_view);
  }

  Perimortem::Memory::Dynamic::Vector<VkPresentModeKHR> present_modes;
  present_modes.forgetful_resize(present_mode_count);
  U32 read_present_mode_count = present_mode_count;
  require_enumeration_read(
      Vulkan::vkGetPhysicalDeviceSurfacePresentModesKHR(
          physical_device, surface, &read_present_mode_count,
          present_modes.get_data()),
      "Vulkan: Failed to read swapchain present modes."_view);
  if (read_present_mode_count == 0) {
    Diagnostics::Log::fatal("Vulkan: No swapchain present modes found."_view);
  }

  for (U32 i = 0; i < read_present_mode_count; i++) {
    if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return VK_PRESENT_MODE_MAILBOX_KHR;
    }
  }

  // FIFO is required by Vulkan. Use it when mailbox presentation is absent
  // so support does not depend on an optional low latency present mode.
  return VK_PRESENT_MODE_FIFO_KHR;
}

static auto build(
    const Context& ctx,
    U32 width,
    U32 height,
    VkSwapchainKHR old_swapchain,
    auto& swapchain_state) -> Bool {
  swapchain_state.device = ctx.get_device();
  const VkSurfaceKHR surface = ctx.get_surface();

  const VkSurfaceFormatKHR surface_format =
      choose_surface_format(ctx.get_physical_device(), surface);
  const VkPresentModeKHR present_mode =
      choose_present_mode(ctx.get_physical_device(), surface);

  swapchain_state.format = surface_format.format;
  swapchain_state.extent = {width, height};

  VkSurfaceCapabilitiesKHR surface_capabilities = {};
  require_success(
      Vulkan::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          ctx.get_physical_device(), surface, &surface_capabilities),
      "Vulkan: Failed to query swapchain surface capabilities."_view);

  // Win32 fixes the extent to its client area. Wayland lets the caller choose
  // within the surface limits, so use the requested size only in that case.
  if (surface_capabilities.currentExtent.width != UINT32_MAX) {
    swapchain_state.extent = surface_capabilities.currentExtent;
  } else {
    swapchain_state.extent.width = Scalar::clamp(
        width, surface_capabilities.minImageExtent.width,
        surface_capabilities.maxImageExtent.width);
    swapchain_state.extent.height = Scalar::clamp(
        height, surface_capabilities.minImageExtent.height,
        surface_capabilities.maxImageExtent.height);
  }
  if (!swapchain_state.extent.width || !swapchain_state.extent.height) {
    return False;
  }

  // An extra image gives rendering room while presentation holds another.
  // The surface's maximum still limits that request, so this is a buffering
  // preference rather than a promise that acquisition cannot stall.
  U32 image_count = surface_capabilities.minImageCount + 1;
  if (surface_capabilities.maxImageCount > 0 &&
      image_count > surface_capabilities.maxImageCount) {
    image_count = surface_capabilities.maxImageCount;
  }

  const U32 queue_family = ctx.get_graphics_queue_family();

  VkSwapchainCreateInfoKHR swapchain_info = {
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchain_info.surface = surface;
  swapchain_info.minImageCount = image_count;
  swapchain_info.imageFormat = swapchain_state.format;
  swapchain_info.imageColorSpace = surface_format.colorSpace;
  swapchain_info.imageExtent = swapchain_state.extent;
  swapchain_info.imageArrayLayers = 1;
  swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchain_info.queueFamilyIndexCount = 1;
  swapchain_info.pQueueFamilyIndices = &queue_family;
  swapchain_info.preTransform = surface_capabilities.currentTransform;
  swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_info.presentMode = present_mode;
  swapchain_info.clipped = VK_TRUE;
  swapchain_info.oldSwapchain = old_swapchain;

  require_success(
      Vulkan::vkCreateSwapchainKHR(
          swapchain_state.device, &swapchain_info, nullptr,
          &swapchain_state.swapchain),
      "Vulkan: Failed to create swapchain."_view);

  swapchain_state.image_count = 0;
  require_success(
      Vulkan::vkGetSwapchainImagesKHR(
          swapchain_state.device, swapchain_state.swapchain,
          &swapchain_state.image_count, nullptr),
      "Vulkan: Failed to query swapchain images."_view);
  if (swapchain_state.image_count > Swapchain::max_images) {
    Diagnostics::Log::fatal("Vulkan: Too many swapchain images."_view);
  }

  require_enumeration_read(
      Vulkan::vkGetSwapchainImagesKHR(
          swapchain_state.device, swapchain_state.swapchain,
          &swapchain_state.image_count, swapchain_state.images.get_data()),
      "Vulkan: Failed to read swapchain images."_view);
  if (swapchain_state.image_count == 0) {
    Diagnostics::Log::fatal("Vulkan: No swapchain images found."_view);
  }

  for (U32 i = 0; i < swapchain_state.image_count; i++) {
    VkImageViewCreateInfo image_view_info = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    image_view_info.image = swapchain_state.images[i];
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = swapchain_state.format;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    require_success(
        Vulkan::vkCreateImageView(
            swapchain_state.device, &image_view_info, nullptr,
            &swapchain_state.image_views[i]),
        "Vulkan: Failed to create swapchain image view."_view);
  }

  return True;
}

static auto destroy(const auto& storage) -> void {
  if (!storage.device || !storage.swapchain) {
    return;
  }

  for (U32 index = 0; index < storage.image_count; index++) {
    Vulkan::vkDestroyImageView(
        storage.device, storage.image_views[index], nullptr);
  }
  Vulkan::vkDestroySwapchainKHR(storage.device, storage.swapchain, nullptr);
}

auto Photophore::Swapchain::create(const Context& ctx, U32 width, U32 height)
    -> Swapchain {
  if (!ctx.get_device() || !width || !height) {
    return Swapchain();
  }

  Swapchain output;
  if (!build(ctx, width, height, VK_NULL_HANDLE, output.storage)) {
    return Swapchain();
  }
  return output;
}

auto Photophore::Swapchain::recreate(const Context& ctx, U32 width, U32 height)
    -> void {
  // The caller has completed use of the old images. Supply the old chain while
  // constructing its replacement, then release the old views and handle only
  // after Vulkan has observed that relationship.
  Storage replacement;
  if (!build(ctx, width, height, storage.swapchain, replacement)) {
    replacement = Storage();
  }
  destroy(storage);
  storage = replacement;
}

Photophore::Swapchain::~Swapchain() {
  destroy(storage);
}

Photophore::Swapchain::Swapchain(Swapchain&& other) noexcept
    : storage(other.storage) {
  other.storage.swapchain = VK_NULL_HANDLE;
  other.storage.image_count = 0;
}

auto Photophore::Swapchain::operator=(Swapchain&& other) noexcept
    -> Swapchain& {
  if (this == &other) {
    return *this;
  }

  destroy(storage);
  storage = other.storage;
  other.storage.swapchain = VK_NULL_HANDLE;
  other.storage.image_count = 0;
  return *this;
}

auto Photophore::Swapchain::acquire_next_image(
    VkSemaphore signal_semaphore) const -> U32 {
  U32 index = 0;
  const VkResult result = Vulkan::vkAcquireNextImageKHR(
      storage.device, storage.swapchain, UINT64_MAX, signal_semaphore,
      VK_NULL_HANDLE, &index);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return UINT32_MAX;
  }
  // SUBOPTIMAL still acquires an image and signals the semaphore. Submitting
  // that image consumes the signal before this frame slot can be used again.
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    Diagnostics::Log::fatal("Vulkan: Failed to acquire swapchain image."_view);
  }
  return index;
}

auto Photophore::Swapchain::get_swapchain() const -> VkSwapchainKHR {
  return storage.swapchain;
}

auto Photophore::Swapchain::get_extent() const -> VkExtent2D {
  return storage.extent;
}

auto Photophore::Swapchain::get_format() const -> VkFormat {
  return storage.format;
}

auto Photophore::Swapchain::get_image(U32 index) const -> VkImage {
  return storage.images[index];
}

auto Photophore::Swapchain::get_image_view(U32 index) const -> VkImageView {
  return storage.image_views[index];
}

auto Photophore::Swapchain::get_image_count() const -> U32 {
  return storage.image_count;
}
