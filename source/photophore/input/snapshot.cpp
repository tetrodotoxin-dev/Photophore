// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/input/snapshot.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Input;

auto Snapshot::set(KeyBits& keys, Key key) -> void {
  const Count index = Count(key);
  if (key == Key::None || index >= key_count) {
    return;
  }

  keys[index / word_bits] |= U64(1) << (index % word_bits);
}

auto Snapshot::contains(const KeyBits& keys, Key key) -> Bool {
  const Count index = Count(key);
  if (key == Key::None || index >= key_count) {
    return False;
  }

  return Bool((keys[index / word_bits] & (U64(1) << (index % word_bits))) != 0);
}

auto Snapshot::add_modifier_groups(KeyBits& keys) -> void {
  if (contains(keys, Key::LeftShift) || contains(keys, Key::RightShift)) {
    set(keys, Key::Shift);
  }

  if (contains(keys, Key::LeftControl) || contains(keys, Key::RightControl)) {
    set(keys, Key::Control);
  }

  if (contains(keys, Key::LeftAlt) || contains(keys, Key::RightAlt)) {
    set(keys, Key::Alt);
  }

  if (contains(keys, Key::LeftSuper) || contains(keys, Key::RightSuper)) {
    set(keys, Key::Super);
  }
}

auto Snapshot::collect(View::Vector<Key> keys, const Mapping& mapping)
    -> KeyBits {
  KeyBits output;
  for (const Key physical : keys) {
    set(output, mapping.resolve(physical));
  }

  add_modifier_groups(output);
  return output;
}

auto Snapshot::create(
    View::Vector<Key> current_keys,
    View::Vector<Key> previous_keys,
    const Mapping& mapping,
    Pointer pointer) -> Snapshot {
  const KeyBits current = collect(current_keys, mapping);
  const KeyBits previous = collect(previous_keys, mapping);
  KeyBits changed;
  for (Count index = 0; index < word_count; index++) {
    changed[index] = current[index] ^ previous[index];
  }

  return Snapshot(current, changed, pointer);
}

auto Snapshot::is_current(Key key) const -> Bool {
  return contains(current, key);
}

auto Snapshot::is_pressed(Key key) const -> Bool {
  return contains(current, key) && contains(changed, key);
}

auto Snapshot::is_released(Key key) const -> Bool {
  return !contains(current, key) && contains(changed, key);
}
