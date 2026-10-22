// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// Exercise acquisition, submission and presentation with real owner lifetimes.
// Failure to present all frames fails the check rather than skipping it.
#include <stdio.h>

#include "photophore/platform/window.hpp"
#include "photophore/vulkan/renderer.hpp"

int main() {
  Photophore::Platform::Window window(96, 64, "Photophore presentation check");
  if (window.get_event_status() !=
      Photophore::Platform::Window::EventStatus::Ready) {
    return 1;
  }

  Photophore::Vulkan::Renderer renderer(
      window.get_presentation(),
      window.get_logical_width() * window.get_scale(),
      window.get_logical_height() * window.get_scale());
  renderer.resize(0, 0);
  Photophore::Vulkan::Renderer::Frame minimized;
  if (renderer.begin_frame(minimized)) {
    return 5;
  }
  renderer.resize(
      window.get_logical_width() * window.get_scale(),
      window.get_logical_height() * window.get_scale());

  unsigned completed = 0;
  for (unsigned attempt = 0; attempt < 12 && completed < 3; ++attempt) {
    if (window.poll_events() !=
        Photophore::Platform::Window::EventStatus::Ready) {
      return 2;
    }
    Photophore::Vulkan::Renderer::Frame frame;
    if (renderer.begin_frame(frame)) {
      renderer.end_frame(frame);
      ++completed;
    }
  }
  renderer.wait_idle();
  printf("Presented %u frames through Vulkan.\n", completed);
  return completed == 3 ? 0 : 4;
}
