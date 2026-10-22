// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "implementation/vulkan/dispatch.hpp"

#ifndef PERI_WASM
#include <mutex>

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/library.hpp"
#endif

using namespace Perimortem::Core;

#define PHOTOPHORE_VULKAN(name) PFN_##name Photophore::Vulkan::name = nullptr;
#include "implementation/vulkan/functions.hpp"
#undef PHOTOPHORE_VULKAN

#ifndef PERI_WASM
static std::mutex loader_mutex;
static Perimortem::System::Library* loader = nullptr;
static Count contexts = 0;
#endif

// symbol is used only while acquire holds loader_mutex and the Vulkan module is
// live. Failed lookup becomes a null entry in the all symbols completeness
// test.
auto Photophore::Vulkan::symbol(View::Bytes name) -> void* {
#ifdef PERI_WASM
  return nullptr;
#else
  Perimortem::Memory::Allocator::Arena errors;
  return loader->symbol(name, errors)
      .visit(
          [](void* address) { return address; },
          [](View::Bytes) -> void* { return nullptr; });
#endif
}

auto Photophore::Vulkan::acquire() -> Bool {
#ifdef PERI_WASM
  return False;
#else
  // The first Context publishes the complete function table while holding the
  // loader mutex. Later Context values retain that immutable table by count and
  // call its entries directly.
  std::lock_guard guard(loader_mutex);
  if (contexts) {
    ++contexts;
    return True;
  }
  Perimortem::Memory::Allocator::Arena errors;
#ifdef PERI_WINDOWS
  constexpr auto filename = "vulkan-1.dll"_view;
#else
  constexpr auto filename = "libvulkan.so.1"_view;
#endif
  loader =
      Perimortem::System::Library::open(filename, errors)
          .visit(
              [](Perimortem::System::Library& library) {
                return new Perimortem::System::Library(Data::take(library));
              },
              [](View::Bytes) -> Perimortem::System::Library* {
                return nullptr;
              });
  if (!loader) {
    return False;
  }
  bool complete = true;
#define PHOTOPHORE_VULKAN(name)                                   \
  name = reinterpret_cast<PFN_##name>(                            \
      symbol(NullTerminated::to_view(#name, sizeof(#name) - 1))); \
  complete = complete && name;
#include "implementation/vulkan/functions.hpp"
#undef PHOTOPHORE_VULKAN
  if (!complete) {
    delete loader;
    loader = nullptr;
    return False;
  }
  contexts = 1;
  return True;
#endif
}

auto Photophore::Vulkan::release() -> void {
#ifndef PERI_WASM
  std::lock_guard guard(loader_mutex);
  if (--contexts) {
    return;
  }
  // The final Context arrives here after its native resources have unwound.
  // Clearing the table with the module lets a future acquisition publish one
  // fresh and internally consistent set of addresses.
  delete loader;
  loader = nullptr;
#define PHOTOPHORE_VULKAN(name) name = nullptr;
#include "implementation/vulkan/functions.hpp"
#undef PHOTOPHORE_VULKAN
#endif
}
