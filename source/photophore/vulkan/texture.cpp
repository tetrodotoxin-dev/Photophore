// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/vulkan/texture.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/view/null_terminated.hpp"

#include "implementation/vulkan/dispatch.hpp"
#include "photophore/image/sampler_2d.hpp"

using namespace Photophore::Vulkan;
using namespace Perimortem::Core;
using namespace Photophore::Image;

static auto require_success(VkResult result, View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Diagnostics::Log::fatal(message);
  }
}

auto Texture::create(
    const Context& context,
    const Texture2D& resource,
    VkImageView image_view,
    VkDescriptorSetLayout descriptor_set_layout) -> Texture {
  Texture texture;
  if (image_view == VK_NULL_HANDLE || resource.is_empty()) {
    return texture;
  }

  texture.device = context.get_device();

  // Sampler2D is the application facing value for Vulkan filter and address
  // state. One conversion point keeps cache equality aligned with native state.
  VkSamplerCreateInfo sampler_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  VkFilter filter = VK_FILTER_LINEAR;
  switch (resource.get_sampler().get_filtering()) {
  case Sampler2D::Filtering::Linear:
    filter = VK_FILTER_LINEAR;
    break;
  case Sampler2D::Filtering::Nearest:
    filter = VK_FILTER_NEAREST;
    break;
  }

  sampler_info.magFilter = filter;
  sampler_info.minFilter = filter;

  VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  switch (resource.get_sampler().get_addressing()) {
  case Sampler2D::Addressing::Zero:
    address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    break;
  case Sampler2D::Addressing::Clamp:
    address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    break;
  case Sampler2D::Addressing::Wrap:
    address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    break;
  }

  sampler_info.addressModeU = address_mode;
  sampler_info.addressModeV = address_mode;
  sampler_info.addressModeW = address_mode;
  require_success(
      Dispatch::vkCreateSampler(
          context.get_device(), &sampler_info, nullptr, &texture.sampler),
      "Vulkan: Failed to create texture sampler."_view);

  // Each cached sampler binding receives one descriptor set from its own pool.
  // Texture can therefore release that binding independently while another
  // sampler continues to refer to the same uploaded image.
  VkDescriptorPoolSize pool_size = {};
  pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size.descriptorCount = 1;

  VkDescriptorPoolCreateInfo pool_info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};

  pool_info.maxSets = 1;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;
  require_success(
      Dispatch::vkCreateDescriptorPool(
          context.get_device(), &pool_info, nullptr, &texture.descriptor_pool),
      "Vulkan: Failed to create texture descriptor pool."_view);

  VkDescriptorSetAllocateInfo set_info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};

  set_info.descriptorPool = texture.descriptor_pool;
  set_info.descriptorSetCount = 1;
  set_info.pSetLayouts = &descriptor_set_layout;
  require_success(
      Dispatch::vkAllocateDescriptorSets(
          context.get_device(), &set_info, &texture.descriptor_set),
      "Vulkan: Failed to allocate texture descriptor set."_view);

  VkDescriptorImageInfo descriptor = {};
  // Pipelines creates every sampled image layout with binding zero. The write
  // stores this sampler and the image view in that binding. TextureImage has
  // already transitioned the view to shader read layout before this call.
  descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  descriptor.imageView = image_view;
  descriptor.sampler = texture.sampler;

  VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = texture.descriptor_set;
  write.dstBinding = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &descriptor;
  Dispatch::vkUpdateDescriptorSets(context.get_device(), 1, &write, 0, nullptr);
  return texture;
}

static auto release(
    VkDevice device,
    VkDescriptorPool descriptor_pool,
    VkSampler sampler) -> void {
  if (!device) {
    return;
  }

  // Destroying the pool releases its descriptor set before the sampler named
  // by that set, preserving their native reference order.
  Dispatch::vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
  Dispatch::vkDestroySampler(device, sampler, nullptr);
}

Texture::~Texture() {
  release(device, descriptor_pool, sampler);
}

Texture::Texture(Texture&& other) noexcept
    : device(other.device),
      sampler(other.sampler),
      descriptor_pool(other.descriptor_pool),
      descriptor_set(other.descriptor_set) {
  other.device = VK_NULL_HANDLE;
}

auto Texture::operator=(Texture&& other) noexcept -> Texture& {
  if (this == &other) {
    return *this;
  }

  release(device, descriptor_pool, sampler);
  device = other.device;
  sampler = other.sampler;
  descriptor_pool = other.descriptor_pool;
  descriptor_set = other.descriptor_set;
  other.device = VK_NULL_HANDLE;
  return *this;
}
