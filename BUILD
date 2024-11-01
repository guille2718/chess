cc_binary(
    name = "blindfold",
    srcs = ["blindfold.cc"],
    deps = [
        ":chess_board",
        "@abseil-cpp//absl/random",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "chess_board",
    hdrs = ["chess_board.h"],
    srcs = ["chess_board.cc"],
    deps = [
        "@abseil-cpp//absl/status:status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@nlohmann_json//:json",
    ],
)
