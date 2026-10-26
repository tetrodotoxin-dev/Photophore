// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/vulkan/dispatch.hpp"

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

Dispatch Dispatch::functions;

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

auto Dispatch::get() -> const Dispatch& {
  return functions;
}

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

  Dispatch available;
  auto load = []<typename Function>(
                  Function& function, View::Bytes name) -> Bool {
    function = Data::bit_cast<Function>(Dispatch::symbol(name));
    return function != nullptr;
  };

  Bool complete = True;
  complete =
      load(available.vkAcquireNextImageKHR, "vkAcquireNextImageKHR"_view) &&
      complete;
  complete = load(
                 available.vkAllocateCommandBuffers,
                 "vkAllocateCommandBuffers"_view) &&
             complete;
  complete = load(
                 available.vkAllocateDescriptorSets,
                 "vkAllocateDescriptorSets"_view) &&
             complete;
  complete =
      load(available.vkAllocateMemory, "vkAllocateMemory"_view) && complete;
  complete =
      load(available.vkBeginCommandBuffer, "vkBeginCommandBuffer"_view) &&
      complete;
  complete =
      load(available.vkBindBufferMemory, "vkBindBufferMemory"_view) && complete;
  complete =
      load(available.vkBindImageMemory, "vkBindImageMemory"_view) && complete;
  complete = load(available.vkCmdBeginRendering, "vkCmdBeginRendering"_view) &&
             complete;
  complete =
      load(available.vkCmdBindDescriptorSets, "vkCmdBindDescriptorSets"_view) &&
      complete;
  complete =
      load(available.vkCmdBindPipeline, "vkCmdBindPipeline"_view) && complete;
  complete =
      load(available.vkCmdBindVertexBuffers, "vkCmdBindVertexBuffers"_view) &&
      complete;
  complete =
      load(available.vkCmdCopyBufferToImage, "vkCmdCopyBufferToImage"_view) &&
      complete;
  complete = load(available.vkCmdDraw, "vkCmdDraw"_view) && complete;
  complete =
      load(available.vkCmdEndRendering, "vkCmdEndRendering"_view) && complete;
  complete =
      load(available.vkCmdPipelineBarrier2, "vkCmdPipelineBarrier2"_view) &&
      complete;
  complete =
      load(available.vkCmdPushConstants, "vkCmdPushConstants"_view) && complete;
  complete =
      load(available.vkCmdSetScissor, "vkCmdSetScissor"_view) && complete;
  complete =
      load(available.vkCmdSetViewport, "vkCmdSetViewport"_view) && complete;
  complete = load(available.vkCreateBuffer, "vkCreateBuffer"_view) && complete;
  complete = load(available.vkCreateCommandPool, "vkCreateCommandPool"_view) &&
             complete;
  complete =
      load(available.vkCreateDescriptorPool, "vkCreateDescriptorPool"_view) &&
      complete;
  complete = load(
                 available.vkCreateDescriptorSetLayout,
                 "vkCreateDescriptorSetLayout"_view) &&
             complete;
  complete = load(available.vkCreateDevice, "vkCreateDevice"_view) && complete;
  complete = load(available.vkCreateFence, "vkCreateFence"_view) && complete;
  complete = load(
                 available.vkCreateGraphicsPipelines,
                 "vkCreateGraphicsPipelines"_view) &&
             complete;
  complete = load(available.vkCreateImage, "vkCreateImage"_view) && complete;
  complete =
      load(available.vkCreateImageView, "vkCreateImageView"_view) && complete;
  complete =
      load(available.vkCreateInstance, "vkCreateInstance"_view) && complete;
  complete =
      load(available.vkCreatePipelineLayout, "vkCreatePipelineLayout"_view) &&
      complete;
  complete =
      load(available.vkCreateSampler, "vkCreateSampler"_view) && complete;
  complete =
      load(available.vkCreateSemaphore, "vkCreateSemaphore"_view) && complete;
  complete =
      load(available.vkCreateShaderModule, "vkCreateShaderModule"_view) &&
      complete;
  complete =
      load(available.vkCreateSwapchainKHR, "vkCreateSwapchainKHR"_view) &&
      complete;
  complete =
      load(available.vkDestroyBuffer, "vkDestroyBuffer"_view) && complete;
  complete =
      load(available.vkDestroyCommandPool, "vkDestroyCommandPool"_view) &&
      complete;
  complete =
      load(available.vkDestroyDescriptorPool, "vkDestroyDescriptorPool"_view) &&
      complete;
  complete = load(
                 available.vkDestroyDescriptorSetLayout,
                 "vkDestroyDescriptorSetLayout"_view) &&
             complete;
  complete =
      load(available.vkDestroyDevice, "vkDestroyDevice"_view) && complete;
  complete = load(available.vkDestroyFence, "vkDestroyFence"_view) && complete;
  complete = load(available.vkDestroyImage, "vkDestroyImage"_view) && complete;
  complete =
      load(available.vkDestroyImageView, "vkDestroyImageView"_view) && complete;
  complete =
      load(available.vkDestroyInstance, "vkDestroyInstance"_view) && complete;
  complete =
      load(available.vkDestroyPipeline, "vkDestroyPipeline"_view) && complete;
  complete =
      load(available.vkDestroyPipelineLayout, "vkDestroyPipelineLayout"_view) &&
      complete;
  complete =
      load(available.vkDestroySampler, "vkDestroySampler"_view) && complete;
  complete =
      load(available.vkDestroySemaphore, "vkDestroySemaphore"_view) && complete;
  complete =
      load(available.vkDestroyShaderModule, "vkDestroyShaderModule"_view) &&
      complete;
  complete = load(available.vkDestroySurfaceKHR, "vkDestroySurfaceKHR"_view) &&
             complete;
  complete =
      load(available.vkDestroySwapchainKHR, "vkDestroySwapchainKHR"_view) &&
      complete;
  complete =
      load(available.vkDeviceWaitIdle, "vkDeviceWaitIdle"_view) && complete;
  complete =
      load(available.vkEndCommandBuffer, "vkEndCommandBuffer"_view) && complete;
  complete = load(
                 available.vkEnumerateDeviceExtensionProperties,
                 "vkEnumerateDeviceExtensionProperties"_view) &&
             complete;
  complete = load(
                 available.vkEnumerateInstanceExtensionProperties,
                 "vkEnumerateInstanceExtensionProperties"_view) &&
             complete;
  complete = load(
                 available.vkEnumerateInstanceLayerProperties,
                 "vkEnumerateInstanceLayerProperties"_view) &&
             complete;
  complete = load(
                 available.vkEnumeratePhysicalDevices,
                 "vkEnumeratePhysicalDevices"_view) &&
             complete;
  complete =
      load(available.vkFreeCommandBuffers, "vkFreeCommandBuffers"_view) &&
      complete;
  complete = load(available.vkFreeMemory, "vkFreeMemory"_view) && complete;
  complete = load(
                 available.vkGetBufferMemoryRequirements,
                 "vkGetBufferMemoryRequirements"_view) &&
             complete;
  complete =
      load(available.vkGetDeviceQueue, "vkGetDeviceQueue"_view) && complete;
  complete = load(
                 available.vkGetImageMemoryRequirements,
                 "vkGetImageMemoryRequirements"_view) &&
             complete;
  complete =
      load(available.vkGetInstanceProcAddr, "vkGetInstanceProcAddr"_view) &&
      complete;
  complete =
      load(available.vkGetFenceStatus, "vkGetFenceStatus"_view) && complete;
  complete = load(
                 available.vkGetPhysicalDeviceFeatures,
                 "vkGetPhysicalDeviceFeatures"_view) &&
             complete;
  complete = load(
                 available.vkGetPhysicalDeviceFeatures2,
                 "vkGetPhysicalDeviceFeatures2"_view) &&
             complete;
  complete = load(
                 available.vkGetPhysicalDeviceMemoryProperties,
                 "vkGetPhysicalDeviceMemoryProperties"_view) &&
             complete;
  complete = load(
                 available.vkGetPhysicalDeviceProperties,
                 "vkGetPhysicalDeviceProperties"_view) &&
             complete;
  complete = load(
                 available.vkGetPhysicalDeviceQueueFamilyProperties,
                 "vkGetPhysicalDeviceQueueFamilyProperties"_view) &&
             complete;
  complete = load(
                 available.vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
                 "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"_view) &&
             complete;
  complete = load(
                 available.vkGetPhysicalDeviceSurfaceFormatsKHR,
                 "vkGetPhysicalDeviceSurfaceFormatsKHR"_view) &&
             complete;
  complete = load(
                 available.vkGetPhysicalDeviceSurfacePresentModesKHR,
                 "vkGetPhysicalDeviceSurfacePresentModesKHR"_view) &&
             complete;
  complete = load(
                 available.vkGetPhysicalDeviceSurfaceSupportKHR,
                 "vkGetPhysicalDeviceSurfaceSupportKHR"_view) &&
             complete;
  complete =
      load(available.vkGetSwapchainImagesKHR, "vkGetSwapchainImagesKHR"_view) &&
      complete;
  complete = load(available.vkMapMemory, "vkMapMemory"_view) && complete;
  complete =
      load(available.vkQueuePresentKHR, "vkQueuePresentKHR"_view) && complete;
  complete = load(available.vkQueueSubmit, "vkQueueSubmit"_view) && complete;
  complete =
      load(available.vkQueueWaitIdle, "vkQueueWaitIdle"_view) && complete;
  complete =
      load(available.vkResetCommandBuffer, "vkResetCommandBuffer"_view) &&
      complete;
  complete = load(available.vkResetFences, "vkResetFences"_view) && complete;
  complete = load(available.vkUnmapMemory, "vkUnmapMemory"_view) && complete;
  complete =
      load(available.vkUpdateDescriptorSets, "vkUpdateDescriptorSets"_view) &&
      complete;
  complete =
      load(available.vkWaitForFences, "vkWaitForFences"_view) && complete;
  if (!complete) {
    release_loader();
    return False;
  }

  functions = available;
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
  functions = Dispatch();

#endif
}
