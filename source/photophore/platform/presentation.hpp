// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Platform {

// Presentation is the borrowed native surface input consumed by Context. Kind
// selects the matching platform implementation, while host and surface carry
// its two native handles. Wayland supplies wl_display and wl_surface. Win32
// supplies HINSTANCE and HWND. This common record keeps those platform headers
// outside application facing rendering interfaces.
//
// Window supplies the lifetime of both handles and remains live through the
// Vulkan surface created from them. Copies preserve the same borrowed handles.
// Kind::None represents an unavailable presentation path.
class Presentation {
 public:
  enum class Kind : U64 {
    None,
    Wayland,
    Win32,
  };

  static constexpr auto create(Kind kind, void* host, void* surface)
      -> Presentation {
    return Presentation(kind, host, surface);
  }

  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_host() const -> void* { return host; }
  constexpr auto get_surface() const -> void* { return surface; }

  constexpr auto is_valid() const -> Bool {
    return Bool(kind != Kind::None && host && surface);
  }

 private:
  constexpr Presentation(Kind kind, void* host, void* surface)
      : kind(kind), host(host), surface(surface) {}

  Kind kind = Kind::None;
  void* host = nullptr;
  void* surface = nullptr;
};

static_assert(sizeof(Presentation) == sizeof(U64) + sizeof(void*) * 2);
static_assert(__is_trivially_copyable(Presentation));
static_assert(__is_standard_layout(Presentation));

}  // namespace Photophore::Platform
