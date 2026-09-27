// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "photophore/platform/input.hpp"
#include "photophore/platform/presentation.hpp"

namespace Photophore::Platform {

// Window owns a native toplevel and the event state needed to observe it. The
// host implementations supply creation, polling and destruction in their own
// files, while application code uses the same Window and Input operations.
// A failed creation remains destructible and reports EventStatus::Failed.
//
// Dimensions and pointer positions share client coordinates. Multiplying by
// scale gives framebuffer dimensions. Wayland supplies an integer buffer scale,
// while Win32 uses physical client pixels with scale one. A size or scale
// change sets needs_resize until the caller has handled it and calls
// clear_resize.
//
// The native callbacks retain this object's address. Create, poll and destroy
// it on the same thread, and keep it in place while those callbacks exist.
// Presentation borrows its handles, so destroy the renderer's Vulkan surface
// before destroying the Window.
class Window {
 public:
  // EventStatus keeps an ordinary compositor close separate from a host
  // failure so an application can preserve its process outcome.
  enum class EventStatus : U8 {
    Ready,
    Closed,
    Failed,
  };

  Window() = default;
  Window(U32 width, U32 height, const char* title);
  ~Window();
  Window(Window&&) = delete;
  auto operator=(Window&&) -> Window& = delete;
  Window(const Window&) = delete;
  auto operator=(const Window&) -> Window& = delete;

  auto poll_events() -> EventStatus;
  auto get_event_status() const -> EventStatus;

  auto get_logical_width() const -> U32;
  auto get_logical_height() const -> U32;
  auto get_scale() const -> U32;
  auto get_needs_resize() const -> Bool;
  auto clear_resize() -> void;

  // A completed event poll replaces the stored snapshot. get_input borrows
  // that value until the next poll, while copying it preserves an observation.
  // Mapping changes take effect when the next snapshot is collected.
  auto get_input() const -> const Perimortem::System::Input&;
  auto get_input_mapping() -> Perimortem::System::Input::Mapping&;
  auto get_input_mapping() const -> const Perimortem::System::Input::Mapping&;

  auto get_presentation() const -> Photophore::Platform::Presentation;

 private:
  Input input_collector;
  Perimortem::System::Input::Mapping input_mapping;
  Perimortem::System::Input input_snapshot;
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
