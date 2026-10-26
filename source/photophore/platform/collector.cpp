// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/collector.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Input;
using namespace Photophore::Platform;

auto Collector::set_key(Key key, Bool down) -> void {
  Count index = Count(key);
  if (key == Key::None || index >= key_count) {
    return;
  }

  U64 mask = U64(1) << (index % word_bits);
  if (down) {
    current[index / word_bits] |= mask;
  } else {
    current[index / word_bits] &= ~mask;
  }
}

auto Collector::clear_keyboard() -> void {
  for (Count index = Count(Key::Escape); index < Count(Key::MousePrimary);
       index++) {
    set_key(Key(index), False);
  }
}

auto Collector::clear_pointer() -> void {
  for (Count index = Count(Key::MousePrimary); index <= Count(Key::MouseEight);
       index++) {
    set_key(Key(index), False);
  }

  pointer_focused = False;
}

auto Collector::collect(const Mapping& mapping) -> Snapshot {
  Storage::Vector<Key, key_count> current_keys;
  Storage::Vector<Key, key_count> previous_keys;
  Count current_count = 0;
  Count previous_count = 0;
  for (Count index = 1; index < key_count; index++) {
    U64 mask = U64(1) << (index % word_bits);
    if ((current[index / word_bits] & mask) != 0) {
      current_keys[current_count++] = Key(index);
    }

    if ((previous[index / word_bits] & mask) != 0) {
      previous_keys[previous_count++] = Key(index);
    }
  }

  // Each focused interval establishes its own pointer motion baseline. The
  // first observation in a new interval therefore begins with a zero delta.
  Pointer pointer_state = {
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

  Snapshot output = Snapshot::create(
      current_keys.slice(0, current_count),
      previous_keys.slice(0, previous_count), mapping, pointer_state);

  // Both key sets use the current Mapping. Advancing after value construction
  // preserves their shared interpretation, while clearing scroll gives each
  // completed observation exactly the accumulated wheel movement.
  previous = current;
  previous_pointer_x = pointer_x;
  previous_pointer_y = pointer_y;
  previous_pointer_focused = pointer_focused;
  scroll_x = 0;
  scroll_y = 0;
  return output;
}

auto Collector::set_pointer(R32 x, R32 y) -> void {
  if (!pointer_focused) {
    previous_pointer_focused = False;
  }

  pointer_x = x;
  pointer_y = y;
  pointer_focused = True;
}

auto Collector::scroll(R32 x, R32 y) -> void {
  if (pointer_focused) {
    scroll_x += x;
    scroll_y += y;
  }
}
