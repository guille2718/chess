#include <vector>

#include <absl/random/random.h>
#include <absl/strings/str_format.h>
#include <absl/strings/str_join.h>
#include <absl/strings/str_split.h>
#include <absl/time/time.h>

#include "chess_board.h"
#include "term_utils.h"

auto main(int argc, char* argv[]) -> int {
  if (argc != 3) {
    std::cout << "Must have exactly two arguments\n";
    return -1;
  }

  std::string_view path = argv[1];
  std::string_view command = argv[2];

  // auto problems_or = chess::LoadFenFile(
  //     "/home/guille/chess/problems/polgar_mate_in_ones.json");
  auto problems_or = chess::LoadFenFile(path);

  if (!problems_or.ok()) {
    std::cout << "Error loading FEN file: " << problems_or.status() << '\n';
    return -1;
  }

  const std::vector<chess::ChessBoard> problems = *std::move(problems_or);

  absl::BitGen gen;

  int i;
  if (absl::SimpleAtoi(command, &i)) {
    --i;
    if (i < 0 || i >= problems.size()) {
      std::cout << "Problem out of range\n";
      return -1;
    }
  } else {
    // Assume the user wants to start from the beginning.
    i = 0;
  }

  auto total_duration = absl::ZeroDuration();
  int num_attempts = 0;

  absl::Time start = absl::Now();
  while (true) {
    const int pos = i;
    // const int pos = absl::Uniform(gen, 0U, problems.size());
    // const bool rotate = absl::Bernoulli(gen, 0.5);

    if (pos >= problems.size()) {
      std::cout << "All problems solved!\n";
      return 0;
    }
    const bool rotate = false;
    auto problem = problems[pos];
    if (rotate) {
      problem.Rotate();
      auto info = problem.Info();
      absl::StrAppend(&info, " (Rotated)");
      problem.SetInfo(info);
    }

    terminal::ClearScreen();
    if (num_attempts > 0) {
      std::cout << "Average time per problem: " << total_duration / num_attempts
                << '\n';
      std::cout << "Time taken to solve the last problem: "
                << absl::Now() - start << '\n';
    }
    start = absl::Now();
    std::cout << "Total solving time " << total_duration << '\n';
    problem.Print(/* show_info = */ true,
                  /* language = */ chess::ChessLanguage::kEnglish);
    std::cout
        << "-----------------------------------------------------------\n";
    std::string input;
    std::getline(std::cin, input);
    total_duration += absl::Now() - start;
    ++num_attempts;
    ++i;
  }

  return 0;
}
