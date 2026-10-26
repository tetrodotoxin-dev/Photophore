// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/window.hpp"

#include "toolchain/validation/test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/view/null_terminated.hpp"

#include "photophore/vulkan/renderer.hpp"

using namespace Photophore::Platform;
using namespace Photophore::Vulkan;
using namespace Toolchain::Validation;
using namespace Perimortem::Core;

static Harness PlatformWindow = {
  .name = "Window",
};

VALIDATION_TEST(PlatformWindow, explicit_null_title_is_rejected) {
  constexpr auto title = "Visible\0Hidden"_view;
  EXPECT_EQ(title.get_size(), Count(14));
  EXPECT_EQ(title[7], U8(0));

  EXPECT_NOT(Window::create(64, 64, title));
}

// Absence must stop at acquisition. Asking for a frame from that result must
// not attempt any native driver calls through an uninitialized context.
VALIDATION_TEST(PlatformWindow, unavailable_renderer) {
  Presentation unavailable;
  Renderer renderer(unavailable, 64, 64);
  EXPECT(renderer.get_context().get_device() == VK_NULL_HANDLE);

  Renderer::Frame frame;
  EXPECT(!renderer.begin_frame(frame));
  EXPECT(!renderer.resize(96, 64));
}

#ifndef TOOLCHAIN_WASM

// Move assignment releases the target provider and transfers the source's
// stable provider allocation. A second native Window also exercises shared host
// registration before the assignment retires one of them.
VALIDATION_TEST(PlatformWindow, native_move_assignment) {
  Window::create(71, 53, "Photophore move source"_view)
      .visit(
          [&](Window& source) {
            const U32 source_width = source.get_logical_width();
            const U32 source_height = source.get_logical_height();
            Window::create(37, 29, "Photophore move target"_view)
                .visit(
                    [&](Window& target) {
                      target = Data::take(source);
                      EXPECT_EQ(target.get_logical_width(), source_width);
                      EXPECT_EQ(target.get_logical_height(), source_height);
                      EXPECT(target.get_presentation().is_valid());
                    },

                    [&] { EXPECT(False); });
          },

          [&] { EXPECT(False); });
}

#endif
