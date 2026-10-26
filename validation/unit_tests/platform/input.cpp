// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/input.hpp"

#include "toolchain/validation/unit_test.hpp"

using namespace Photophore::Platform;
using namespace Toolchain::Validation;

static Harness PlatformInput = {.name = "Photophore::Platform::Input"};

VALIDATION_TEST(PlatformInput, frame_boundaries) {
  Input input;
  Photophore::Input::Mapping mapping;
  input.set_key(Input::Key::A, True);
  auto first = input.collect(mapping);
  EXPECT(first.is_pressed(Input::Key::A));
  auto held = input.collect(mapping);
  EXPECT(held.is_current(Input::Key::A));
  EXPECT(!held.is_pressed(Input::Key::A));

  input.clear_keyboard();
  auto released = input.collect(mapping);
  EXPECT(released.is_released(Input::Key::A));
  EXPECT(first.is_current(Input::Key::A));
}

VALIDATION_TEST(PlatformInput, mapped_controls) {
  Input input;
  Photophore::Input::Mapping mapping;
  mapping.remap(Input::Key::A, Input::Key::ArrowLeft);
  mapping.remap(Input::Key::Q, Input::Key::ArrowLeft);
  input.set_key(Input::Key::A, True);
  input.set_key(Input::Key::Q, True);
  EXPECT(input.collect(mapping).is_pressed(Input::Key::ArrowLeft));

  input.set_key(Input::Key::A, False);
  auto held = input.collect(mapping);
  EXPECT(held.is_current(Input::Key::ArrowLeft));
  EXPECT(!held.is_released(Input::Key::ArrowLeft));
}

VALIDATION_TEST(PlatformInput, pointer_focus) {
  Input input;
  Photophore::Input::Mapping mapping;
  input.set_pointer(10, 20);
  input.set_key(Input::Key::MousePrimary, True);
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
  EXPECT(left.is_released(Input::Key::MousePrimary));
  EXPECT(!left.get_pointer().active);
  input.set_pointer(900, 800);
  EXPECT_EQ(input.collect(mapping).get_pointer().delta_x, R32(0));
}

#ifdef PERI_WINDOWS
VALIDATION_TEST(PlatformInput, physical_keys) {
  EXPECT(Input::translate_keyboard(0x001E) == Input::Key::A);
  EXPECT(Input::translate_keyboard(0x001C) == Input::Key::Enter);
  EXPECT(Input::translate_keyboard(0xE01C) == Input::Key::NumpadEnter);
  EXPECT(Input::translate_keyboard(0xE01D) == Input::Key::RightControl);
  EXPECT(Input::translate_keyboard(0x0076) == Input::Key::F24);
  EXPECT(Input::translate_keyboard(0) == Input::Key::None);
}
#endif
