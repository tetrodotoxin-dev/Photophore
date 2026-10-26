// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/storage/vector.hpp"
#include "perimortem/core/view/vector.hpp"

#include "photophore/input/key.hpp"
#include "photophore/input/mapping.hpp"
#include "photophore/input/pointer.hpp"

namespace Photophore::Input {

// Snapshot is one immutable observation of mapped controls and pointer state.
// Platform collection applies one Mapping to both the current and previous
// physical keys before publication. A mapping change therefore preserves the
// transition history of a held key under its newly selected application name.
// Copies retain the complete observation without platform or window state.
class Snapshot {
 public:
  constexpr Snapshot() = default;

  static EXPORTED(PHOTOPHORE) auto create(
      Perimortem::Core::View::Vector<Key> current,
      Perimortem::Core::View::Vector<Key> previous,
      const Mapping& mapping,
      Pointer pointer = Pointer()) -> Snapshot;

  EXPORTED(PHOTOPHORE) auto is_current(Key key) const -> Bool;
  EXPORTED(PHOTOPHORE) auto is_pressed(Key key) const -> Bool;
  EXPORTED(PHOTOPHORE) auto is_released(Key key) const -> Bool;

  constexpr auto get_current_word(Count index) const -> U64 {
    return index < word_count ? current[index] : U64(0);
  }

  constexpr auto get_changed_word(Count index) const -> U64 {
    return index < word_count ? changed[index] : U64(0);
  }

  constexpr auto get_pointer() const -> const Pointer& { return pointer; }

 private:
  static constexpr Count word_bits = sizeof(U64) * 8;
  static constexpr Count word_count = (key_count + word_bits - 1) / word_bits;
  using KeyBits = Perimortem::Core::Storage::Vector<U64, word_count>;

  constexpr Snapshot(KeyBits current, KeyBits changed, Pointer pointer)
      : current(current), changed(changed), pointer(pointer) {}

  static auto collect(
      Perimortem::Core::View::Vector<Key> keys,
      const Mapping& mapping) -> KeyBits;
  static auto set(KeyBits& keys, Key key) -> void;
  static auto contains(const KeyBits& keys, Key key) -> Bool;
  static auto add_modifier_groups(KeyBits& keys) -> void;

  KeyBits current;
  KeyBits changed;
  Pointer pointer;
};

static_assert(sizeof(Snapshot) <= 80);

}  // namespace Photophore::Input
