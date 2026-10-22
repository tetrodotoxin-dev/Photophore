# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "sources",
    srcs = glob(["**"]),
)

cc_binary(
    name = "demo",
    srcs = [
        "demo.cpp",
        "program.cpp",
        "program.hpp",
    ],
    data = glob(["shaders/*.glsl"]),
    deps = ["//:vulkan"],
)
