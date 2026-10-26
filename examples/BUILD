# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "sources",
    srcs = glob(["**"]),
)

cc_library(
    name = "program",
    srcs = ["program.cpp"],
    hdrs = ["program.hpp"],
    deps = [
        "//:description",
        "//:frame",
        "@perimortem//:core",
    ],
)

cc_library(
    name = "program_static",
    srcs = ["program.cpp"],
    hdrs = ["program.hpp"],
    deps = ["//source:photophore_static"],
)

cc_binary(
    name = "demo",
    srcs = ["demo.cpp"],
    data = glob(["shaders/*.glsl"]),
    deps = [
        ":program",
        "//:vulkan",
        "@perimortem//:serialization",
    ],
)
