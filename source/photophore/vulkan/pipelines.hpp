// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/vector.hpp"
#include "perimortem/memory/thread/vector.hpp"

#include "photophore/description/program.hpp"
#include "photophore/frame/batch.hpp"
#include "photophore/image/texture_2d.hpp"
#include "photophore/vulkan/context.hpp"
#include "photophore/vulkan/renderer.hpp"
#include "photophore/vulkan/shader_program.hpp"
#include "photophore/vulkan/texture.hpp"
#include "photophore/vulkan/texture_image.hpp"

namespace Photophore::Vulkan {

// Pipelines records ordered Frame::Batch values as Vulkan draw commands. It
// creates each validated Description::Program and fixed Frame::Pipeline pair
// once, uploads each shared pixel Matrix once, and creates one descriptor set
// for each Matrix and Sampler2D pair. Later frames reuse those native objects.
//
// Construction stores references to a live Context and the complete Program
// inventory, including every nested View and name. Both remain accessible until
// Pipelines destruction. Each locator has a stable, unique address within the
// inventory. `rebuild` replaces shader realizations after a swapchain color
// format change while preserving uploaded images and descriptor sets.
//
// The supported profile uses a unit quad, triangle list, source alpha blending,
// one host input range and sampled textures at slot zero in distinct sets.
// Construction treats an invalid Description as a fatal authored-program error.
// `record` returns False when a Batch falls outside the profile. Batch Views
// need remain accessible only until that call returns.
//
// Each ImageEntry remembers the Renderer frame fence and generation of its
// latest draw. Cache retirement combines completed GPU use with the shared
// matrix reservation count, allowing inactive textures to leave the cache
// while current submissions continue on the queue.
class Pipelines {
  using Descriptions = Perimortem::Core::View::Vector<Description::Program>;
  using Batches = Perimortem::Core::View::Vector<Frame::Batch>;

 public:
  EXPORTED(PHOTOPHORE) Pipelines(
      const Context& context,
      VkFormat color_format,
      Descriptions descriptions);
  EXPORTED(PHOTOPHORE) ~Pipelines();
  Pipelines(const Pipelines&) = delete;
  Pipelines(Pipelines&&) = delete;
  auto operator=(const Pipelines&) -> Pipelines& = delete;
  auto operator=(Pipelines&&) -> Pipelines& = delete;

  EXPORTED(PHOTOPHORE) auto rebuild(VkFormat color_format) -> void;
  EXPORTED(PHOTOPHORE) auto record(
      const Renderer::Frame& frame,
      Batches batches) -> Bool;

 private:
  struct ImageEntry {
    // Stores one pixel Matrix upload and the latest frame use that controls its
    // destruction.
    Image::Texture2D resource;
    TextureImage image;
    VkFence completion = VK_NULL_HANDLE;
    const Count* generation = nullptr;
    Count observed_generation = 0;
  };

  struct TextureEntry {
    // Stores one descriptor set and sampler for an accepted Texture2D value.
    Image::Texture2D resource;
    Texture texture;
  };

  struct ProgramEntry {
    // Stores one VkPipeline selected by Program address and fixed draw state.
    const Description::Program* description = nullptr;
    Frame::Pipeline pipeline;
    ShaderProgram shader;
  };

  const Context& context;
  Descriptions descriptions;
  VkFormat color_format = VK_FORMAT_UNDEFINED;
  VkDescriptorSetLayout descriptor_layout = VK_NULL_HANDLE;
  VkBuffer vertex_buffer = VK_NULL_HANDLE;
  VkDeviceMemory vertex_memory = VK_NULL_HANDLE;
  Perimortem::Memory::Thread::Vector<ProgramEntry> realizations;
  Perimortem::Memory::Thread::Vector<ImageEntry> images;
  Perimortem::Memory::Thread::Vector<TextureEntry> textures;
};

}  // namespace Photophore::Vulkan
