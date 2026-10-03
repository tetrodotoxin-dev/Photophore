// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/frame/batch.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem;

Photophore::Frame::Batch::Batch(
    Program program,
    Perimortem::Memory::Dynamic::Vector<Texture2D>&& resources,
    Perimortem::Memory::Dynamic::Bytes&& inputs,
    Transform transform,
    Perimortem::Math::Size<U32, 2> size_pixels,
    Pipeline pipeline,
    Count vertex_count,
    S64 z_index,
    Count authored_order)
    : program(program),
      resources(Perimortem::Core::Data::take(resources)),
      inputs(Perimortem::Core::Data::take(inputs)),
      transform(transform),
      size_pixels(size_pixels),
      pipeline(pipeline),
      vertex_count(vertex_count),
      z_index(z_index),
      authored_order(authored_order) {}

auto Photophore::Frame::Batch::get_inputs() const
    -> Perimortem::Core::View::Bytes {
  return inputs.get_view();
}
