#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <absl/random/random.h>
#include <absl/status/status.h>
#include <absl/strings/str_format.h>
#include <absl/strings/str_join.h>
#include <absl/strings/str_split.h>
#include <absl/time/time.h>

#include "chess_board.h"
#include "term_utils.h"

namespace chess {

auto RunMemoryTrainer(const std::filesystem::path& path,
                      int problem_number) -> absl::Status {
  --problem_number;
  auto problems_or = LoadFenFile(path);
  if (!problems_or.ok()) {
    return problems_or.status();
  }
  const std::vector<ChessBoard> problems = *std::move(problems_or);

  absl::BitGen gen;

  terminal::ClearScreen();
  const auto& problem = problems[problem_number];

  problem.Print(/* show_info = */ true,
                /* language = */ chess::ChessLanguage::kEnglish);
  std::cout << "-----------------------------------------------------------\n";
  std::cout << "Press enter when done memorizing...";
  std::string input;
  std::getline(std::cin, input);
  input.clear();

  const auto board_pieces = problem.BoardPieces();

  while (true) {
    terminal::ClearScreen();

    const int question_type = absl::Uniform(gen, 0, 2);

    if (question_type == 0) {
      // Spot check square
      const bool from_pieces = absl::Bernoulli(gen, 0.75);
      BoardPosition pos;
      if (from_pieces) {
        int i = absl::Uniform(gen, 0U, board_pieces.size());
        pos = board_pieces[i].position;
      } else {
        pos.file = absl::Uniform(absl::IntervalClosed, gen, 1, 8);
        pos.rank = absl::Uniform(absl::IntervalClosed, gen, 1, 8);
      }
      std::cout << "What is on " << ToString(pos) << "?\n";
      std::getline(std::cin, input);

      if (input == "exit") {
        return absl::OkStatus();
      }

      if (input == "empty" || input == "nothing" || input.empty()) {
        input = "none";
      }

      if (input != "none" && input.size() != 1) {
        std::cout << "Invalid response\n";
        continue;
      }

      std::optional<Piece> user_piece;
      if (input != "none") {
        auto piece_or = FromFenPieceChar(input[0]);
        if (!piece_or.ok()) {
          std::cout << "Invalid piece notation\n";
          continue;
        }
        user_piece = *piece_or;
      }

      std::optional<Piece> actual_piece = problem.AtPosition(pos);

      std::string answer;
      if (actual_piece.has_value()) {
        answer = absl::StrFormat(
            "a %s %s", actual_piece->color == Color::White ? "white" : "Black",
            ToString(actual_piece->type, ChessLanguage::kEnglishFull));
      } else {
        answer = "none";
      }

      if (actual_piece == user_piece) {
        std::cout << "Correct!\n";
      } else {
        std::cout << "Incorrect!. It's " << answer << '\n';
      }
    } else if (question_type == 1) {
      // Rank description
      int rank_number = absl::Uniform(absl::IntervalClosed, gen, 1, 8);
      std::cout << "What's on rank number " << rank_number << "? ";
      input.clear();
      std::getline(std::cin, input);
      if (input == "exit") {
        return absl::OkStatus();
      }

      std::vector<std::string_view> parts =
          absl::StrSplit(input, ' ', absl::SkipEmpty());

      std::vector<BoardPiece> user_pieces;
      for (const auto part : parts) {
        auto bp_or = BoardPiece::FromString(part);
        if (!bp_or.ok()) {
          std::cout << "Invalid board piece notation: " << bp_or.status()
                    << '\n';
          break;
        }
        user_pieces.push_back(*bp_or);
      }

      std::ranges::sort(user_pieces, [](const auto& a, const auto& b) {
        return a.position < b.position;
      });

      std::vector<BoardPiece> actual_pieces;
      for (const auto& bp : board_pieces) {
        if (bp.position.rank == rank_number) {
          actual_pieces.push_back(bp);
        }
      }
      std::ranges::sort(actual_pieces, [](const auto& a, const auto& b) {
        return a.position < b.position;
      });

      bool equal = true;
      if (actual_pieces.size() != user_pieces.size()) {
        equal = false;
      } else {
        for (int i = 0; i < user_pieces.size(); ++i) {
          if (user_pieces[i] != actual_pieces[i]) {
            equal = false;
            break;
          }
        }
      }

      if (equal) {
        std::cout << "Correct!\n";
      } else {
        std::string answer = absl::StrJoin(
            actual_pieces, " ", [](std::string* out, const BoardPiece& bp) {
              absl::StrAppend(out, ToString(bp));
            });
        std::cout << "Incorrect, the answer is: " << answer << '\n';
      }
    }

    std::getline(std::cin, input);
    input.clear();
  }

  return absl::OkStatus();
}

}  // namespace chess

auto main(int argc, char* argv[]) -> int {
  if (argc != 3) {
    std::cout << "Must have exactly two arguments\n";
    return -1;
  }

  const auto path = std::filesystem::path{argv[1]};
  if (!std::filesystem::exists(path)) {
    std::cout << "File does not exist\n";
    return -1;
  }

  std::string problem_number_str = argv[2];
  int problem_number;
  if (!absl::SimpleAtoi(problem_number_str, &problem_number)) {
    std::cout << "Couldn't parse '" << problem_number_str << "' as a number\n";
    return -1;
  }

  auto status = chess::RunMemoryTrainer(path, problem_number);

  if (status.ok()) {
    return 0;
  }


  std::cout << status << '\n';

  return -1;
}
