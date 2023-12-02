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
    path = "/usr/local/Cellar/opencv/4.6.0_1",
)

new_local_repository(
    name = "opencv_linux",
    build_file = "BUILD.opencv",
    path = "/usr",
)

http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-8028a87c96df0fff5ab58daeec30c43ce6fb0d20",
    urls = ["https://github.com/abseil/abseil-cpp/archive/8028a87c96df0fff5ab58daeec30c43ce6fb0d20.zip"],
)

http_archive(
    name = "bazel_skylib",
    sha256 = "f7be3474d42aae265405a592bb7da8e171919d74c16f082a5457840f06054728",
    urls = ["https://github.com/bazelbuild/bazel-skylib/releases/download/1.2.1/bazel-skylib-1.2.1.tar.gz"],
)

http_archive(
    name = "com_google_googletest",
    sha256 = "69a7d89712b067e4980234912387cf2e2234c7795d03cbad79f837fc29aa12f2",
    strip_prefix = "googletest-e07617d6c692a96e126f11f85c3e38e46b10b4d0",
    urls = ["https://github.com/google/googletest/archive/e07617d6c692a96e126f11f85c3e38e46b10b4d0.zip"],
)

http_archive(
    name = "com_googlesource_code_re2",
    sha256 = "0a890c2aa0bb05b2ce906a15efb520d0f5ad4c7d37b8db959c43772802991887",
    strip_prefix = "re2-a427f10b9fb4622dd6d8643032600aa1b50fbd12",
    urls = ["https://github.com/google/re2/archive/a427f10b9fb4622dd6d8643032600aa1b50fbd12.zip"],  # 2022-06-09
)

http_archive(
    name = "com_google_protobuf",
    sha256 = "c369381e0d0ec3c45430edb40a52d93f5ebbdd0dd12180a6f773ca0fee771ae5",
    strip_prefix = "protobuf-90b73ac3f0b10320315c2ca0d03a5a9b095d2f66",
    urls = [
        "https://github.com/protocolbuffers/protobuf/archive/90b73ac3f0b10320315c2ca0d03a5a9b095d2f66.zip",
    ],
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

http_archive(
    name = "rules_proto",
    sha256 = "80d3a4ec17354cccc898bfe32118edd934f851b03029d63ef3fc7c8663a7415c",
    strip_prefix = "rules_proto-5.3.0-21.5",
    urls = [
        "https://github.com/bazelbuild/rules_proto/archive/refs/tags/5.3.0-21.5.tar.gz",
    ],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()
