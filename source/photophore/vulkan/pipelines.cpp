// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/vulkan/pipelines.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/scalar.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "implementation/vulkan/dispatch.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Vulkan;

struct UnitQuadVertex {
  R32 position[2];
  R32 texture_uv[2];
};

// Pipelines supplies one fixed interleaved quad buffer. Description admission
// proves that shader locations, component widths and stride consume this exact
// record, keeping vertex allocation independent of each Program description.
static auto supports_unit_quad(
    const Photophore::Description::Program& description) -> Bool {
  auto inputs = description.vertex_inputs;
  return Bool(
      inputs.get_size() == 2 && inputs.get_data()[0].location == 0 &&
      inputs.get_data()[0].components == 2 &&
      inputs.get_data()[0].offset == 0 &&
      inputs.get_data()[0].stride == sizeof(UnitQuadVertex) &&
      inputs.get_data()[1].location == 1 &&
      inputs.get_data()[1].components == 2 &&
      inputs.get_data()[1].offset == sizeof(R32) * 2 &&
      inputs.get_data()[1].stride == sizeof(UnitQuadVertex));
}

static auto validate_descriptions(
    const Context& context,
    View::Vector<Photophore::Description::Program> descriptions) -> void {
  // The borrowed Program inventory becomes the complete realization catalogue
  // for this Pipelines instance. Admission establishes one supported rendering
  // profile and a unique locator for every entry before cache state is created.
  if (descriptions.is_empty()) {
    Diagnostics::Log::fatal(
        "Vulkan: No generated Programs were supplied."_view);
  }
  for (Count index = 0; index < descriptions.get_size(); index++) {
    const Photophore::Description::Program& description =
        descriptions.get_data()[index];
    if (description.locator == nullptr || description.modules.is_empty() ||
        description.host_input_ranges.get_size() != 1 ||
        description.host_size == 0 ||
        description.parameters_offset > description.host_size ||
        description.parameters_size >
            description.host_size - description.parameters_offset ||
        description.descriptors.is_empty() ||
        (description.requires_float64 && !context.supports_float64())) {
      Diagnostics::Log::fatal(
          "Vulkan: A generated Program is not supported by this device."_view);
    }
    for (Count descriptor_index = 0;
         descriptor_index < description.descriptors.get_size();
         descriptor_index++) {
      const Photophore::Description::DescriptorBinding& descriptor =
          description.descriptors[descriptor_index];
      if (descriptor.slot != 0 ||
          descriptor.resource !=
              Photophore::Description::Resource::SampledTexture2D) {
        Diagnostics::Log::fatal(
            "Vulkan: A generated descriptor is not supported by this device."_view);
      }
      for (Count prior = 0; prior < descriptor_index; prior++) {
        if (description.descriptors[prior].set == descriptor.set) {
          Diagnostics::Log::fatal(
              "Vulkan: Generated texture descriptors use distinct sets."_view);
        }
      }
    }
    for (Count previous = 0; previous < index; previous++) {
      if (descriptions.get_data()[previous].locator == description.locator) {
        Diagnostics::Log::fatal(
            "Vulkan: Generated Program locators must be unique."_view);
      }
    }
  }
}

static auto create_descriptor_layout(VkDevice device) -> VkDescriptorSetLayout {
  // Every admitted resource is one fragment sampled Texture2D at binding zero.
  // One shared layout can therefore represent each declared descriptor set.
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
  if (Photophore::Vulkan::vkCreateDescriptorSetLayout(
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
  static constexpr UnitQuadVertex vertices[] = {
    {{0.0f, 0.0f}, {0.0f, 0.0f}}, {{1.0f, 0.0f}, {1.0f, 0.0f}},
    {{1.0f, 1.0f}, {1.0f, 1.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}},
    {{1.0f, 1.0f}, {1.0f, 1.0f}}, {{0.0f, 1.0f}, {0.0f, 1.0f}},
  };

  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = sizeof(vertices);
  buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (Photophore::Vulkan::vkCreateBuffer(
          context.get_device(), &buffer_info, nullptr, &vertex_buffer) !=
      VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to create vertex buffer."_view);
  }

  VkMemoryRequirements requirements = {};
  Photophore::Vulkan::vkGetBufferMemoryRequirements(
      context.get_device(), vertex_buffer, &requirements);
  constexpr VkMemoryPropertyFlags properties =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  U32 memory_type =
      context.find_memory_type(requirements.memoryTypeBits, properties);
  if (memory_type == UINT32_MAX) {
    Diagnostics::Log::fatal("Vulkan: No compatible vertex memory."_view);
  }
  VkMemoryAllocateInfo allocation = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocation.allocationSize = requirements.size;
  allocation.memoryTypeIndex = memory_type;
  if (Photophore::Vulkan::vkAllocateMemory(
          context.get_device(), &allocation, nullptr, &vertex_memory) !=
          VK_SUCCESS ||
      Photophore::Vulkan::vkBindBufferMemory(
          context.get_device(), vertex_buffer, vertex_memory, 0) !=
          VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to allocate vertex memory."_view);
  }

  void* mapped = nullptr;
  if (Photophore::Vulkan::vkMapMemory(
          context.get_device(), vertex_memory, 0, sizeof(vertices), 0,
          &mapped) != VK_SUCCESS) {
    Diagnostics::Log::fatal("Vulkan: Failed to map vertex memory."_view);
  }
  Data::copy(
      Data::cast<U8>(mapped), Data::cast<const U8>(vertices), sizeof(vertices));
  Photophore::Vulkan::vkUnmapMemory(context.get_device(), vertex_memory);
}

static auto destroy_vertex_buffer(
    const Context& context,
    VkBuffer& vertex_buffer,
    VkDeviceMemory& vertex_memory) -> void {
  if (vertex_buffer) {
    Photophore::Vulkan::vkDestroyBuffer(
        context.get_device(), vertex_buffer, nullptr);
    vertex_buffer = VK_NULL_HANDLE;
  }
  if (vertex_memory) {
    Photophore::Vulkan::vkFreeMemory(
        context.get_device(), vertex_memory, nullptr);
    vertex_memory = VK_NULL_HANDLE;
  }
}

static auto find_description(
    View::Vector<Photophore::Description::Program> descriptions,
    const U8* locator) -> const Photophore::Description::Program* {
  for (Count index = 0; index < descriptions.get_size(); index++) {
    const Photophore::Description::Program& description =
        descriptions.get_data()[index];
    if (description.locator == locator) {
      return &description;
    }
  }

  return nullptr;
}

static auto find_realization(
    auto& realizations,
    const U8* locator,
    Photophore::Frame::Pipeline pipeline) -> decltype(&realizations[0]) {
  for (Count index = 0; index < realizations.get_size(); index++) {
    auto& realization = realizations[index];
    if (realization.description->locator == locator &&
        realization.pipeline == pipeline) {
      return &realization;
    }
  }
  return nullptr;
}

static auto find_image(
    auto& images,
    const Photophore::Image::Texture2D& resource) -> decltype(&images[0]) {
  for (Count index = 0; index < images.get_size(); index++) {
    auto& entry = images[index];
    if (&entry.resource.get_image() == &resource.get_image()) {
      return &entry;
    }
  }

  return nullptr;
}

static auto find_texture(
    auto& textures,
    const Photophore::Image::Texture2D& resource) -> Texture* {
  for (Count index = 0; index < textures.get_size(); index++) {
    auto& entry = textures[index];
    if (entry.resource.matches(resource)) {
      return &entry.texture;
    }
  }

  return nullptr;
}

static auto realize_pipeline(
    const Context& context,
    View::Vector<Photophore::Description::Program> descriptions,
    VkFormat color_format,
    VkDescriptorSetLayout descriptor_layout,
    auto& realizations,
    const Photophore::Frame::Batch& batch) -> decltype(&realizations[0]) {
  const U8* locator = batch.get_program().get_locator();
  Photophore::Frame::Pipeline pipeline = batch.get_pipeline();
  auto* retained = find_realization(realizations, locator, pipeline);
  if (retained != nullptr) {
    return retained;
  }

  const Photophore::Description::Program* description =
      find_description(descriptions, locator);
  if (description == nullptr || color_format == VK_FORMAT_UNDEFINED) {
    return nullptr;
  }

  // Vulkan indexes pipeline layouts densely through the highest descriptor set.
  // Filling each index preserves the Program's declared set numbers, including
  // programs that reserve intermediate sets for later generated stages.
  Count set_count = 0;
  for (const Photophore::Description::DescriptorBinding& descriptor :
       description->descriptors) {
    set_count = Perimortem::Core::Scalar::max(set_count, descriptor.set + 1);
  }
  Perimortem::Memory::Dynamic::Vector<VkDescriptorSetLayout> layouts;
  for (Count set = 0; set < set_count; set++) {
    layouts.insert(descriptor_layout);
  }

  Count index = realizations.get_size();
  // Direct typed storage gives Pipelines one stable destruction path for each
  // native shader realization admitted to the cache.
  realizations.resize(index + 1);
  auto& realization = realizations[index];
  realization.description = description;
  realization.pipeline = pipeline;
  realization.shader = ShaderProgram::create(
      context.get_device(), color_format, *description, pipeline,
      layouts.get_view());
  return &realization;
}

static auto realize_image(
    const Context& context,
    auto& images,
    const Photophore::Image::Texture2D& resource) -> decltype(&images[0]) {
  auto* retained = find_image(images, resource);
  if (retained) {
    return retained;
  }

  Count index = images.get_size();
  // Matrix identity is the upload key. Sampler policy belongs to TextureEntry,
  // allowing several Texture2D values to share these image bytes.
  images.resize(index + 1);
  auto& entry = images[index];
  entry.resource = resource;
  entry.image = TextureImage::create(context, resource);
  return &entry;
}

static auto realize_texture(
    const Context& context,
    auto& textures,
    VkDescriptorSetLayout descriptor_layout,
    const Photophore::Image::Texture2D& resource,
    const TextureImage& image) -> Texture* {
  Texture* retained = find_texture(textures, resource);
  if (retained) {
    return retained;
  }

  Count index = textures.get_size();
  // Texture2D equality combines Matrix identity with sampler state, which is
  // the exact native state represented by a Texture cache entry.
  textures.resize(index + 1);
  auto& entry = textures[index];
  entry.resource = resource;
  entry.texture =
      Texture::create(context, resource, image.get_view(), descriptor_layout);
  return &entry.texture;
}

static auto validate(
    View::Vector<Photophore::Description::Program> descriptions,
    View::Vector<Photophore::Frame::Batch> batches) -> Bool {
  // Batch admission precedes command recording. Each batch matches the
  // selected Program description and the fixed unit quad pipeline profile.
  for (const Photophore::Frame::Batch& batch : batches) {
    const Photophore::Description::Program* description =
        find_description(descriptions, batch.get_program().get_locator());
    Photophore::Frame::Pipeline pipeline = batch.get_pipeline();
    BAIL_IF(
        description == nullptr || batch.get_vertex_count() == 0 ||
        batch.get_vertex_count() > U32(-1) ||
        pipeline.get_geometry() !=
            Photophore::Frame::Pipeline::Geometry::UnitQuad2D ||
        pipeline.get_topology() !=
            Photophore::Frame::Pipeline::Topology::TriangleList ||
        pipeline.get_blend() != Photophore::Frame::Pipeline::Blend::Alpha ||
        !supports_unit_quad(*description) ||
        batch.get_resources().get_size() !=
            description->descriptors.get_size() ||
        batch.get_inputs().get_size() != description->parameters_size ||
        batch.get_size_pixels().is_empty());
    for (const Photophore::Image::Texture2D& resource : batch.get_resources()) {
      BAIL_IF(!resource.is_drawable());
    }
  }
  return True;
}

static auto make_host_inputs(
    const Photophore::Description::Program& description,
    const Photophore::Frame::Batch& batch,
    U32 width,
    U32 height) -> Perimortem::Memory::Dynamic::Bytes {
  Perimortem::Memory::Dynamic::Bytes inputs;
  inputs.append(0, description.host_size);
  auto access = inputs.get_access();
  // Parameters already use the generated target layout. Host roles fill only
  // values owned by current placement and presentation state.
  if (description.parameters_size != 0) {
    Data::copy(
        access.get_data() + description.parameters_offset,
        batch.get_inputs().get_data(), description.parameters_size);
  }

  const auto& transform = batch.get_transform();
  const auto size = batch.get_size_pixels();
  for (const Photophore::Description::HostField& field :
       description.host_fields) {
    if (field.offset > description.host_size ||
        field.size > description.host_size - field.offset) {
      return {};
    }
    if (field.role == Photophore::Description::HostRole::Parameter) {
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
    R32 values[4] = {};
    if (field.role == Photophore::Description::HostRole::TransformX) {
      values[0] = R32(transform.get_xx() * size[0]);
      values[1] = R32(transform.get_xy() * size[1]);
      values[2] = R32(transform.get_x());
      values[3] = R32(width);
    } else if (field.role == Photophore::Description::HostRole::TransformY) {
      values[0] = R32(transform.get_yx() * size[0]);
      values[1] = R32(transform.get_yy() * size[1]);
      values[2] = R32(transform.get_y());
      values[3] = R32(height);
    }
    Data::copy(access.get_data() + field.offset, values, 4);
  }
  return inputs;
}

static auto sweep_textures(const Context& context, auto& images, auto& textures)
    -> void {
  // Matrix reservations identify images retained only by the image cache and
  // matching descriptor entries. The submission fence then establishes that
  // earlier draws have finished reading the native image and its descriptors.
  Count index = 0;
  while (index < images.get_size()) {
    auto& image = images[index];
    const auto* identity = &image.resource.get_image();
    Count cached_reservations = 1;
    for (Count texture_index = 0; texture_index < textures.get_size();
         texture_index++) {
      const auto& texture = textures[texture_index];
      cached_reservations += &texture.resource.get_image() == identity;
    }
    if (image.resource.get_reservations() != cached_reservations) {
      index++;
      continue;
    }

    // Renderer advances a slot generation after waiting for its preceding
    // submission. A changed generation therefore proves this image's recorded
    // use completed even while the fence represents a newer frame.
    if (image.completion && *image.generation == image.observed_generation) {
      VkResult status = Photophore::Vulkan::vkGetFenceStatus(
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
      const auto& texture = textures[texture_index];
      if (&texture.resource.get_image() != identity) {
        texture_index++;
        continue;
      }

      textures.remove(texture_index);
    }
    images.remove(index);
  }
}

Photophore::Vulkan::Pipelines::Pipelines(
    const Context& context,
    VkFormat color_format,
    View::Vector<Photophore::Description::Program> descriptions)
    : context(context), descriptions(descriptions) {
  validate_descriptions(context, descriptions);
  descriptor_layout = create_descriptor_layout(context.get_device());
  create_vertex_buffer(context, vertex_buffer, vertex_memory);
  rebuild(color_format);
}

Photophore::Vulkan::Pipelines::~Pipelines() {
  // Device completion closes every cached GPU use. Native dependencies then
  // unwind from descriptor sets and images through pipelines and shared state.
  Photophore::Vulkan::vkDeviceWaitIdle(context.get_device());
  textures.clear();
  images.clear();
  realizations.clear();
  destroy_vertex_buffer(context, vertex_buffer, vertex_memory);
  if (descriptor_layout) {
    Photophore::Vulkan::vkDestroyDescriptorSetLayout(
        context.get_device(), descriptor_layout, nullptr);
  }
}

auto Photophore::Vulkan::Pipelines::rebuild(VkFormat color_format) -> void {
  // The caller finishes use of the old pipelines before rebuilding. Only
  // shader realizations depend on the attachment format, so uploaded images
  // and sampled descriptors remain reusable across this change.
  realizations.clear();
  this->color_format = color_format;
}

auto Photophore::Vulkan::Pipelines::record(
    const Renderer::Frame& frame,
    View::Vector<Photophore::Frame::Batch> batches) -> Bool {
  VkCommandBuffer command_buffer = frame.command_buffer;
  U32 width = frame.width;
  U32 height = frame.height;
  BAIL_IF(
      !command_buffer || width == 0 || height == 0 || !frame.completion ||
      !frame.completion_generation || !validate(descriptions, batches));

  // The first pass realizes every native pipeline, image and descriptor set.
  // The second pass records draws from an established resource set and builds
  // host input bytes from the presentation extent current for this frame.
  for (const Photophore::Frame::Batch& batch : batches) {
    auto* program = realize_pipeline(
        context, descriptions, color_format, descriptor_layout, realizations,
        batch);
    BAIL_IF(program == nullptr);
    for (const Photophore::Image::Texture2D& resource : batch.get_resources()) {
      auto* image = realize_image(context, images, resource);
      BAIL_IF(image == nullptr);
      auto* texture = realize_texture(
          context, textures, descriptor_layout, resource, image->image);
      BAIL_IF(texture == nullptr);
    }
  }
  for (const Photophore::Frame::Batch& batch : batches) {
    auto* realization = find_realization(
        realizations, batch.get_program().get_locator(), batch.get_pipeline());
    BAIL_IF(realization == nullptr);
    Perimortem::Memory::Dynamic::Bytes inputs =
        make_host_inputs(*realization->description, batch, width, height);
    BAIL_IF(inputs.get_size() != realization->description->host_size);
    realization->shader.bind(command_buffer);
    constexpr VkDeviceSize vertex_offset = 0;
    Photophore::Vulkan::vkCmdBindVertexBuffers(
        command_buffer, 0, 1, &vertex_buffer, &vertex_offset);
    for (Count descriptor_index = 0;
         descriptor_index < realization->description->descriptors.get_size();
         descriptor_index++) {
      const auto& resource = batch.get_resources()[descriptor_index];
      Texture* texture = find_texture(textures, resource);
      auto* image = find_image(images, resource);
      BAIL_IF(image == nullptr);
      image->completion = frame.completion;
      image->generation = frame.completion_generation;
      image->observed_generation = *frame.completion_generation;
      BAIL_IF(texture == nullptr);
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
