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
    module = "photophore",
)

_COMPONENTS = {
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
}

_COMPONENT_DEPS = {
    "description": ["@perimortem//:core"],
    "frame": [
        "@perimortem//:core",
        "@perimortem//:memory",
    ],
    "image": [
        "@perimortem//:core",
        "@perimortem//:memory",
    ],
    "input": ["@perimortem//:core"],
    "platform": [
        "@perimortem//:core",
        "@vulkan_headers",
    ],
    "scene": ["@perimortem//:core"],
    "vulkan": [
        "@perimortem//:core",
        "@perimortem//:memory",
        "@vulkan_headers",
    ],
}

library(
    name = "photophore_static",
    srcs = _LIBRARY["srcs"],
    hdrs = _LIBRARY["hdrs"],
    component_deps = _COMPONENT_DEPS,
    components = _COMPONENTS,
    implementation_deps = _LIBRARY["implementation_deps"],
    includes = _LIBRARY["includes"],
    module = _LIBRARY["module"],
    deps = [
        "@perimortem_static//:core",
        "@perimortem_static//:memory",
        "@perimortem_static//:system",
        "@vulkan_headers",
    ],
)

library(
    name = "photophore_shared",
    srcs = _LIBRARY["srcs"],
    hdrs = _LIBRARY["hdrs"],
    implementation_deps = _LIBRARY["implementation_deps"],
    includes = _LIBRARY["includes"],
    linkage = "shared",
    module = _LIBRARY["module"],
    shared_lib_name = select({
        WINDOWS: "photophore.dll",
        "//conditions:default": "libphotophore.so",
    }),
    deps = [
        "@perimortem//:core",
        "@perimortem//:memory",
        "@perimortem//:system",
        "@vulkan_headers",
    ],
)
