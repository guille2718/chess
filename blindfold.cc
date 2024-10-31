#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <absl/random/random.h>
#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <absl/strings/ascii.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_format.h>
#include <absl/strings/str_join.h>
#include <absl/strings/str_split.h>
#include <nlohmann/json.hpp>

namespace chess {

namespace {

// 1-based
auto FileToLetter(int file) -> char { return 'a' + file - 1; }

auto LoadFile(const std::filesystem::path& path) -> std::string {
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();

  return buffer.str();
}

}  // namespace

enum class PieceType : char {
  Rook,
  Knight,
  Bishop,
  Queen,
  King,
  Pawn,
};

auto FromFenPiece(char c) -> absl::StatusOr<PieceType> {
  c = absl::ascii_tolower(c);

  if (c == 'r') {
    return PieceType::Rook;
  }
  if (c == 'n') {
    return PieceType::Knight;
  }
  if (c == 'b') {
    return PieceType::Bishop;
  }
  if (c == 'q') {
    return PieceType::Queen;
  }
  if (c == 'k') {
    return PieceType::King;
  }
  if (c == 'p') {
    return PieceType::Pawn;
  }

  return absl::InvalidArgumentError(
      absl::StrFormat("Invalid FEN piece type: '%c'", c));
}

enum class ChessLanguage : char {
  kSpanish,
  kEnglish,
  kUnicode,
};

auto ToString(PieceType type,
              ChessLanguage language = ChessLanguage::kEnglish) -> std::string {
  if (language == ChessLanguage::kEnglish) {
    switch (type) {
      case PieceType::Rook:
        return "R";
      case PieceType::Knight:
        return "N";
      case PieceType::Bishop:
        return "B";
      case PieceType::Queen:
        return "Q";
      case PieceType::King:
        return "K";
      case PieceType::Pawn:
        return "P";
      default:
        return "";
    }
  } else if (language == ChessLanguage::kUnicode) {
    switch (type) {
      case PieceType::Rook:
        return "🨂 ";
      case PieceType::Knight:
        return "🨄 ";
      case PieceType::Bishop:
        return "🨃 ";
      case PieceType::Queen:
        return "🨁 ";
      case PieceType::King:
        return "🨀 ";
      case PieceType::Pawn:
        return "🨅 ";
      default:
        return "";
    }
  }

  return "";
  // 🨀🨁🨂🨃🨄🨅
}

struct BoardPosition {
  int file;
  int rank;

  auto IsValid() const -> bool {
    return file >= 1 && file <= 8 && rank >= 1 && rank <= 8;
  }
};

auto ToString(const BoardPosition& position) -> std::string {
  std::string ret = "aa";
  ret[0] = FileToLetter(position.file);
  ret[1] = '0' + position.rank;
  return ret;
}

struct BoardPiece {
  BoardPosition position;
  PieceType type;
};

auto ToString(const BoardPiece& piece,
              ChessLanguage language = ChessLanguage::kEnglish) -> std::string {
  return absl::StrCat(ToString(piece.type, language), ToString(piece.position));
};

struct ChessBoard {
  std::vector<BoardPiece> white;
  std::vector<BoardPiece> black;

  bool white_to_move = true;

  const ChessLanguage language = ChessLanguage::kEnglish;
  auto Print() const -> void {
    std::cout << "White: ";
    for (const auto& piece : white) {
      std::cout << ToString(piece, language) << ", ";
    }
    std::cout << '\n';

    std::cout << "Black: ";
    for (const auto& piece : black) {
      std::cout << ToString(piece, language) << ", ";
    }
    std::cout << '\n';
    if (white_to_move) {
      std::cout << "White to move\n";
    } else {
      std::cout << "Black to move\n";
    }
  }
};

// TODO
auto ToString(const ChessBoard& board) -> std::string {
  std::string ret;
  return ret;
}

// https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
auto FromFen(std::string_view fen) -> absl::StatusOr<ChessBoard> {
  std::vector<std::string_view> parts = absl::StrSplit(fen, ' ');

  if (parts.empty()) {
    return absl::InvalidArgumentError(
        "FEN Must contain at least piece placement data");
  }

  ChessBoard ret;

  if (parts.size() >= 2) {
    ret.white_to_move = parts[1] == "w";
  }

  std::string_view ppd = parts[0];

  std::vector<std::string_view> ranks = absl::StrSplit(ppd, '/');

  if (ranks.size() != 8) {
    return absl::InvalidArgumentError("There must be 8 ranks in the FEN");
  }


  int rank = 8;
  for (auto rank_str : ranks) {
    if (rank_str.size() > 8) {
      return absl::InvalidArgumentError(absl::StrFormat("FEN rank must have at most 8 files, has %d", rank_str.size()));
    }
    int file = 1;
    for (const char current_char : rank_str) {

      if (absl::ascii_isdigit(current_char)) {
        file += current_char - '0';
        continue;
      }

      auto piece_type_or = FromFenPiece(current_char);
      if (!piece_type_or.ok()) {
        return piece_type_or.status();
      }
      const auto piece_type = *piece_type_or;
      const bool white = absl::ascii_isupper(current_char);

      BoardPiece piece;
      piece.type = piece_type;
      piece.position.file = file;
      piece.position.rank = rank;

      if (white) {
        ret.white.push_back(piece);
      } else {
        ret.black.push_back(piece);
      }

      ++file;
    }
    --rank;
  }

  return ret;
}

}  // namespace chess

auto main() -> int {
  // std::vector<std::string> polgars{
  //     "3q1rk1/5pbp/5Qp1/8/8/2B5/5PPP/6K1 w - - 0 1",
  //     "2r2rk1/2q2p1p/6pQ/4P1N1/8/8/PPP5/2KR4 w - - 0 1",
  //     "r2q1rk1/pp1p1p1p/5PpQ/8/4N3/8/PP3PPP/R5K1 w - - 0 1",
  //     "6r1/7k/2p1pPp1/3p4/8/1R6/5PPP/5K2 w - - 0 1",
  //     "1r4k1/1q3p2/5Bp1/8/8/8/PP6/1K5R w - - 0 1",
  //     "r4rk1/5p1p/8/8/8/8/1BP5/2KR4 w - - 0 1",
  //     "4r2k/4r1p1/6p1/8/2B5/8/1PP5/2KR4 w - - 0 1",
  //     "8/2r1N1pk/8/8/8/2q2p2/2P5/2KR4 w - - 0 1",
  //     "r7/4KNkp/8/8/b7/8/8/1R6 w - - 0 1",
  //     "2kr4/3n4/2p5/8/5B2/8/6PP/5B1K w - - 0 1",
  //     "r1b1kb1r/5ppp/8/6B1/8/8/5PPP/3R3K w - - 0 1",
  //     "r4rk1/p6p/1n6/6N1/3B4/3B4/6PP/7K w - - 0 1",
  //     "r1bqk1nr/pppp1ppp/2n5/2b1p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR w KQkq - 0 1",
  //     "rnbqkbnr/ppppp2p/5p2/6p1/3PP3/8/PPP2PPP/RNBQKBNR w KQkq - 0 1",
  //     "6k1/5ppp/r1p5/3b4/8/1pB5/1Pr2PPP/3RR1K1 w - - 0 1",
  //     "rnbq1rk1/ppp1nppp/3bp3/3p3Q/3P4/3BPN2/PPP2PPP/RNB1K2R w KQ - 0 1",
  //     "6k1/p1p2rpp/1q6/2p5/4P3/PQ6/1P4PP/3R3K w - - 0 1",
  //     "rnb4k/p5pp/8/4N3/8/1B6/PPP5/2K4R w - - 0 1",
  //     "6r1/2Q2P2/5k2/5P2/5K2/8/8/8 w - - 0 1",
  //     "8/3pkP2/4p3/8/8/3K4/8/5R2 w - - 0 1",
  //     "3Nnr2/R2PkP2/4p3/8/8/4K3/8/3R4 w - - 0 1",
  //     "2kr4/3p4/8/4B3/8/3B4/3K4/8 w - - 0 1",
  //     "2kr4/8/8/8/Q7/6B1/6K1/8 w - - 0 1",
  //     "3r4/1pk5/3pP3/2N5/8/8/8/2R4K w - - 0 1",
  //     "8/8/3Rp3/1P2k3/3Np3/2B5/8/5RK1 w - - 0 1",
  //     "5rk1/8/6P1/8/7Q/8/6K1/8 w - - 0 1",
  //     "8/5B2/4p3/4kp2/3Np3/8/4P3/Q2R2K1 w - - 0 1",
  //     "7k/8/8/5BN1/8/8/8/6RK w - - 0 1",
  //     "4kq2/3p4/4B1P1/B7/8/8/8/4R2K w - - 0 1",
  //     "5rk1/8/7P/6N1/8/8/1B6/2K5 w - - 0 1",
  //     "5nk1/6b1/8/8/8/2Q5/7K/6R1 w - - 0 1",
  //     "3bk3/R3p3/3P4/8/8/8/8/4KR2 w - - 0 1",
  //     "k1r5/8/P7/8/1Q6/8/8/6K1 w - - 0 1",
  //     "2kr4/2p3R1/3P4/8/8/8/8/1R4K1 w - - 0 1",
  //     "6rk/7p/8/8/4B3/8/8/K6R w - - 0 1",
  //     "6rk/6n1/5R2/8/8/8/8/6K1 w - - 0 1",
  //     "1rkr2R1/1pnb4/8/8/3Q4/8/8/3R2K1 w - - 0 1",
  //     "3qkn2/1R6/4N3/8/8/8/8/5K2 w - - 0 1",
  //     "4b3/3k4/4q3/B7/5Q2/8/8/6K1 w - - 0 1",
  //     "5k2/4np2/5N2/2B5/8/8/8/6RK w - - 0 1",
  //     "6nk/2R5/8/6N1/8/8/8/7K w - - 0 1",
  //     "4k3/3nn3/4N3/1N6/3K4/8/8/8 w - - 0 1",
  //     "8/8/8/6R1/4k3/2KN1r2/8/8 w - - 0 1",
  //     "8/8/8/7R/3k4/1KR5/5P2/6B1 w - - 0 1",
  //     "8/8/8/8/3kr3/1K6/3P3Q/8 w - - 0 1",
  //     "8/8/5R2/1K1k4/8/5P2/6B1/6B1 w - - 0 1",
  //     "8/6Q1/8/8/3Pk3/5q2/3K4/8 w - - 0 1",
  //     "8/5P2/5k2/5B2/5K2/8/8/8 w - - 0 1",
  //     "4Q3/8/8/5N1N/6k1/8/6P1/6K1 w - - 0 1",
  //     "8/8/8/1k6/b7/1N1P4/8/5K1Q w - - 0 1",
  //     "8/8/2Q3N1/5p1p/6k1/8/8/6K1 w - - 0 1",
  //     "8/8/R7/3k4/3PN3/2K5/8/8 w - - 0 1",
  //     "5n2/2PPk1PR/8/4K3/8/8/8/8 w - - 0 1",
  //     "5r2/6k1/5r2/8/7Q/8/2B5/6K1 w - - 0 1",
  //     "4k3/4P3/3PK3/8/8/8/8/8 w - - 0 1",
  //     "kn6/n7/1PP5/8/8/8/8/7K w - - 0 1",
  //     "4K3/4B3/4k3/8/8/8/8/1Q6 w - - 0 1",
  //     "8/8/6k1/6B1/8/6N1/Q7/7K w - - 0 1",
  //     "8/8/8/8/8/4k3/4P3/Q3K3 w - - 0 1",
  //     "8/8/8/8/6k1/5N1N/6P1/1Q4K1 w - - 0 1",
  //     "k1K1N3/8/8/8/8/8/8/6B1 w - - 0 1",
  //     "k7/8/P1N5/8/N7/8/8/5K2 w - - 0 1",
  //     "1k6/1Pp5/P7/8/3N4/8/8/5K2 w - - 0 1",
  //     "k2N4/8/1PN5/8/8/8/8/7K w - - 0 1",
  //     "k7/1p6/2N5/8/2N5/8/8/7K w - - 0 1",
  //     "kr6/pp6/8/3N4/8/8/8/7K w - - 0 1",
  //     "7k/1K5B/8/8/8/8/1R6/B7 w - - 0 1",
  //     "7k/8/5KB1/8/8/8/8/B7 w - - 0 1",
  //     "8/6K1/8/7k/8/5RB1/8/3B4 w - - 0 1",
  //     "k7/3N4/8/1N6/8/8/8/3B3K w - - 0 1",
  //     "3BB3/5N1K/8/7k/8/8/8/8 w - - 0 1",
  //     "2K5/k1N5/8/8/8/8/8/B7 w - - 0 1",
  //     "1k6/7R/KN6/8/8/8/8/8 w - - 0 1",
  //     "7k/R7/5N2/8/8/8/8/7K w - - 0 1",
  //     "7k/8/5N2/8/8/8/8/6RK w - - 0 1",
  //     "8/8/R7/3N4/3rk3/4N3/4P3/K7 w - - 0 1",
  //     "8/3Np2R/4k3/2R2N2/8/8/8/K7 w - - 0 1",
  //     "8/3N4/7R/8/2pkp3/8/3P4/7K w - - 0 1",
  //     "8/8/8/8/2R5/3k3B/8/2B4K w - - 0 1",
  //     "2R5/3k4/5P2/4P2K/8/8/8/1B6 w - - 0 1",
  //     "8/8/8/8/2R5/3kb2B/3p4/7K w - - 0 1",
  //     "8/8/1kP5/R2P4/7B/8/8/7K w - - 0 1",
  //     "8/6p1/5pk1/7R/B7/8/8/7K w - - 0 1",
  //     "3k4/8/8/3NN3/8/8/8/R6K w - - 0 1",
  //     "7R/5p2/6k1/5p2/8/8/8/1KQ5 w - - 0 1",
  //     "8/4p3/3pk3/6K1/8/8/8/1Q6 w - - 0 1",
  //     "8/8/5k2/4p1p1/8/1Q6/B7/6K1 w - - 0 1",
  //     "8/8/8/8/4k3/4P3/Q3P3/7K w - - 0 1",
  //     "6k1/R7/8/8/8/8/8/1R4K1 w - - 0 1",
  //     "3k4/8/3BB3/8/8/8/8/R6K w - - 0 1",
  //     "8/8/8/8/4k3/3N4/2N3K1/3Q4 w - - 0 1",
  //     "8/3N4/8/1N6/4k3/8/6K1/3Q4 w - - 0 1",
  //     "R7/4k3/8/4P3/Q7/8/8/4K3 w - - 0 1",
  //     "7R/4k3/8/2P3P1/Q7/8/8/4K3 w - - 0 1",
  //     "8/8/2Pk4/1P3K2/8/8/8/Q7 w - - 0 1",
  //     "8/5p1p/6k1/Q7/7P/8/8/6K1 w - - 0 1",
  //     "5Q2/8/8/8/6p1/8/2NNk3/2K5 w - - 0 1",
  //     "1Q6/8/2K5/3N4/4k3/5N2/6P1/8 w - - 0 1",
  //     "K7/8/6N1/5n2/4k3/3n4/2N5/Q7 w - - 0 1",
  //     "8/8/8/8/8/3NkN2/Q7/4K3 w - - 0 1",
  //     "8/8/8/1N5Q/8/3Nk3/8/4K3 w - - 0 1",
  //     "5Q2/8/8/8/4k3/1N1N4/6K1/8 w - - 0 1",
  //     "1K1k4/8/8/3N4/8/7Q/8/8 w - - 0 1",
  //     "6N1/8/4k3/2K5/8/8/8/7Q w - - 0 1",
  //     "K7/8/8/4kN2/4p3/Q3N3/8/8 w - - 0 1",
  //     "K7/8/Q7/5N2/4p3/4Nk2/8/8 w - - 0 1",
  //     "K7/8/Q7/5Nk1/4p3/4N3/8/8 w - - 0 1",
  //     "K7/8/8/5N2/4p3/4N3/5k2/Q7 w - - 0 1",
  //     "6Bk/Q6P/8/8/K7/8/8/8 w - - 0 1",
  //     "3k4/8/8/3NN3/8/8/8/4K2Q w - - 0 1",
  //     "4k3/8/8/3NN3/8/8/8/4K2Q w - - 0 1",
  //     "5k2/8/8/3NN3/8/8/8/4K2Q w - - 0 1",
  //     "2N5/8/8/2k5/8/2K5/8/5Q2 w - - 0 1",
  //     "5N2/5N2/8/5k2/8/8/8/3Q1K2 w - - 0 1",
  //     "2K1k3/8/4B3/8/7Q/8/8/8 w - - 0 1",
  //     "4B3/4B3/4k3/8/K7/8/8/1Q6 w - - 0 1",
  //     "8/8/8/8/Q7/8/4k3/2K3B1 w - - 0 1",
  //     "8/8/7k/K4Q2/8/B7/8/8 w - - 0 1",
  //     "8/5B2/5kP1/3K4/8/8/7Q/8 w - - 0 1",
  //     "7k/B7/8/8/K1Q1B3/8/8/8 w - - 0 1",
  //     "R7/4k3/8/4K3/7P/7Q/8/8 w - - 0 1",
  //     "6R1/5k2/8/4K3/8/7Q/8/8 w - - 0 1",
  //     "1Qn5/5p2/5k2/6R1/5K2/8/8/8 w - - 0 1",
  //     "2R5/3k4/5P2/4P2K/8/8/8/6Q1 w - - 0 1",
  //     "2R5/3kp2R/3p4/8/7K/8/8/3Q4 w - - 0 1",
  //     "4B3/8/4k3/8/4K3/8/8/Q7 w - - 0 1",
  //     "3rkr2/7Q/4K3/8/8/8/8/8 w - - 0 1",
  //     "k7/rp6/8/8/8/8/8/4KQ2 w - - 0 1",
  //     "2Q5/4k3/8/K7/8/8/Q7/8 w - - 0 1",
  //     "8/5k2/8/K3Q3/Q7/8/8/8 w - - 0 1",
  //     "4Q3/8/5k2/K7/8/8/7Q/8 w - - 0 1",
  //     "Q7/4k3/8/4K3/8/8/8/R7 w - - 0 1",
  //     "5K1k/7p/8/8/8/8/7B/8 w - - 0 1",
  //     "7k/7B/7K/8/7B/8/8/8 w - - 0 1",
  //     "7k/8/7K/3B4/8/8/8/6B1 w - - 0 1",
  //     "5K1k/7p/3N4/8/8/8/8/8 w - - 0 1",
  //     "2K1k3/8/6N1/1N6/8/8/8/8 w - - 0 1",
  //     "4k3/8/4K3/8/P7/Q7/8/8 w - - 0 1",
  //     "4k3/8/4K3/8/8/8/8/R7 w - - 0 1",
  //     "R7/4k3/8/4K3/8/8/8/7R w - - 0 1",
  //     "8/8/4NN2/8/8/5k2/8/R4K2 w - - 0 1",
  //     "8/8/4N3/8/4p3/4k3/8/4K2R w - - 0 1",
  //     "8/8/8/3BB3/8/4k3/8/R3K3 w - - 0 1",
  //     "4K3/8/4k3/8/3PP3/8/8/R7 w - - 0 1",
  //     "1Q6/8/4NN2/8/8/3k4/8/4K3 w - - 0 1",
  //     "8/4Q3/4NN2/8/8/4k3/8/4K3 w - - 0 1",
  //     "8/1Q6/4N3/8/4p3/3k4/8/4K3 w - - 0 1",
  //     "8/1Q6/4N3/8/4p3/4k3/8/4K3 w - - 0 1",
  //     "8/5Q2/8/8/3pp3/3k4/8/4K3 w - - 0 1",
  //     "4K3/8/4k3/Q7/3P4/8/8/8 w - - 0 1",
  //     "4k3/8/4K3/8/4Q3/8/8/8 w - - 0 1",
  //     "2k5/8/3K4/3Q4/8/8/8/8 w - - 0 1",
  //     "R7/3k4/8/3K4/6p1/6Q1/8/8 w - - 0 1",
  //     "R7/3k4/8/3K4/3Q4/8/8/8 w - - 0 1",
  //     "8/8/1Q6/3BB3/8/3k4/8/4K3 w - - 0 1",
  //     "1Q6/8/8/3BB3/8/4k3/8/4K3 w - - 0 1",
  //     "7r/1r4p1/RpkpN3/2pb1P2/5pq1/8/1PQ2KP1/4R3 w - - 0 1",
  //     "8/1b6/2kp4/2pN4/4P1r1/8/2BK4/8 w - - 0 1",
  //     "8/3nb3/3pk3/5n2/1q6/4P3/3NK3/B5Q1 w - - 0 1",
  //     "2r2k2/pp2b2p/3p4/2rB4/7P/8/PPP2P2/2KR2R1 w - - 0 1",
  //     "1kn5/n5p1/6P1/2N5/1p6/1P6/1K2b1B1/2R5 w - - 0 1",
  //     "7R/3k4/3pb3/2p5/P2b4/1Pn5/5rQ1/2K5 w - - 0 1",
  //     "3q1rrb/2p2pkp/1p2b1p1/p5PP/8/2NB4/PPPQ1P2/2KR2R1 w - - 0 1",
  //     "r2q1b1r/pppk2pp/2np1n2/8/2B5/2N2N2/PPP1QPPP/R4RK1 w - - 0 1",
  //     "r1bq1r2/pppn1kbp/3p1np1/8/4QB2/2N5/PPP2PPP/3RRBK1 w - - 0 1",
  //     "1nr1r3/pbkq1ppp/2pp4/8/5N2/5Q2/PPPB1PPP/3R1RK1 w - - 0 1",
  //     "2rk4/2p5/2B1P3/5r2/8/5q2/5P2/4R1K1 w - - 0 1",
  //     "q2b4/1R6/1PKP1p1k/2PB1P1p/8/7P/8/6R1 w - - 0 1",
  //     "rnb1r1kb/pppq1p2/4p2p/2B4P/6N1/2NP4/PPPQ1P2/R5RK w - - 0 1",
  //     "1kr1r3/2p1q1bp/3p2p1/p4p2/P7/3P2P1/1BPQ1PBP/1R3RK1 w - - 0 1",
  //     "rn1qkb1r/1bpp1ppp/p3P3/1p6/3P1Bn1/1BN2p2/PPP2PPP/R2QR1K1 w kq - 0 1",
  //     "2r2r1k/2q1p2p/p2pbPp1/1p6/3B2PP/8/PPPQN3/1K1R3R w - - 0 1",
  //     "3r1rk1/ppq2p2/5Bp1/4p2p/4Q3/1BN5/PPP2PPP/6K1 w - - 0 1",
  //     "r3k2r/pppqBp1p/3p2p1/2nN1b2/2P5/P7/1P1Q1PPP/R3R1K1 w - - 0 1",
  //     "3r2rk/pp3pbp/4p1p1/2pqN3/8/1P1P3P/PBP2PP1/3QR1K1 w - - 0 1",
  //     "bk6/p1p5/Pp3p2/5nq1/8/8/1P2QPP1/2R3K1 w - - 0 1",
  //     "2kr1b1r/p2nq1pp/1p6/3p1b2/3Q1B2/4PN2/PPP1BPPP/R3K2R w KQ - 0 1",
  //     "3rr1kb/ppq2p2/2p3NB/4p2P/4B3/8/PPP1Q3/2K3R1 w - - 0 1",
  //     "r4bk1/pbp2p2/1pq1p3/4Q3/2P5/1P4P1/PB2NP1P/3R2K1 w - - 0 1",
  //     "2krr3/1p3ppp/p4n2/5b2/1bN1pB2/2N4P/PPP2PP1/3R1RK1 w - - 0 1",
  //     "r4r1k/ppp2pp1/2n4p/2b3N1/3q4/2N5/PPQ2PPP/R3R1K1 w - - 0 1",
  //     "r2qkb1r/pp1bn1pp/2n5/4pp2/4N3/1P3N2/PBPPBPPP/R2Q1RK1 w kq - 0 1",
  //     "6k1/5pp1/2r4p/8/6nq/3Q4/1B3PPP/4R1K1 w - - 0 1",
  //     "4rkn1/1p3p2/2pP3P/8/3B4/8/P4PP1/2R3K1 w - - 0 1",
  //     "3r1rk1/1Q3p2/4p3/p3P3/1pqn1P2/5P2/PPPR4/1K5R w - - 0 1",
  //     "rnbq3r/1pp1b1pp/p2k4/5Q2/6P1/2N4P/PPPP1P2/R1B2RK1 w - - 0 1",
  //     "r2qk2r/pbpp2p1/1pn1pn1p/8/4QP2/BPN4P/P1PP2P1/2KR1B1R w kq - 0 1",
  //     "1r1q1rk1/4p1p1/p1bpP2p/1p6/5n2/2PQ4/PPB2PPP/R3R1K1 w - - 0 1",
  //     "r4nr1/5bkp/6p1/1p1p4/1nq5/P4PB1/1PP1NQP1/1K1R3R w - - 0 1",
  //     "1nkr2r1/2pn1p2/p7/8/P1NP2q1/6Pp/1P1Q1P1P/1BR1R1K1 w - - 0 1",
  //     "2r2rk1/p1qn1p2/1p2p3/8/4P3/1P3P2/PBP1Q1P1/2K4R w - - 0 1",
  //     "4rk2/1bp3p1/5p2/p7/2B1rN2/1P4P1/P4P1P/3R2K1 w - - 0 1",
  //     "5rkr/8/8/8/8/8/8/1Q4K1 w - - 0 1",
  //     "3bkr2/R7/8/7N/8/8/8/7K w - - 0 1",
  //     "3qr3/2p1k3/8/2N1P3/8/8/6Q1/7K w - - 0 1",
  //     "rn1q1b1r/ppp1kBpp/3p4/4N3/8/2P5/PPP2PPP/R1Bb1RK1 w - - 0 1",
  //     "3q1b1r/4kBpp/3p4/4N3/8/2N5/5PPP/6K1 w - - 0 1",
  //     "1r1q1r1k/6pp/5p2/4N3/2B5/8/1PP5/2K4R w - - 0 1",
  //     "6k1/6pn/6N1/8/8/5BP1/6KP/8 w - - 0 1",
  //     "rn1qkbnr/ppp2ppp/8/8/4Np2/5b2/PPPPQ1PP/R1B1KB1R w KQkq - 0 1",
  //     "5nkr/5ppp/8/3N4/8/8/5PPP/6K1 w - - 0 1",
  //     "3qkb1r/3pn1pp/8/5N2/8/8/5PPP/4KB1R w - - 0 1",
  //     "r1bqkb1r/pp1npppp/2p2n2/8/3PN3/8/PPP1QPPP/R1B1KBNR w KQkq - 0 1",
  //     "r1bq2nr/pppnb1pp/3p1k2/7Q/4P3/8/PPP2PPP/RNB1K2R w KQ - 0 1",
  //     "1k3br1/p1p2p1p/2P5/8/8/8/5P2/3R1K2 w - - 0 1",
  //     "3B1r1k/5p1p/4b2N/8/8/4n3/6PP/3R3K w - - 0 1",
  //     "5rk1/5p1p/5Bn1/8/6N1/8/7P/7K w - - 0 1",
  //     "r3kb1r/1p3ppp/8/3np1B1/1p6/3B4/PP3PPP/R3K2R w KQkq - 0 1",
  //     "5qk1/6np/8/5nN1/8/7Q/7P/7K w - - 0 1",
  //     "6k1/6np/6N1/5N2/8/8/7P/7K w - - 0 1",
  //     "r4r1k/5p1p/8/8/7B/8/7P/6RK w - - 0 1",
  //     "6kr/4qp1p/5p1B/8/8/8/2P5/2KR4 w - - 0 1",
  //     "6k1/5p1p/5p1B/8/8/r7/2PP3R/3KR3 w - - 0 1",
  //     "5k2/3r1ppp/8/3Q4/8/8/PPP1R3/1K2R3 w - - 0 1",
  //     "q2r3k/1pp2Q2/8/8/8/8/1P6/2KR4 w - - 0 1",
  //     "5kr1/5p1p/5P1p/8/8/4B3/PP4r1/1K2R3 w - - 0 1",
  //     "r1bq1knr/ppp3pp/2n5/3p3Q/3P1B2/2N5/PPP3PP/R4RK1 w - - 0 1",
  //     "nkq1rr2/1bp1n1pp/p7/1pB1p3/8/2NBQ3/PPP2PPP/R4RK1 w - - 0 1",
  //     "2r3r1/pp3p1k/1bn1qP2/3pN3/3R4/8/PPPQ2PP/2K2R2 w - - 0 1",
  //     "r2qk2r/pbppPppp/1p6/8/2P2n1Q/BP6/P4PPP/3RR1K1 w - - 0 1",
  //     "r2qkr2/pbppP1pp/1p3B2/3P2b1/2P2p2/1P6/P4PPP/3RR1K1 w - - 0 1",
  //     "r2q1rk1/pb1pqn1p/1b1Np1P1/2n1Bp2/7P/8/PPP1QP2/2KR2R1 w - - 0 1",
  //     "8/3B1k2/2Pb3K/8/5p2/5q2/8/Q7 w - - 0 1",
  //     "rr6/pk6/p3b1p1/P3Bp2/4nP2/4P2p/1K5P/2R4R w - - 0 1",
  //     "4R3/6p1/4QpPk/3q1P1p/1p6/1P3P1K/2p5/3r4 w - - 0 1",
  //     "1r6/k1p2p2/6p1/2N4p/1PNn2nP/5bP1/KP3P2/R7 w - - 0 1",
  //     "1kb3r1/1p5p/1pp4Q/3p1pp1/3PqP2/2P3KP/1P4PB/R7 w - - 0 1",
  //     "6kb/1p5p/1p5B/r1pK3P/P1B5/6n1/2P3P1/1R6 w - - 0 1",
  //     "r2r1k2/1p4RR/p1b1pP2/2n5/8/2Np4/PP4PP/6K1 w - - 0 1",
  //     "3r1bk1/1p6/p4Pp1/6N1/1P6/P2p3P/2r3P1/B2R2K1 w - - 0 1",
  //     "r2q2kn/ppp2R1R/2np2p1/7p/2P4P/1PN3P1/P1BB1P2/6K1 w - - 0 1",
  //     "3k1r2/6R1/p2N3p/Pp1P2bP/1Pn5/4pP1B/4K3/8 w - - 0 1",
  //     "2kr3r/2pb3p/p4np1/1N1Pp3/4Pp2/7P/P1P2PP1/1R3RK1 w - - 0 1",
  //     "r3k2r/pp1b3p/1qnP2P1/5pN1/2p1p3/2Q3P1/PPP2P1P/R4RK1 w - - 0 1",
  //     "4r3/1p3p2/p1nq2kp/P3p1n1/1PBr4/5Q2/4NP1P/2R3RK w - - 0 1",
  //     "r3r2k/2p4p/pp2b3/5p2/P2R2p1/1PB5/2P2PPP/3R2K1 w - - 0 1",
  //     "4N1rb/1p2p3/2n1R1Pk/2P5/7K/3B4/8/8 w - - 0 1",
  //     "r2q1rk1/1p5p/p1n1b1p1/3p1N2/8/1P6/PBPQ1PPP/R2R2K1 w - - 0 1",
  //     "1kb3r1/5p2/1pP5/1P5p/3p2rP/B2P2P1/6K1/R3R3 w - - 0 1",
  //     "5K1k/7b/8/8/pr6/2P5/1B4p1/7R w - - 0 1",
  //     "4r3/7p/p5bP/6R1/k7/3p4/1KP5/3B4 w - - 0 1",
  //     "5r2/7p/p5bP/6R1/k7/8/1KP2r2/3BB3 w - - 0 1",
  //     "8/8/p5p1/q6p/kb1p1P1P/8/1KP2Q2/3B4 w - - 0 1",
  //     "3R4/r4pk1/8/r4PP1/1p6/8/PKB5/8 w - - 0 1",
  //     "4r3/p2nr1pp/1p1k1p2/n4P2/1KPP4/2N1P1P1/P5BP/1R2R3 w - - 0 1",
  //     "2r2r2/p2b2p1/1p1p1k1p/2n2n1P/8/P1N5/BPP2P2/2K1R1R1 w - - 0 1",
  //     "r5k1/2p4p/p3P1P1/8/5b2/1BB2P2/PP4rP/1K2R3 w - - 0 1",
  //     "1kb3r1/2R2p2/1p2p3/pN2B2p/5nq1/8/PP1Q2P1/6K1 w - - 0 1",
  //     "5n1r/7k/1p5p/p1p4P/P1B2p2/5P2/1P2R1P1/2K5 w - - 0 1",
  //     "1r4n1/4Bk2/5P1p/5P2/2P5/p2K1B2/1p6/1R6 w - - 0 1",
  //     "6R1/ppkb3p/2pb4/5p1q/8/1PN3P1/P1Q2PKP/4r3 w - - 0 1",
  //     "5r2/p7/8/7R/1k6/1P3p2/PK6/8 w - - 0 1",
  //     "8/p6r/P1RN2bk/7p/5K2/6P1/8/8 w - - 0 1",
  //     "8/p5br/P1RN2bk/8/6P1/6K1/8/8 w - - 0 1",
  //     "4r3/p3r3/k5b1/b5p1/2P2nP1/5Q1P/1P2B3/R4K2 w - - 0 1",
  //     "4r3/p3r3/k6p/b5pP/2PP1q2/4pQN1/1P2B1P1/3K3R w - - 0 1",
  //     "1kr4r/p1ppb1pp/1p3q2/P3N3/3P1n2/1P3B2/R1PQ1PPP/4R1K1 w - - 0 1",
  //     "3r1r1b/pppq1pkp/2n1p1P1/4P3/4B3/3P1p2/PPPQ1P1P/2KR2R1 w - - 0 1",
  //     "3k1r2/1p1pn1p1/r5qp/2p3B1/2Nb4/3P2Q1/1PP3PP/1K1R3R w - - 0 1",
  //     "r1bqk1r1/pp1pn1pp/2p5/5p2/2QP4/2N2N1P/PPP2PP1/R1B1R1K1 w q - 0 1",
  //     "8/3P4/4p3/3kr3/8/5K2/7p/2R5 w - - 0 1",
  //     "8/8/p7/kb6/8/1K4B1/1P6/8 w - - 0 1",
  //     "8/8/7p/R2B2nk/6p1/2r3P1/6K1/8 w - - 0 1",
  //     "4r3/p5p1/kb6/3N3R/5Pn1/2P5/1PK3B1/8 w - - 0 1",
  //     "1r3r2/2q2p1k/4pP2/4n2B/p3P3/8/1PPQ4/1K1R3R w - - 0 1",
  //     "r4rk1/6bn/3p2Np/5Np1/6P1/8/PP5P/2KR1R2 w - - 0 1",
  //     "r3kr2/ppp2p2/1n1p1B2/n1N1P3/3P2p1/6Pq/PPP2Q1P/3RR1K1 w q - 0 1",
  //     "2rr4/2pk1Bp1/1p6/1PP5/1B1p1p2/5P2/5K2/6R1 w - - 0 1",
  //     "2r1b1r1/2p2k1p/2N1p3/3qB1pP/p3NP2/2P2R2/1PK3P1/8 w - - 0 1",
  //     "3rr3/2pnk2p/1p1p4/p2P1p2/4p2q/PPQ5/2P2PBP/3R1R1K w - - 0 1",
  //     "2n3r1/k4p1p/b4q2/p1p5/P2p1B2/3P4/2PQ1PB1/1R4K1 w - - 0 1",
  //     "2qr2k1/3N1pPp/3p2pB/p2P4/P7/7P/5PP1/4R1K1 w - - 0 1",
  //     "5r2/2p5/3k4/1p1P2b1/2PK4/7B/8/4R3 w - - 0 1",
  //     "8/7p/6pk/1p6/2p3PP/2Br1BK1/1P6/8 w - - 0 1",
  //     "6R1/2p1kp1p/5n2/2PPK3/p4P1r/B7/8/8 w - - 0 1",
  //     "R7/2pk4/1pN3p1/1Pn3P1/2Q3bP/5q2/1P2r3/2K5 w - - 0 1",
  //     "4rq2/pp2r2P/1k4p1/n2P1pP1/3K3R/3B2B1/2PQ4/7R w - - 0 1",
  //     "3r1nr1/1pqb1p1k/pn2p1p1/R7/8/1P1B1N1P/1BP2PP1/3QR1K1 w - - 0 1",
  //     "8/8/3B1rp1/2P2kb1/7p/1p1K4/1P4B1/8 w - - 0 1",
  //     "2b1r2r/ppq1kp1p/1np1pn1B/8/3N4/2P4P/PP3PP1/R2QRBK1 w - - 0 1",
  //     "1k2r3/p2nq1pp/BpP2b2/8/2R5/5Q1P/P2N1PP1/6K1 w - - 0 1",
  //     "1k6/pq6/4Np2/4bP2/2p2B2/P1P3P1/KQ4r1/7R w - - 0 1",
  //     "8/3p2p1/p1p1k1P1/P3N1P1/2PKR1P1/7r/8/5b2 w - - 0 1",
  //     "QR4rk/6p1/4q1p1/5p2/p7/P7/1P6/K7 w - - 0 1",
  //     "6br/1p2P1k1/2p3Pp/2Br1P2/8/1NK5/2P5/8 w - - 0 1",
  //     "3r2bk/p5qp/1p6/2p1NpN1/2P1nP2/1P6/PB4P1/K6R w - - 0 1",
  //     "6k1/1p2Pp1r/2b2P1Q/2p5/P7/5qP1/1P6/2R3K1 w - - 0 1",
  //     "6b1/5P1k/3K3p/8/3B4/8/6p1/8 w - - 0 1",
  //     "2r5/5Knk/3p3B/3Pp3/4P3/1Pp5/8/7R w - - 0 1",
  //     "2r4r/1k6/b2p2q1/pp1Pp2p/4Pp2/PP3P2/1K1R1QP1/6BR w - - 0 1",
  //     "3rr1k1/2p2p2/p1q3p1/1pP2n2/1P2p3/P1B4R/2Q2PPP/3R2K1 w - - 0 1",
  //     "3rr3/pp1q1p1p/2p1b1pk/3p4/3Q2PP/1P3B2/P1P1RPK1/4R3 w - - 0 1",
  //     "1nr5/ppkn4/2p2p2/6p1/3N2Pp/4B2P/PPPR1P2/6K1 w - - 0 1",
  //     "7r/4K3/R7/4k3/4p3/5p2/4N3/8 w - - 0 1",
  //     "8/5p2/p5p1/kp3P2/4n1PB/PP6/2K5/8 w - - 0 1",
  //     "8/rn4p1/1k4P1/p2P4/Pp3N2/1K6/2R5/8 w - - 0 1",
  //     "8/3nr3/B2bkp2/4p3/1P4P1/8/2K1N3/7R w - - 0 1",
  //     "8/1R6/3bkp2/4pn2/3pK1P1/3P4/8/8 w - - 0 1",
  //     "r2q1k1r/pp2b1pp/3p1n2/4p1N1/4P1b1/PQN1B3/1P3PPP/3RR1K1 w - - 0 1",
  //     "2r2k1r/pbq1pp1p/1pn2N2/2n3B1/3p4/1P1P1PP1/P2NQ2P/1K2R2R w - - 0 1",
  //     "r1b1k2r/1ppp3p/p3pQ2/1NP5/1PK5/P3Pn1P/5P2/8 w - - 0 1",
  //     "4k3/4Pp2/3P4/8/8/5Q2/1K6/4q3 w - - 0 1",
  //     "4k3/8/5PN1/3Q4/8/5p2/4n2K/5q2 w - - 0 1",
  //     "5r2/k7/Np6/7p/1P2b1pP/4Pr2/R4PKB/8 w - - 0 1",
  //     "8/8/8/8/8/5N2/1pr3PP/r1k1K2R w K - 0 1",
  //     "rb6/k1p4R/P1P5/PpK5/8/8/8/5B2 w - b6 0 1",
  // };

  std::vector<std::string> polgars;

  nlohmann::json problems = nlohmann::json::parse(chess::LoadFile("/home/guille/chess/problems.json"));
  for (const auto& problem : problems["problems"]) {
    polgars.push_back(problem["fen"]);
  }

  absl::BitGen gen;

  while(true) {
    int pos = absl::Uniform(gen, 0U, polgars.size());
    std::string_view fen = polgars[pos];
    std::string url =
        absl::StrFormat("https://lichess.org/analysis/%s?color=white",
                        absl::StrJoin(absl::StrSplit(fen, ' '), "_"));
    std::cout << "Showing Polgar's #" << pos +1 << ". Lichess: " << url << '\n';
    auto board = chess::FromFen(polgars[pos]);

    if (!board.ok()) {
      std::cout << "Error: " << board.status() << '\n';
    }

    board->Print();
    std::cout << "-----------------------------------------------------------\n";
    getchar();
  }

  return 0;
}
