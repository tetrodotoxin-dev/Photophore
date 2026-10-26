// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/wayland/client.hpp"

#ifdef TOOLCHAIN_LINUX

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Platform;

constexpr U32 destroy_flag = 1;
constexpr U32 compositor_release_version = 7;
constexpr U32 keyboard_release_version = 3;
constexpr U32 pointer_release_version = 3;
constexpr U32 seat_release_version = 5;

enum class DisplayRequest : U32 { Synchronize, GetRegistry };
enum class RegistryRequest : U32 { Bind };
enum class CompositorRequest : U32 { CreateSurface, CreateRegion, Release };
enum class CoreSurfaceRequest : U32 {
  Destroy,
  Attach,
  Damage,
  Frame,
  SetOpaqueRegion,
  SetInputRegion,
  Commit,
  SetBufferTransform,
  SetBufferScale
};

enum class SeatRequest : U32 { GetPointer, GetKeyboard, GetTouch, Release };
enum class KeyboardRequest : U32 { Release };
enum class PointerRequest : U32 { SetCursor, Release };

static auto create(
    Wayland::ABI::Proxy* proxy,
    U32 opcode,
    const Wayland::ABI::Interface& interface) -> Wayland::ABI::Proxy* {
  void* new_id = nullptr;
  return Wayland::ABI::wl_proxy_marshal_flags(
      proxy, opcode, &interface, Wayland::ABI::wl_proxy_get_version(proxy), 0,
      new_id);
}

static auto release(Wayland::ABI::Proxy* proxy, U32 minimum_version, U32 opcode)
    -> void {
  if (!proxy) {
    return;
  }

  if (Wayland::ABI::wl_proxy_get_version(proxy) < minimum_version) {
    Wayland::ABI::wl_proxy_destroy(proxy);
    return;
  }

  Wayland::ABI::wl_proxy_marshal_flags(
      proxy, opcode, nullptr, Wayland::ABI::wl_proxy_get_version(proxy),
      destroy_flag);
}

auto Wayland::Client::connect() -> Wayland::ABI::Display* {
  return Wayland::ABI::wl_display_connect(nullptr);
}

auto Wayland::Client::disconnect(Wayland::ABI::Display* display) -> void {
  if (display) {
    Wayland::ABI::wl_display_disconnect(display);
  }
}

auto Wayland::Client::get_registry(Wayland::ABI::Display* display)
    -> Wayland::ABI::Proxy* {
  if (!display) {
    return nullptr;
  }

  auto* proxy = Data::bit_cast<Wayland::ABI::Proxy*>(display);
  return create(
      proxy, U32(DisplayRequest::GetRegistry),
      Wayland::ABI::wl_registry_interface);
}

auto Wayland::Client::bind(
    Wayland::ABI::Proxy* registry,
    U32 name,
    const Wayland::ABI::Interface& interface,
    U32 version) -> Wayland::ABI::Proxy* {
  if (!registry) {
    return nullptr;
  }

  void* new_id = nullptr;
  return Wayland::ABI::wl_proxy_marshal_flags(
      registry, U32(RegistryRequest::Bind), &interface, version, 0, name,
      interface.name, version, new_id);
}

auto Wayland::Client::select_version(
    U32 advertised,
    const Wayland::ABI::Interface& interface,
    U32 maximum) -> U32 {
  U32 selected = advertised < maximum ? advertised : maximum;
  if (interface.version <= 0) {
    return 0;
  }

  const U32 available = U32(interface.version);
  return selected < available ? selected : available;
}

auto Wayland::Client::create_surface(Wayland::ABI::Proxy* compositor)
    -> Wayland::ABI::Proxy* {
  return compositor ? create(
                          compositor, U32(CompositorRequest::CreateSurface),
                          Wayland::ABI::wl_surface_interface)
                    : nullptr;
}

auto Wayland::Client::get_keyboard(Wayland::ABI::Proxy* seat)
    -> Wayland::ABI::Proxy* {
  return seat ? create(
                    seat, U32(SeatRequest::GetKeyboard),
                    Wayland::ABI::wl_keyboard_interface)
              : nullptr;
}

auto Wayland::Client::get_pointer(Wayland::ABI::Proxy* seat)
    -> Wayland::ABI::Proxy* {
  return seat ? create(
                    seat, U32(SeatRequest::GetPointer),
                    Wayland::ABI::wl_pointer_interface)
              : nullptr;
}

auto Wayland::Client::listen(
    Wayland::ABI::Proxy* proxy,
    Wayland::ABI::Dispatcher* callback,
    const void* callback_context,
    void* context) -> S32 {
  if (!proxy || !callback) {
    return -1;
  }

  return Wayland::ABI::wl_proxy_add_dispatcher(
      proxy, callback, callback_context, context);
}

auto Wayland::Client::commit(Wayland::ABI::Proxy* surface) -> void {
  Wayland::ABI::wl_proxy_marshal_flags(
      surface, U32(CoreSurfaceRequest::Commit), nullptr,
      Wayland::ABI::wl_proxy_get_version(surface), 0);
}

auto Wayland::Client::set_buffer_scale(Wayland::ABI::Proxy* surface, S32 scale)
    -> void {
  Wayland::ABI::wl_proxy_marshal_flags(
      surface, U32(CoreSurfaceRequest::SetBufferScale), nullptr,
      Wayland::ABI::wl_proxy_get_version(surface), 0, scale);
}

auto Wayland::Client::release_compositor(Wayland::ABI::Proxy* compositor)
    -> void {
  release(
      compositor, compositor_release_version, U32(CompositorRequest::Release));
}

auto Wayland::Client::release_seat(Wayland::ABI::Proxy* seat) -> void {
  release(seat, seat_release_version, U32(SeatRequest::Release));
}

auto Wayland::Client::release_keyboard(Wayland::ABI::Proxy* keyboard) -> void {
  release(keyboard, keyboard_release_version, U32(KeyboardRequest::Release));
}

auto Wayland::Client::release_pointer(Wayland::ABI::Proxy* pointer) -> void {
  release(pointer, pointer_release_version, U32(PointerRequest::Release));
}

auto Wayland::Client::destroy_surface(Wayland::ABI::Proxy* surface) -> void {
  if (surface) {
    Wayland::ABI::wl_proxy_marshal_flags(
        surface, U32(CoreSurfaceRequest::Destroy), nullptr,
        Wayland::ABI::wl_proxy_get_version(surface), destroy_flag);
  }
}

auto Wayland::Client::destroy_proxy(Wayland::ABI::Proxy* proxy) -> void {
  if (proxy) {
    Wayland::ABI::wl_proxy_destroy(proxy);
  }
}

auto Wayland::Client::dispatch_pending(Wayland::ABI::Display* display) -> S32 {
  return Wayland::ABI::wl_display_dispatch_pending(display);
}

auto Wayland::Client::prepare_read(Wayland::ABI::Display* display) -> S32 {
  return Wayland::ABI::wl_display_prepare_read(display);
}

auto Wayland::Client::cancel_read(Wayland::ABI::Display* display) -> void {
  Wayland::ABI::wl_display_cancel_read(display);
}

auto Wayland::Client::read_events(Wayland::ABI::Display* display) -> S32 {
  return Wayland::ABI::wl_display_read_events(display);
}

auto Wayland::Client::flush(Wayland::ABI::Display* display) -> S32 {
  return Wayland::ABI::wl_display_flush(display);
}

auto Wayland::Client::roundtrip(Wayland::ABI::Display* display) -> S32 {
  return Wayland::ABI::wl_display_roundtrip(display);
}

auto Wayland::Client::get_descriptor(Wayland::ABI::Display* display) -> S32 {
  return Wayland::ABI::wl_display_get_fd(display);
}

#endif
