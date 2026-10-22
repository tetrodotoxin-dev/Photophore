// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"

#include "photophore/description/program.hpp"
#include "photophore/frame/pipeline.hpp"

namespace Photophore::Vulkan {

// ShaderProgram owns one VkPipelineLayout and VkPipeline compiled from a
// Description::Program, fixed Frame::Pipeline state and swapchain color format.
// create reads the borrowed description synchronously. Vulkan consumes the
// temporary module handles and entry names during creation, leaving the
// completed pipeline independent of those temporary native objects.
//
// The supplied VkDevice remains live through this value. Retained push constant
// ranges and descriptor set counts validate later recording calls before their
// commands enter the caller's command buffer. Recording borrows parameter bytes
// for the call. Pipeline replacement and destruction follow completion of every
// command buffer that used the native handles.
class ShaderProgram {
 public:
  static EXPORTED(PHOTOPHORE) auto create(
      VkDevice device,
      VkFormat color_format,
      const Description::Program& source,
      Photophore::Frame::Pipeline pipeline,
      Perimortem::Core::View::Vector<VkDescriptorSetLayout>
          descriptor_set_layouts) -> ShaderProgram;

  ShaderProgram() = default;
  EXPORTED(PHOTOPHORE) ~ShaderProgram();
  // Move operations exchange complete native states. The donor receives the
  // destination's earlier state and releases it through its ordinary lifetime.
  EXPORTED(PHOTOPHORE) ShaderProgram(ShaderProgram&&) noexcept;
  EXPORTED(PHOTOPHORE)
  auto operator=(ShaderProgram&&) noexcept -> ShaderProgram&;
  ShaderProgram(const ShaderProgram&) = delete;
  auto operator=(const ShaderProgram&) = delete;

  EXPORTED(PHOTOPHORE) auto bind(VkCommandBuffer command_buffer) const -> void;
  EXPORTED(PHOTOPHORE)
  auto bind_descriptor_set(
      VkCommandBuffer command_buffer,
      VkDescriptorSet descriptor_set,
      Count set = 0) const -> void;
  EXPORTED(PHOTOPHORE)
  auto push_constants(
      VkCommandBuffer command_buffer,
      Perimortem::Core::View::Bytes source,
      Count range_index = 0) const -> void;
  EXPORTED(PHOTOPHORE)
  auto draw(VkCommandBuffer command_buffer, Count vertex_count) const -> void;

  EXPORTED(PHOTOPHORE) auto get_layout() const -> VkPipelineLayout;
  EXPORTED(PHOTOPHORE) auto get_pipeline() const -> VkPipeline;

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

}  // namespace Photophore::Vulkan
