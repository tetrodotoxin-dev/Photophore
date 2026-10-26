// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stdio.h>
#include <stdlib.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/scalar.hpp"
#include "perimortem/core/time.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/matrix.hpp"

#include "perimortem/serialization/png.hpp"
#include "perimortem/serialization/rgba8.hpp"

#include "examples/program.hpp"
#include "perimortem/math/domain.hpp"
#include "perimortem/math/size.hpp"
#include "photophore/frame/batch.hpp"
#include "photophore/image/srgba8.hpp"
#include "photophore/image/texture_2d.hpp"
#include "photophore/input/snapshot.hpp"
#include "photophore/platform/window.hpp"
#include "photophore/vulkan/pipelines.hpp"
#include "photophore/vulkan/renderer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Math;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Photophore::Image;
using namespace Photophore::Vulkan;

static constexpr U32 sprite_extent_pixels = 192;

// This standard RGBA PNG has red and green upper quadrants followed by blue
// and yellow lower quadrants. Decoding its authored rows makes orientation
// visible through the complete PNG, Matrix, upload and shader path.
static constexpr U8 orientation_png[] = {
  0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
  0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04,
  0x08, 0x06, 0x00, 0x00, 0x00, 0xA9, 0xF1, 0x9E, 0x7E, 0x00, 0x00, 0x00,
  0x1A, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0xF8, 0xCF, 0xC0, 0xF0,
  0x1F, 0x84, 0x91, 0x20, 0x9A, 0x00, 0x94, 0x0F, 0x04, 0x0C, 0x60, 0x8C,
  0x21, 0x00, 0x00, 0x66, 0x16, 0x23, 0xDD, 0xE4, 0x60, 0xDB, 0x58, 0x00,
  0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
};

static auto make_size(U32 width, U32 height) -> Size<U32, 2> {
  Static::Vector<U32, 2> dimensions = {{width, height}};
  return Size<U32, 2>(dimensions);
}

static auto matches(const Srgba8& pixel, U8 red, U8 green, U8 blue) -> Bool {
  return pixel.red == red && pixel.green == green && pixel.blue == blue &&
         pixel.alpha == 0xFF;
}

static auto as_srgba8(const Matrix<Rgba8>& decoded) -> Matrix<Srgba8> {
  Dynamic::Vector<Srgba8> pixels(decoded.get_values().get_size());
  for (const Rgba8& source : decoded.get_values()) {
    pixels.emplace(
        Srgba8::from_rgba(source.red, source.green, source.blue, source.alpha));
  }

  // The embedded demo image is authored for sRGB display. This conversion is
  // the point where decoded channel bytes acquire Photophore's color contract.
  return Matrix<Srgba8>::create(decoded.get_domain(), pixels.get_view());
}

static auto has_expected_orientation(const Matrix<Srgba8>& image) -> Bool {
  if (image.get_column_count() != 4 || image.get_row_count() != 4) {
    return False;
  }

  const auto pixels = image.get_values();
  return matches(pixels[0], 0xFF, 0x00, 0x00) &&
         matches(pixels[3], 0x00, 0xFF, 0x00) &&
         matches(pixels[12], 0x00, 0x00, 0xFF) &&
         matches(pixels[15], 0xFF, 0xFF, 0x00);
}

static auto make_overlay() -> Matrix<Srgba8> {
  // A second independent image exercises another descriptor set in the same
  // fragment stage and makes the composed shader result easy to recognize.
  constexpr U32 width = 64;
  constexpr U32 height = 64;
  Dynamic::Vector<Srgba8> pixels;
  for (U32 row = 0; row < height; row++) {
    for (U32 column = 0; column < width; column++) {
      U8 value = (row + column) * 2;
      pixels.emplace(Srgba8::from_rgba(0x28, value, 0xE8, 0xFF));
    }
  }

  return Matrix<Srgba8>::create(
      Domain<2, Boundary::Clip, Boundary::Clip>(make_size(width, height)),
      pixels.get_view());
}

static auto make_inputs(R32 red, R32 green, R32 blue, R32 alpha)
    -> Dynamic::Bytes {
  // Batch parameter bytes already follow the generated host layout. Pipelines
  // places this tone after its two transform rows in the shared push range.
  R32 tone[] = {red, green, blue, alpha};
  Dynamic::Bytes inputs;
  inputs.append(0, sizeof(tone));
  Data::copy(inputs.get_access().get_data(), tone, Data::array_size(tone));
  return inputs;
}

static auto make_batch(
    Photophore::Frame::Program program,
    const Texture2D& image,
    const Texture2D& overlay,
    R64 x,
    R64 y,
    R32 red,
    R32 green,
    R32 blue,
    S64 z_index,
    Count order) -> Photophore::Frame::Batch {
  Dynamic::Vector<Texture2D> resources;
  // Resource order follows the Program descriptor inventory: base image at set
  // zero and overlay at set one. Each Batch retains both through recording.
  resources.insert(image);
  resources.insert(overlay);
  return Photophore::Frame::Batch(
      program, Data::take(resources), make_inputs(red, green, blue, 1.0f),
      Photophore::Frame::Transform::create(x, y, 1.0, 1.0, 0.0),
      make_size(sprite_extent_pixels, sprite_extent_pixels),
      Photophore::Frame::Pipeline(), 6, z_index, order);
}

static auto read_frame_limit(int count, char** values) -> Count {
  if (count < 2) {
    return 0;
  }

  char* end = nullptr;
  U64 limit = strtoull(values[1], &end, 10);
  return end != values[1] && *end == 0 ? limit : 0;
}

static auto configure_controls(Photophore::Platform::Window& window) -> void {
  using Key = Photophore::Input::Key;
  auto& mapping = window.get_input_mapping();
  // WASD and the arrow keys become two physical bindings for the same logical
  // controls. The application therefore consumes one mapped vocabulary across
  // every Window implementation.
  mapping.remap(Key::W, Key::ArrowUp);
  mapping.remap(Key::A, Key::ArrowLeft);
  mapping.remap(Key::S, Key::ArrowDown);
  mapping.remap(Key::D, Key::ArrowRight);
}

static auto move_sprite(
    const Photophore::Input::Snapshot& input,
    R64 elapsed_seconds,
    U32 width_pixels,
    U32 height_pixels,
    R64& x,
    R64& y) -> void {
  using Key = Photophore::Input::Key;
  constexpr R64 movement_per_second = 240.0;
  const R64 movement = movement_per_second * elapsed_seconds;
  if (input.is_current(Key::ArrowLeft)) {
    x -= movement;
  }

  if (input.is_current(Key::ArrowRight)) {
    x += movement;
  }

  if (input.is_current(Key::ArrowUp)) {
    y -= movement;
  }

  if (input.is_current(Key::ArrowDown)) {
    y += movement;
  }

  // Clamping uses the range where a complete sprite fits. A smaller framebuffer
  // collapses that range to the origin, keeping resize behavior deterministic.
  R64 width = width_pixels;
  R64 height = height_pixels;
  R64 extent = sprite_extent_pixels;
  x = Scalar::clamp(x, 0.0, Scalar::max(0.0, width - extent));
  y = Scalar::clamp(y, 0.0, Scalar::max(0.0, height - extent));
}

static auto advance_bounce(R64 elapsed_seconds, R64& offset, R64& direction)
    -> void {
  constexpr R64 speed = 60.0;
  constexpr R64 extent = 120.0;
  offset += direction * speed * elapsed_seconds;
  // Reflection preserves elapsed movement through each turn, giving the
  // animation a stable speed on both sides of the bounce.
  if (offset > extent) {
    offset = extent * 2.0 - offset;
    direction = -1.0;
  } else if (offset < 0.0) {
    offset = -offset;
    direction = 1.0;
  }
}

int main(int count, char** values) {
  using Key = Photophore::Input::Key;
  constexpr U32 initial_width = 800;
  constexpr U32 initial_height = 600;
  Photophore::Platform::Window window(
      initial_width, initial_height, "Photophore interactive rendering demo");
  if (window.get_event_status() !=
      Photophore::Platform::Window::EventStatus::Ready) {
    fputs("Photophore could not create a native window.\n", stderr);
    return 1;
  }
  configure_controls(window);

  Renderer renderer(
      window.get_presentation(), initial_width * window.get_scale(),
      initial_height * window.get_scale());
  if (!renderer.get_context().get_device() ||
      !renderer.get_swapchain().get_swapchain()) {
    fputs("Photophore could not create a Vulkan renderer.\n", stderr);
    return 2;
  }

  const Photophore::Description::Program description =
      Photophore::Examples::SpriteProgram::get_description();
  // Description arrays and their locator have process lifetime in program.cpp.
  // Pipelines borrows this local top level value for the complete render loop.
  const View::Vector<Photophore::Description::Program> descriptions(
      &description, 1);
  Pipelines pipelines(
      renderer.get_context(), renderer.get_swapchain().get_format(),
      descriptions);
  Matrix<Srgba8> orientation = as_srgba8(
      Png::decode(
          View::Bytes(orientation_png, Data::array_size(orientation_png))));
  if (!has_expected_orientation(orientation)) {
    fputs("Photophore could not decode the orientation PNG.\n", stderr);
    return 3;
  }
  Texture2D image(
      Data::take(orientation),
      Sampler2D(Sampler2D::Addressing::Clamp, Sampler2D::Filtering::Nearest));
  Texture2D overlay(make_overlay());
  const Photophore::Frame::Program program =
      Photophore::Examples::SpriteProgram::get_locator();
  const Count frame_limit = read_frame_limit(count, values);
  Count frames = 0;
  R64 controlled_x = 64.0;
  R64 controlled_y = 96.0;
  R64 bounce_offset = 0.0;
  R64 bounce_direction = 1.0;
  Time previous_time = Time::now();
  fputs(
      "Controls: WASD or arrows move, Space changes tone, Escape exits.\n",
      stdout);
  fputs(
      "PNG corners: red and green on top, blue and yellow on bottom.\n",
      stdout);

  while (window.poll_events() ==
             Photophore::Platform::Window::EventStatus::Ready &&
         (frame_limit == 0 || frames < frame_limit)) {
    const auto& input = window.get_input();
    if (input.is_pressed(Key::Escape)) {
      break;
    }

    // Window reports logical extent and display scale separately. Swapchain
    // recreation may change the attachment format that keys pipeline
    // realization.
    if (window.get_needs_resize()) {
      Bool format_changed = renderer.resize(
          window.get_logical_width() * window.get_scale(),
          window.get_logical_height() * window.get_scale());
      if (format_changed) {
        pipelines.rebuild(renderer.get_swapchain().get_format());
      }
      window.clear_resize();
    }

    const Time current_time = Time::now();
    // A bounded elapsed interval keeps debugger stops and suspended windows
    // from becoming one unusually large movement step after rendering resumes.
    const R64 elapsed_seconds = Scalar::min(
        previous_time.measure(current_time).convert_to_seconds(), 0.1);
    previous_time = current_time;
    move_sprite(
        input, elapsed_seconds, renderer.get_width(), renderer.get_height(),
        controlled_x, controlled_y);
    advance_bounce(elapsed_seconds, bounce_offset, bounce_direction);

    Renderer::Frame frame;
    if (!renderer.begin_frame(frame)) {
      continue;
    }

    Dynamic::Vector<Photophore::Frame::Batch> batches;
    // Two batches share one multistage Program and both Texture2D values while
    // varying host parameters, placement, depth and authored order per draw.
    const Bool alternate_tone = input.is_current(Key::Space);
    batches.emplace(make_batch(
        program, image, overlay, controlled_x, controlled_y,
        alternate_tone ? 0.45f : 1.0f, alternate_tone ? 1.0f : 0.75f,
        alternate_tone ? 0.6f : 0.9f, 0, 0));
    batches.emplace(make_batch(
        program, image, overlay, 480.0 - bounce_offset, 304.0, 0.55f, 0.8f,
        1.0f, 1, 1));

    // record appends the complete batch sequence to the acquired frame. Ending
    // the frame consumes that command buffer and advances presentation.
    Bool recorded = pipelines.record(frame, batches.get_view());
    renderer.end_frame(frame);
    if (!recorded) {
      renderer.wait_idle();
      fputs("Photophore rejected the demo frame.\n", stderr);
      return 4;
    }
    frames++;
  }

  renderer.wait_idle();
  printf("Photophore presented %llu frames.\n", frames);
  return 0;
}
