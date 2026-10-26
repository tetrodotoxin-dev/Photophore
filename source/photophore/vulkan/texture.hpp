// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "toolchain/toolchain.hpp"

#include "photophore/image/sampler_2d.hpp"
#include "photophore/vulkan/context.hpp"

namespace Photophore::Vulkan {

// Texture realizes one Sampler2D policy for a supplied TextureImage view. It
// contains a VkSampler and a descriptor pool with exactly one set whose binding
// zero names that image view. Separate Texture values can therefore sample one
// uploaded Pixels2D using different policies.
//
// Context's device and the supplied TextureImage view remain live through every
// recorded use. Pipelines releases matching Texture values after their frame
// completion, then releases the referenced TextureImage allocation.
class Texture {
 public:
  static EXPORTED(PHOTOPHORE) auto create(
      const Context& context,
      Image::Sampler2D sampler,
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
