// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "toolchain/toolchain.h"

#ifdef TOOLCHAIN_WINDOWS

#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>

#include "photophore/platform/window.hpp"

namespace Photophore::Platform::Win32 {

// Provider supplies the Windows operations for one public Window. Its
// stable allocation is stored in GWLP_USERDATA and owns the HWND and shared
// class reservation through native destruction.
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
  Provider() = default;
  auto initialize(U32 width, U32 height, Perimortem::Core::View::Bytes title)
      -> void;

  HINSTANCE module = nullptr;
  HWND surface = nullptr;
  Bool class_acquired = False;
};

}  // namespace Photophore::Platform::Win32

#endif
