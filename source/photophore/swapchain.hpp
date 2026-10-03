// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/perimortem.hpp"

#include "photophore/context.hpp"

namespace Photophore {

// The presentation system supplies images in an order that is independent of
// Renderer's frame slots. Swapchain owns the native chain and a view for each
// returned image. It borrows the Context's device and surface, and its image
// handles are usable only until recreation or destruction.
//
// The caller must finish pending use of the old images before recreating the
// chain. Renderer waits for both submissions and presentation fences. The
// selected format may change, so dependent pipelines need rebuilding when
// resize reports a format change. An absent device or zero creation extent
// produces an empty chain. Once native creation begins, Vulkan failures use the
// fatal diagnostic path.
class Swapchain {
 public:
  static EXPORTED(
      PHOTOPHORE) auto create(const Context& ctx, U32 width, U32 height)
      -> Swapchain;

  Swapchain() = default;
  EXPORTED(PHOTOPHORE) ~Swapchain();
  EXPORTED(PHOTOPHORE) Swapchain(Swapchain&&) noexcept;
  EXPORTED(PHOTOPHORE) auto operator=(Swapchain&&) noexcept -> Swapchain&;
  Swapchain(const Swapchain&) = delete;
  auto operator=(const Swapchain&) = delete;

  // Destroys and recreates the swapchain at the new physical pixel dimensions.
  EXPORTED(PHOTOPHORE)
  auto recreate(const Context& ctx, U32 width, U32 height) -> void;

  // Acquires the next presentable image index. Blocks until one is available.
  // Returns UINT32_MAX on a resize signal (caller should recreate).
  EXPORTED(PHOTOPHORE)
  auto acquire_next_image(VkSemaphore signal_semaphore) const -> U32;

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

    Perimortem::Core::Static::Vector<VkImage, max_images> images;
    Perimortem::Core::Static::Vector<VkImageView, max_images> image_views;
    U32 image_count = 0;
  };

  Storage storage;
};

}  // namespace Photophore
