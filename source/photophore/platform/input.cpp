// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/input.hpp"

auto Photophore::Platform::Input::set_key(
    Perimortem::System::Input::Key key,
    Bool down) -> void {
  Count index = Count(key);
  if (key == Perimortem::System::Input::Key::None ||
      index >= Perimortem::System::Input::key_count) {
    return;
  }

  U64 mask = U64(1) << (index % word_bits);
  if (down) {
    current[index / word_bits] |= mask;
  } else {
    current[index / word_bits] &= ~mask;
  }
}

auto Photophore::Platform::Input::clear_keyboard() -> void {
  for (Count index = Count(Perimortem::System::Input::Key::Escape);
       index < Count(Perimortem::System::Input::Key::MousePrimary); index++) {
    set_key(Perimortem::System::Input::Key(index), False);
  }
}

auto Photophore::Platform::Input::clear_pointer() -> void {
  for (Count index = Count(Perimortem::System::Input::Key::MousePrimary);
       index <= Count(Perimortem::System::Input::Key::MouseEight); index++) {
    set_key(Perimortem::System::Input::Key(index), False);
  }
  pointer_focused = False;
}

auto Photophore::Platform::Input::collect(
    const Perimortem::System::Input::Mapping& mapping)
    -> Perimortem::System::Input {
  Perimortem::Core::Static::Vector<
      Perimortem::System::Input::Key, Perimortem::System::Input::key_count>
      current_keys;
  Perimortem::Core::Static::Vector<
      Perimortem::System::Input::Key, Perimortem::System::Input::key_count>
      previous_keys;
  Count current_count = 0;
  Count previous_count = 0;
  for (Count index = 1; index < Perimortem::System::Input::key_count; index++) {
    U64 mask = U64(1) << (index % word_bits);
    if ((current[index / word_bits] & mask) != 0) {
      current_keys[current_count++] = Perimortem::System::Input::Key(index);
    }
    if ((previous[index / word_bits] & mask) != 0) {
      previous_keys[previous_count++] = Perimortem::System::Input::Key(index);
    }
  }

  // Coordinates before a focus change do not share a motion baseline with
  // coordinates after it. Keep the first observed delta at zero instead of
  // reporting travel from an unrelated surface or an old pointer position.
  Perimortem::System::Input::Pointer pointer_state = {
    pointer_x,
    pointer_y,
    pointer_focused && previous_pointer_focused ? pointer_x - previous_pointer_x
                                                : R32(0),
    pointer_focused && previous_pointer_focused ? pointer_y - previous_pointer_y
                                                : R32(0),
    scroll_x,
    scroll_y,
    pointer_focused,
  };
  Perimortem::System::Input output = Perimortem::System::Input::create(
      current_keys.slice(0, current_count),
      previous_keys.slice(0, previous_count), mapping, pointer_state);

  // Both key sets were interpreted with the current mapping. Advance only
  // after making the value snapshot, and consume scroll once per observation
  // so repeated reads of that snapshot cannot consume more input.
  previous = current;
  previous_pointer_x = pointer_x;
  previous_pointer_y = pointer_y;
  previous_pointer_focused = pointer_focused;
  scroll_x = 0;
  scroll_y = 0;
  return output;
}

auto Photophore::Platform::Input::set_pointer(R32 x, R32 y) -> void {
  if (!pointer_focused) {
    previous_pointer_focused = False;
  }
  pointer_x = x;
  pointer_y = y;
  pointer_focused = True;
}

auto Photophore::Platform::Input::scroll(R32 x, R32 y) -> void {
  if (pointer_focused) {
    scroll_x += x;
    scroll_y += y;
  }
}
