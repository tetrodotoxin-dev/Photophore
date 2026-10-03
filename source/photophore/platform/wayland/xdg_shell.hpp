// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Platform::Wayland {

// A Wayland surface has no desktop window behavior until it receives a shell
// role. XdgShell binds xdg_wm_base and owns the xdg_surface and xdg_toplevel
// proxies that supply configuration, close requests and compositor ping
// replies. The underlying wl_surface and callback receiver remain borrowed from
// Window.
//
// The protocol tables describe newer messages, but this owner binds version one
// and issues only its requests. Configure serials are acknowledged before the
// surface is committed. Destruction releases the toplevel before its shell
// surface, and Window then releases the underlying surface and connection.
// Listeners retain this owner's address, which is why it cannot be moved.
class XdgShell {
 public:
  struct Events {
    void* context = nullptr;
    void (*configure)(void*, S32, S32) = nullptr;
    void (*close)(void*) = nullptr;
  };

  XdgShell() = default;
  ~XdgShell();
  XdgShell(XdgShell&&) = delete;
  auto operator=(XdgShell&&) -> XdgShell& = delete;
  XdgShell(const XdgShell&) = delete;
  auto operator=(const XdgShell&) -> XdgShell& = delete;

  static auto recognizes(const char* interface) -> Bool;

  auto bind(void* registry, U32 name) -> Bool;
  auto create_toplevel(
      void* wayland_surface,
      const char* title,
      const char* application_id,
      Events events) -> Bool;
  auto destroy() -> void;

 private:
  Events events;
  void* wm_base = nullptr;
  void* surface = nullptr;
  void* toplevel = nullptr;
  void* wayland_surface = nullptr;
};

}  // namespace Photophore::Platform::Wayland
