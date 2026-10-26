// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stdio.h>
#include <stdlib.h>

#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/time.hpp"
#include "perimortem/core/math/domain.hpp"
#include "perimortem/core/math/scalar.hpp"
#include "perimortem/core/math/size.hpp"
#include "perimortem/core/storage/vector.hpp"
#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/matrix.hpp"
#include "perimortem/memory/thread/bytes.hpp"
#include "perimortem/memory/thread/vector.hpp"
#include "perimortem/serialization/formats/png.hpp"
#include "perimortem/serialization/formats/rgba8.hpp"

#include "examples/program.hpp"
#include "photophore/frame/batch.hpp"
#include "photophore/image/srgba8.hpp"
#include "photophore/image/texture_2d.hpp"
#include "photophore/input/snapshot.hpp"
#include "photophore/platform/window.hpp"
#include "photophore/vulkan/pipelines.hpp"
#include "photophore/vulkan/renderer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Photophore::Examples;
using namespace Photophore::Frame;
using namespace Photophore::Image;
using namespace Photophore::Input;
using namespace Photophore::Platform;
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

static auto make_size(U32 width, U32 height) -> Math::Size<U32, 2> {
  Storage::Vector<U32, 2> dimensions = {{width, height}};
  return Math::Size<U32, 2>(dimensions);
}

static auto matches(const Srgba8& pixel, U8 red, U8 green, U8 blue) -> Bool {
  return pixel.red == red && pixel.green == green && pixel.blue == blue &&
         pixel.alpha == 0xFF;
}

static auto as_srgba8(const Matrix<Formats::Rgba8>& decoded) -> Matrix<Srgba8> {
  const View::Vector<Formats::Rgba8> decoded_pixels =
      decoded.get_view().get_values();
  Thread::Vector<Srgba8> pixels(decoded_pixels.get_size());
  for (Count index = 0; index < decoded_pixels.get_size(); index++) {
    const Formats::Rgba8& source = decoded_pixels.get_data()[index];
    pixels.insert(
        Srgba8::from_rgba(source.red, source.green, source.blue, source.alpha));
  }

  // PNG decoding preserves channel bytes without a rendering transfer function.
  // The embedded asset is authored as sRGB, so this copy establishes the
  // Srgba8 contract consumed by Texture2D.
  return Matrix<Srgba8>::create(decoded.get_domain(), pixels.get_view());
}

static auto has_expected_orientation(const Matrix<Srgba8>& image) -> Bool {
  if (image.get_column_count() != 4 || image.get_row_count() != 4) {
    return False;
  }

  const auto pixels = image.get_view().get_values();
  return matches(pixels[0], 0xFF, 0x00, 0x00) &&
         matches(pixels[3], 0x00, 0xFF, 0x00) &&
         matches(pixels[12], 0x00, 0x00, 0xFF) &&
         matches(pixels[15], 0xFF, 0xFF, 0x00);
}

static auto make_overlay() -> Matrix<Srgba8> {
  constexpr U32 width = 64;
  constexpr U32 height = 64;
  Thread::Vector<Srgba8> pixels;
  for (U32 row = 0; row < height; row++) {
    for (U32 column = 0; column < width; column++) {
      U8 value = (row + column) * 2;
      pixels.insert(Srgba8::from_rgba(0x28, value, 0xE8, 0xFF));
    }
  }

  return Matrix<Srgba8>::create(
      Math::Domain<2>(make_size(width, height)), pixels.get_view());
}

static auto make_inputs(R32 red, R32 green, R32 blue, R32 alpha)
    -> Thread::Bytes {
  // Batch parameter bytes already follow the generated host layout. Pipelines
  // places this tone after its two transform rows in the shared push range.
  R32 tone[] = {red, green, blue, alpha};
  Thread::Bytes inputs;
  inputs.append(0, sizeof(tone));
  Data::copy(inputs.get_access().get_data(), tone, Data::array_size(tone));
  return inputs;
}

static auto make_batch(
    Program program,
    const Texture2D& image,
    const Texture2D& overlay,
    R64 x,
    R64 y,
    R32 red,
    R32 green,
    R32 blue,
    S64 z_index,
    Count order) -> Batch {
  Thread::Vector<Texture2D> resources;
  // Resource order follows the Program descriptor inventory: base image at set
  // zero and overlay at set one. Each Batch retains both through recording.
  resources.insert(image);
  resources.insert(overlay);
  return Batch(
      program, Data::take(resources), make_inputs(red, green, blue, 1.0f),
      Transform::create(x, y, 1.0, 1.0, 0.0),
      make_size(sprite_extent_pixels, sprite_extent_pixels), Pipeline(), 6,
      z_index, order);
}

static auto read_frame_limit(S32 count, char** values) -> Count {
  if (count < 2) {
    return 0;
  }

  char* end = nullptr;
  U64 limit = strtoull(values[1], &end, 10);
  return end != values[1] && *end == 0 ? limit : 0;
}

static auto configure_controls(Window& window) -> void {
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
    const Snapshot& input,
    R64 elapsed_seconds,
    U32 width_pixels,
    U32 height_pixels,
    R64& x,
    R64& y) -> void {
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

  // The maximum position subtracts the complete sprite extent. A framebuffer
  // smaller than that extent selects zero, placing the oversized sprite at the
  // origin instead of producing a negative coordinate.
  R64 width = width_pixels;
  R64 height = height_pixels;
  R64 extent = sprite_extent_pixels;
  x = Math::Scalar::clamp(x, 0.0, Math::Scalar::max(0.0, width - extent));
  y = Math::Scalar::clamp(y, 0.0, Math::Scalar::max(0.0, height - extent));
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

static auto run(Window& window, S32 count, char** values) -> S32 {
  constexpr U32 initial_width = 800;
  constexpr U32 initial_height = 600;
  configure_controls(window);

  Renderer renderer(
      window.get_presentation(), initial_width * window.get_scale(),
      initial_height * window.get_scale());
  if (!renderer.get_context().get_device() ||
      !renderer.get_swapchain().get_swapchain()) {
    fputs("Photophore could not create a Vulkan renderer.\n", stderr);
    return 2;
  }

  using DescriptionProgram = decltype(SpriteProgram::get_description());
  const DescriptionProgram description = SpriteProgram::get_description();
  // Description arrays and shader modules have process lifetime in program.cpp.
  // Pipelines borrows this local top level value for the complete render loop.
  const View::Vector<DescriptionProgram> descriptions(description, 1);
  Pipelines pipelines(
      renderer.get_context(), renderer.get_swapchain().get_format(),
      descriptions);
  Matrix<Srgba8> orientation =
      as_srgba8(Formats::Png::decode(View::Bytes(orientation_png)));
  if (!has_expected_orientation(orientation)) {
    fputs("Photophore could not decode the orientation PNG.\n", stderr);
    return 3;
  }

  Texture2D image = Texture2D::create(
      Pixels2D::create(Data::take(orientation)),
      Sampler2D::create(
          Sampler2D::Addressing::Clamp, Sampler2D::Filtering::Nearest));
  Texture2D overlay = Texture2D::create(Pixels2D::create(make_overlay()));
  const Program program = SpriteProgram::get_program();
  const Count frame_limit = read_frame_limit(count, values);
  Count frames = 0;
  R64 controlled_x = 64.0;
  R64 controlled_y = 96.0;
  R64 bounce_offset = 0.0;
  R64 bounce_direction = 1.0;
  Diagnostics::Time previous_time = Diagnostics::Time::now();
  fputs(
      "Controls: WASD or arrows move, Space changes tone, Escape exits.\n",
      stdout);
  fputs(
      "PNG corners: red and green on top, blue and yellow on bottom.\n",
      stdout);

  while (window.poll_events() == Window::EventStatus::Ready &&
         (frame_limit == 0 || frames < frame_limit)) {
    const auto& input = window.get_input();
    if (input.is_pressed(Key::Escape)) {
      break;
    }

    // Window size uses logical coordinates while Vulkan images use physical
    // pixels. A format change after resize invalidates shader pipeline
    // realizations. Uploaded texture images remain compatible.
    if (window.get_needs_resize()) {
      Bool format_changed = renderer.resize(
          window.get_logical_width() * window.get_scale(),
          window.get_logical_height() * window.get_scale());
      if (format_changed) {
        pipelines.rebuild(renderer.get_swapchain().get_format());
      }

      window.clear_resize();
    }

    const Diagnostics::Time current_time = Diagnostics::Time::now();
    // Movement consumes at most one tenth of a second per rendered frame.
    // Debugger stops and suspended windows therefore resume without applying
    // their complete paused interval as one movement step.
    const R64 elapsed_seconds = Math::Scalar::min(
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

    Thread::Vector<Batch> batches;
    // Two batches share one multistage Program and both Texture2D values while
    // varying host parameters, placement, depth and authored order per draw.
    const Bool alternate_tone = input.is_current(Key::Space);
    batches.insert(make_batch(
        program, image, overlay, controlled_x, controlled_y,
        alternate_tone ? 0.45f : 1.0f, alternate_tone ? 1.0f : 0.75f,
        alternate_tone ? 0.6f : 0.9f, 0, 0));
    batches.insert(make_batch(
        program, image, overlay, 480.0 - bounce_offset, 304.0, 0.55f, 0.8f,
        1.0f, 1, 1));

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

S32 main(S32 count, char** values) {
  constexpr U32 initial_width = 800;
  constexpr U32 initial_height = 600;

  return Window::create(
             initial_width, initial_height,
             "Photophore interactive rendering demo"_view)
      .visit(
          [count, values](Window& window) {
            return run(window, count, values);
          },

          [] {
            fputs("Photophore could not create a native window.\n", stderr);
            return S32(1);
          });
}
