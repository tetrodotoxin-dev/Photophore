// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/window.hpp"

#include "toolchain/toolchain.h"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/view/null_terminated.hpp"

#ifdef TOOLCHAIN_LINUX
#include "photophore/platform/wayland/provider.hpp"
#elifdef TOOLCHAIN_WINDOWS
#include "photophore/platform/win32/provider.hpp"
#endif

using namespace Perimortem::Core;
using namespace Photophore::Input;
using namespace Photophore::Platform;

auto Window::create(U32 width, U32 height, View::Bytes title)
    -> Storage::Option<Window> {

#ifdef TOOLCHAIN_LINUX

  auto* provider = Wayland::Provider::create(width, height, title);

#elifdef TOOLCHAIN_WINDOWS

  auto* provider = Win32::Provider::create(width, height, title);

#else

  Diagnostics::Log::error(
      "Photophore: No registered Window implementation for this platform "
      "target."_view);

  Provider* provider = nullptr;

#endif

  if (!provider) {
    return {};
  }

  return Window(*provider);
}

Window::~Window() {
  if (provider) {
    provider->destroy();
  }
}

Window::Window(Window&& source) noexcept : provider(source.provider) {
  source.provider = nullptr;
}

auto Window::operator=(Window&& source) noexcept -> Window& {
  if (this == &source) {
    return *this;
  }

  if (provider) {
    provider->destroy();
  }

  provider = source.provider;
  source.provider = nullptr;
  return *this;
}

auto Window::poll_events() -> EventStatus {
  return provider ? provider->poll_events() : EventStatus::Failed;
}

auto Window::get_event_status() const -> EventStatus {
  return provider ? provider->get_event_status() : EventStatus::Failed;
}

auto Window::get_logical_width() const -> U32 {
  return provider ? provider->get_logical_width() : 0;
}

auto Window::get_logical_height() const -> U32 {
  return provider ? provider->get_logical_height() : 0;
}

auto Window::get_scale() const -> U32 {
  return provider ? provider->get_scale() : 1;
}

auto Window::get_needs_resize() const -> Bool {
  return provider ? provider->get_needs_resize() : False;
}

auto Window::clear_resize() -> void {
  if (provider) {
    provider->clear_resize();
  }
}

auto Window::get_input() const -> const Snapshot& {
  return provider->get_input();
}

auto Window::get_input_mapping() -> Mapping& {
  return provider->get_input_mapping();
}

auto Window::get_input_mapping() const -> const Mapping& {
  return provider->get_input_mapping();
}

auto Window::get_presentation() const -> Presentation {
  return provider ? provider->get_presentation() : Presentation();
}

auto Window::Provider::get_event_status() const -> EventStatus {
  if (failed) {
    return EventStatus::Failed;
  }

  return close_requested ? EventStatus::Closed : EventStatus::Ready;
}

auto Window::Provider::get_logical_width() const -> U32 {
  return logical_width;
}

auto Window::Provider::get_logical_height() const -> U32 {
  return logical_height;
}

auto Window::Provider::get_scale() const -> U32 {
  return scale;
}

auto Window::Provider::get_needs_resize() const -> Bool {
  return needs_resize;
}

auto Window::Provider::clear_resize() -> void {
  needs_resize = False;
}

auto Window::Provider::get_input() const -> const Snapshot& {
  return input_snapshot;
}

auto Window::Provider::get_input_mapping() -> Mapping& {
  return input_mapping;
}

auto Window::Provider::get_input_mapping() const -> const Mapping& {
  return input_mapping;
}
