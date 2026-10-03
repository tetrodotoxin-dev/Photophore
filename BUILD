# Copyright (c) 2023-present Matt Kaes and contributors
load("@tetro_toolchain//:defs.bzl", "package", "vscode")

# Vulkan and windowing belong to this engine. Pixel storage and codecs stay in
# Perimortem and carry no dependency on the rendering system.
package(
    module = "photophore",
    shared = "//source:photophore_shared",
    static = "//source:photophore",
)

alias(
    name = "tests",
    actual = "//tests:tests",
)

alias(
    name = "benchmarks",
    actual = "//benchmarks:benchmarks",
)

alias(
    name = "demo",
    actual = "//examples:demo",
)

vscode()
