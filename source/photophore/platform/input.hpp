// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "photophore/input/mapping.hpp"
#include "photophore/input/snapshot.hpp"

namespace Photophore::Platform {

// Input accumulates physical keyboard, pointer button, motion and scroll events
// between application observations. `collect` maps both the current and
// previous physical states with the same application Mapping before publishing
// one immutable Snapshot.
//
// collect returns a value snapshot and advances the comparison point. Scroll
// accumulates through that call, while pointer motion measures the distance
// from the previous focused observation. A focus change starts a fresh motion
// baseline. Each Window contains one Input and uses it on the window thread.
class Input {
 public:
  using Key = Photophore::Input::Key;
  constexpr Input() = default;

  EXPORTED(PHOTOPHORE) auto set_key(Key key, Bool down) -> void;
  EXPORTED(PHOTOPHORE) auto clear_keyboard() -> void;
  EXPORTED(PHOTOPHORE) auto clear_pointer() -> void;
  EXPORTED(PHOTOPHORE) auto set_pointer(R32 x, R32 y) -> void;
  EXPORTED(PHOTOPHORE) auto scroll(R32 x, R32 y) -> void;
  constexpr auto is_pointer_active() const -> Bool { return pointer_focused; }
  EXPORTED(PHOTOPHORE)
  auto collect(const Photophore::Input::Mapping& mapping)
      -> Photophore::Input::Snapshot;

  // Translation maps the selected host's physical keyboard and button codes
  // into stable game controls. Text entry remains a separate interpretation of
  // the user's keyboard layout.
  static EXPORTED(PHOTOPHORE) auto translate_keyboard(U32 code) -> Key;
  static EXPORTED(PHOTOPHORE) auto translate_button(U32 code) -> Key;

 private:
  static constexpr Count word_bits = sizeof(U64) * 8;
  static constexpr Count word_count =
      (Photophore::Input::key_count + word_bits - 1) / word_bits;
  using KeyBits = Perimortem::Core::Static::Vector<U64, word_count>;

  KeyBits current;
  KeyBits previous;
  R32 pointer_x = 0;
  R32 pointer_y = 0;
  R32 previous_pointer_x = 0;
  R32 previous_pointer_y = 0;
  R32 scroll_x = 0;
  R32 scroll_y = 0;
  Bool pointer_focused = False;
  Bool previous_pointer_focused = False;
};

}  // namespace Photophore::Platform
