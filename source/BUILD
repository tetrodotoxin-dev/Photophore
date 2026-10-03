# Copyright (c) 2023-present Matt Kaes and contributors

load("@tetro_toolchain//:defs.bzl", "WINDOWS", "library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "sources",
    srcs = glob(["**"]),
)

exports_files(["photophore/platform/wayland/sdk.BUILD"])

_LIBRARY = dict(
    srcs = glob(["photophore/**/*.cpp"]),
    hdrs = glob(
        [
            "photophore/**/*.h",
            "photophore/**/*.hpp",
        ],
        allow_empty = True,
    ),
    includes = ["."],
    deps = [
        "@perimortem//:core",
        "@perimortem//:math",
        "@perimortem//:memory",
        "@perimortem//:serialization",
        "@perimortem//:system",
        "@vulkan_headers//:vulkan_headers",
    ],
    implementation_deps = ["@wayland"],
)

library(
    srcs = _LIBRARY["srcs"],
    hdrs = _LIBRARY["hdrs"],
    includes = _LIBRARY["includes"],
    deps = _LIBRARY["deps"],
    implementation_deps = _LIBRARY["implementation_deps"],
)

library(
    name = "photophore_shared",
    srcs = _LIBRARY["srcs"],
    hdrs = _LIBRARY["hdrs"],
    includes = _LIBRARY["includes"],
    deps = _LIBRARY["deps"],
    implementation_deps = _LIBRARY["implementation_deps"],
    linkage = "shared",
    shared_lib_name = select({
        WINDOWS: "photophore.dll",
        "//conditions:default": "libphotophore.so",
    }),
)
