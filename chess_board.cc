#include "chess_board.h"

#include <array>
#include <fstream>
#include <optional>

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

auto PieceListString(const std::vector<BoardPiece>& pieces,
                     const PieceType type,
                     const Color color,
                     ChessLanguage language) -> std::string {
  std::vector<std::string> piece_strings;
  for (const auto& piece : pieces) {
    if (piece.piece.type != type || piece.piece.color != color) {
      continue;
    }
    piece_strings.push_back(ToString(piece, language));
  }

  return absl::StrJoin(piece_strings, ", ");
}

}  // namespace

auto BoardPosition::FromString(std::string_view str)
    -> absl::StatusOr<BoardPosition> {
  BoardPosition ret;

  if (str.size() != 2) {
    return absl::InvalidArgumentError(
        "Position must consist of exactly two chacaters.");
  }

  ret.file = 1 + str[0] - 'a';
  ret.rank = str[1] - '0';

  if (!ret.IsValid()) {
    return absl::InvalidArgumentError(
        absl::StrFormat("Invalid position string '%s'", str));
  }

  return ret;
}

auto ToString(Color color) -> std::string {
  return color == Color::White ? "white" : "black";
}

auto FromFenPieceChar(char c) -> absl::StatusOr<Piece> {

  Piece ret;
  ret.color = absl::ascii_isupper(c) ? Color::White : Color::Black;

  c = absl::ascii_tolower(c);

  if (c == 'r') {
    ret.type = PieceType::Rook;
  } else if (c == 'n') {
    ret.type = PieceType::Knight;
  } else if (c == 'b') {
    ret.type = PieceType::Bishop;
  } else if (c == 'q') {
    ret.type = PieceType::Queen;
  } else if (c == 'k') {
    ret.type = PieceType::King;
  } else if (c == 'p') {
    ret.type = PieceType::Pawn;
  } else {
    return absl::InvalidArgumentError(
        absl::StrFormat("Invalid FEN piece type: '%c'", c));
  }

  return ret;
}

auto ToString(PieceType type, ChessLanguage language) -> std::string {
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
  } else if (language == ChessLanguage::kSpanish) {
    switch (type) {
      case PieceType::Rook:
        return "T";
      case PieceType::Knight:
        return "C";
      case PieceType::Bishop:
        return "A";
      case PieceType::Queen:
        return "D";
      case PieceType::King:
        return "R";
      case PieceType::Pawn:
        return "P";
      default:
        return "";
    }
  } else if (language == ChessLanguage::kEnglishFull) {
    switch (type) {
      case PieceType::Rook:
        return "rook";
      case PieceType::Knight:
        return "knight";
      case PieceType::Bishop:
        return "bishop";
      case PieceType::Queen:
        return "queen";
      case PieceType::King:
        return "king";
      case PieceType::Pawn:
        return "pawn";
      default:
        return "";
    }
  }

  return "";
}

auto ToString(const BoardPosition& position) -> std::string {
  std::string ret = "aa";
  ret[0] = FileToLetter(position.file);
  ret[1] = '0' + position.rank;
  return ret;
}

auto ToString(const BoardPiece& piece, ChessLanguage language) -> std::string {
  std::string ret = absl::StrCat(ToString(piece.piece.type, language),
                                 ToString(piece.position));

  if (piece.piece.color == Color::Black) {
    ret = absl::AsciiStrToLower(ret);
  }
  return ret;
}

auto BoardPiece::FromString(std::string_view str)
    -> absl::StatusOr<BoardPiece> {
  BoardPiece ret;

  if (str.size() != 3) {
    return absl::InvalidArgumentError(
        "BoardPiece string must consist of three characters");
  }

  auto piece_or = FromFenPieceChar(str[0]);
  if (!piece_or.ok()) {
    return piece_or.status();
  }

  ret.piece = *piece_or;

  str.remove_prefix(1);
  auto position_or = BoardPosition::FromString(str);
  if (!position_or.ok()) {
    return position_or.status();
  }
  ret.position = *position_or;

  return ret;
}

// https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
auto ChessBoard::FromFen(std::string_view fen) -> absl::StatusOr<ChessBoard> {
  std::vector<std::string_view> parts = absl::StrSplit(fen, ' ');

  if (parts.empty()) {
    return absl::InvalidArgumentError(
        "FEN Must contain at least piece placement data");
  }

  ChessBoard ret;
  if (parts.size() >= 2) {
    ret.white_to_move_ = parts[1] == "w";
  }

  std::string_view ppd = parts[0];

  std::vector<std::string_view> ranks = absl::StrSplit(ppd, '/');

  if (ranks.size() != 8) {
    return absl::InvalidArgumentError("There must be 8 ranks in the FEN");
  }

  int rank = 8;
  for (auto rank_str : ranks) {
    if (rank_str.size() > 8) {
      return absl::InvalidArgumentError(absl::StrFormat(
          "FEN rank must have at most 8 files, has %d", rank_str.size()));
    }
    int file = 1;
    for (const char current_char : rank_str) {
      if (absl::ascii_isdigit(current_char)) {
        file += current_char - '0';
        continue;
      }

      auto piece_or = FromFenPieceChar(current_char);
      if (!piece_or.ok()) {
        return piece_or.status();
      }
      const auto piece_type = *piece_or;

      BoardPiece board_piece;
      board_piece.piece = piece_type;
      board_piece.position.file = file;
      board_piece.position.rank = rank;

      ret.pieces_.push_back(board_piece);

      ++file;
    }
    --rank;
  }

  return ret;
}

auto LoadFenFile(const std::filesystem::path& path)
    -> absl::StatusOr<std::vector<ChessBoard>> {
  std::vector<ChessBoard> ret;

  nlohmann::json problems = nlohmann::json::parse(chess::LoadFile(path));
  for (const auto& problem : problems["problems"]) {
    auto board = ChessBoard::FromFen(problem["fen"].get<std::string>());
    if (!board.ok()) {
      return board.status();
    }

    if (problem.contains("info")) {
      const auto& field = problem["info"];
      if (field.is_string()) {
        board->SetInfo(field.get<std::string>());
      }
    }

    ret.push_back(*board);
  }

  return ret;
}

auto ChessBoard::SetInfo(const std::string& info) -> void { info_ = info; }

auto ChessBoard::Info() const -> std::string { return info_; }

auto ChessBoard::Print(bool show_info, ChessLanguage language) const -> void {
  std::cout << "FEN: " << Fen() << '\n';
  constexpr auto kPieceTypes = std::array{
      PieceType::King,   PieceType::Queen,  PieceType::Rook,
      PieceType::Bishop, PieceType::Knight, PieceType::Pawn,
  };
  std::cout << "White:\n";
  for (const auto& type : kPieceTypes) {
    const auto str = PieceListString(pieces_, type, Color::White, language);
    if (!str.empty()) {
      std::cout << " - " << str << '\n';
    }
  }

  std::cout << "Black:\n";
  for (const auto& type : kPieceTypes) {
    const auto str = PieceListString(pieces_, type, Color::Black, language);
    if (!str.empty()) {
      std::cout << " - " << str << '\n';
    }
  }

  if (white_to_move_) {
    std::cout << "White to move\n";
  } else {
    std::cout << "Black to move\n";
  }

  if (!info_.empty() && show_info) {
    std::cout << "Info: " << info_ << '\n';
  }
  const std::string analysis_url =
      absl::StrFormat("https://lichess.org/analysis/%s?color=white",
                      absl::StrJoin(absl::StrSplit(Fen(), ' '), "_"));
  std::cout << "Analysis: " << analysis_url << '\n';
}

auto ChessBoard::Fen() const -> std::string {
  std::vector<std::vector<std::optional<Piece>>> full_board;
  full_board.resize(8);
  for (auto& rank : full_board) {
    rank.resize(8);
  }

  for (const auto& board_piece : pieces_) {
    const int rank = 8 - board_piece.position.rank;
    const int file = board_piece.position.file - 1;

    full_board[rank][file] = board_piece.piece;
  }

  std::vector<std::string> rank_strings;
  for (const auto& rank : full_board) {
    std::string rank_str;

    int prev_file = 1;
    int curr_file = 1;
    bool was_empty = true;
    for (const auto& piece : rank) {
      if (piece.has_value()) {
        was_empty = false;
        std::string piece_str = ToString(piece->type, ChessLanguage::kEnglish);
        if (piece->color == Color::White) {
          piece_str = absl::AsciiStrToUpper(piece_str);
        } else {
          piece_str = absl::AsciiStrToLower(piece_str);
        }

        if (prev_file < curr_file) {
          absl::StrAppend(&rank_str, curr_file - prev_file);
        }
        absl::StrAppend(&rank_str, piece_str);
        ++curr_file;
        prev_file = curr_file;
      } else {
        ++curr_file;
      }
    }
    if (prev_file < curr_file) {
      absl::StrAppend(&rank_str, curr_file - prev_file);
    }
    if (was_empty) {
      rank_str = "8";
    }

    rank_strings.push_back(rank_str);
  }

  const std::string to_play = white_to_move_ ? "w" : "b";

  return absl::StrFormat("%s %s - - 0 1", absl::StrJoin(rank_strings, "/"),
                         to_play);
}

void ChessBoard::Rotate() {
  white_to_move_ = !white_to_move_;

  for (auto& piece : pieces_) {
    piece.position.rank = 9 - piece.position.rank;
    piece.position.file = 9 - piece.position.file;
    piece.piece.color =
        piece.piece.color == Color::Black ? Color::White : Color::Black;
  }
}

auto ChessBoard::AtPosition(const BoardPosition& position) const
    -> std::optional<Piece> {
  for (const auto& board_piece : pieces_) {
    if (board_piece.position == position) {
      return board_piece.piece;
    }
  }
  return std::nullopt;
}

auto ChessBoard::BoardPieces() const -> std::vector<BoardPiece> {
  return pieces_;
}

}  // namespace chess
