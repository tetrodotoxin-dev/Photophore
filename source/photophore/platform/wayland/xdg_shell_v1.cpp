// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/wayland/xdg_shell_v1.hpp"

#ifdef TOOLCHAIN_LINUX

#include <string.h>

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Platform;

constexpr U32 destroy_flag = 1;
constexpr U32 destroy_request = 0;

enum class WmBaseRequest : U32 { Destroy, CreatePositioner, GetSurface, Pong };

enum class XdgSurfaceRequest : U32 {
  Destroy,
  GetToplevel,
  GetPopup,
  SetWindowGeometry,
  Acknowledge
};

enum class ToplevelRequest : U32 {
  Destroy,
  SetParent,
  SetTitle,
  SetApplication
};

extern const Wayland::ABI::Interface photophore_xdg_v1_positioner_interface;
extern const Wayland::ABI::Interface photophore_xdg_v1_surface_interface;
extern const Wayland::ABI::Interface photophore_xdg_v1_toplevel_interface;
extern const Wayland::ABI::Interface photophore_xdg_v1_popup_interface;

// Each type array follows the corresponding wire arguments in order. An
// interface entry tells libwayland which proxy to create for a new object.
// Scalar arguments retain a null entry.
static const Wayland::ABI::Interface* create_positioner_types[] = {
  &photophore_xdg_v1_positioner_interface};

static const Wayland::ABI::Interface* get_surface_types[] = {
  &photophore_xdg_v1_surface_interface,
  &Wayland::ABI::wl_surface_interface,
};

const Wayland::ABI::Message wm_base_requests[] = {
  {"destroy", "", nullptr},
  {"create_positioner", "n", create_positioner_types},
  {"get_xdg_surface", "no", get_surface_types},
  {"pong", "u", nullptr},
};

const Wayland::ABI::Message wm_base_events[] = {
  {"ping", "u", nullptr},
};

const Wayland::ABI::Interface wm_base_interface = {
  "xdg_wm_base", 1, 4, wm_base_requests, 1, wm_base_events,
};

const Wayland::ABI::Message positioner_requests[] = {
  {"destroy", "", nullptr},
  {"set_size", "ii", nullptr},
  {"set_anchor_rect", "iiii", nullptr},
  {"set_anchor", "u", nullptr},
  {"set_gravity", "u", nullptr},
  {"set_constraint_adjustment", "u", nullptr},
  {"set_offset", "ii", nullptr},
};

const Wayland::ABI::Interface photophore_xdg_v1_positioner_interface = {
  "xdg_positioner", 1, 7, positioner_requests, 0, nullptr,
};

static const Wayland::ABI::Interface* get_toplevel_types[] = {
  &photophore_xdg_v1_toplevel_interface};

static const Wayland::ABI::Interface* get_popup_types[] = {
  &photophore_xdg_v1_popup_interface,
  &photophore_xdg_v1_surface_interface,
  &photophore_xdg_v1_positioner_interface,
};

const Wayland::ABI::Message surface_requests[] = {
  {"destroy", "", nullptr},
  {"get_toplevel", "n", get_toplevel_types},
  {"get_popup", "n?oo", get_popup_types},
  {"set_window_geometry", "iiii", nullptr},
  {"ack_configure", "u", nullptr},
};

const Wayland::ABI::Message surface_events[] = {
  {"configure", "u", nullptr},
};

const Wayland::ABI::Interface photophore_xdg_v1_surface_interface = {
  "xdg_surface", 1, 5, surface_requests, 1, surface_events,
};

static const Wayland::ABI::Interface* set_parent_types[] = {
  &photophore_xdg_v1_toplevel_interface};

static const Wayland::ABI::Interface* show_window_menu_types[] = {
  &Wayland::ABI::wl_seat_interface,
  nullptr,
  nullptr,
  nullptr,
};

static const Wayland::ABI::Interface* move_types[] = {
  &Wayland::ABI::wl_seat_interface,
  nullptr,
};

static const Wayland::ABI::Interface* resize_types[] = {
  &Wayland::ABI::wl_seat_interface,
  nullptr,
  nullptr,
};

static const Wayland::ABI::Interface* set_fullscreen_types[] = {
  &Wayland::ABI::wl_output_interface};

const Wayland::ABI::Message toplevel_requests[] = {
  {"destroy", "", nullptr},
  {"set_parent", "?o", set_parent_types},
  {"set_title", "s", nullptr},
  {"set_app_id", "s", nullptr},
  {"show_window_menu", "ouii", show_window_menu_types},
  {"move", "ou", move_types},
  {"resize", "ouu", resize_types},
  {"set_max_size", "ii", nullptr},
  {"set_min_size", "ii", nullptr},
  {"set_maximized", "", nullptr},
  {"unset_maximized", "", nullptr},
  {"set_fullscreen", "?o", set_fullscreen_types},
  {"unset_fullscreen", "", nullptr},
  {"set_minimized", "", nullptr},
};

const Wayland::ABI::Message toplevel_events[] = {
  {"configure", "iia", nullptr},
  {"close", "", nullptr},
};

const Wayland::ABI::Interface photophore_xdg_v1_toplevel_interface = {
  "xdg_toplevel", 1, 14, toplevel_requests, 2, toplevel_events,
};

static const Wayland::ABI::Interface* popup_grab_types[] = {
  &Wayland::ABI::wl_seat_interface,
  nullptr,
};

const Wayland::ABI::Message popup_requests[] = {
  {"destroy", "", nullptr},
  {"grab", "ou", popup_grab_types},
};

const Wayland::ABI::Message popup_events[] = {
  {"configure", "iiii", nullptr},
  {"popup_done", "", nullptr},
};

const Wayland::ABI::Interface photophore_xdg_v1_popup_interface = {
  "xdg_popup", 1, 2, popup_requests, 2, popup_events,
};

static auto c_string(View::Bytes value) -> const char* {
  if (value.get_size() == 0 || value[value.get_size() - 1] != 0) {
    return nullptr;
  }

  return Data::cast<const char>(value.get_data());
}

auto Wayland::XdgShellV1::matches(const char* interface) -> Bool {
  return Bool(interface && !strcmp(interface, wm_base_interface.name));
}

auto Wayland::XdgShellV1::bind(Wayland::ABI::Proxy* registry, U32 name)
    -> Wayland::ABI::Proxy* {
  return Wayland::Client::bind(registry, name, wm_base_interface, 1);
}

auto Wayland::XdgShellV1::get_surface(
    Wayland::ABI::Proxy* shell,
    Wayland::ABI::Proxy* surface) -> Wayland::ABI::Proxy* {
  if (!shell || !surface) {
    return nullptr;
  }

  void* new_id = nullptr;
  return Wayland::ABI::wl_proxy_marshal_flags(
      shell, U32(WmBaseRequest::GetSurface),
      &photophore_xdg_v1_surface_interface,
      Wayland::ABI::wl_proxy_get_version(shell), 0, new_id, surface);
}

auto Wayland::XdgShellV1::get_toplevel(Wayland::ABI::Proxy* surface)
    -> Wayland::ABI::Proxy* {
  if (!surface) {
    return nullptr;
  }

  void* new_id = nullptr;
  return Wayland::ABI::wl_proxy_marshal_flags(
      surface, U32(XdgSurfaceRequest::GetToplevel),
      &photophore_xdg_v1_toplevel_interface,
      Wayland::ABI::wl_proxy_get_version(surface), 0, new_id);
}

auto Wayland::XdgShellV1::acknowledge(Wayland::ABI::Proxy* surface, U32 serial)
    -> void {
  Wayland::ABI::wl_proxy_marshal_flags(
      surface, U32(XdgSurfaceRequest::Acknowledge), nullptr,
      Wayland::ABI::wl_proxy_get_version(surface), 0, serial);
}

auto Wayland::XdgShellV1::pong(Wayland::ABI::Proxy* shell, U32 serial) -> void {
  Wayland::ABI::wl_proxy_marshal_flags(
      shell, U32(WmBaseRequest::Pong), nullptr,
      Wayland::ABI::wl_proxy_get_version(shell), 0, serial);
}

auto Wayland::XdgShellV1::set_title(
    Wayland::ABI::Proxy* toplevel,
    View::Bytes title) -> Bool {
  const char* value = c_string(title);
  if (!toplevel || !value) {
    return False;
  }

  Wayland::ABI::wl_proxy_marshal_flags(
      toplevel, U32(ToplevelRequest::SetTitle), nullptr,
      Wayland::ABI::wl_proxy_get_version(toplevel), 0, value);
  return True;
}

auto Wayland::XdgShellV1::set_application(
    Wayland::ABI::Proxy* toplevel,
    View::Bytes application) -> Bool {
  const char* value = c_string(application);
  if (!toplevel || !value) {
    return False;
  }

  Wayland::ABI::wl_proxy_marshal_flags(
      toplevel, U32(ToplevelRequest::SetApplication), nullptr,
      Wayland::ABI::wl_proxy_get_version(toplevel), 0, value);
  return True;
}

auto Wayland::XdgShellV1::destroy(Wayland::ABI::Proxy* proxy) -> void {
  if (!proxy) {
    return;
  }

  Wayland::ABI::wl_proxy_marshal_flags(
      proxy, destroy_request, nullptr,
      Wayland::ABI::wl_proxy_get_version(proxy), destroy_flag);
}

#endif
