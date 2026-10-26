// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/window.hpp"

#include "toolchain/validation/test.hpp"

#include "perimortem/core/view/null_terminated.hpp"

#include "photophore/vulkan/renderer.hpp"

using namespace Photophore::Platform;
using namespace Photophore::Vulkan;
using namespace Toolchain::Validation;

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
