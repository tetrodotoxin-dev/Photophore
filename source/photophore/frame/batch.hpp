// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/math/size.hpp"
#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/memory/thread/bytes.hpp"
#include "perimortem/memory/thread/vector.hpp"

#include "photophore/frame/pipeline.hpp"
#include "photophore/frame/program.hpp"
#include "photophore/frame/transform.hpp"
#include "photophore/image/texture_2d.hpp"

namespace Photophore::Frame {

// Batch is one complete draw submitted to Pipelines. Construction moves the
// parameter bytes and resource list into the batch, while each Texture2D keeps
// its shared pixel Matrix accessible. Transform and size are copied values, and
// Program stores the explicit identity published by Description::Program.
//
// `operator>` orders `z_index` first and `authored_order` second. Pipelines
// records the resulting sequence and reads each Batch only during that call.
// The Batch keeps CPU resources accessible. Renderer frame completion governs
// their corresponding Vulkan use.
class Batch {
 public:
  Batch() = default;

  EXPORTED(PHOTOPHORE) Batch(
      Program program,
      Perimortem::Memory::Thread::Vector<Image::Texture2D>&& resources,
      Perimortem::Memory::Thread::Bytes&& inputs,
      Transform transform,
      Perimortem::Core::Math::Size<U32, 2> size_pixels,
      Pipeline pipeline,
      Count vertex_count,
      S64 z_index,
      Count authored_order);

  constexpr auto get_program() const -> Program { return program; }
  constexpr auto get_resources() const
      -> Perimortem::Core::View::Vector<Image::Texture2D> {
    return resources.get_view();
  }

  EXPORTED(PHOTOPHORE) auto get_inputs() const -> Perimortem::Core::View::Bytes;
  constexpr auto get_transform() const -> const Transform& { return transform; }
  constexpr auto get_size_pixels() const
      -> Perimortem::Core::Math::Size<U32, 2> {
    return size_pixels;
  }

  constexpr auto get_pipeline() const -> Pipeline { return pipeline; }
  constexpr auto get_vertex_count() const -> Count { return vertex_count; }
  constexpr auto get_z_index() const -> S64 { return z_index; }
  constexpr auto get_authored_order() const -> Count { return authored_order; }
  constexpr auto operator>(const Batch& rhs) const -> Bool {
    return z_index > rhs.z_index ||
           (z_index == rhs.z_index && authored_order > rhs.authored_order);
  }

 private:
  Program program;
  Perimortem::Memory::Thread::Vector<Image::Texture2D> resources;
  Perimortem::Memory::Thread::Bytes inputs;
  Transform transform;
  Perimortem::Core::Math::Size<U32, 2> size_pixels;
  Pipeline pipeline;
  Count vertex_count = 0;
  S64 z_index = 0;
  Count authored_order = 0;
};

}  // namespace Photophore::Frame
