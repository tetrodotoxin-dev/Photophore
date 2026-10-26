// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/collector.hpp"

#ifdef TOOLCHAIN_WASM

using namespace Photophore::Platform;

auto Collector::translate_keyboard(U32) -> Key {
  return Key::None;
}

auto Collector::translate_button(U32) -> Key {
  return Key::None;
}

#endif
