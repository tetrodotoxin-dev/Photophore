// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/perimortem.hpp"

#include "photophore/context.hpp"
#include "photophore/texture_2d.hpp"

namespace Photophore {

// Texture pairs a sampler with a borrowed image view in one descriptor set.
// It owns the sampler and a pool containing that set. Destroying the pool also
// releases the set, while the TextureImage remains owned separately.
//
// The supplied view and device must survive every use of this descriptor.
// Neither destruction nor replacement waits for the GPU. Pipelines destroys
// these bindings before the image resources they refer to.
class Texture {
 public:
  static EXPORTED(PHOTOPHORE) auto create(
      const Context& context,
      const Photophore::Texture2D& resource,
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

}  // namespace Photophore
