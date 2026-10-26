// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "photophore/platform/collector.hpp"
#include "photophore/platform/presentation.hpp"

namespace Photophore::Platform {

// Window manages one native top level surface and publishes its size, input
// Snapshots and Presentation. Construction opens the host connection and
// creates the surface. `poll_events` advances native callbacks and replaces the
// stored input observation. EventStatus distinguishes normal operation, a user
// close request and a platform failure.
//
// Dimensions and pointer positions share client coordinates. Multiplying by
// scale gives framebuffer dimensions. Wayland supplies an integer buffer scale,
// while Win32 uses physical client pixels with scale one. A size or scale
// change sets needs_resize until the caller has handled it and calls
// clear_resize.
//
// Native callbacks store the Window address. The value therefore remains at one
// address and on one thread from construction through destruction. Handles in
// Presentation remain accessible until Window destruction, so a Renderer made
// from them ends first.
class Window {
 public:
  // EventStatus keeps an ordinary compositor close separate from a platform
  // failure so an application can preserve its process outcome.
  enum class EventStatus : U8 {
    Ready,
    Closed,
    Failed,
  };

  Window() = default;
  EXPORTED(PHOTOPHORE) Window(U32 width, U32 height, const char* title);
  EXPORTED(PHOTOPHORE) ~Window();
  Window(Window&&) = delete;
  auto operator=(Window&&) -> Window& = delete;
  Window(const Window&) = delete;
  auto operator=(const Window&) -> Window& = delete;

  EXPORTED(PHOTOPHORE) auto poll_events() -> EventStatus;
  EXPORTED(PHOTOPHORE) auto get_event_status() const -> EventStatus;

  EXPORTED(PHOTOPHORE) auto get_logical_width() const -> U32;
  EXPORTED(PHOTOPHORE) auto get_logical_height() const -> U32;
  EXPORTED(PHOTOPHORE) auto get_scale() const -> U32;
  EXPORTED(PHOTOPHORE) auto get_needs_resize() const -> Bool;
  EXPORTED(PHOTOPHORE) auto clear_resize() -> void;

  // A completed poll replaces the stored input Snapshot. `get_input` returns a
  // reference that remains valid through the next poll, while copying it
  // preserves that observation. Mapping changes apply to the next Snapshot.
  EXPORTED(PHOTOPHORE) auto get_input() const
      -> const Photophore::Input::Snapshot&;
  EXPORTED(PHOTOPHORE) auto get_input_mapping() -> Photophore::Input::Mapping&;
  EXPORTED(PHOTOPHORE) auto get_input_mapping() const
      -> const Photophore::Input::Mapping&;

  EXPORTED(PHOTOPHORE) auto get_presentation() const
      -> Photophore::Platform::Presentation;

 private:
  Collector input_collector;
  Photophore::Input::Mapping input_mapping;
  Photophore::Input::Snapshot input_snapshot;
  void* native = nullptr;
  void* connection = nullptr;
  void* surface = nullptr;
  U32 logical_width = 0;
  U32 logical_height = 0;
  U32 initial_width = 0;
  U32 initial_height = 0;
  U32 scale = 1;
  Bool close_requested = False;
  Bool failed = False;
  Bool needs_resize = False;
};

}  // namespace Photophore::Platform
