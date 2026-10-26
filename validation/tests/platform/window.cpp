// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/window.hpp"

#include "toolchain/validation/test.hpp"

#include "photophore/vulkan/renderer.hpp"

using namespace Photophore::Platform;
using namespace Photophore::Vulkan;
using namespace Toolchain::Validation;

static Harness PlatformWindow = {
  .name = "Window",
};

VALIDATION_TEST(PlatformWindow, empty_window_fails) {
  Window window;
  EXPECT(window.get_event_status() == Window::EventStatus::Failed);
  EXPECT(window.poll_events() == Window::EventStatus::Failed);
}

// Absence must stop at acquisition. Asking for a frame from that result must
// not attempt any native driver calls through an uninitialized context.
VALIDATION_TEST(PlatformWindow, unavailable_renderer) {
  Window window;
  Renderer renderer(window.get_presentation(), 64, 64);
  EXPECT(renderer.get_context().get_device() == VK_NULL_HANDLE);

  Renderer::Frame frame;
  EXPECT(!renderer.begin_frame(frame));
  EXPECT(!renderer.resize(96, 64));
}
