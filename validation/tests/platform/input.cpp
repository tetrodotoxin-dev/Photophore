// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/validation/test.hpp"

#include "photophore/platform/collector.hpp"

using namespace Photophore::Platform;
using namespace Toolchain::Validation;
using namespace Photophore::Input;

static Harness PlatformInput = {.name = "Photophore::Platform::Collector"};

VALIDATION_TEST(PlatformInput, frame_boundaries) {
  Collector input;
  Mapping mapping;
  input.set_key(Key::A, True);

  auto first = input.collect(mapping);
  EXPECT(first.is_pressed(Key::A));

  auto held = input.collect(mapping);
  EXPECT(held.is_current(Key::A));
  EXPECT(!held.is_pressed(Key::A));

  input.clear_keyboard();

  auto released = input.collect(mapping);
  EXPECT(released.is_released(Key::A));
  EXPECT(first.is_current(Key::A));
}

VALIDATION_TEST(PlatformInput, mapped_controls) {
  Collector input;
  Mapping mapping;
  mapping.remap(Key::A, Key::ArrowLeft);
  mapping.remap(Key::Q, Key::ArrowLeft);
  input.set_key(Key::A, True);
  input.set_key(Key::Q, True);
  EXPECT(input.collect(mapping).is_pressed(Key::ArrowLeft));

  input.set_key(Key::A, False);

  auto held = input.collect(mapping);
  EXPECT(held.is_current(Key::ArrowLeft));
  EXPECT(!held.is_released(Key::ArrowLeft));
}

VALIDATION_TEST(PlatformInput, pointer_focus) {
  Collector input;
  Mapping mapping;
  input.set_pointer(10, 20);
  input.set_key(Key::MousePrimary, True);

  auto entered = input.collect(mapping);
  EXPECT_EQ(entered.get_pointer().delta_x, R32(0));

  input.set_pointer(13, 18);
  input.scroll(1, -2);
  input.scroll(0, -3);

  auto moved = input.collect(mapping);
  EXPECT_EQ(moved.get_pointer().delta_x, R32(3));
  EXPECT_EQ(moved.get_pointer().delta_y, R32(-2));
  EXPECT_EQ(moved.get_pointer().scroll_y, R32(-5));
  EXPECT_EQ(input.collect(mapping).get_pointer().scroll_y, R32(0));

  input.clear_pointer();

  auto left = input.collect(mapping);
  EXPECT(left.is_released(Key::MousePrimary));
  EXPECT(!left.get_pointer().active);
  input.set_pointer(900, 800);
  EXPECT_EQ(input.collect(mapping).get_pointer().delta_x, R32(0));
}

#ifdef TOOLCHAIN_WINDOWS

VALIDATION_TEST(PlatformInput, physical_keys) {
  EXPECT(Collector::translate_keyboard(0x001E) == Key::A);
  EXPECT(Collector::translate_keyboard(0x001C) == Key::Enter);
  EXPECT(Collector::translate_keyboard(0xE01C) == Key::NumpadEnter);
  EXPECT(Collector::translate_keyboard(0xE01D) == Key::RightControl);
  EXPECT(Collector::translate_keyboard(0x0076) == Key::F24);
  EXPECT(Collector::translate_keyboard(0) == Key::None);
}

#endif
