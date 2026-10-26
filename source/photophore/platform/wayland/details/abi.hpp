// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "toolchain/toolchain.h"

#ifdef TOOLCHAIN_LINUX

namespace Photophore::Platform::Wayland::Details::ABI {

// These declarations describe the stable data and function ABI exported by
// libwayland-client.so.0. Photophore passes each record back to that runtime
// for message transport and never interprets private Display or Proxy storage.
struct Display {};
struct Proxy {};

using Fixed = S32;

struct Array {
  CppSize size;
  CppSize allocation;
  void* data;
};

union Argument {
  S32 i;
  U32 u;
  Fixed f;
  const char* s;
  void* o;
  U32 n;
  Array* a;
  S32 h;
};

struct Interface {
  const char* name;
  S32 version;
  S32 method_count;
  const struct Message* methods;
  S32 event_count;
  const struct Message* events;
};

struct Message {
  const char* name;
  const char* signature;
  const Interface** types;
};

using Dispatcher = auto(const void*, void*, U32, const Message*, Argument*)
    -> S32;

C_LINKAGE auto wl_display_connect(const char* name) -> Display*;
C_LINKAGE auto wl_display_disconnect(Display* display) -> void;
C_LINKAGE auto wl_display_dispatch_pending(Display* display) -> S32;
C_LINKAGE auto wl_display_prepare_read(Display* display) -> S32;
C_LINKAGE auto wl_display_cancel_read(Display* display) -> void;
C_LINKAGE auto wl_display_read_events(Display* display) -> S32;
C_LINKAGE auto wl_display_flush(Display* display) -> S32;
C_LINKAGE auto wl_display_roundtrip(Display* display) -> S32;
C_LINKAGE auto wl_display_get_fd(Display* display) -> S32;

C_LINKAGE auto wl_proxy_marshal_flags(
    Proxy* proxy,
    U32 opcode,
    const Interface* interface,
    U32 version,
    U32 flags,
    ...) -> Proxy*;

C_LINKAGE auto wl_proxy_add_dispatcher(
    Proxy* proxy,
    Dispatcher* callback,
    const void* callback_context,
    void* context) -> S32;

C_LINKAGE auto wl_proxy_destroy(Proxy* proxy) -> void;
C_LINKAGE auto wl_proxy_get_version(Proxy* proxy) -> U32;

C_LINKAGE const Interface wl_registry_interface;
C_LINKAGE const Interface wl_compositor_interface;
C_LINKAGE const Interface wl_surface_interface;
C_LINKAGE const Interface wl_seat_interface;
C_LINKAGE const Interface wl_keyboard_interface;
C_LINKAGE const Interface wl_pointer_interface;
C_LINKAGE const Interface wl_output_interface;

}  // namespace Photophore::Platform::Wayland::Details::ABI

#endif
