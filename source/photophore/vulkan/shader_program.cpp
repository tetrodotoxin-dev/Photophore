// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/vulkan/shader_program.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "implementation/vulkan/dispatch.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Photophore::Vulkan;

static constexpr Count max_shader_modules = 8;

// Native conversion remains local to pipeline construction. Description types
// stay independent of Vulkan while every accepted value has one exact mapping.
static auto require_success(VkResult result, View::Bytes message) -> void {
  if (result != VK_SUCCESS) {
    Diagnostics::Log::fatal(message);
  }
}

static auto to_vk_stage(Photophore::Description::Stage stage)
    -> VkShaderStageFlagBits {
  switch (stage) {
  case Photophore::Description::Stage::Vertex:
    return VK_SHADER_STAGE_VERTEX_BIT;
  case Photophore::Description::Stage::Pixel:
    return VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  Diagnostics::Log::fatal("Vulkan: Unknown render stage."_view);
  return VK_SHADER_STAGE_VERTEX_BIT;
}

static auto to_vk_stage_flags(
    View::Vector<Photophore::Description::Stage> stages) -> VkShaderStageFlags {
  VkShaderStageFlags flags = 0;
  const auto* stage_data = stages.get_data();
  for (Count i = 0; i < stages.get_size(); i++) {
    flags |= to_vk_stage(stage_data[i]);
  }

  if (flags == 0) {
    Diagnostics::Log::fatal("Vulkan: Empty render stage list."_view);
  }

  return flags;
}

static auto make_shader_module(
    VkDevice device,
    const Photophore::Description::Module& source) -> VkShaderModule {
  if (source.words.is_empty()) {
    Diagnostics::Log::fatal("Vulkan: Invalid render module."_view);
  }

  VkShaderModuleCreateInfo info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  info.codeSize = source.words.get_size() * sizeof(U32);
  info.pCode = source.words.get_data();

  VkShaderModule module = VK_NULL_HANDLE;
  require_success(
      Photophore::Vulkan::vkCreateShaderModule(device, &info, nullptr, &module),
      "Vulkan: Failed to create render module."_view);
  return module;
}

static auto vertex_format(Count components) -> VkFormat {
  switch (components) {
  case 1:
    return VK_FORMAT_R32_SFLOAT;
  case 2:
    return VK_FORMAT_R32G32_SFLOAT;
  case 3:
    return VK_FORMAT_R32G32B32_SFLOAT;
  case 4:
    return VK_FORMAT_R32G32B32A32_SFLOAT;
  }
  Diagnostics::Log::fatal("Vulkan: Invalid vertex input width."_view);
  return VK_FORMAT_UNDEFINED;
}

static auto to_vk_topology(Photophore::Frame::Pipeline::Topology topology)
    -> VkPrimitiveTopology {
  switch (topology) {
  case Photophore::Frame::Pipeline::Topology::TriangleList:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  }
  Diagnostics::Log::fatal("Vulkan: Invalid primitive topology."_view);
  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

auto Photophore::Vulkan::ShaderProgram::create(
    VkDevice device,
    VkFormat color_format,
    const Photophore::Description::Program& render,
    Photophore::Frame::Pipeline pipeline,
    View::Vector<VkDescriptorSetLayout> descriptor_set_layouts)
    -> ShaderProgram {
  const auto source_modules = render.modules;
  const auto source_host_input_ranges = render.host_input_ranges;
  if (source_modules.is_empty() ||
      source_modules.get_size() > max_shader_modules) {
    Diagnostics::Log::fatal("Vulkan: Invalid render module list."_view);
  }

  if (source_host_input_ranges.get_size() > max_push_constant_ranges) {
    Diagnostics::Log::fatal("Vulkan: Too many render host input ranges."_view);
  }

  ShaderProgram program;
  program.device = device;
  program.push_constant_count = source_host_input_ranges.get_size();
  program.descriptor_set_count = descriptor_set_layouts.get_size();
  const auto* host_input_data = source_host_input_ranges.get_data();
  // Description ranges use the generated host layout. Vulkan consumes their
  // same byte offsets and widths, with a four byte granularity at this
  // boundary.
  for (Count i = 0; i < program.push_constant_count; i++) {
    const auto& source_range = host_input_data[i];
    if (source_range.size == 0 || source_range.offset > U32(-1) ||
        source_range.size > U32(-1) ||
        ((source_range.offset | source_range.size) & 3) != 0) {
      Diagnostics::Log::fatal("Vulkan: Invalid render host input range."_view);
    }

    auto& target_range = program.push_constant_ranges[i];
    target_range.stageFlags = to_vk_stage_flags(source_range.stages);
    target_range.offset = U32(source_range.offset);
    target_range.size = U32(source_range.size);
  }

  // Vertex reflection is admitted before native allocation. Every attribute
  // uses one interleaved binding and shares the stride proven by the first
  // entry, giving the fixed unit quad buffer one exact interpretation.
  Static::Vector<VkVertexInputBindingDescription, 1> vertex_bindings;
  Static::Vector<VkVertexInputAttributeDescription, 8> vertex_attributes;
  auto reflected_vertex_inputs = render.vertex_inputs;
  if (!reflected_vertex_inputs.is_empty()) {
    BAIL_IF(reflected_vertex_inputs.get_size() > vertex_attributes.get_size());
    Count stride = reflected_vertex_inputs.get_data()[0].stride;
    BAIL_IF(stride == 0 || stride > U32(-1));
    vertex_bindings[0] = {
      .binding = 0,
      .stride = U32(stride),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    for (Count index = 0; index < reflected_vertex_inputs.get_size(); index++) {
      const Photophore::Description::VertexInput& input =
          reflected_vertex_inputs.get_data()[index];
      BAIL_IF(
          input.location > U32(-1) || input.offset > U32(-1) ||
          input.stride != stride);
      vertex_attributes[index] = {
        .location = U32(input.location),
        .binding = 0,
        .format = vertex_format(input.components),
        .offset = U32(input.offset),
      };
    }
  }

  // Vulkan reads modules and entry names during pipeline creation. These
  // temporary arrays retain both through that call, after which the compiled
  // pipeline contains the executable state used for recording.
  Static::Vector<VkShaderModule, max_shader_modules> shader_modules;
  Static::Vector<VkPipelineShaderStageCreateInfo, max_shader_modules> stages;
  Static::Vector<Dynamic::Bytes, max_shader_modules> entry_names;
  const auto* module_data = source_modules.get_data();
  for (Count i = 0; i < source_modules.get_size(); i++) {
    const auto& module = module_data[i];
    shader_modules[i] = make_shader_module(device, module);

    stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[i].stage = to_vk_stage(module.stage);
    stages[i].module = shader_modules[i];
    if (module.entry.is_empty()) {
      stages[i].pName = "main";
    } else {
      entry_names[i] = module.entry;
      entry_names[i].append(0);
      stages[i].pName =
          Data::cast<const char>(entry_names[i].get_access().get_data());
    }
  }

  VkPipelineVertexInputStateCreateInfo vertex_input = {
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vertex_input.vertexBindingDescriptionCount =
      reflected_vertex_inputs.is_empty() ? 0 : 1;
  vertex_input.pVertexBindingDescriptions =
      reflected_vertex_inputs.is_empty() ? nullptr : vertex_bindings.get_data();
  vertex_input.vertexAttributeDescriptionCount =
      U32(reflected_vertex_inputs.get_size());
  vertex_input.pVertexAttributeDescriptions =
      reflected_vertex_inputs.is_empty() ? nullptr
                                         : vertex_attributes.get_data();

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  input_assembly.topology = to_vk_topology(pipeline.get_topology());

  // Pipelines admits the fixed rendering profile before reaching this point.
  // This block is its Vulkan realization: filled triangles, one sample and one
  // color attachment using conventional source alpha composition.
  VkPipelineViewportStateCreateInfo viewport_state = {
    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer = {
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo multisampling = {
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState blend_attachment = {};
  switch (pipeline.get_blend()) {
  case Photophore::Frame::Pipeline::Blend::Alpha:
    blend_attachment.blendEnable = VK_TRUE;
    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    break;
  }
  blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo color_blending = {
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &blend_attachment;

  // Viewport and scissor follow each acquired image. Dynamic state lets one
  // realization serve every extent that shares its attachment format.
  Static::Vector<VkDynamicState, 2> dynamic_states = {{
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  }};
  VkPipelineDynamicStateCreateInfo dynamic_state = {
    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamic_state.dynamicStateCount = U32(dynamic_states.get_size());
  dynamic_state.pDynamicStates = dynamic_states.get_data();

  VkPipelineLayoutCreateInfo layout_info = {
    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  // The layout joins the descriptor set sequence built by Pipelines with the
  // generated push ranges copied above. Later recording uses these same counts
  // to validate set and host input selection.
  layout_info.setLayoutCount = U32(descriptor_set_layouts.get_size());
  layout_info.pSetLayouts = descriptor_set_layouts.get_data();
  layout_info.pushConstantRangeCount = U32(program.push_constant_count);
  layout_info.pPushConstantRanges = program.push_constant_ranges.get_data();

  require_success(
      Photophore::Vulkan::vkCreatePipelineLayout(
          device, &layout_info, nullptr, &program.layout),
      "Vulkan: Failed to create render pipeline layout."_view);

  // Dynamic rendering places the color format directly in the pipeline. A
  // swapchain format change therefore selects a fresh realization while the
  // Program description and uploaded textures remain reusable.
  VkPipelineRenderingCreateInfo rendering = {
    VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  rendering.colorAttachmentCount = 1;
  rendering.pColorAttachmentFormats = &color_format;

  VkGraphicsPipelineCreateInfo pipeline_info = {
    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  pipeline_info.pNext = &rendering;
  pipeline_info.stageCount = U32(source_modules.get_size());
  pipeline_info.pStages = stages.get_data();
  pipeline_info.pVertexInputState = &vertex_input;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.pDynamicState = &dynamic_state;
  pipeline_info.layout = program.layout;

  require_success(
      Photophore::Vulkan::vkCreateGraphicsPipelines(
          device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
          &program.pipeline),
      "Vulkan: Failed to create render pipeline."_view);
  // Pipeline creation has consumed every module and entry name. Releasing the
  // transient modules here leaves ShaderProgram with its two durable handles.
  for (Count i = 0; i < source_modules.get_size(); i++) {
    Photophore::Vulkan::vkDestroyShaderModule(
        device, shader_modules[i], nullptr);
  }

  return program;
}

Photophore::Vulkan::ShaderProgram::~ShaderProgram() {
  if (!device) {
    return;
  }

  Photophore::Vulkan::vkDestroyPipeline(device, pipeline, nullptr);
  Photophore::Vulkan::vkDestroyPipelineLayout(device, layout, nullptr);
}

Photophore::Vulkan::ShaderProgram::ShaderProgram(
    ShaderProgram&& other) noexcept {
  *this = Data::take(other);
}

auto Photophore::Vulkan::ShaderProgram::operator=(
    ShaderProgram&& other) noexcept -> ShaderProgram& {
  if (this == &other) {
    return *this;
  }

  Data::swap(device, other.device);
  Data::swap(layout, other.layout);
  Data::swap(pipeline, other.pipeline);
  Data::swap(push_constant_ranges, other.push_constant_ranges);
  Data::swap(push_constant_count, other.push_constant_count);
  Data::swap(descriptor_set_count, other.descriptor_set_count);
  return *this;
}

auto Photophore::Vulkan::ShaderProgram::bind(
    VkCommandBuffer command_buffer) const -> void {
  Photophore::Vulkan::vkCmdBindPipeline(
      command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

auto Photophore::Vulkan::ShaderProgram::bind_descriptor_set(
    VkCommandBuffer command_buffer,
    VkDescriptorSet descriptor_set,
    Count set) const -> void {
  if (set >= descriptor_set_count) {
    Diagnostics::Log::fatal("Vulkan: Invalid render descriptor."_view);
  }

  Photophore::Vulkan::vkCmdBindDescriptorSets(
      command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, U32(set), 1,
      &descriptor_set, 0, nullptr);
}

auto Photophore::Vulkan::ShaderProgram::push_constants(
    VkCommandBuffer command_buffer,
    View::Bytes source,
    Count range_index) const -> void {
  if (range_index >= push_constant_count) {
    Diagnostics::Log::fatal("Vulkan: Invalid render host inputs."_view);
  }

  const auto& range = push_constant_ranges[range_index];
  if (source.is_empty() || source.get_size() > range.size ||
      (source.get_size() & 3) != 0) {
    Diagnostics::Log::fatal("Vulkan: Invalid render host input size."_view);
  }

  Photophore::Vulkan::vkCmdPushConstants(
      command_buffer, layout, range.stageFlags, range.offset,
      U32(source.get_size()), source.get_data());
}

auto Photophore::Vulkan::ShaderProgram::draw(
    VkCommandBuffer command_buffer,
    Count vertex_count) const -> void {
  if (vertex_count == 0 || vertex_count > U32(-1)) {
    Diagnostics::Log::fatal("Vulkan: Invalid render vertex count."_view);
  }

  Photophore::Vulkan::vkCmdDraw(command_buffer, U32(vertex_count), 1, 0, 0);
}

auto Photophore::Vulkan::ShaderProgram::get_layout() const -> VkPipelineLayout {
  return layout;
}

auto Photophore::Vulkan::ShaderProgram::get_pipeline() const -> VkPipeline {
  return pipeline;
}
