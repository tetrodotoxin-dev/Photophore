// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "perimortem/system/input.hpp"

namespace Photophore::Platform {

// A frame may observe input less often than the host delivers events. Input
// keeps the physical control state between observations, then maps both the
// current and previous state with the same application mapping. Remapping a
// held key therefore does not manufacture a press merely because its name
// changed. A press and release entirely between observations is not queued.
//
// collect returns a value snapshot and advances the comparison point. Scroll
// is accumulated until that call, while pointer motion is measured from the
// previous observation. Leaving and reentering the surface resets that motion
// baseline so positions from different focus intervals do not become a jump.
// Each Window owns one accumulator, used on its event thread.
class Input {
 public:
  using Key = Perimortem::System::Input::Key;
  constexpr Input() = default;

  auto set_key(Key key, Bool down) -> void;
  auto clear_keyboard() -> void;
  auto clear_pointer() -> void;
  auto set_pointer(R32 x, R32 y) -> void;
  auto scroll(R32 x, R32 y) -> void;
  constexpr auto is_pointer_active() const -> Bool { return pointer_focused; }
  auto collect(const Perimortem::System::Input::Mapping& mapping)
      -> Perimortem::System::Input;

  // Codes belong to the selected host's physical keyboard and button space.
  // Text interpretation follows the user's layout and belongs to a separate
  // service, so changing keyboard layouts cannot rearrange game controls.
  static auto translate_keyboard(U32 code) -> Key;
  static auto translate_button(U32 code) -> Key;

 private:
  static constexpr Count word_bits = sizeof(U64) * 8;
  static constexpr Count word_count =
      (Perimortem::System::Input::key_count + word_bits - 1) / word_bits;
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
