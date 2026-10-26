// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/view/bytes.hpp"

namespace Photophore::Vulkan {

// Dispatch is Photophore's process wide function table for the native Vulkan
// loader. The first Context acquisition opens the library and resolves every
// required function into one complete table under a lock. Each live Context
// holds one reservation, and the final release clears the table before closing
// the library. Recording reads the immutable table after acquisition.
//
// The pointers come from loader exports and serve every Context, including
// windows backed by different devices. Presentation providers receive
// `vkGetInstanceProcAddr` from this table and create their own platform
// surface.
class Dispatch {
 public:
  static auto acquire() -> Bool;
  static auto release() -> void;
  static auto get() -> const Dispatch&;

  PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = nullptr;
  PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = nullptr;
  PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets = nullptr;
  PFN_vkAllocateMemory vkAllocateMemory = nullptr;
  PFN_vkBeginCommandBuffer vkBeginCommandBuffer = nullptr;
  PFN_vkBindBufferMemory vkBindBufferMemory = nullptr;
  PFN_vkBindImageMemory vkBindImageMemory = nullptr;
  PFN_vkCmdBeginRendering vkCmdBeginRendering = nullptr;
  PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets = nullptr;
  PFN_vkCmdBindPipeline vkCmdBindPipeline = nullptr;
  PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers = nullptr;
  PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage = nullptr;
  PFN_vkCmdDraw vkCmdDraw = nullptr;
  PFN_vkCmdEndRendering vkCmdEndRendering = nullptr;
  PFN_vkCmdPipelineBarrier2 vkCmdPipelineBarrier2 = nullptr;
  PFN_vkCmdPushConstants vkCmdPushConstants = nullptr;
  PFN_vkCmdSetScissor vkCmdSetScissor = nullptr;
  PFN_vkCmdSetViewport vkCmdSetViewport = nullptr;
  PFN_vkCreateBuffer vkCreateBuffer = nullptr;
  PFN_vkCreateCommandPool vkCreateCommandPool = nullptr;
  PFN_vkCreateDescriptorPool vkCreateDescriptorPool = nullptr;
  PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout = nullptr;
  PFN_vkCreateDevice vkCreateDevice = nullptr;
  PFN_vkCreateFence vkCreateFence = nullptr;
  PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines = nullptr;
  PFN_vkCreateImage vkCreateImage = nullptr;
  PFN_vkCreateImageView vkCreateImageView = nullptr;
  PFN_vkCreateInstance vkCreateInstance = nullptr;
  PFN_vkCreatePipelineLayout vkCreatePipelineLayout = nullptr;
  PFN_vkCreateSampler vkCreateSampler = nullptr;
  PFN_vkCreateSemaphore vkCreateSemaphore = nullptr;
  PFN_vkCreateShaderModule vkCreateShaderModule = nullptr;
  PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
  PFN_vkDestroyBuffer vkDestroyBuffer = nullptr;
  PFN_vkDestroyCommandPool vkDestroyCommandPool = nullptr;
  PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool = nullptr;
  PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout = nullptr;
  PFN_vkDestroyDevice vkDestroyDevice = nullptr;
  PFN_vkDestroyFence vkDestroyFence = nullptr;
  PFN_vkDestroyImage vkDestroyImage = nullptr;
  PFN_vkDestroyImageView vkDestroyImageView = nullptr;
  PFN_vkDestroyInstance vkDestroyInstance = nullptr;
  PFN_vkDestroyPipeline vkDestroyPipeline = nullptr;
  PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = nullptr;
  PFN_vkDestroySampler vkDestroySampler = nullptr;
  PFN_vkDestroySemaphore vkDestroySemaphore = nullptr;
  PFN_vkDestroyShaderModule vkDestroyShaderModule = nullptr;
  PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
  PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = nullptr;
  PFN_vkDeviceWaitIdle vkDeviceWaitIdle = nullptr;
  PFN_vkEndCommandBuffer vkEndCommandBuffer = nullptr;
  PFN_vkEnumerateDeviceExtensionProperties
      vkEnumerateDeviceExtensionProperties = nullptr;
  PFN_vkEnumerateInstanceExtensionProperties
      vkEnumerateInstanceExtensionProperties = nullptr;
  PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties =
      nullptr;
  PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = nullptr;
  PFN_vkFreeCommandBuffers vkFreeCommandBuffers = nullptr;
  PFN_vkFreeMemory vkFreeMemory = nullptr;
  PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements = nullptr;
  PFN_vkGetDeviceQueue vkGetDeviceQueue = nullptr;
  PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = nullptr;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
  PFN_vkGetFenceStatus vkGetFenceStatus = nullptr;
  PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures = nullptr;
  PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2 = nullptr;
  PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties =
      nullptr;
  PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = nullptr;
  PFN_vkGetPhysicalDeviceQueueFamilyProperties
      vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
  PFN_vkGetPhysicalDeviceSurfaceFormatsKHR
      vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
      vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
  PFN_vkGetPhysicalDeviceSurfaceSupportKHR
      vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
  PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
  PFN_vkMapMemory vkMapMemory = nullptr;
  PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;
  PFN_vkQueueSubmit vkQueueSubmit = nullptr;
  PFN_vkQueueWaitIdle vkQueueWaitIdle = nullptr;
  PFN_vkResetCommandBuffer vkResetCommandBuffer = nullptr;
  PFN_vkResetFences vkResetFences = nullptr;
  PFN_vkUnmapMemory vkUnmapMemory = nullptr;
  PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets = nullptr;
  PFN_vkWaitForFences vkWaitForFences = nullptr;

 private:
  static auto load(Dispatch& available) -> Bool;
  static auto symbol(Perimortem::Core::View::Bytes name) -> void*;
  static Dispatch functions;
};

}  // namespace Photophore::Vulkan
