// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "toolchain/toolchain.hpp"

#include "photophore/image/texture_2d.hpp"
#include "photophore/vulkan/context.hpp"

namespace Photophore::Vulkan {

// Texture is the Vulkan sampling realization of one Texture2D. It contains a
// VkSampler and a descriptor pool with exactly one set whose binding zero names
// the supplied TextureImage view. Separate Texture values pair one uploaded
// Matrix with different Sampler2D policies.
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
  auto operator=(const Texture&) -> Texture& = delete;

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
