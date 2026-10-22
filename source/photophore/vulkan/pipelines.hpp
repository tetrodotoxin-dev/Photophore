// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "photophore/description/program.hpp"
#include "photophore/frame/batch.hpp"
#include "photophore/image/texture_2d.hpp"
#include "photophore/vulkan/context.hpp"
#include "photophore/vulkan/renderer.hpp"
#include "photophore/vulkan/shader_program.hpp"
#include "photophore/vulkan/texture.hpp"
#include "photophore/vulkan/texture_image.hpp"

namespace Photophore::Vulkan {

// Pipelines turns ordered Frame::Batch values into Vulkan draw commands. It
// realizes each admitted Description::Program and fixed Frame::Pipeline once,
// uploads each shared pixel matrix once, and realizes each matrix plus
// Sampler2D pair as one descriptor set. These caches keep native setup work out
// of repeated frame recording.
//
// Construction borrows a live Context and the complete Program inventory,
// including every nested view and name. Renderer and that inventory remain live
// through the Pipelines lifetime. Each locator is stable and unique within the
// inventory. rebuild replaces shader realizations after a swapchain color
// format change while preserving uploaded images and descriptor sets.
//
// The admitted profile uses a unit quad, triangle list, source alpha blending,
// one host input range and sampled textures at slot zero in distinct sets.
// Description contract violations use the fatal diagnostic path. record
// returns False when a batch falls outside that admitted profile and borrows
// batches only for the call.
//
// Each ImageEntry remembers the Renderer frame fence and generation of its
// latest draw. Cache retirement combines completed GPU use with the shared
// matrix reservation count, allowing inactive textures to leave the cache
// while current submissions continue on the queue.
class Pipelines {
 public:
  EXPORTED(PHOTOPHORE)
  Pipelines(
      const Context& context,
      VkFormat color_format,
      Perimortem::Core::View::Vector<Description::Program> descriptions);
  EXPORTED(PHOTOPHORE) ~Pipelines();
  Pipelines(const Pipelines&) = delete;
  Pipelines(Pipelines&&) = delete;
  auto operator=(const Pipelines&) -> Pipelines& = delete;
  auto operator=(Pipelines&&) -> Pipelines& = delete;

  EXPORTED(PHOTOPHORE) auto rebuild(VkFormat color_format) -> void;
  EXPORTED(PHOTOPHORE)
  auto record(
      const Renderer::Frame& frame,
      Perimortem::Core::View::Vector<Photophore::Frame::Batch> batches) -> Bool;

 private:
  struct ImageEntry {
    // One pixel matrix upload shared by every sampler realization.
    Image::Texture2D resource;
    TextureImage image;
    VkFence completion = VK_NULL_HANDLE;
    const Count* generation = nullptr;
    Count observed_generation = 0;
  };

  struct TextureEntry {
    // One descriptor and sampler for an admitted Texture2D value.
    Image::Texture2D resource;
    Texture texture;
  };

  struct ProgramEntry {
    // One VkPipeline selected by Program identity and fixed draw state.
    const Description::Program* description = nullptr;
    Frame::Pipeline pipeline;
    ShaderProgram shader;
  };

  const Context& context;
  Perimortem::Core::View::Vector<Description::Program> descriptions;
  VkFormat color_format = VK_FORMAT_UNDEFINED;
  VkDescriptorSetLayout descriptor_layout = VK_NULL_HANDLE;
  VkBuffer vertex_buffer = VK_NULL_HANDLE;
  VkDeviceMemory vertex_memory = VK_NULL_HANDLE;
  Perimortem::Memory::Dynamic::Vector<ProgramEntry> realizations;
  Perimortem::Memory::Dynamic::Vector<ImageEntry> images;
  Perimortem::Memory::Dynamic::Vector<TextureEntry> textures;
};

}  // namespace Photophore::Vulkan
