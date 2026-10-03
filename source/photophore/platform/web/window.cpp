// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/perimortem.h"

#ifdef PERI_WASM
#include "photophore/platform/window.hpp"
using namespace Photophore::Platform;

// A native Vulkan window cannot be acquired from the browser host. Keep the
// normal Window failure observation so callers can choose another presenter.
Window::Window(U32 width, U32 height, const char*) {
  logical_width = initial_width = width;
  logical_height = initial_height = height;
  failed = True;
}
Window::~Window() = default;
auto Window::poll_events() -> EventStatus {
  return get_event_status();
}
auto Window::get_presentation() const -> Presentation {
  return Presentation::create(Presentation::Kind::None, nullptr, nullptr);
}
auto Input::translate_keyboard(U32) -> Key {
  return Key::None;
}
auto Input::translate_button(U32) -> Key {
  return Key::None;
}
#endif
