// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/vulkan/pipelines.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/math/scalar.hpp"
#include "perimortem/core/storage/vector.hpp"
#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/thread/bytes.hpp"

#include "photophore/vulkan/details/program.hpp"
#include "photophore/vulkan/dispatch.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Vulkan;
using namespace Perimortem::Memory;

static auto require_supported_descriptions(
    const Context& context,
    auto descriptions) -> void {
  switch (Details::Program::inspect(descriptions, context.supports_float64())) {
  case Details::Program::Status::Accepted:
    return;
  case Details::Program::Status::EmptyInventory:
    Diagnostics::Log::fatal(
        "Vulkan: No generated Programs were supplied."_view);
    return;
  case Details::Program::Status::InvalidDescription:
  case Details::Program::Status::UnsupportedFloat64:
    Diagnostics::Log::fatal(
        "Vulkan: A generated Program is not supported by this device."_view);
    return;
  case Details::Program::Status::InvalidDescriptor:
    Diagnostics::Log::fatal(
        "Vulkan: A generated descriptor is not supported by this device."_view);
    return;
  case Details::Program::Status::RepeatedDescriptorSet:
    Diagnostics::Log::fatal(
        "Vulkan: Generated texture descriptors use distinct sets."_view);
    return;
  case Details::Program::Status::RepeatedIdentity:
    Diagnostics::Log::fatal(
        "Vulkan: Generated Program identities must be unique."_view);
    return;
  }
}

static auto create_descriptor_layout(VkDevice device) -> VkDescriptorSetLayout {
  // Every accepted resource is one fragment sampled Texture2D at binding zero.
  // Each declared descriptor set can therefore use this same layout.
  VkDescriptorSetLayout descriptor_layout = VK_NULL_HANDLE;
  VkDescriptorSetLayoutBinding binding = {};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};

  info.bindingCount = 1;
  info.pBindings = &binding;
  if (Dispatch::get().vkCreateDescriptorSetLayout(
          device, &info, nullptr, &descriptor_layout) != VK_SUCCESS) {
    Diagnostics::Log::fatal(
        "Vulkan: Failed to create the generated descriptor layout."_view);
  }

  return descriptor_layout;
}

static auto create_vertex_buffer(
    const Context& context,
    VkBuffer& vertex_buffer,
    VkDeviceMemory& vertex_memory) -> void {
  // Two triangles cover normalized unit coordinates. Frame transforms place
  // the shared immutable geometry and scale it to the batch pixel extent.
  static constexpr Storage::Vector<Details::Program::UnitQuadVertex, 6>
      vertices = {
        {Details::Program::UnitQuadVertex{{{0.0f, 0.0f}}, {{0.0f, 0.0f}}},
         {{{1.0f, 0.0f}}, {{1.0f, 0.0f}}},
         {{{1.0f, 1.0f}}, {{1.0f, 1.0f}}},
         {{{0.0f, 0.0f}}, {{0.0f, 0.0f}}},
         {{{1.0f, 1.0f}}, {{1.0f, 1.0f}}},
         {{{0.0f, 1.0f}}, {{0.0f, 1.0f}}}}};

  constexpr Count vertex_bytes =
      vertices.get_size() * sizeof(Details::Program::UnitQuadVertex);

  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = vertex_bytes;
  buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (Dispatch::get().vkCreateBuffer(
          context.get_device(), &buffer_info, nullptr, &vertex_buffer) !=
      VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to create vertex buffer."_view);
  }

  VkMemoryRequirements requirements = {};
  Dispatch::get().vkGetBufferMemoryRequirements(
      context.get_device(), vertex_buffer, &requirements);

  constexpr VkMemoryPropertyFlags properties =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  U32 memory_type =
      context.find_memory_type(requirements.memoryTypeBits, properties);
  if (memory_type == U32(-1)) {
    Diagnostics::Log::fatal("Vulkan: No compatible vertex memory."_view);
  }

  VkMemoryAllocateInfo allocation = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocation.allocationSize = requirements.size;
  allocation.memoryTypeIndex = memory_type;
  if (Dispatch::get().vkAllocateMemory(
          context.get_device(), &allocation, nullptr, &vertex_memory) !=
          VK_SUCCESS ||
      Dispatch::get().vkBindBufferMemory(
          context.get_device(), vertex_buffer, vertex_memory, 0) !=
          VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to allocate vertex memory."_view);
  }

  void* mapped = nullptr;
  if (Dispatch::get().vkMapMemory(
          context.get_device(), vertex_memory, 0, vertex_bytes, 0, &mapped) !=
      VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to map vertex memory."_view);
  }

  Data::copy(
      Data::cast<U8>(mapped), Data::cast<const U8>(vertices.get_data()),
      vertex_bytes);
  Dispatch::get().vkUnmapMemory(context.get_device(), vertex_memory);
}

static auto destroy_vertex_buffer(
    const Context& context,
    VkBuffer& vertex_buffer,
    VkDeviceMemory& vertex_memory) -> void {
  if (vertex_buffer) {
    Dispatch::get().vkDestroyBuffer(
        context.get_device(), vertex_buffer, nullptr);
    vertex_buffer = VK_NULL_HANDLE;
  }

  if (vertex_memory) {
    Dispatch::get().vkFreeMemory(context.get_device(), vertex_memory, nullptr);
    vertex_memory = VK_NULL_HANDLE;
  }
}

static auto find_realization(
    auto& realizations,
    Identity::Uuid identity,
    auto pipeline) -> decltype(realizations.get_data()) {
  for (Count index = 0; index < realizations.get_size(); index++) {
    auto& realization = realizations.get_data()[index];
    if (realization.description->identity == identity &&
        realization.pipeline == pipeline) {
      return &realization;
    }
  }

  return nullptr;
}

static auto find_image(auto& images, const auto& content)
    -> decltype(images.get_data()) {
  for (Count index = 0; index < images.get_size(); index++) {
    auto& entry = images.get_data()[index];
    if (entry.content == content) {
      return &entry;
    }
  }

  return nullptr;
}

static auto find_texture(auto& textures, const auto& resource) -> Texture* {
  for (Count index = 0; index < textures.get_size(); index++) {
    auto& entry = textures.get_data()[index];
    if (entry.resource == resource) {
      return &entry.texture;
    }
  }

  return nullptr;
}

static auto realize_pipeline(
    const Context& context,
    auto descriptions,
    VkFormat color_format,
    VkDescriptorSetLayout descriptor_layout,
    auto& realizations,
    const auto& batch) -> decltype(realizations.get_data()) {
  const Identity::Uuid identity = batch.get_program().get_identity();
  auto pipeline = batch.get_pipeline();
  auto* retained = find_realization(realizations, identity, pipeline);
  if (retained != nullptr) {
    return retained;
  }

  const auto* description = Details::Program::find(descriptions, identity);
  if (description == nullptr || color_format == VK_FORMAT_UNDEFINED) {
    return nullptr;
  }

  // Vulkan indexes pipeline layouts densely through the highest descriptor set.
  // Filling each index preserves the Program's declared set numbers, including
  // programs that reserve intermediate sets for later generated stages.
  Count set_count = 0;
  for (Count index = 0; index < description->descriptors.get_size(); index++) {
    const auto& descriptor = description->descriptors[index];
    set_count = Math::Scalar::max(set_count, descriptor.set + 1);
  }

  Thread::Vector<VkDescriptorSetLayout> layouts;
  for (Count set = 0; set < set_count; set++) {
    layouts.insert(descriptor_layout);
  }

  Count index = realizations.get_size();
  // The cache constructs each ShaderProgram in typed storage. Clearing the
  // vector consequently destroys every native pipeline through the same path.
  realizations.resize(index + 1);

  auto& realization = realizations.get_data()[index];
  realization.description = description;
  realization.pipeline = pipeline;
  realization.shader = ShaderProgram::create(
      context.get_device(), color_format, *description, pipeline,
      layouts.get_view());
  return &realization;
}

auto Pipelines::realize_image(const Image::Texture2D& resource) -> ImageEntry* {
  const auto& content = resource.get_content();
  auto* retained = find_image(images, content);
  if (retained) {
    retained->used = True;
    return retained;
  }

  // Pixels2D identity is the upload key. Sampler policy belongs to
  // TextureEntry, so several Texture2D values can share one native image.
  ImageEntry created = {
    .content = content,
    .image = TextureImage::create(context, content),
    .used = True,
  };

  auto& entry = images.insert(Data::take(created));
  return &entry;
}

auto Pipelines::realize_texture(
    const Image::Texture2D& resource,
    const TextureImage& image) -> Texture* {
  Texture* retained = find_texture(textures, resource);
  if (retained) {
    return retained;
  }

  // Texture2D equality combines Pixels2D identity with sampler state, which is
  // the exact native state represented by a Texture cache entry.
  TextureEntry created = {
    .resource = resource,
    .texture = Texture::create(
        context, resource.get_sampler(), image.get_view(), descriptor_layout),
  };

  auto& entry = textures.insert(Data::take(created));
  return &entry.texture;
}

static auto make_host_inputs(
    const auto& description,
    const auto& batch,
    U32 width,
    U32 height) -> Thread::Bytes {
  Thread::Bytes inputs;
  inputs.append(0, description.host_size);

  auto access = inputs.get_access();
  // Parameter bytes already use the generated target layout. HostField roles
  // write the transform rows derived from current placement and presentation
  // extent without changing the parameter range.
  if (description.parameters_size != 0) {
    Data::copy(
        access.get_data() + description.parameters_offset,
        batch.get_inputs().get_data(), description.parameters_size);
  }

  const auto& transform = batch.get_transform();
  const auto size = batch.get_size_pixels();
  for (Count field_index = 0; field_index < description.host_fields.get_size();
       field_index++) {
    const auto& field = description.host_fields[field_index];
    if (field.offset > description.host_size ||
        field.size > description.host_size - field.offset) {
      return {};
    }

    if (field.role == decltype(field.role)::Parameter) {
      if (field.offset < description.parameters_offset ||
          field.offset + field.size >
              description.parameters_offset + description.parameters_size) {
        return {};
      }

      continue;
    }

    if (field.size != sizeof(R32) * 4) {
      return {};
    }

    Storage::Vector<R32, 4> values;
    if (field.role == decltype(field.role)::TransformX) {
      values[0] = R32(transform.get_xx() * size[0]);
      values[1] = R32(transform.get_xy() * size[1]);
      values[2] = R32(transform.get_x());
      values[3] = R32(width);
    } else if (field.role == decltype(field.role)::TransformY) {
      values[0] = R32(transform.get_yx() * size[0]);
      values[1] = R32(transform.get_yy() * size[1]);
      values[2] = R32(transform.get_y());
      values[3] = R32(height);
    }

    Data::copy(
        access.get_data() + field.offset, values.get_data(), values.get_size());
  }

  return inputs;
}

static auto sweep_textures(const Context& context, auto& images, auto& textures)
    -> void {
  // Content not selected by the current recording pass can leave the cache
  // after its last GPU use completes. Application references do not control
  // backend retention. Selecting the Texture2D again recreates its realization.
  Count index = 0;
  while (index < images.get_size()) {
    auto& image = images.get_data()[index];
    if (image.used) {
      index++;
      continue;
    }

    // Renderer advances a slot generation after waiting for its preceding
    // submission. A changed generation therefore proves this image's recorded
    // use completed even while the fence represents a newer frame.
    if (image.completion && *image.generation == image.observed_generation) {
      VkResult status = Dispatch::get().vkGetFenceStatus(
          context.get_device(), image.completion);
      if (status == VK_NOT_READY) {
        index++;
        continue;
      }

      if (status != VK_SUCCESS) {
        Diagnostics::Log::fatal(
            "Photophore: Failed to observe resource completion."_view);
      }
    }

    // Descriptor sets refer to the image view. Releasing matching Texture
    // entries first preserves the Vulkan lifetime order for that reference.
    Count texture_index = 0;
    while (texture_index < textures.get_size()) {
      const auto& texture = textures.get_data()[texture_index];
      if (!(texture.resource.get_content() == image.content)) {
        texture_index++;
        continue;
      }

      textures.remove(texture_index);
    }

    images.remove(index);
  }
}

Pipelines::Pipelines(
    const Context& context,
    VkFormat color_format,
    Descriptions descriptions)
    : context(context), descriptions(descriptions) {
  require_supported_descriptions(context, descriptions);
  descriptor_layout = create_descriptor_layout(context.get_device());
  create_vertex_buffer(context, vertex_buffer, vertex_memory);
  rebuild(color_format);
}

Pipelines::~Pipelines() {
  // Device completion proves that no queued command refers to a cached object.
  // Destruction then releases descriptor sets before image views, shader
  // pipelines before their common layout, and finally the vertex buffer.
  Dispatch::get().vkDeviceWaitIdle(context.get_device());
  textures.clear();
  images.clear();
  realizations.clear();
  destroy_vertex_buffer(context, vertex_buffer, vertex_memory);
  if (descriptor_layout) {
    Dispatch::get().vkDestroyDescriptorSetLayout(
        context.get_device(), descriptor_layout, nullptr);
  }
}

auto Pipelines::rebuild(VkFormat color_format) -> void {
  // The caller finishes use of the old pipelines before rebuilding. Only
  // shader realizations depend on the attachment format, so uploaded images
  // and sampled descriptors remain reusable across this change.
  realizations.clear();
  this->color_format = color_format;
}

auto Pipelines::record(const Renderer::Frame& frame, Batches batches) -> Bool {
  VkCommandBuffer command_buffer = frame.command_buffer;
  U32 width = frame.width;
  U32 height = frame.height;
  if (!command_buffer || width == 0 || height == 0 || !frame.completion ||
      !frame.completion_generation ||
      !Details::Program::accepts(descriptions, batches)) {
    return False;
  }

  for (Count index = 0; index < images.get_size(); index++) {
    images.get_data()[index].used = False;
  }

  // The first pass creates every missing pipeline, image and descriptor set.
  // The second pass can then record the complete batch sequence without native
  // allocation between draw commands. Host input bytes use this frame's
  // presentation extent.
  for (Count batch_index = 0; batch_index < batches.get_size(); batch_index++) {
    const auto& batch = batches[batch_index];
    auto* program = realize_pipeline(
        context, descriptions, color_format, descriptor_layout, realizations,
        batch);
    if (program == nullptr) {
      return False;
    }

    const auto resources = batch.get_resources();
    for (Count resource_index = 0; resource_index < resources.get_size();
         resource_index++) {
      const auto& resource = resources.get_data()[resource_index];
      auto* image = realize_image(resource);
      if (image == nullptr) {
        return False;
      }

      auto* texture = realize_texture(resource, image->image);
      if (texture == nullptr) {
        return False;
      }
    }
  }

  for (Count batch_index = 0; batch_index < batches.get_size(); batch_index++) {
    const auto& batch = batches[batch_index];
    auto* realization = find_realization(
        realizations, batch.get_program().get_identity(), batch.get_pipeline());
    if (realization == nullptr) {
      return False;
    }

    Thread::Bytes inputs =
        make_host_inputs(*realization->description, batch, width, height);
    if (inputs.get_size() != realization->description->host_size) {
      return False;
    }

    realization->shader.bind(command_buffer);

    constexpr VkDeviceSize vertex_offset = 0;
    Dispatch::get().vkCmdBindVertexBuffers(
        command_buffer, 0, 1, &vertex_buffer, &vertex_offset);
    for (Count descriptor_index = 0;
         descriptor_index < realization->description->descriptors.get_size();
         descriptor_index++) {
      const auto& resource = batch.get_resources().get_data()[descriptor_index];
      Texture* texture = find_texture(textures, resource);
      auto* image = find_image(images, resource.get_content());
      if (image == nullptr) {
        return False;
      }

      image->completion = frame.completion;
      image->generation = frame.completion_generation;
      image->observed_generation = *frame.completion_generation;
      if (texture == nullptr) {
        return False;
      }

      realization->shader.bind_descriptor_set(
          command_buffer, texture->get_descriptor_set(),
          realization->description->descriptors[descriptor_index].set);
    }

    realization->shader.push_constants(command_buffer, inputs.get_view());
    realization->shader.draw(command_buffer, batch.get_vertex_count());
  }

  sweep_textures(context, images, textures);
  return True;
}
