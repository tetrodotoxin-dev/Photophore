// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "toolchain/toolchain.h"

#ifdef TOOLCHAIN_LINUX

#include "perimortem/core/view/bytes.hpp"

#include "photophore/platform/wayland/details/client.hpp"

namespace Photophore::Platform::Wayland::Details::XdgShellV1 {

// XdgShellV1 is Photophore's private binding for the stable xdg-shell version
// one wire contract. Its source controlled message tables fix the supported
// protocol independently of the host's scanner and protocol-data packages.
// The adapter exposes only the operations used by Window while libwayland owns
// the core proxy ABI, message transport and object identifiers.
enum class WmBaseEvent : U32 { Ping };
enum class SurfaceEvent : U32 { Configure };
enum class ToplevelEvent : U32 { Configure, Close };

auto matches(Perimortem::Core::View::Bytes interface) -> Bool;
auto bind(ABI::Proxy* registry, U32 name) -> ABI::Proxy*;

auto get_surface(ABI::Proxy* shell, ABI::Proxy* surface) -> ABI::Proxy*;
auto get_toplevel(ABI::Proxy* surface) -> ABI::Proxy*;
auto acknowledge(ABI::Proxy* surface, U32 serial) -> void;
auto pong(ABI::Proxy* shell, U32 serial) -> void;
auto set_title(ABI::Proxy* toplevel, Perimortem::Core::View::Bytes title)
    -> Bool;
auto set_application(
    ABI::Proxy* toplevel,
    Perimortem::Core::View::Bytes application) -> Bool;
auto destroy(ABI::Proxy* proxy) -> void;

}  // namespace Photophore::Platform::Wayland::Details::XdgShellV1

#endif
