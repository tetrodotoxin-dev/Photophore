load("@tetro_toolchain//:defs.bzl", "benchmarks")
package(default_visibility = ["//visibility:public"])
filegroup(name = "sources", srcs = glob(["**"]))
benchmarks(name = "benchmarks", srcs = glob(["**/*.cpp"]), deps = ["//:photophore"])
