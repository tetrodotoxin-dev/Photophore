// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/vulkan.hpp"

#ifndef PERI_WASM
#include <mutex>

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/library.hpp"
#endif

using namespace Perimortem::Core;
using namespace Photophore;

#define PHOTOPHORE_VULKAN(name) PFN_##name Vulkan::name = nullptr;
#include "photophore/vulkan_functions.hpp"
#undef PHOTOPHORE_VULKAN

#ifndef PERI_WASM
static std::mutex loader_mutex;
static Perimortem::System::Library* loader = nullptr;
static Count contexts = 0;
#endif

auto Vulkan::symbol(View::Bytes name) -> void* {
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

auto Vulkan::acquire() -> Bool {
#ifdef PERI_WASM
  return False;
#else
  // Publication happens while opening the first context. Established contexts
  // keep this table immutable and use its function pointers without locking.
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
#define PHOTOPHORE_VULKAN(name)                                                \
  name = reinterpret_cast<PFN_##name>(symbol(NullTerminated::to_view(#name))); \
  complete = complete && name;
#include "photophore/vulkan_functions.hpp"
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

auto Vulkan::release() -> void {
#ifndef PERI_WASM
  std::lock_guard guard(loader_mutex);
  if (--contexts) {
    return;
  }
  // Resource destruction precedes this release. With no remaining context,
  // the module can close and a subsequent acquisition may load fresh addresses.
  delete loader;
  loader = nullptr;
#define PHOTOPHORE_VULKAN(name) name = nullptr;
#include "photophore/vulkan_functions.hpp"
#undef PHOTOPHORE_VULKAN
#endif
}
