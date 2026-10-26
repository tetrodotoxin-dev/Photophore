// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/toolchain.h"

#ifdef TOOLCHAIN_LINUX

#include <linux/input-event-codes.h>

#include "toolchain/validation/test.hpp"

#include "photophore/platform/collector.hpp"

using namespace Toolchain::Validation;
using namespace Photophore::Input;
using namespace Photophore::Platform;

static Harness WaylandInput = {
  .name = "Platform::Wayland::Input",
};

VALIDATION_TEST(WaylandInput, key_translation) {
  EXPECT(Collector::translate_keyboard(KEY_A) == Key::A);
  EXPECT(Collector::translate_keyboard(KEY_F24) == Key::F24);
  EXPECT(Collector::translate_keyboard(KEY_RO) == Key::InternationalRo);
  EXPECT(Collector::translate_keyboard(KEY_RESERVED) == Key::None);
  EXPECT(Collector::translate_button(BTN_LEFT) == Key::MousePrimary);
  EXPECT(Collector::translate_button(BTN_BACK) == Key::MouseBack);
}

#endif
