// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/memory/allocator/global.hpp"

namespace Photophore::Platform {

// State gives every native Window provider the same process allocation and
// construction contract. Supported provider types fit Global's guaranteed
// alignment. A failed allocation returns null without invoking the factory.
// Destroy completes the matching object lifetime before releasing its bytes.
class State {
 public:
  template <typename Type, typename Factory>
  static auto create(Factory factory) -> Type* {
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
  static auto destroy(Type* value) -> void {
    if (!value) {
      return;
    }

    value->~Type();
    Perimortem::Memory::Allocator::Global::release(
        Perimortem::Core::Data::cast<U8>(value));
  }
};

}  // namespace Photophore::Platform
