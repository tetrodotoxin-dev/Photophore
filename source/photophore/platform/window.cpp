// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/window.hpp"

using namespace Photophore::Platform;

auto Window::get_event_status() const -> Window::EventStatus {
  if (failed || !connection) {
    return EventStatus::Failed;
  }
  return close_requested ? EventStatus::Closed : EventStatus::Ready;
}

auto Window::get_logical_width() const -> U32 {
  return logical_width;
}

auto Window::get_logical_height() const -> U32 {
  return logical_height;
}

auto Window::get_scale() const -> U32 {
  return scale;
}

auto Window::get_needs_resize() const -> Bool {
  return needs_resize;
}

auto Window::clear_resize() -> void {
  needs_resize = False;
}

auto Window::get_input() const -> const Perimortem::System::Input& {
  return input_snapshot;
}

auto Window::get_input_mapping() -> Perimortem::System::Input::Mapping& {
  return input_mapping;
}

auto Window::get_input_mapping() const
    -> const Perimortem::System::Input::Mapping& {
  return input_mapping;
}
