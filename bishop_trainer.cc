#include <algorithm>
#include <iostream>
#include <memory>

#include <absl/random/random.h>
#include <absl/strings/ascii.h>

#include "chess_board.h"
#include "term_utils.h"

namespace chess {

namespace {

auto CompareSortedPositions(const std::vector<BoardPosition>& a,
                            const std::vector<BoardPosition>& b) -> bool {
  if (a.size() != b.size()) {
    return false;
  }

  for (int i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

auto JoinPositions(const std::vector<BoardPosition>& positions,
                   std::string_view separator = " ") -> std::string {
  std::string ret = absl::StrJoin(
      positions, separator, [](std::string* out, const BoardPosition& pos) {
        absl::StrAppend(out, ToString(pos));
      });

  return ret;
}

// Sorts and removes: duplicates and invalid positions.
void Normalize(std::vector<BoardPosition>& positions) {
  std::ranges::sort(positions);
  positions.erase(std::unique(positions.begin(), positions.end()),
                  positions.end());
  positions.erase(std::remove_if(positions.begin(), positions.end(),
                                 [](const auto& e) { return !e.IsValid(); }),
                  positions.end());
}

auto RandomBoardPosition(absl::BitGen& gen) -> BoardPosition {
  return BoardPosition{
      .file = absl::Uniform(absl::IntervalClosedClosed, gen, 1, 8),
      .rank = absl::Uniform(absl::IntervalClosedClosed, gen, 1, 8),
  };
}

auto ParsePositions(std::string_view positions)
    -> absl::StatusOr<std::vector<BoardPosition>> {
  std::vector<BoardPosition> ret;
  std::vector<std::string_view> parts = absl::StrSplit(positions, ' ');
  for (const auto part : parts) {
    auto pos = BoardPosition::FromString(part);
    if (!pos.ok()) {
      return pos.status();
    }
    ret.push_back(*pos);
  }

  return ret;
}

}  // namespace

class AbstractTrainer {
 public:
  virtual ~AbstractTrainer() = default;

  virtual auto Run(absl::BitGen& gen) const -> void = 0;
};

struct BishopIntercept : public AbstractTrainer {
  ~BishopIntercept() override = default;

  auto Run(absl::BitGen& gen) const -> void override {
    const BoardPosition a = RandomBoardPosition(gen);
    BoardPosition b = RandomBoardPosition(gen);
    // Don't want to have the same square
    while (a == b) {
      b = RandomBoardPosition(gen);
    }

    const int dx = b.file - a.file;
    const int dy = b.rank - a.rank;

    std::vector<BoardPosition> solutions;
    if ( (dx+dy) % 2 == 0) {
      // This means there exist integer solutions to the equation
      // (dx,dy) + s(1, 1) + t(1, -1).
      // That is: solution exists.
      const int s = (dx + dy) / 2;
      const int t = (dx - dy) / 2;

      if (s == 0 || t == 0) {
        // They're on the same diagional.
        solutions = {a};
      } else {
        const auto solution1 = BoardPosition{
            .file = a.file + s,
            .rank = a.rank + s,
        };
        const auto solution2 = BoardPosition{
            .file = a.file + t,
            .rank = a.rank - t,
        };

        solutions.push_back(solution1);
        solutions.push_back(solution2);
        Normalize(solutions);
      }
    }

    std::string solution_str;
    if (solutions.empty()) {
      solution_str = "None";
    } else {
      solution_str = JoinPositions(solutions);
    }

    std::cout << "You have a bishop on " << ToString(a)
              << ". From which accessible squares does it attack "
              << ToString(b) << "? ";

    std::string input;
    std::getline(std::cin, input);

    std::vector<std::string_view> parts = absl::StrSplit(input, ' ');
    std::vector<BoardPosition> user_solutions;

    if (parts.empty() ||
        (parts.size() == 1 && absl::AsciiStrToLower(parts[0]) == "none")) {
      // Empty
    } else {
      for (const auto& part : parts) {
        const auto pos = BoardPosition::FromString(part);
        if (!pos.ok() || !pos->IsValid()) {
          std::cout << "Invalid position: '" << part << "'\n";
          return;
        }
        user_solutions.push_back(*pos);
      }
      Normalize(user_solutions);
    }

    if (!CompareSortedPositions(user_solutions, solutions)) {
      std::cout << "Incorrect! The solution is: " << solution_str << '\n';
      return;
    }

    std::cout << "Correct!\n";
  }
};

struct SquareColor : public AbstractTrainer {
  ~SquareColor() override = default;

  auto Run(absl::BitGen& gen) const -> void override {
    BoardPosition position = RandomBoardPosition(gen);

    std::cout << "Guess the color of the square " << ToString(position) << ": ";
    char input = std::getchar();
    input = absl::ascii_tolower(input);
    const auto guessed = input == 'w' ? Color::White : Color::Black;
    if (guessed == position.Color()) {
      std::cout << "Correct!\n";
    } else {
      std::cout << "Incorrect! It is " << ToString(position.Color()) << '\n';
    }
  }
};

struct BishopEndpoints : public AbstractTrainer {
  ~BishopEndpoints() override = default;

  auto Run(absl::BitGen& gen) const -> void override {
    const BoardPosition position = RandomBoardPosition(gen);

    std::vector<BoardPosition> endpoints;
    endpoints.emplace_back(
        BoardPosition{.file = 1, .rank = position.rank - (position.file - 1)});
    endpoints.emplace_back(
        BoardPosition{.file = 1, .rank = position.rank + (position.file - 1)});

    endpoints.emplace_back(
        BoardPosition{.file = 8, .rank = position.rank - (position.file - 8)});
    endpoints.emplace_back(
        BoardPosition{.file = 8, .rank = position.rank + (position.file - 8)});

    endpoints.emplace_back(
        BoardPosition{.file = position.file - (position.rank - 1), .rank = 1});
    endpoints.emplace_back(
        BoardPosition{.file = position.file + (position.rank - 1), .rank = 1});
    endpoints.emplace_back(
        BoardPosition{.file = position.file - (position.rank - 8), .rank = 8});
    endpoints.emplace_back(
        BoardPosition{.file = position.file + (position.rank - 8), .rank = 8});

    Normalize(endpoints);

    std::string endpoints_string = JoinPositions(endpoints);

    std::cout << "What are the endpoints of B" << ToString(position)
              << "?\n";

    std::string input;
    bool valid = false;
    std::vector<BoardPosition> input_endpoints;
    while (!valid) {
      input.clear();
      std::getline(std::cin, input);
      auto positions_or = ParsePositions(input);

      if (!positions_or.ok()) {
        std::cout << "Invalid positions: '" << positions_or.status()
                  << "'. Please try again\n";
        continue;
      }
      input_endpoints = *positions_or;
      valid = true;
    }
    std::ranges::sort(input_endpoints);

    if (input_endpoints.size() != endpoints.size()) {

      std::cout << "Incorrect! It's " << endpoints_string << '\n';
      return;
    }

    for (int i = 0 ; i != input_endpoints.size() ; ++i) {
      std::string orig_sorted_str = JoinPositions(input_endpoints);

      if (input_endpoints[i] != endpoints[i]){
        std::cout << "Incorrect! It's " << endpoints_string << '\n';
        std::cout << "You wrote:      " << orig_sorted_str << '\n';
        return;
      }
    }

    std::cout << "Correct!\n";
  }
};

void RunBishopTrainer() {
  absl::BitGen gen;

  std::vector<std::unique_ptr<AbstractTrainer>> trainers;

  // trainers.emplace_back(std::make_unique<SquareColor>());
  // trainers.emplace_back(std::make_unique<BishopEndpoints>());
  trainers.emplace_back(std::make_unique<BishopIntercept>());

  while(true){
    terminal::ClearScreen();
    const int pos = absl::Uniform(gen, 0U, trainers.size());
    const auto& trainer = trainers[pos];
    trainer->Run(gen);
    std::string ignored;
    std::getline(std::cin, ignored);
  }
}

}  // namespace chess

auto main() -> int {
  chess::RunBishopTrainer();
  return 0;
}
