# Copyright (c) 2023-present Matt Kaes and contributors

load("@tetro_toolchain//:defs.bzl", "WINDOWS", "library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "sources",
    srcs = glob(["**"]),
)

_LIBRARY = dict(
    srcs = glob(["photophore/**/*.cpp"]),
    hdrs = glob(
        [
            "photophore/**/*.h",
            "photophore/**/*.hpp",
        ],
        allow_empty = True,
    ),
    implementation_deps = ["//source/implementation:headers"],
    includes = ["."],
    deps = [
        "@perimortem//:core",
        "@perimortem//:math",
        "@perimortem//:memory",
        "@perimortem//:serialization",
        "@perimortem//:system",
        "@vulkan_headers",
    ],
)

_COMPONENTS = {
    "description": [],
    "image": [],
    "platform": [],
    "frame": ["image"],
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
}

library(
    srcs = _LIBRARY["srcs"],
    hdrs = _LIBRARY["hdrs"],
    components = _COMPONENTS,
    implementation_deps = _LIBRARY["implementation_deps"],
    includes = _LIBRARY["includes"],
    deps = _LIBRARY["deps"],
)

library(
    name = "photophore_shared",
    srcs = _LIBRARY["srcs"],
    hdrs = _LIBRARY["hdrs"],
    implementation_deps = _LIBRARY["implementation_deps"],
    includes = _LIBRARY["includes"],
    linkage = "shared",
    shared_lib_name = select({
        WINDOWS: "photophore.dll",
        "//conditions:default": "libphotophore.so",
    }),
    deps = _LIBRARY["deps"],
)
