// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/frame/batch.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Photophore::Frame;

// Construction captures one draw observation. Moving resources and parameter
// bytes makes their lifetime exactly the Batch lifetime while placement and
// ordering remain inexpensive copied values.
Batch::Batch(
    Program program,
    Thread::Vector<Image::Texture2D>&& resources,
    Thread::Bytes&& inputs,
    Transform transform,
    Math::Size<U32, 2> size_pixels,
    Pipeline pipeline,
    Count vertex_count,
    S64 z_index,
    Count authored_order)
    : program(program),
      resources(Data::take(resources)),
      inputs(Data::take(inputs)),
      transform(transform),
      size_pixels(size_pixels),
      pipeline(pipeline),
      vertex_count(vertex_count),
      z_index(z_index),
      authored_order(authored_order) {}

auto Batch::get_inputs() const -> View::Bytes {
  return inputs.get_view();
}
