#include <vector>

#include <absl/random/random.h>
#include <absl/strings/str_format.h>
#include <absl/strings/str_join.h>
#include <absl/strings/str_split.h>

#include "chess_board.h"

auto main() -> int {
  auto problems_or = chess::LoadFenFile(
      "/home/guille/chess/problems/polgar_mate_in_ones.json");

  if (!problems_or.ok()) {
    std::cout << "Error loading FEN file: " << problems_or.status() << '\n';
    return -1;
  }

  const std::vector<chess::ChessBoard> problems = *std::move(problems_or);

  absl::BitGen gen;

  while (true) {
    int pos = absl::Uniform(gen, 0U, problems.size());
    const auto& problem = problems[pos];

    problem.Print();
    std::cout
        << "-----------------------------------------------------------\n";
    getchar();
  }

  return 0;
}
