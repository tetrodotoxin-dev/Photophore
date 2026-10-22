// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/window.hpp"

#include "photophore/vulkan/renderer.hpp"
#include "toolchain/validation/unit_test.hpp"

static Toolchain::Validation::Harness PlatformWindow = {
  .name = "Photophore::Platform::Window",
};

VALIDATION_TEST(PlatformWindow, empty_window_fails) {
  Photophore::Platform::Window window;
  EXPECT(
      window.get_event_status() ==
      Photophore::Platform::Window::EventStatus::Failed);
  EXPECT(
      window.poll_events() ==
      Photophore::Platform::Window::EventStatus::Failed);
}

// Absence must stop at acquisition. Asking for a frame from that result must
// not attempt any native driver calls through an uninitialized context.
VALIDATION_TEST(PlatformWindow, unavailable_renderer) {
  Photophore::Platform::Window window;
  Photophore::Vulkan::Renderer renderer(window.get_presentation(), 64, 64);
  EXPECT(renderer.get_context().get_device() == VK_NULL_HANDLE);
  Photophore::Vulkan::Renderer::Frame frame;
  EXPECT(!renderer.begin_frame(frame));
  EXPECT(!renderer.resize(96, 64));
}
