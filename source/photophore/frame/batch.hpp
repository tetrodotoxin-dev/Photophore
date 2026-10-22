// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/math/size.hpp"
#include "photophore/frame/pipeline.hpp"
#include "photophore/frame/program.hpp"
#include "photophore/frame/transform.hpp"
#include "photophore/image/texture_2d.hpp"

namespace Photophore::Frame {

// Batch is one complete draw submitted to Pipelines. Construction moves the
// parameter bytes and resource list into the batch, while each Texture2D keeps
// its shared pixel matrix alive. Transform and size are copied values, and
// Program lends the stable locator published by Description::Program.
//
// operator> orders z_index first and authored_order second, giving callers a
// deterministic sorting rule before submission. Pipelines records the order it
// receives and borrows each Batch for that call. The batch lifetime retains CPU
// resources, while Renderer frame completion establishes their GPU lifetime.
class Batch {
 public:
  Batch() = default;

  EXPORTED(PHOTOPHORE)
  Batch(
      Program program,
      Perimortem::Memory::Dynamic::Vector<Image::Texture2D>&& resources,
      Perimortem::Memory::Dynamic::Bytes&& inputs,
      Transform transform,
      Perimortem::Math::Size<U32, 2> size_pixels,
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
  constexpr auto get_size_pixels() const -> Perimortem::Math::Size<U32, 2> {
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
  Perimortem::Memory::Dynamic::Vector<Image::Texture2D> resources;
  Perimortem::Memory::Dynamic::Bytes inputs;
  Transform transform;
  Perimortem::Math::Size<U32, 2> size_pixels;
  Pipeline pipeline;
  Count vertex_count = 0;
  S64 z_index = 0;
  Count authored_order = 0;
};

}  // namespace Photophore::Frame
