# Copyright (c) 2023-present Matt Kaes and contributors
load("@tetro_toolchain//:library.bzl", "shared_library")

package(default_visibility = ["//visibility:public"])

# Vulkan and windowing belong to this engine. Pixel storage and codecs stay
# in Perimortem and carry no dependency on the rendering system.
shared_library(
    name = "photophore",
    deps = ["@perimortem", "@vulkan_headers//:vulkan_headers", "@wayland"],
)

exports_files(["source/platform/wayland/sdk.BUILD"])
