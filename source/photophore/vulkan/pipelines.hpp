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
// once, uploads each Pixels2D value once, and creates one descriptor set for
// each Texture2D content and sampler pair. Later frames reuse those native
// objects.
//
// Construction stores references to a live Context and the complete Program
// inventory, including every nested View and name. Both remain accessible until
// Pipelines destruction. Each Program identity is set and unique within the
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
// latest draw and whether the current recording pass selected it. Unselected
// content leaves the cache after that GPU use completes. Application references
// remain independent and can recreate a realization through a later Batch.
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
    // Stores one Pixels2D upload and the latest frame use that controls its
    // destruction. `used` records selection by the current recording pass.
    Image::Pixels2D content;
    TextureImage image;
    VkFence completion = VK_NULL_HANDLE;
    const Count* generation = nullptr;
    Count observed_generation = 0;
    Bool used = False;
  };

  struct TextureEntry {
    // Stores one descriptor set and sampler for an accepted Texture2D value.
    Image::Texture2D resource;
    Texture texture;
  };

  struct ProgramEntry {
    // Program identity and fixed draw state form the cache key. Description
    // storage remains in the construction inventory, while ShaderProgram owns
    // the format specific VkPipeline and layout selected by that key.
    const Description::Program* description = nullptr;
    Frame::Pipeline pipeline;
    ShaderProgram shader;
  };

  auto realize_image(const Image::Texture2D& resource) -> ImageEntry*;
  auto realize_texture(
      const Image::Texture2D& resource,
      const TextureImage& image) -> Texture*;

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
