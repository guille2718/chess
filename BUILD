cc_binary(
    name = "blindfold",
    srcs = ["blindfold.cc"],
    deps = [
        ":chess_board",
        ":term_utils",
        "@abseil-cpp//absl/random",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/time",
    ],
)

cc_binary(
    name = "memory",
    srcs = ["memory.cc"],
    deps = [
        ":chess_board",
        ":term_utils",
        "@abseil-cpp//absl/random",
        "@abseil-cpp//absl/status:status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/time",
    ],
)

cc_library(
    name = "chess_board",
    srcs = ["chess_board.cc"],
    hdrs = ["chess_board.h"],
    deps = [
        "@abseil-cpp//absl/status:status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "term_utils",
    srcs = ["term_utils.cc"],
    hdrs = ["term_utils.h"],
    deps = [
    ],
)

cc_binary(
    name = "bishop_trainer",
    srcs = ["bishop_trainer.cc"],
    deps = [
        ":chess_board",
        ":term_utils",
        "@abseil-cpp//absl/random",
        "@abseil-cpp//absl/strings",
    ],
)
