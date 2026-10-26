// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/memory/allocator/global.hpp"

namespace Photophore::Platform {

// Platform providers allocate native state through Perimortem's process
// allocator so every host follows the same construction and release contract.
// The allocation has enough alignment for any supported provider state. A
// failed allocation returns null without invoking the factory.
template <typename Type, typename Factory>
auto create_state(Factory factory) -> Type* {
  static_assert(
      alignof(Type) <=
      Perimortem::Memory::Allocator::Global::allocation_alignment);
  auto allocation =
      Perimortem::Memory::Allocator::Global::allocate(sizeof(Type));
  if (allocation.get_capacity() < sizeof(Type)) {
    return nullptr;
  }

  return &Toolchain::Memory::Lifetime::construct<Type>(
      allocation.get_data(), factory);
}

template <typename Type>
auto destroy_state(Type* value) -> void {
  if (!value) {
    return;
  }

  value->~Type();
  Perimortem::Memory::Allocator::Global::release(
      Perimortem::Core::Data::cast<U8>(value));
}

}  // namespace Photophore::Platform
