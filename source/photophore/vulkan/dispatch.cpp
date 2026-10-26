// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "implementation/vulkan/dispatch.hpp"

#ifndef TOOLCHAIN_WASM
#include "toolchain/memory/lifetime.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/synchronization/mutex.hpp"
#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/allocator/global.hpp"
#include "perimortem/system/process/library.hpp"
#endif

using namespace Perimortem::Core;
using namespace Photophore::Vulkan;

#define PHOTOPHORE_VULKAN(name) PFN_##name Dispatch::name = nullptr;

#include "implementation/vulkan/functions.hpp"
#undef PHOTOPHORE_VULKAN

#ifndef TOOLCHAIN_WASM

using namespace Toolchain::Memory;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

static Synchronization::Mutex loader_mutex;
static Process::Library* loader = nullptr;
static Count contexts = 0;

static auto release_loader() -> void {
  if (!loader) {
    return;
  }

  loader->~Library();
  Allocator::Global::release(Data::cast<U8>(loader));
  loader = nullptr;
}

#endif

// `symbol` runs while `acquire` holds loader_mutex and the Vulkan module is
// loaded. A failed lookup returns null, causing the complete function inventory
// to be rejected before Context creation continues.
auto Dispatch::symbol(View::Bytes name) -> void* {

#ifdef TOOLCHAIN_WASM

  return nullptr;

#else

  Allocator::Arena errors;
  return loader->symbol(name, errors)
      .visit(
          [](void* address) { return address; },
          [](View::Bytes) -> void* { return nullptr; });

#endif
}

auto Dispatch::acquire() -> Bool {

#ifdef TOOLCHAIN_WASM

  return False;

#else

  // The first Context publishes the complete function table while holding the
  // loader mutex. Later Context values retain that immutable table by count and
  // call its entries directly.

  Synchronization::Mutex::Lock lock(loader_mutex);
  if (contexts) {
    ++contexts;
    return True;
  }

  Allocator::Arena errors;

#ifdef TOOLCHAIN_WINDOWS

  constexpr auto filename = "vulkan-1.dll"_view;

#else

  constexpr auto filename = "libvulkan.so.1"_view;

#endif

  loader = Process::Library::open(filename, errors)
               .visit(
                   [](Process::Library& library) {
                     auto allocation =
                         Allocator::Global::allocate(sizeof(Process::Library));
                     if (allocation.get_capacity() < sizeof(Process::Library)) {
                       Process::Library* unavailable = nullptr;
                       return unavailable;
                     }

                     return &Lifetime::construct<Process::Library>(
                         allocation.get_data(),
                         [&library] { return Data::take(library); });
                   },

                   [](View::Bytes) -> Process::Library* { return nullptr; });

  if (!loader) {
    return False;
  }

  bool complete = true;

#define PHOTOPHORE_VULKAN(name)                                           \
  name = Data::bit_cast<PFN_##name>(                                      \
      symbol(View::NullTerminated(#name, sizeof(#name) - 1).get_view())); \
  complete = complete && name;

#include "implementation/vulkan/functions.hpp"
#undef PHOTOPHORE_VULKAN
  if (!complete) {
    release_loader();
    return False;
  }

  contexts = 1;
  return True;

#endif
}

auto Dispatch::release() -> void {

#ifndef TOOLCHAIN_WASM

  Synchronization::Mutex::Lock lock(loader_mutex);
  if (--contexts) {
    return;
  }

  // The final Context arrives here after its native resources have unwound.
  // Clearing the table with the module lets a future acquisition publish one
  // fresh and internally consistent set of addresses.

  release_loader();

#define PHOTOPHORE_VULKAN(name) name = nullptr;

#include "implementation/vulkan/functions.hpp"
#undef PHOTOPHORE_VULKAN
#endif
}
