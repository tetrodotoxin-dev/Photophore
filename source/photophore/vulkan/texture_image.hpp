// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "toolchain/toolchain.hpp"

#include "photophore/image/texture_2d.hpp"
#include "photophore/vulkan/context.hpp"

namespace Photophore::Vulkan {

// TextureImage owns one sRGB VkImage, its device memory and its sampled view
// for a shared Srgba8 Matrix. `create` stages tightly packed pixels, records
// the layout transitions and waits for the graphics queue. A successful return
// therefore contains an independent device copy and permits immediate staging
// release.
//
// Context's device remains live through this value, and Pipelines observes the
// final frame completion before destruction. Sampling policy lives in Texture,
// allowing one TextureImage to serve several filters and addressing modes.
class TextureImage {
 public:
  static EXPORTED(PHOTOPHORE) auto create(
      const Context& context,
      const Image::Texture2D& resource) -> TextureImage;

  TextureImage() = default;
  EXPORTED(PHOTOPHORE) ~TextureImage();
  EXPORTED(PHOTOPHORE) TextureImage(TextureImage&&) noexcept;
  EXPORTED(PHOTOPHORE) auto operator=(TextureImage&&) noexcept -> TextureImage&;
  TextureImage(const TextureImage&) = delete;
  auto operator=(const TextureImage&) -> TextureImage& = delete;

  constexpr auto get_view() const -> VkImageView { return image_view; }

 private:
  VkDevice device = VK_NULL_HANDLE;
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView image_view = VK_NULL_HANDLE;
};

}  // namespace Photophore::Vulkan
