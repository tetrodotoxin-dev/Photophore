// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/perimortem.hpp"

#include "photophore/context.hpp"
#include "photophore/texture_2d.hpp"

namespace Photophore {

// TextureImage uploads a CPU pixel matrix into an sRGB Vulkan image and owns
// its memory and view. Upload uses a temporary staging buffer and waits for the
// queue before releasing it, so the source pixels are no longer borrowed when
// create returns. The device must outlive this resource, and GPU reads must
// have finished before destruction.
//
// Sampling policy lives in Texture. Separating it from this uploaded image lets
// Pipelines reuse one pixel allocation for several filters or addressing modes.
class TextureImage {
 public:
  static EXPORTED(PHOTOPHORE) auto create(
      const Context& context,
      const Photophore::Texture2D& resource) -> TextureImage;

  TextureImage() = default;
  EXPORTED(PHOTOPHORE) ~TextureImage();
  EXPORTED(PHOTOPHORE) TextureImage(TextureImage&&) noexcept;
  EXPORTED(PHOTOPHORE) auto operator=(TextureImage&&) noexcept -> TextureImage&;
  TextureImage(const TextureImage&) = delete;
  auto operator=(const TextureImage&) = delete;

  constexpr auto get_view() const -> VkImageView { return image_view; }

 private:
  VkDevice device = VK_NULL_HANDLE;
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView image_view = VK_NULL_HANDLE;
};

}  // namespace Photophore
