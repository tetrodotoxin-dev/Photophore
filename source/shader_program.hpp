// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"

#include "photophore/description/program.hpp"
#include "photophore/frame/pipeline.hpp"

namespace Photophore {

// ShaderProgram turns compiled shader modules, reflected input layouts and
// fixed draw state into a Vulkan pipeline. Creation consumes the description
// synchronously, so its temporary module handles and entry name buffers can be
// released after vkCreateGraphicsPipelines. The resulting pipeline and layout
// remain owned here and require the supplied device to outlive them.
//
// Push constant ranges and descriptor set counts are retained to interpret
// later draws. Those calls record into the caller's command buffer and do not
// retain its parameter bytes. Command buffers using the pipeline must have
// completed before the pipeline is replaced or destroyed.
class ShaderProgram {
 public:
  static auto create(
      VkDevice device,
      VkFormat color_format,
      const Description::Program& source,
      Photophore::Frame::Pipeline pipeline,
      Perimortem::Core::View::Vector<VkDescriptorSetLayout>
          descriptor_set_layouts) -> ShaderProgram;

  ShaderProgram() = default;
  ~ShaderProgram();
  // Moves exchange the owned pipeline and layout. Assignment leaves the old
  // destination resources with the donor, whose eventual destruction releases
  // them. Neither move ends the lifetime of either C++ owner.
  ShaderProgram(ShaderProgram&&) noexcept;
  auto operator=(ShaderProgram&&) noexcept -> ShaderProgram&;
  ShaderProgram(const ShaderProgram&) = delete;
  auto operator=(const ShaderProgram&) = delete;

  auto bind(VkCommandBuffer command_buffer) const -> void;
  auto bind_descriptor_set(
      VkCommandBuffer command_buffer,
      VkDescriptorSet descriptor_set,
      Count set = 0) const -> void;
  auto push_constants(
      VkCommandBuffer command_buffer,
      Perimortem::Core::View::Bytes source,
      Count range_index = 0) const -> void;
  auto draw(VkCommandBuffer command_buffer, Count vertex_count) const -> void;

  auto get_layout() const -> VkPipelineLayout;
  auto get_pipeline() const -> VkPipeline;

 private:
  static constexpr Count max_push_constant_ranges = 8;

  VkDevice device = VK_NULL_HANDLE;
  VkPipelineLayout layout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  Perimortem::Core::Static::
      Vector<VkPushConstantRange, max_push_constant_ranges>
          push_constant_ranges;
  Count push_constant_count = 0;
  Count descriptor_set_count = 0;
};

}  // namespace Photophore
