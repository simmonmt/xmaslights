workspace(
    name = "xmaslights",
)

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "io_bazel_rules_go",
    sha256 = "d6ab6b57e48c09523e93050f13698f708428cfd5e619252e369d377af6597707",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_go/releases/download/v0.43.0/rules_go-v0.43.0.zip",
        "https://github.com/bazelbuild/rules_go/releases/download/v0.43.0/rules_go-v0.43.0.zip",
    ],
)

http_archive(
    name = "bazel_gazelle",
    sha256 = "b7387f72efb59f876e4daae42f1d3912d0d45563eac7cb23d1de0b094ab588cf",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-gazelle/releases/download/v0.34.0/bazel-gazelle-v0.34.0.tar.gz",
        "https://github.com/bazelbuild/bazel-gazelle/releases/download/v0.34.0/bazel-gazelle-v0.34.0.tar.gz",
    ],
)

load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains", "go_rules_dependencies")
load("@bazel_gazelle//:deps.bzl", "gazelle_dependencies")

go_rules_dependencies()

go_register_toolchains(version = "host")

gazelle_dependencies()

# Use gazelle to declare Go dependencies in Bazel.
# gazelle:repository_macro go_repositories.bzl%go_repositories

load("//:go_repositories.bzl", "go_repositories")

go_repositories()

new_local_repository(
    name = "opencv_mac",
    build_file = "BUILD.opencv",
    path = "./deps/opencv/mac",
)

new_local_repository(
    name = "opencv_linux",
    build_file = "BUILD.opencv",
    path = "./deps/opencv/linux",
)

http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-8028a87c96df0fff5ab58daeec30c43ce6fb0d20",
    urls = ["https://github.com/abseil/abseil-cpp/archive/8028a87c96df0fff5ab58daeec30c43ce6fb0d20.zip"],
)

http_archive(
    name = "bazel_skylib",
    sha256 = "cd55a062e763b9349921f0f5db8c3933288dc8ba4f76dd9416aac68acee3cb94",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.5.0/bazel-skylib-1.5.0.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.5.0/bazel-skylib-1.5.0.tar.gz",
    ],
)

# Needed by protobuf
http_archive(
    name = "rules_python",
    sha256 = "9acc0944c94adb23fba1c9988b48768b1bacc6583b52a2586895c5b7491e2e31",
    strip_prefix = "rules_python-0.27.0",
    url = "https://github.com/bazelbuild/rules_python/releases/download/0.27.0/rules_python-0.27.0.tar.gz",
)

load("@rules_python//python:repositories.bzl", "py_repositories")

py_repositories()

http_archive(
    name = "com_google_googletest",
    sha256 = "c8c5fb6bf567995cb5ea5c088c2fbaca6430aebd8173dd7161975cd32cbe0bda",
    strip_prefix = "googletest-76bb2afb8b522d24496ad1c757a49784fbfa2e42",
    urls = ["https://github.com/google/googletest/archive/76bb2afb8b522d24496ad1c757a49784fbfa2e42.zip"],
)

http_archive(
    name = "com_googlesource_code_re2",
    sha256 = "74d8d42e6398cd551752de78416e32a3fd388b83e98d36caf21b9ddc336f8a8d",
    strip_prefix = "re2-7e0c1a9e2417e70e5f0efc323267ac71d1fa0685",
    urls = ["https://github.com/google/re2/archive/7e0c1a9e2417e70e5f0efc323267ac71d1fa0685.zip"],  # 2023-12-02
)

http_archive(
    name = "com_google_protobuf",
    sha256 = "4d0ae83f153c34b288eff840b400566f79e0b323900b10d032df245d65d7e1d2",
    strip_prefix = "protobuf-7bdd38e1f1d4e5c8f2cc1f27e93bd62c085f90e4",
    urls = [
        "https://github.com/protocolbuffers/protobuf/archive/7bdd38e1f1d4e5c8f2cc1f27e93bd62c085f90e4.zip",
    ],
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

http_archive(
    name = "rules_proto",
    sha256 = "903af49528dc37ad2adbb744b317da520f133bc1cbbecbdd2a6c546c9ead080b",
    strip_prefix = "rules_proto-6.0.0-rc0",
    url = "https://github.com/bazelbuild/rules_proto/releases/download/6.0.0-rc0/rules_proto-6.0.0-rc0.tar.gz",
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()
