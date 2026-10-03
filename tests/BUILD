# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@tetro_toolchain//:defs.bzl", "tests")

package(default_visibility = ["//visibility:public"])
filegroup(name = "sources", srcs = glob(["**"]))

tests(
    name = "tests",
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
