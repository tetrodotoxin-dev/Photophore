// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stdio.h>
#include <stdlib.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/matrix.hpp"

#include "perimortem/serialization/rgba8.hpp"

#include "examples/program.hpp"
#include "perimortem/math/domain.hpp"
#include "perimortem/math/size.hpp"
#include "photophore/frame/batch.hpp"
#include "photophore/pipelines.hpp"
#include "photophore/platform/window.hpp"
#include "photophore/renderer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Math;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Photophore;

static auto make_size(U32 width, U32 height) -> Size<U32, 2> {
  Static::Vector<U32, 2> dimensions = {{width, height}};
  return Size<U32, 2>(dimensions);
}

static auto make_checkerboard() -> Matrix<Rgba8> {
  constexpr U32 width = 64;
  constexpr U32 height = 64;
  Dynamic::Vector<Rgba8> pixels;
  for (U32 row = 0; row < height; row++) {
    for (U32 column = 0; column < width; column++) {
      Bool alternate = ((row / 8) + (column / 8)) % 2 != 0;
      pixels.emplace(
          alternate ? Rgba8::from_rgb(0xF2, 0x3D, 0x86)
                    : Rgba8::from_rgb(0x30, 0x18, 0x50));
    }
  }

  return Matrix<Rgba8>::create(
      Domain<2, Boundary::Clip, Boundary::Clip>(make_size(width, height)),
      pixels.get_view());
}

static auto make_overlay() -> Matrix<Rgba8> {
  constexpr U32 width = 64;
  constexpr U32 height = 64;
  Dynamic::Vector<Rgba8> pixels;
  for (U32 row = 0; row < height; row++) {
    for (U32 column = 0; column < width; column++) {
      U8 value = (row + column) * 2;
      pixels.emplace(Rgba8::from_rgba(0x28, value, 0xE8, 0xFF));
    }
  }

  return Matrix<Rgba8>::create(
      Domain<2, Boundary::Clip, Boundary::Clip>(make_size(width, height)),
      pixels.get_view());
}

static auto make_inputs(R32 red, R32 green, R32 blue, R32 alpha)
    -> Dynamic::Bytes {
  R32 tone[] = {red, green, blue, alpha};
  Dynamic::Bytes inputs;
  inputs.append(0, sizeof(tone));
  Data::copy(inputs.get_access().get_data(), tone, Data::array_size(tone));
  return inputs;
}

static auto make_batch(
    Frame::Program program,
    const Texture2D& image,
    const Texture2D& overlay,
    R64 x,
    R64 y,
    R32 red,
    R32 green,
    R32 blue,
    S64 z_index,
    Count order) -> Frame::Batch {
  Dynamic::Vector<Texture2D> resources;
  resources.insert(image);
  resources.insert(overlay);
  return Frame::Batch(
      program, Data::take(resources), make_inputs(red, green, blue, 1.0f),
      Frame::Transform::create(x, y, 1.0, 1.0, 0.0), make_size(192, 192),
      Frame::Pipeline(), 6, z_index, order);
}

static auto read_frame_limit(int count, char** values) -> Count {
  if (count < 2) {
    return 0;
  }

  char* end = nullptr;
  U64 limit = strtoull(values[1], &end, 10);
  return end != values[1] && *end == 0 ? limit : 0;
}

int main(int count, char** values) {
  constexpr U32 initial_width = 800;
  constexpr U32 initial_height = 600;
  Platform::Window window(
      initial_width, initial_height, "Photophore rendering pipeline");
  if (window.get_event_status() != Platform::Window::EventStatus::Ready) {
    fputs("Photophore could not create a native window.\n", stderr);
    return 1;
  }

  Renderer renderer(
      window.get_presentation(), initial_width * window.get_scale(),
      initial_height * window.get_scale());
  if (!renderer.get_context().get_device() ||
      !renderer.get_swapchain().get_swapchain()) {
    fputs("Photophore could not create a Vulkan renderer.\n", stderr);
    return 2;
  }

  const Description::Program description =
      Examples::SpriteProgram::get_description();
  const View::Vector<Description::Program> descriptions(&description, 1);
  Pipelines pipelines(
      renderer.get_context(), renderer.get_swapchain().get_format(),
      descriptions);
  Texture2D image(make_checkerboard());
  Texture2D overlay(make_overlay());
  const Frame::Program program = Examples::SpriteProgram::get_locator();
  const Count frame_limit = read_frame_limit(count, values);
  Count frames = 0;

  while (window.poll_events() == Platform::Window::EventStatus::Ready &&
         (frame_limit == 0 || frames < frame_limit)) {
    if (window.get_needs_resize()) {
      Bool format_changed = renderer.resize(
          window.get_logical_width() * window.get_scale(),
          window.get_logical_height() * window.get_scale());
      if (format_changed) {
        pipelines.rebuild(renderer.get_swapchain().get_format());
      }
      window.clear_resize();
    }

    Renderer::Frame frame;
    if (!renderer.begin_frame(frame)) {
      continue;
    }

    R64 motion = frames % 240;
    if (motion > 120) {
      motion = 240 - motion;
    }
    Dynamic::Vector<Frame::Batch> batches;
    batches.emplace(make_batch(
        program, image, overlay, 64.0 + motion, 96.0, 1.0f, 0.75f, 0.9f, 0, 0));
    batches.emplace(make_batch(
        program, image, overlay, 480.0 - motion, 304.0, 0.55f, 0.8f, 1.0f, 1,
        1));

    Bool recorded = pipelines.record(frame, batches.get_view());
    renderer.end_frame(frame);
    if (!recorded) {
      renderer.wait_idle();
      fputs("Photophore rejected the demo frame.\n", stderr);
      return 3;
    }
    frames++;
  }

  renderer.wait_idle();
  printf("Photophore presented %llu frames.\n", frames);
  return 0;
}
