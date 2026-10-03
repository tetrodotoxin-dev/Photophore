// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "photophore/context.hpp"
#include "photophore/description/program.hpp"
#include "photophore/frame/batch.hpp"
#include "photophore/renderer.hpp"
#include "photophore/shader_program.hpp"
#include "photophore/texture.hpp"
#include "photophore/texture_image.hpp"

namespace Photophore {

// A frame selects programs and resources, while Vulkan needs pipelines, images
// and descriptor sets. Pipelines translates those selections and retains the
// expensive realizations for reuse. The Context and complete
// Description::Program inventory are borrowed for its lifetime, including their
// nested arrays and names. Construction requires a Context with a live device.
// Program locators are compared by address and must
// remain unique and stable while this owner uses them.
//
// Shader pipelines are keyed by program and fixed draw state for the current
// color format. Uploaded images are keyed by shared pixel matrix identity, and
// sampled bindings also include Sampler2D. Changing a sampler can reuse the
// uploaded pixels. record consumes batches in their supplied order and copies
// parameter bytes into Vulkan commands without retaining the batches
// themselves.
//
// The implemented draw path accepts a unit quad, alpha blending, one push
// constant range and sampled textures at binding zero in distinct sets. Bad
// program descriptions are fatal at construction. Unsupported batch values
// return False during recording. Each recorded image retains its latest frame
// completion fence and its reuse generation. Cache retirement requires both
// absence of external CPU owners and completed GPU use, without waiting for the
// whole queue. Renderer must outlive this owner, and each frame must come from
// that same Context and be submitted through its matching end_frame operation.
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
    Texture2D resource;
    TextureImage image;
    VkFence completion = VK_NULL_HANDLE;
    const Count* generation = nullptr;
    Count observed_generation = 0;
  };

  struct TextureEntry {
    Texture2D resource;
    Texture texture;
  };

  struct ProgramEntry {
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

}  // namespace Photophore
