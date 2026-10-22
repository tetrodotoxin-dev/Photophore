# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@tetro_toolchain//:defs.bzl", "LINUX", "benchmarks", "package", "tests", "vscode")

# Vulkan and windowing belong to this engine. Pixel storage and codecs stay in
# Perimortem and carry no dependency on the rendering system.
package(
    components = [
        "description",
        "frame",
        "image",
        "platform",
        "scene",
        "vulkan",
    ],
    module = "photophore",
    shared = "//source:photophore_shared",
    static = "//source:photophore",
)

tests(
    srcs = glob(["validation/unit_tests/**/*.cpp"]),
    deps = [
        ":scene",
        ":vulkan",
    ],
)

benchmarks(
    srcs = glob(["validation/benchmarks/**/*.cpp"]),
    deps = [":frame"],
)

# This test runs a full graphical test suite and requires a working Vulkan
# driver and presentation environment. Linux presentation uses Wayland and
# inherits its runtime directory and display socket only for that platform.
#
# Run this test on the host before committing. Every supported host must pass
# before merging into main.
cc_test(
    name = "presentation",
    size = "small",
    srcs = ["validation/integration_tests/presentation.cpp"],
    env_inherit = select({
        LINUX: [
            "XDG_RUNTIME_DIR",
            "WAYLAND_DISPLAY",
        ],
        "//conditions:default": [],
    }),
    tags = [
        "local",
        "manual",
    ],
    deps = [":vulkan"],
)

alias(
    name = "demo",
    actual = "//examples:demo",
)

vscode()
