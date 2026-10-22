# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "headers",
    hdrs = glob(["**/*.hpp"]),
    strip_include_prefix = "/source",
    visibility = ["//source:__pkg__"],
)
