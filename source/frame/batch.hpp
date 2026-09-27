// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/size_2d.hpp"
#include "photophore/frame/pipeline.hpp"
#include "photophore/frame/program.hpp"
#include "photophore/frame/transform.hpp"
#include "photophore/texture_2d.hpp"

namespace Photophore::Frame {

// Batch captures one draw's values while its source objects are available.
// It owns the parameter bytes and retains each Texture2D's shared Image, so
// later changes to source placement or resource selection cannot rewrite the
// captured values. Its Program locator still borrows a stable identity token.
//
// The comparison orders batches by z_index and then authored_order, allowing
// callers to establish a deterministic draw order before recording. Pipelines
// records the supplied order and does not sort or retain the batches. Keeping a
// batch alive retains CPU resources, but does not itself wait for GPU use.
class Batch {
 public:
  Batch() = default;

  Batch(
      Program program,
      Perimortem::Memory::Dynamic::Vector<Texture2D>&& resources,
      Perimortem::Memory::Dynamic::Bytes&& inputs,
      Transform transform,
      Perimortem::Graphics::Size2D size_pixels,
      Pipeline pipeline,
      Count vertex_count,
      S64 z_index,
      Count authored_order);

  constexpr auto get_program() const -> Program { return program; }
  constexpr auto get_resources() const
      -> Perimortem::Core::View::Vector<Texture2D> {
    return resources.get_view();
  }
  auto get_inputs() const -> Perimortem::Core::View::Bytes;
  constexpr auto get_transform() const -> const Transform& { return transform; }
  constexpr auto get_size_pixels() const -> Perimortem::Graphics::Size2D {
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
  Perimortem::Memory::Dynamic::Vector<Texture2D> resources;
  Perimortem::Memory::Dynamic::Bytes inputs;
  Transform transform;
  Perimortem::Graphics::Size2D size_pixels;
  Pipeline pipeline;
  Count vertex_count = 0;
  S64 z_index = 0;
  Count authored_order = 0;
};

}  // namespace Photophore::Frame
