// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/vulkan/texture_image.hpp"

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "implementation/vulkan/dispatch.hpp"

using namespace Photophore::Vulkan;

static auto require_success(
    VkResult result,
    Perimortem::Core::View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Perimortem::Core::Diagnostics::Log::fatal(message);
  }
}

static auto allocate_memory(
    const Context& context,
    VkMemoryRequirements requirements,
    VkMemoryPropertyFlags properties) -> VkDeviceMemory {
  // Context interprets the device specific type mask while this helper keeps
  // allocation size and requested memory properties beside the Vulkan call.
  VkMemoryAllocateInfo info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  info.allocationSize = requirements.size;
  info.memoryTypeIndex =
      context.find_memory_type(requirements.memoryTypeBits, properties);
  if (info.memoryTypeIndex == UINT32_MAX) {
    Perimortem::Core::Diagnostics::Log::fatal(
        "Vulkan: No compatible texture memory type found."_view);
  }

  VkDeviceMemory memory = VK_NULL_HANDLE;
  require_success(
      Photophore::Vulkan::vkAllocateMemory(
          context.get_device(), &info, nullptr, &memory),
      "Vulkan: Failed to allocate texture memory."_view);
  return memory;
}

static auto transition_image_layout(
    VkCommandBuffer command_buffer,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkPipelineStageFlags2 source_stage,
    VkAccessFlags2 source_access,
    VkPipelineStageFlags2 destination_stage,
    VkAccessFlags2 destination_access) -> void {
  // Texture uploads use synchronization2 barriers so each layout transition
  // also names the producing and consuming access scopes for this color image.
  VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  barrier.srcStageMask = source_stage;
  barrier.srcAccessMask = source_access;
  barrier.dstStageMask = destination_stage;
  barrier.dstAccessMask = destination_access;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkDependencyInfo dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  dependency.imageMemoryBarrierCount = 1;
  dependency.pImageMemoryBarriers = &barrier;
  Photophore::Vulkan::vkCmdPipelineBarrier2(command_buffer, &dependency);
}

auto Photophore::Vulkan::TextureImage::create(
    const Context& context,
    const Photophore::Image::Texture2D& resource) -> TextureImage {
  TextureImage texture;
  texture.device = context.get_device();

  BAIL_IF(!resource.is_drawable());
  const auto& source_image = resource.get_image();
  Perimortem::Core::View::Vector<Photophore::Image::Srgba8> pixels =
      source_image.get_values();
  const U32 width = source_image.get_column_count();
  const U32 height = source_image.get_row_count();
  const VkDeviceSize image_size = VkDeviceSize(width) * height *
                                  Photophore::Image::Srgba8::get_byte_count();
  if (width == 0 || height == 0 || resource.is_empty() ||
      pixels.get_size() * sizeof(Photophore::Image::Srgba8) < image_size) {
    return texture;
  }

  // The device selects its optimal image tiling. A tightly packed host coherent
  // buffer bridges the row major Srgba8 Matrix into that native image layout.
  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = image_size;
  buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer staging = VK_NULL_HANDLE;
  require_success(
      Photophore::Vulkan::vkCreateBuffer(
          context.get_device(), &buffer_info, nullptr, &staging),
      "Vulkan: Failed to create texture staging buffer."_view);

  VkMemoryRequirements staging_requirements = {};
  Photophore::Vulkan::vkGetBufferMemoryRequirements(
      context.get_device(), staging, &staging_requirements);
  constexpr VkMemoryPropertyFlags host_flags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  VkDeviceMemory staging_memory =
      allocate_memory(context, staging_requirements, host_flags);
  require_success(
      Photophore::Vulkan::vkBindBufferMemory(
          context.get_device(), staging, staging_memory, 0),
      "Vulkan: Failed to bind texture staging memory."_view);

  void* mapped = nullptr;
  require_success(
      Photophore::Vulkan::vkMapMemory(
          context.get_device(), staging_memory, 0, image_size, 0, &mapped),
      "Vulkan: Failed to map texture staging memory."_view);
  Perimortem::Core::View::Bytes source_bytes(
      Perimortem::Core::Data::cast<const U8>(pixels.get_data()),
      Count(image_size));
  Perimortem::Core::Access::Bytes destination_bytes(
      Perimortem::Core::Data::cast<U8>(mapped), Count(image_size));
  for (Count index = 0; index < source_bytes.get_size(); index++) {
    destination_bytes.get_data()[index] = source_bytes[index];
  }
  Photophore::Vulkan::vkUnmapMemory(context.get_device(), staging_memory);

  VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  image_info.extent = {width, height, 1};
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  require_success(
      Photophore::Vulkan::vkCreateImage(
          context.get_device(), &image_info, nullptr, &texture.image),
      "Vulkan: Failed to create texture image."_view);

  VkMemoryRequirements image_requirements = {};
  Photophore::Vulkan::vkGetImageMemoryRequirements(
      context.get_device(), texture.image, &image_requirements);
  texture.memory = allocate_memory(
      context, image_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  require_success(
      Photophore::Vulkan::vkBindImageMemory(
          context.get_device(), texture.image, texture.memory, 0),
      "Vulkan: Failed to bind texture image memory."_view);

  VkCommandBuffer command_buffer = context.begin_immediate_commands();
  transition_image_layout(
      command_buffer, texture.image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
      VkAccessFlags2(0), VK_PIPELINE_STAGE_2_TRANSFER_BIT,
      VK_ACCESS_2_TRANSFER_WRITE_BIT);
  VkBufferImageCopy region = {};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.layerCount = 1;
  region.imageExtent = {width, height, 1};
  Photophore::Vulkan::vkCmdCopyBufferToImage(
      command_buffer, staging, texture.image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  transition_image_layout(
      command_buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
  // Immediate submission completes the transfer and shader read transition.
  // Its return establishes both staging retirement and sampled view creation.
  context.submit_immediate_commands(command_buffer);

  Photophore::Vulkan::vkDestroyBuffer(context.get_device(), staging, nullptr);
  Photophore::Vulkan::vkFreeMemory(
      context.get_device(), staging_memory, nullptr);

  VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  view_info.image = texture.image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.layerCount = 1;
  require_success(
      Photophore::Vulkan::vkCreateImageView(
          context.get_device(), &view_info, nullptr, &texture.image_view),
      "Vulkan: Failed to create texture image view."_view);
  return texture;
}

static auto release(
    VkDevice device,
    VkImage image,
    VkDeviceMemory memory,
    VkImageView image_view) -> void {
  if (!device) {
    return;
  }

  // The view refers to the image, and the image binds the allocation. Releasing
  // them in that dependency order mirrors successful creation.
  Photophore::Vulkan::vkDestroyImageView(device, image_view, nullptr);
  Photophore::Vulkan::vkDestroyImage(device, image, nullptr);
  Photophore::Vulkan::vkFreeMemory(device, memory, nullptr);
}

Photophore::Vulkan::TextureImage::~TextureImage() {
  release(device, image, memory, image_view);
}

Photophore::Vulkan::TextureImage::TextureImage(TextureImage&& other) noexcept
    : device(other.device),
      image(other.image),
      memory(other.memory),
      image_view(other.image_view) {
  other.device = VK_NULL_HANDLE;
}

auto Photophore::Vulkan::TextureImage::operator=(TextureImage&& other) noexcept
    -> TextureImage& {
  if (this == &other) {
    return *this;
  }

  release(device, image, memory, image_view);
  device = other.device;
  image = other.image;
  memory = other.memory;
  image_view = other.image_view;
  other.device = VK_NULL_HANDLE;
  return *this;
}
