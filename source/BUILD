# Copyright (c) 2023-present Matt Kaes and contributors

load("@tetro_toolchain//:defs.bzl", "LINUX", "WINDOWS", "library")

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
    includes = ["."],
    linkopts = select({
        LINUX: ["-lwayland-client"],
        "//conditions:default": [],
    }),
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
    includes = _LIBRARY["includes"],
    linkopts = _LIBRARY["linkopts"],
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
    includes = _LIBRARY["includes"],
    linkage = "shared",
    linkopts = [],
    module = _LIBRARY["module"],
    shared_lib_name = select({
        WINDOWS: "photophore.dll",
        "//conditions:default": "libphotophore.so",
    }),
    user_link_flags = select({
        LINUX: [
            "-L/usr/lib",
            "-lwayland-client",
        ],
        "//conditions:default": [],
    }),
    deps = [
        "@perimortem//:core",
        "@perimortem//:memory",
        "@perimortem//:system",
        "@vulkan_headers",
    ],
)
