// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/toolchain.h"

#ifndef TOOLCHAIN_WASM

#include "toolchain/validation/test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/math/domain.hpp"
#include "perimortem/core/math/size.hpp"
#include "perimortem/core/storage/vector.hpp"
#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/matrix.hpp"
#include "perimortem/memory/thread/bytes.hpp"
#include "perimortem/memory/thread/vector.hpp"

#include "examples/program.hpp"
#include "photophore/frame/batch.hpp"
#include "photophore/image/srgba8.hpp"
#include "photophore/image/texture_2d.hpp"
#include "photophore/platform/window.hpp"
#include "photophore/vulkan/pipelines.hpp"
#include "photophore/vulkan/renderer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Photophore::Examples;
using namespace Photophore::Frame;
using namespace Photophore::Image;
using namespace Photophore::Platform;
using namespace Photophore::Vulkan;
using namespace Toolchain::Validation;

static Harness VulkanRendering = {
  .name = "Photophore::Vulkan::Rendering",
};

static auto make_size(U32 width, U32 height) -> Math::Size<U32, 2> {
  Storage::Vector<U32, 2> dimensions = {{width, height}};
  return Math::Size<U32, 2>(dimensions);
}

static auto make_content() -> Pixels2D {
  Thread::Vector<Srgba8> pixels;
  pixels.insert(Srgba8::from_rgb(0xFF, 0x00, 0x00));
  pixels.insert(Srgba8::from_rgb(0x00, 0xFF, 0x00));
  pixels.insert(Srgba8::from_rgb(0x00, 0x00, 0xFF));
  pixels.insert(Srgba8::from_rgb(0xFF, 0xFF, 0x00));

  Matrix<Srgba8> matrix = Matrix<Srgba8>::create(
      Math::Domain<2>(make_size(2, 2)), pixels.get_view());
  return Pixels2D::create(Data::take(matrix));
}

static auto make_inputs() -> Thread::Bytes {
  R32 tone[] = {1.0f, 1.0f, 1.0f, 1.0f};
  Thread::Bytes inputs;
  inputs.append(0, sizeof(tone));
  Data::copy(inputs.get_access().get_data(), tone, Data::array_size(tone));
  return inputs;
}

static auto make_batch(
    Program program,
    const Texture2D& first,
    const Texture2D& second) -> Batch {
  Thread::Vector<Texture2D> resources;
  resources.insert(first);
  resources.insert(second);

  return Batch(
      program, Data::take(resources), make_inputs(),
      Transform::create(4.0, 4.0, 1.0, 1.0, 0.0), make_size(32, 32), Pipeline(),
      6, 0, 0);
}

static auto submit(
    Renderer& renderer,
    Pipelines& pipelines,
    View::Vector<Batch> batches) -> Bool {
  Renderer::Frame frame;
  if (!renderer.begin_frame(frame)) {
    return False;
  }

  const Bool recorded = pipelines.record(frame, batches);
  renderer.end_frame(frame);
  return recorded;
}

// This case creates a native surface and Vulkan resources inside the validation
// runner. One Pixels2D value supplies two sampling policies, survives pipeline
// rebuilding, leaves use during an empty frame and renders again afterward.
VALIDATION_TEST(VulkanRendering, native_pipeline_lifecycle) {
  Window::create(96, 64, "Photophore native rendering check"_view)
      .visit(
          [&](Window& window) {
            Renderer renderer(
                window.get_presentation(),
                window.get_logical_width() * window.get_scale(),
                window.get_logical_height() * window.get_scale());
            ASSERT(renderer.get_context().get_device());
            ASSERT(renderer.get_swapchain().get_swapchain());

            using DescriptionProgram =
                decltype(SpriteProgram::get_description());
            const DescriptionProgram description =
                SpriteProgram::get_description();
            Pipelines pipelines(
                renderer.get_context(), renderer.get_swapchain().get_format(),
                View::Vector<DescriptionProgram>(description, 1));

            Pixels2D content = make_content();
            Texture2D first = Texture2D::create(
                content, Sampler2D::create(
                             Sampler2D::Addressing::Clamp,
                             Sampler2D::Filtering::Nearest));
            Texture2D second = Texture2D::create(
                content,
                Sampler2D::create(
                    Sampler2D::Addressing::Wrap, Sampler2D::Filtering::Linear));

            Thread::Vector<Batch> batches;
            batches.insert(
                make_batch(SpriteProgram::get_program(), first, second));
            ASSERT(submit(renderer, pipelines, batches.get_view()));
            renderer.wait_idle();

            pipelines.rebuild(renderer.get_swapchain().get_format());
            ASSERT(submit(renderer, pipelines, batches.get_view()));
            renderer.wait_idle();

            ASSERT(submit(renderer, pipelines, View::Vector<Batch>()));
            renderer.wait_idle();

            ASSERT(submit(renderer, pipelines, batches.get_view()));
            renderer.wait_idle();
          },

          [&] { EXPECT(False); });
}

// Two Context values retain one process loader table. Releasing the first must
// leave the second operational, while releasing both permits a later Context to
// open and publish a fresh table in the same process.
VALIDATION_TEST(VulkanRendering, context_lifecycle) {
  Window::create(80, 60, "Photophore context one"_view)
      .visit(
          [&](Window& first_window) {
            Window::create(64, 48, "Photophore context two"_view)
                .visit(
                    [&](Window& second_window) {
                      {
                        Context first =
                            Context::create(first_window.get_presentation());
                        Context second =
                            Context::create(second_window.get_presentation());
                        ASSERT(first.get_device());
                        ASSERT(second.get_device());

                        first = Context();
                        ASSERT(second.get_device());

                        const VkCommandBuffer command =
                            second.begin_immediate_commands();
                        ASSERT(command);
                        second.submit_immediate_commands(command);
                      }

                      Context reacquired =
                          Context::create(first_window.get_presentation());
                      EXPECT(reacquired.get_device());
                    },

                    [&] { EXPECT(False); });
          },

          [&] { EXPECT(False); });
}

#endif
