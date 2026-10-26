// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/wayland/details/xdg_shell_v1.hpp"

#ifdef TOOLCHAIN_LINUX

#include "perimortem/core/data.hpp"
#include "perimortem/core/storage/vector.hpp"
#include "perimortem/core/view/null_terminated.hpp"

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

extern const Wayland::Details::ABI::Interface
    photophore_xdg_v1_positioner_interface;
extern const Wayland::Details::ABI::Interface
    photophore_xdg_v1_surface_interface;
extern const Wayland::Details::ABI::Interface
    photophore_xdg_v1_toplevel_interface;
extern const Wayland::Details::ABI::Interface photophore_xdg_v1_popup_interface;

static auto c_string(View::Bytes value) -> const char*;

// Each type table follows the corresponding wire arguments in order. An
// interface entry tells libwayland which proxy to create for a new object.
// Scalar arguments retain a null entry.
static Storage::Vector<const Wayland::Details::ABI::Interface*, 1>
    create_positioner_types = {{&photophore_xdg_v1_positioner_interface}};

static Storage::Vector<const Wayland::Details::ABI::Interface*, 2>
    get_surface_types = {{
      &photophore_xdg_v1_surface_interface,
      &Wayland::Details::ABI::wl_surface_interface,
    }};

const Storage::Vector<Wayland::Details::ABI::Message, 4> wm_base_requests = {{
  Wayland::Details::ABI::Message{
    c_string("destroy\0"_view), c_string("\0"_view), nullptr},

  {c_string("create_positioner\0"_view), c_string("n\0"_view),
   create_positioner_types.get_data()},

  {c_string("get_xdg_surface\0"_view), c_string("no\0"_view),
   get_surface_types.get_data()},

  {c_string("pong\0"_view), c_string("u\0"_view), nullptr},
}};

const Storage::Vector<Wayland::Details::ABI::Message, 1> wm_base_events = {
  {Wayland::Details::ABI::Message{
    c_string("ping\0"_view), c_string("u\0"_view), nullptr}}};

const Wayland::Details::ABI::Interface wm_base_interface = {
  c_string("xdg_wm_base\0"_view),   1,
  S32(wm_base_requests.get_size()), wm_base_requests.get_data(),
  S32(wm_base_events.get_size()),   wm_base_events.get_data(),
};

const Storage::Vector<Wayland::Details::ABI::Message, 7> positioner_requests = {
  {
    Wayland::Details::ABI::Message{
      c_string("destroy\0"_view), c_string("\0"_view), nullptr},

    {c_string("set_size\0"_view), c_string("ii\0"_view), nullptr},
    {c_string("set_anchor_rect\0"_view), c_string("iiii\0"_view), nullptr},
    {c_string("set_anchor\0"_view), c_string("u\0"_view), nullptr},
    {c_string("set_gravity\0"_view), c_string("u\0"_view), nullptr},
    {c_string("set_constraint_adjustment\0"_view), c_string("u\0"_view),
     nullptr},

    {c_string("set_offset\0"_view), c_string("ii\0"_view), nullptr},
  }};

const Wayland::Details::ABI::Interface photophore_xdg_v1_positioner_interface =
    {
      c_string("xdg_positioner\0"_view), 1, S32(positioner_requests.get_size()),
      positioner_requests.get_data(),    0, nullptr,
};

static Storage::Vector<const Wayland::Details::ABI::Interface*, 1>
    get_toplevel_types = {{&photophore_xdg_v1_toplevel_interface}};

static Storage::Vector<const Wayland::Details::ABI::Interface*, 3>
    get_popup_types = {{
      &photophore_xdg_v1_popup_interface,
      &photophore_xdg_v1_surface_interface,
      &photophore_xdg_v1_positioner_interface,
    }};

const Storage::Vector<Wayland::Details::ABI::Message, 5> surface_requests = {{
  Wayland::Details::ABI::Message{
    c_string("destroy\0"_view), c_string("\0"_view), nullptr},

  {c_string("get_toplevel\0"_view), c_string("n\0"_view),
   get_toplevel_types.get_data()},

  {c_string("get_popup\0"_view), c_string("n?oo\0"_view),
   get_popup_types.get_data()},

  {c_string("set_window_geometry\0"_view), c_string("iiii\0"_view), nullptr},
  {c_string("ack_configure\0"_view), c_string("u\0"_view), nullptr},
}};

const Storage::Vector<Wayland::Details::ABI::Message, 1> surface_events = {
  {Wayland::Details::ABI::Message{
    c_string("configure\0"_view), c_string("u\0"_view), nullptr}}};

const Wayland::Details::ABI::Interface photophore_xdg_v1_surface_interface = {
  c_string("xdg_surface\0"_view),   1,
  S32(surface_requests.get_size()), surface_requests.get_data(),
  S32(surface_events.get_size()),   surface_events.get_data(),
};

static Storage::Vector<const Wayland::Details::ABI::Interface*, 1>
    set_parent_types = {{&photophore_xdg_v1_toplevel_interface}};

static Storage::Vector<const Wayland::Details::ABI::Interface*, 4>
    show_window_menu_types = {{
      &Wayland::Details::ABI::wl_seat_interface,
      nullptr,
      nullptr,
      nullptr,
    }};

static Storage::Vector<const Wayland::Details::ABI::Interface*, 2> move_types =
    {{
      &Wayland::Details::ABI::wl_seat_interface,
      nullptr,
    }};

static Storage::Vector<const Wayland::Details::ABI::Interface*, 3>
    resize_types = {{
      &Wayland::Details::ABI::wl_seat_interface,
      nullptr,
      nullptr,
    }};

static Storage::Vector<const Wayland::Details::ABI::Interface*, 1>
    set_fullscreen_types = {{&Wayland::Details::ABI::wl_output_interface}};

const Storage::Vector<Wayland::Details::ABI::Message, 14> toplevel_requests = {{
  Wayland::Details::ABI::Message{
    c_string("destroy\0"_view), c_string("\0"_view), nullptr},

  {c_string("set_parent\0"_view), c_string("?o\0"_view),
   set_parent_types.get_data()},

  {c_string("set_title\0"_view), c_string("s\0"_view), nullptr},
  {c_string("set_app_id\0"_view), c_string("s\0"_view), nullptr},
  {c_string("show_window_menu\0"_view), c_string("ouii\0"_view),
   show_window_menu_types.get_data()},

  {c_string("move\0"_view), c_string("ou\0"_view), move_types.get_data()},
  {c_string("resize\0"_view), c_string("ouu\0"_view), resize_types.get_data()},
  {c_string("set_max_size\0"_view), c_string("ii\0"_view), nullptr},
  {c_string("set_min_size\0"_view), c_string("ii\0"_view), nullptr},
  {c_string("set_maximized\0"_view), c_string("\0"_view), nullptr},
  {c_string("unset_maximized\0"_view), c_string("\0"_view), nullptr},
  {c_string("set_fullscreen\0"_view), c_string("?o\0"_view),
   set_fullscreen_types.get_data()},

  {c_string("unset_fullscreen\0"_view), c_string("\0"_view), nullptr},
  {c_string("set_minimized\0"_view), c_string("\0"_view), nullptr},
}};

const Storage::Vector<Wayland::Details::ABI::Message, 2> toplevel_events = {{
  Wayland::Details::ABI::Message{
    c_string("configure\0"_view), c_string("iia\0"_view), nullptr},

  {c_string("close\0"_view), c_string("\0"_view), nullptr},
}};

const Wayland::Details::ABI::Interface photophore_xdg_v1_toplevel_interface = {
  c_string("xdg_toplevel\0"_view),   1,
  S32(toplevel_requests.get_size()), toplevel_requests.get_data(),
  S32(toplevel_events.get_size()),   toplevel_events.get_data(),
};

static Storage::Vector<const Wayland::Details::ABI::Interface*, 2>
    popup_grab_types = {{
      &Wayland::Details::ABI::wl_seat_interface,
      nullptr,
    }};

const Storage::Vector<Wayland::Details::ABI::Message, 2> popup_requests = {{
  Wayland::Details::ABI::Message{
    c_string("destroy\0"_view), c_string("\0"_view), nullptr},

  {c_string("grab\0"_view), c_string("ou\0"_view), popup_grab_types.get_data()},
}};

const Storage::Vector<Wayland::Details::ABI::Message, 2> popup_events = {{
  Wayland::Details::ABI::Message{
    c_string("configure\0"_view), c_string("iiii\0"_view), nullptr},

  {c_string("popup_done\0"_view), c_string("\0"_view), nullptr},
}};

const Wayland::Details::ABI::Interface photophore_xdg_v1_popup_interface = {
  c_string("xdg_popup\0"_view),   1,
  S32(popup_requests.get_size()), popup_requests.get_data(),
  S32(popup_events.get_size()),   popup_events.get_data(),
};

static auto c_string(View::Bytes value) -> const char* {
  if (value.get_size() == 0 || value[value.get_size() - 1] != 0) {
    return nullptr;
  }

  return Data::cast<const char>(value.get_data());
}

auto Wayland::Details::XdgShellV1::matches(View::Bytes interface) -> Bool {
  return interface == "xdg_wm_base"_view;
}

auto Wayland::Details::XdgShellV1::bind(
    Wayland::Details::ABI::Proxy* registry,
    U32 name) -> Wayland::Details::ABI::Proxy* {
  return Wayland::Details::Client::bind(registry, name, wm_base_interface, 1);
}

auto Wayland::Details::XdgShellV1::get_surface(
    Wayland::Details::ABI::Proxy* shell,
    Wayland::Details::ABI::Proxy* surface) -> Wayland::Details::ABI::Proxy* {
  if (!shell || !surface) {
    return nullptr;
  }

  void* new_id = nullptr;
  return Wayland::Details::ABI::wl_proxy_marshal_flags(
      shell, U32(WmBaseRequest::GetSurface),
      &photophore_xdg_v1_surface_interface,
      Wayland::Details::ABI::wl_proxy_get_version(shell), 0, new_id, surface);
}

auto Wayland::Details::XdgShellV1::get_toplevel(
    Wayland::Details::ABI::Proxy* surface) -> Wayland::Details::ABI::Proxy* {
  if (!surface) {
    return nullptr;
  }

  void* new_id = nullptr;
  return Wayland::Details::ABI::wl_proxy_marshal_flags(
      surface, U32(XdgSurfaceRequest::GetToplevel),
      &photophore_xdg_v1_toplevel_interface,
      Wayland::Details::ABI::wl_proxy_get_version(surface), 0, new_id);
}

auto Wayland::Details::XdgShellV1::acknowledge(
    Wayland::Details::ABI::Proxy* surface,
    U32 serial) -> void {
  Wayland::Details::ABI::wl_proxy_marshal_flags(
      surface, U32(XdgSurfaceRequest::Acknowledge), nullptr,
      Wayland::Details::ABI::wl_proxy_get_version(surface), 0, serial);
}

auto Wayland::Details::XdgShellV1::pong(
    Wayland::Details::ABI::Proxy* shell,
    U32 serial) -> void {
  Wayland::Details::ABI::wl_proxy_marshal_flags(
      shell, U32(WmBaseRequest::Pong), nullptr,
      Wayland::Details::ABI::wl_proxy_get_version(shell), 0, serial);
}

auto Wayland::Details::XdgShellV1::set_title(
    Wayland::Details::ABI::Proxy* toplevel,
    View::Bytes title) -> Bool {
  const char* value = c_string(title);
  if (!toplevel || !value) {
    return False;
  }

  Wayland::Details::ABI::wl_proxy_marshal_flags(
      toplevel, U32(ToplevelRequest::SetTitle), nullptr,
      Wayland::Details::ABI::wl_proxy_get_version(toplevel), 0, value);
  return True;
}

auto Wayland::Details::XdgShellV1::set_application(
    Wayland::Details::ABI::Proxy* toplevel,
    View::Bytes application) -> Bool {
  const char* value = c_string(application);
  if (!toplevel || !value) {
    return False;
  }

  Wayland::Details::ABI::wl_proxy_marshal_flags(
      toplevel, U32(ToplevelRequest::SetApplication), nullptr,
      Wayland::Details::ABI::wl_proxy_get_version(toplevel), 0, value);
  return True;
}

auto Wayland::Details::XdgShellV1::destroy(Wayland::Details::ABI::Proxy* proxy)
    -> void {
  if (!proxy) {
    return;
  }

  Wayland::Details::ABI::wl_proxy_marshal_flags(
      proxy, destroy_request, nullptr,
      Wayland::Details::ABI::wl_proxy_get_version(proxy), destroy_flag);
}

#endif
