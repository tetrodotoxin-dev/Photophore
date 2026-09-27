# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@tetro_toolchain//:validation.bzl", "test")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])
filegroup(name = "sources", srcs = glob(["**"]))

test(
    name = "test",
    size = "small",
    testonly = True,
    srcs = glob(["**/*.cpp"], exclude = ["presentation.cpp"]),
    deps = [
        "//:photophore",
    ],
)

# A real compositor and Vulkan device are required. This is an explicit
# integration gate, while the unit suite remains usable on a headless host.
cc_test(
    name = "presentation",
    size = "small",
    srcs = ["presentation.cpp"],
    env_inherit = [
        "XDG_RUNTIME_DIR",
        "WAYLAND_DISPLAY",
    ],
    tags = [
        "local",
        "manual",
    ],
    deps = ["//:photophore"],
)
