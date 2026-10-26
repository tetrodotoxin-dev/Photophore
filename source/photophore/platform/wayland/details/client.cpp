// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/wayland/details/client.hpp"

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
    Wayland::Details::ABI::Proxy* proxy,
    U32 opcode,
    const Wayland::Details::ABI::Interface& interface)
    -> Wayland::Details::ABI::Proxy* {
  void* new_id = nullptr;
  return Wayland::Details::ABI::wl_proxy_marshal_flags(
      proxy, opcode, &interface,
      Wayland::Details::ABI::wl_proxy_get_version(proxy), 0, new_id);
}

static auto release(
    Wayland::Details::ABI::Proxy* proxy,
    U32 minimum_version,
    U32 opcode) -> void {
  if (!proxy) {
    return;
  }

  if (Wayland::Details::ABI::wl_proxy_get_version(proxy) < minimum_version) {
    Wayland::Details::ABI::wl_proxy_destroy(proxy);
    return;
  }

  Wayland::Details::ABI::wl_proxy_marshal_flags(
      proxy, opcode, nullptr,
      Wayland::Details::ABI::wl_proxy_get_version(proxy), destroy_flag);
}

auto Wayland::Details::Client::connect() -> Wayland::Details::ABI::Display* {
  return Wayland::Details::ABI::wl_display_connect(nullptr);
}

auto Wayland::Details::Client::disconnect(
    Wayland::Details::ABI::Display* display) -> void {
  if (display) {
    Wayland::Details::ABI::wl_display_disconnect(display);
  }
}

auto Wayland::Details::Client::get_registry(
    Wayland::Details::ABI::Display* display) -> Wayland::Details::ABI::Proxy* {
  if (!display) {
    return nullptr;
  }

  auto* proxy = Data::bit_cast<Wayland::Details::ABI::Proxy*>(display);
  return create(
      proxy, U32(DisplayRequest::GetRegistry),
      Wayland::Details::ABI::wl_registry_interface);
}

auto Wayland::Details::Client::bind(
    Wayland::Details::ABI::Proxy* registry,
    U32 name,
    const Wayland::Details::ABI::Interface& interface,
    U32 version) -> Wayland::Details::ABI::Proxy* {
  if (!registry) {
    return nullptr;
  }

  void* new_id = nullptr;
  return Wayland::Details::ABI::wl_proxy_marshal_flags(
      registry, U32(RegistryRequest::Bind), &interface, version, 0, name,
      interface.name, version, new_id);
}

auto Wayland::Details::Client::select_version(
    U32 advertised,
    const Wayland::Details::ABI::Interface& interface,
    U32 maximum) -> U32 {
  U32 selected = advertised < maximum ? advertised : maximum;
  if (interface.version <= 0) {
    return 0;
  }

  const U32 available = U32(interface.version);
  return selected < available ? selected : available;
}

auto Wayland::Details::Client::create_surface(
    Wayland::Details::ABI::Proxy* compositor) -> Wayland::Details::ABI::Proxy* {
  return compositor ? create(
                          compositor, U32(CompositorRequest::CreateSurface),
                          Wayland::Details::ABI::wl_surface_interface)
                    : nullptr;
}

auto Wayland::Details::Client::get_keyboard(Wayland::Details::ABI::Proxy* seat)
    -> Wayland::Details::ABI::Proxy* {
  return seat ? create(
                    seat, U32(SeatRequest::GetKeyboard),
                    Wayland::Details::ABI::wl_keyboard_interface)
              : nullptr;
}

auto Wayland::Details::Client::get_pointer(Wayland::Details::ABI::Proxy* seat)
    -> Wayland::Details::ABI::Proxy* {
  return seat ? create(
                    seat, U32(SeatRequest::GetPointer),
                    Wayland::Details::ABI::wl_pointer_interface)
              : nullptr;
}

auto Wayland::Details::Client::listen(
    Wayland::Details::ABI::Proxy* proxy,
    Wayland::Details::ABI::Dispatcher* callback,
    const void* callback_context,
    void* context) -> S32 {
  if (!proxy || !callback) {
    return -1;
  }

  return Wayland::Details::ABI::wl_proxy_add_dispatcher(
      proxy, callback, callback_context, context);
}

auto Wayland::Details::Client::commit(Wayland::Details::ABI::Proxy* surface)
    -> void {
  Wayland::Details::ABI::wl_proxy_marshal_flags(
      surface, U32(CoreSurfaceRequest::Commit), nullptr,
      Wayland::Details::ABI::wl_proxy_get_version(surface), 0);
}

auto Wayland::Details::Client::set_buffer_scale(
    Wayland::Details::ABI::Proxy* surface,
    S32 scale) -> void {
  Wayland::Details::ABI::wl_proxy_marshal_flags(
      surface, U32(CoreSurfaceRequest::SetBufferScale), nullptr,
      Wayland::Details::ABI::wl_proxy_get_version(surface), 0, scale);
}

auto Wayland::Details::Client::release_compositor(
    Wayland::Details::ABI::Proxy* compositor) -> void {
  release(
      compositor, compositor_release_version, U32(CompositorRequest::Release));
}

auto Wayland::Details::Client::release_seat(Wayland::Details::ABI::Proxy* seat)
    -> void {
  release(seat, seat_release_version, U32(SeatRequest::Release));
}

auto Wayland::Details::Client::release_keyboard(
    Wayland::Details::ABI::Proxy* keyboard) -> void {
  release(keyboard, keyboard_release_version, U32(KeyboardRequest::Release));
}

auto Wayland::Details::Client::release_pointer(
    Wayland::Details::ABI::Proxy* pointer) -> void {
  release(pointer, pointer_release_version, U32(PointerRequest::Release));
}

auto Wayland::Details::Client::destroy_surface(
    Wayland::Details::ABI::Proxy* surface) -> void {
  if (surface) {
    Wayland::Details::ABI::wl_proxy_marshal_flags(
        surface, U32(CoreSurfaceRequest::Destroy), nullptr,
        Wayland::Details::ABI::wl_proxy_get_version(surface), destroy_flag);
  }
}

auto Wayland::Details::Client::destroy_proxy(
    Wayland::Details::ABI::Proxy* proxy) -> void {
  if (proxy) {
    Wayland::Details::ABI::wl_proxy_destroy(proxy);
  }
}

auto Wayland::Details::Client::dispatch_pending(
    Wayland::Details::ABI::Display* display) -> S32 {
  return Wayland::Details::ABI::wl_display_dispatch_pending(display);
}

auto Wayland::Details::Client::prepare_read(
    Wayland::Details::ABI::Display* display) -> S32 {
  return Wayland::Details::ABI::wl_display_prepare_read(display);
}

auto Wayland::Details::Client::cancel_read(
    Wayland::Details::ABI::Display* display) -> void {
  Wayland::Details::ABI::wl_display_cancel_read(display);
}

auto Wayland::Details::Client::read_events(
    Wayland::Details::ABI::Display* display) -> S32 {
  return Wayland::Details::ABI::wl_display_read_events(display);
}

auto Wayland::Details::Client::flush(Wayland::Details::ABI::Display* display)
    -> S32 {
  return Wayland::Details::ABI::wl_display_flush(display);
}

auto Wayland::Details::Client::roundtrip(
    Wayland::Details::ABI::Display* display) -> S32 {
  return Wayland::Details::ABI::wl_display_roundtrip(display);
}

auto Wayland::Details::Client::get_descriptor(
    Wayland::Details::ABI::Display* display) -> S32 {
  return Wayland::Details::ABI::wl_display_get_fd(display);
}

#endif
