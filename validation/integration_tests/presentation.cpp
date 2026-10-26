// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// Runs acquisition, submission and presentation through one live Window and
// Renderer. Failure to present every requested frame fails the check rather
// than skipping it.
#include <stdio.h>

#include "perimortem/core/data.hpp"
#include "perimortem/core/view/null_terminated.hpp"

#include "photophore/platform/window.hpp"
#include "photophore/vulkan/renderer.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Platform;
using namespace Photophore::Vulkan;

static auto present(Window& window) -> S32 {
  Renderer renderer(
      window.get_presentation(),
      window.get_logical_width() * window.get_scale(),
      window.get_logical_height() * window.get_scale());
  renderer.resize(0, 0);

  Renderer::Frame minimized;
  if (renderer.begin_frame(minimized)) {
    return 5;
  }

  renderer.resize(
      window.get_logical_width() * window.get_scale(),
      window.get_logical_height() * window.get_scale());

  U32 completed = 0;
  for (U32 attempt = 0; attempt < 12 && completed < 3; ++attempt) {
    if (window.poll_events() != Window::EventStatus::Ready) {
      return 2;
    }

    Renderer::Frame frame;
    if (renderer.begin_frame(frame)) {
      renderer.end_frame(frame);
      ++completed;
    }
  }

  renderer.wait_idle();

  constexpr auto completion_format =
      "Presented %u frames through Vulkan.\n\0"_view;
  printf(Data::cast<const char>(completion_format.get_data()), completed);
  return completed == 3 ? 0 : 4;
}

S32 main() {
  return Window::create(96, 64, "Photophore presentation check"_view)
      .visit(
          [](Window& window) { return present(window); },
          [] { return S32(1); });
}
