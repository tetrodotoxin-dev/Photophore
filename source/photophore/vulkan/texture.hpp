// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/perimortem.hpp"

#include "photophore/image/texture_2d.hpp"
#include "photophore/vulkan/context.hpp"

namespace Photophore::Vulkan {

// Texture owns the VkSampler and descriptor pool for one sampled Texture2D
// realization. The pool contains exactly one descriptor set whose binding zero
// refers to a TextureImage view. Separate Texture values allow one uploaded
// matrix to use several Sampler2D policies.
//
// Context's device and the supplied TextureImage view remain live through every
// recorded use. Pipelines releases matching Texture values after their frame
// completion, then releases the referenced TextureImage allocation.
class Texture {
 public:
  static EXPORTED(PHOTOPHORE) auto create(
      const Context& context,
      const Image::Texture2D& resource,
      VkImageView image_view,
      VkDescriptorSetLayout descriptor_set_layout) -> Texture;

  Texture() = default;
  EXPORTED(PHOTOPHORE) ~Texture();
  EXPORTED(PHOTOPHORE) Texture(Texture&&) noexcept;
  EXPORTED(PHOTOPHORE) auto operator=(Texture&&) noexcept -> Texture&;
  Texture(const Texture&) = delete;
  auto operator=(const Texture&) = delete;

  constexpr auto get_descriptor_set() const -> VkDescriptorSet {
    return descriptor_set;
  }

 private:
  VkDevice device = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
  VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
};

}  // namespace Photophore::Vulkan
