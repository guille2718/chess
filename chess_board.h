#pragma once

#include <compare>
#include <filesystem>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <absl/strings/ascii.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_format.h>
#include <absl/strings/str_join.h>
#include <absl/strings/str_split.h>
#include <nlohmann/json.hpp>

namespace chess {

enum class Color : bool {
  White,
  Black,
};

auto ToString(Color color) -> std::string;

enum class PieceType : char {
  Rook,
  Knight,
  Bishop,
  Queen,
  King,
  Pawn,
};

struct Piece {
  PieceType type;
  Color color;

  friend auto operator == (const Piece& a, const Piece& b) -> bool {
    return a.color == b.color && a.type == b.type;
  }
};

auto FromFenPieceChar(char c) -> absl::StatusOr<Piece>;

enum class ChessLanguage : char {
  kSpanish,
  kEnglish,
  kUnicode,
  kEnglishFull,  // Full words for pieces, i.e.: rook, knight, etc.
};

auto ToString(PieceType type,
              ChessLanguage language = ChessLanguage::kEnglish) -> std::string;

struct BoardPosition {
  int file;
  int rank;

  auto IsValid() const -> bool {
    return file >= 1 && file <= 8 && rank >= 1 && rank <= 8;
  }

  static auto FromString(std::string_view str) -> absl::StatusOr<BoardPosition>;

  auto Color() const -> Color {
    return (file + rank) % 2 == 0 ? Color::White : Color::Black;
  }

  friend constexpr auto operator<=>(
      const BoardPosition& a, const BoardPosition& b) -> std::strong_ordering {

    if (a.rank > b.rank) {
      return std::strong_ordering::less;
    }

    if (a.rank < b.rank) {
      return std::strong_ordering::greater;
    }

    return a.file <=> b.file;
  }

  friend auto operator==(const BoardPosition&,
                         const BoardPosition&) -> bool = default;
};

auto ToString(const BoardPosition& position) -> std::string;

// Basically a pair of position & piece.
struct BoardPiece {
  BoardPosition position;
  Piece piece;

  static auto FromString(std::string_view str) -> absl::StatusOr<BoardPiece>;

  friend auto operator ==(const BoardPiece& a,
                          const BoardPiece&b) -> bool {
    return a.position == b.position && a.piece == b.piece;
  }
};

auto ToString(const BoardPiece& piece,
              ChessLanguage language = ChessLanguage::kEnglish) -> std::string;

class ChessBoard {
 public:
  ChessBoard() = default;

  static auto FromFen(std::string_view fen) -> absl::StatusOr<ChessBoard>;

  auto Print(bool show_info = false,
             ChessLanguage language = ChessLanguage::kEnglish) const -> void;

  auto Fen() const -> std::string;

  auto SetInfo(const std::string& info) -> void;
  auto Info() const -> std::string;

  auto AtPosition(const BoardPosition& position) const -> std::optional<Piece>;

  auto BoardPieces() const -> std::vector<BoardPiece>;

  // Changes the board by:
  // 1) Rotating 180 degrees the position of all the pieces.
  // 2) Flipping the color of whose turn it's to play.
  void Rotate();

 private:
  std::vector<BoardPiece> pieces_;
  std::string info_;
  bool white_to_move_ = true;
};

auto LoadFenFile(const std::filesystem::path& path)
    -> absl::StatusOr<std::vector<ChessBoard>>;

}  // namespace chess
