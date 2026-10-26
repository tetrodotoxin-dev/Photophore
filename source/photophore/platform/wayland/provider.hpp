// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "toolchain/toolchain.h"

#ifdef TOOLCHAIN_LINUX

#include "photophore/platform/wayland/client.hpp"
#include "photophore/platform/wayland/xdg_shell_v1.hpp"
#include "photophore/platform/window.hpp"

namespace Photophore::Platform::Wayland {

// Provider supplies the Linux operations for one public Window. Its
// stable allocation owns the display connection, protocol objects and callback
// state. Libwayland dispatches each event with this provider's callback record.
class Provider final : public Window::Provider {
 public:
  using EventStatus = Photophore::Platform::Window::EventStatus;

  static auto create(U32 width, U32 height, Perimortem::Core::View::Bytes title)
      -> Provider*;

  ~Provider() = default;
  auto poll_events() -> EventStatus override;
  auto get_presentation() -> Presentation override;
  auto destroy() -> void override;

 private:
  struct CallbackContext {
    Provider* value = nullptr;
  };

  static auto callback_provider(const void* context) -> Provider&;
  Provider() = default;
  auto initialize(U32 width, U32 height, Perimortem::Core::View::Bytes title)
      -> void;
  auto register_global(
      Wayland::ABI::Proxy* registry,
      U32 name,
      Perimortem::Core::View::Bytes interface,
      U32 version) -> void;
  auto remove_global(U32 name) -> void;
  auto update_seat(U32 capabilities) -> void;
  auto clear_input() -> void;

  Wayland::ABI::Display* display = nullptr;
  Wayland::ABI::Proxy* registry = nullptr;
  Wayland::ABI::Proxy* compositor = nullptr;
  Wayland::ABI::Proxy* shell = nullptr;
  Wayland::ABI::Proxy* surface = nullptr;
  Wayland::ABI::Proxy* shell_surface = nullptr;
  Wayland::ABI::Proxy* toplevel = nullptr;
  Wayland::ABI::Proxy* seat = nullptr;
  Wayland::ABI::Proxy* keyboard = nullptr;
  Wayland::ABI::Proxy* pointer = nullptr;
  U32 seat_name = 0;
  Bool keyboard_focused = False;
  CallbackContext callback_context;
};

}  // namespace Photophore::Platform::Wayland

#endif
