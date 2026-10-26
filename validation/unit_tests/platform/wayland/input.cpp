// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/perimortem.h"

#ifdef PERI_LINUX

#include <linux/input-event-codes.h>

#include "photophore/platform/input.hpp"
#include "toolchain/validation/unit_test.hpp"

using namespace Toolchain::Validation;

static Harness WaylandInput = {
  .name = "Platform::Wayland::Input",
};

VALIDATION_TEST(WaylandInput, key_translation) {
  EXPECT(
      Photophore::Platform::Input::translate_keyboard(KEY_A) ==
      Photophore::Input::Key::A);
  EXPECT(
      Photophore::Platform::Input::translate_keyboard(KEY_F24) ==
      Photophore::Input::Key::F24);
  EXPECT(
      Photophore::Platform::Input::translate_keyboard(KEY_RO) ==
      Photophore::Input::Key::InternationalRo);
  EXPECT(
      Photophore::Platform::Input::translate_keyboard(KEY_RESERVED) ==
      Photophore::Input::Key::None);
  EXPECT(
      Photophore::Platform::Input::translate_button(BTN_LEFT) ==
      Photophore::Input::Key::MousePrimary);
  EXPECT(
      Photophore::Platform::Input::translate_button(BTN_BACK) ==
      Photophore::Input::Key::MouseBack);
}

#endif
