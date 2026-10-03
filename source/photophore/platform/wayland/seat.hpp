// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "photophore/platform/input.hpp"

namespace Photophore::Platform::Wayland {

// Wayland calls a group of keyboard, pointer and touch input a seat. The
// compositor publishes that group as wl_seat and reports when its capabilities
// change. Seat owns our subscription to the first advertised group and creates
// keyboard and pointer proxies only while those capabilities are available.
// Touch input and choosing among several seats are not implemented here.
//
// Window supplies a borrowed surface and Input accumulator. Focus events decide
// which controls belong to that surface, and losing focus or a device clears
// held controls so the next observation can report their release. Callbacks
// retain this object's address, so it stays in place until its proxies are
// destroyed. Window destroys Seat before its surface and display connection.
class Seat {
 public:
  explicit Seat(Photophore::Platform::Input& values) : values(values) {}
  ~Seat();
  Seat(Seat&&) = delete;
  auto operator=(Seat&&) -> Seat& = delete;
  Seat(const Seat&) = delete;
  auto operator=(const Seat&) -> Seat& = delete;

  auto attach(void* surface) -> Bool;
  auto register_global(
      void* registry,
      U32 name,
      const char* interface,
      U32 version) -> void;
  auto remove_global(U32 name) -> void;
  auto destroy() -> void;

 private:
  void* seat = nullptr;
  void* keyboard = nullptr;
  void* pointer = nullptr;
  void* surface = nullptr;
  U32 seat_name = 0;
  Photophore::Platform::Input& values;
  Bool keyboard_focused = False;
};

}  // namespace Photophore::Platform::Wayland
