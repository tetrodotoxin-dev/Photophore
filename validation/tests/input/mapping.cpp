// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/input/mapping.hpp"

#include "toolchain/validation/test.hpp"

#include "perimortem/core/view/vector.hpp"

#include "photophore/input/snapshot.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Input;
using namespace Toolchain::Validation;

static Harness InputMapping = {
  .name = "Photophore::Input::Mapping",
};

VALIDATION_TEST(InputMapping, operations) {
  Mapping mapping;
  EXPECT(mapping.resolve(Key::A) == Key::A);
  EXPECT_NOT(mapping.remap(Key::None, Key::B));
  EXPECT_NOT(mapping.remap(Key(0xFF), Key::B));
  EXPECT_NOT(mapping.remap(Key::A, Key(0xFF)));

  EXPECT(mapping.remap(Key::A, Key::B));
  EXPECT(mapping.remap(Key::B, Key::C));
  EXPECT(mapping.resolve(Key::A) == Key::B);
  EXPECT(mapping.resolve(Key::B) == Key::C);

  EXPECT(mapping.unmap(Key::A));
  EXPECT(mapping.resolve(Key::A) == Key::None);
  EXPECT(mapping.reset(Key::A));
  EXPECT(mapping.resolve(Key::A) == Key::A);

  mapping.reset();
  EXPECT(mapping.resolve(Key::B) == Key::B);
  EXPECT(mapping.resolve(Key::None) == Key::None);
  EXPECT(mapping.resolve(Key(0xFF)) == Key::None);
}

// Left and right modifiers publish one common application key. Switching sides
// keeps that grouped key held rather than creating a release and press pair.
VALIDATION_TEST(InputMapping, modifier_groups) {
  const Key previous[] = {Key::LeftShift, Key::LeftControl};
  const Key current[] = {Key::RightShift, Key::RightControl};
  const Snapshot snapshot = Snapshot::create(
      View::Vector<Key>(current), View::Vector<Key>(previous), Mapping());
  EXPECT(snapshot.is_current(Key::Shift));
  EXPECT(snapshot.is_current(Key::Control));
  EXPECT_NOT(snapshot.is_pressed(Key::Shift));
  EXPECT_NOT(snapshot.is_released(Key::Shift));
  EXPECT(snapshot.is_released(Key::LeftShift));
  EXPECT(snapshot.is_pressed(Key::RightShift));
}
