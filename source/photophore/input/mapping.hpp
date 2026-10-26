// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "photophore/input/key.hpp"

namespace Photophore::Input {

// Mapping is the policy that assigns each physical Key its application Key.
// Several controls may share one destination, while None disables a source.
// Resolution performs one translation step so mappings cannot form cycles or
// make the meaning of a key depend on insertion order.
class Mapping {
 public:
  constexpr Mapping() { reset(); }

  constexpr auto remap(Key source, Key destination) -> Bool {
    if (!is_source(source) || !is_key(destination)) {
      return False;
    }

    destinations[Count(source)] = destination;
    return True;
  }

  constexpr auto unmap(Key source) -> Bool { return remap(source, Key::None); }

  constexpr auto reset(Key source) -> Bool {
    if (!is_source(source)) {
      return False;
    }

    destinations[Count(source)] = source;
    return True;
  }

  constexpr auto reset() -> void {
    for (Count index = 0; index < key_count; index++) {
      destinations[index] = Key(index);
    }
  }

  constexpr auto resolve(Key source) const -> Key {
    return is_source(source) ? destinations[Count(source)] : Key::None;
  }

 private:
  static constexpr auto is_key(Key key) -> Bool {
    return Count(key) < key_count;
  }

  static constexpr auto is_source(Key key) -> Bool {
    return key != Key::None && is_key(key);
  }

  Perimortem::Core::Static::Vector<Key, key_count> destinations;
};

}  // namespace Photophore::Input
