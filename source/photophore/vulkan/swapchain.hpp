// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "toolchain/toolchain.hpp"

#include "perimortem/core/storage/vector.hpp"

#include "photophore/vulkan/context.hpp"

namespace Photophore::Vulkan {

// Swapchain contains one `VkSwapchainKHR` and one `VkImageView` for every image
// returned by its surface. The `Context` device and surface remain accessible
// through this value. `Renderer` pairs acquired image indices with separate
// presentation fences because image order is independent of rotating frame
// slots.
//
// `create` returns an empty value for an empty device or extent. A populated
// value contains at most `max_images` handles and remains valid through
// recreation or destruction. `Renderer` settles submission and presentation use
// before `recreate`, then reports a color format change so `Pipelines` can
// rebuild its format dependent shader realizations. Native creation failures
// use the fatal diagnostic path after Vulkan allocation begins.
class Swapchain {
 public:
  static EXPORTED(PHOTOPHORE) auto
      create(const Context& ctx, U32 width, U32 height) -> Swapchain;

  Swapchain() = default;
  EXPORTED(PHOTOPHORE) ~Swapchain();
  EXPORTED(PHOTOPHORE) Swapchain(Swapchain&&) noexcept;
  EXPORTED(PHOTOPHORE) auto operator=(Swapchain&&) noexcept -> Swapchain&;
  Swapchain(const Swapchain&) = delete;
  auto operator=(const Swapchain&) -> Swapchain& = delete;

  // Replaces the chain at the new physical pixel dimensions after Renderer has
  // settled use of the current images.
  EXPORTED(PHOTOPHORE) auto recreate(const Context& ctx, U32 width, U32 height)
      -> void;

  // Waits for the next presentable image and signals the supplied semaphore.
  // U32(-1) carries Vulkan's resize request to Renderer.
  EXPORTED(PHOTOPHORE) auto acquire_next_image(
      VkSemaphore signal_semaphore) const -> U32;

  EXPORTED(PHOTOPHORE) auto get_swapchain() const -> VkSwapchainKHR;
  EXPORTED(PHOTOPHORE) auto get_extent() const -> VkExtent2D;
  EXPORTED(PHOTOPHORE) auto get_format() const -> VkFormat;
  EXPORTED(PHOTOPHORE) auto get_image(U32 index) const -> VkImage;
  EXPORTED(PHOTOPHORE) auto get_image_view(U32 index) const -> VkImageView;
  EXPORTED(PHOTOPHORE) auto get_image_count() const -> U32;

  static constexpr U32 max_images = 8;

 private:
  struct Storage {
    VkDevice device = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent = {};

    Perimortem::Core::Storage::Vector<VkImage, max_images> images;
    Perimortem::Core::Storage::Vector<VkImageView, max_images> image_views;
    U32 image_count = 0;
  };

  Storage storage;
};

}  // namespace Photophore::Vulkan
