# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load(
    "@tetro_toolchain//:defs.bzl",
    "LINUX",
    "format",
    "package",
    "snapshot",
    "validation",
    "vscode",
)

# Photophore's component interfaces share one runtime. The component graph
# exposes the public header relationships while each realization remains in the
# selected library.
package(
    components = {
        "description": [],
        "frame": ["image"],
        "image": [],
        "input": [],
        "platform": ["input"],
        "scene": [
            "frame",
            "image",
        ],
        "vulkan": [
            "description",
            "frame",
            "image",
            "platform",
        ],
    },
    linkage = "shared",
    module = "photophore",
    shared = "//source:photophore_shared",
    static = "//source:photophore_static",
)

format()

snapshot()

validation(
    test_deps = ["//examples:program"],
    web_test_deps = ["//examples:program_static"],
    web_threads = False,
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
