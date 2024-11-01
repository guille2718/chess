#include "chess_board.h"

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
  return absl::StrCat(ToString(piece.type, language), ToString(piece.position));
}

auto ToString(const ChessBoard& board) -> std::string {
  std::string ret;
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
  ret.fen_ = fen;

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
        ret.white_.push_back(piece);
      } else {
        ret.black_.push_back(piece);
      }

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

auto ChessBoard::SetInfo(const std::string& info) -> void {
  info_ = info;
}

auto ChessBoard::Info() const -> std::string {
  return info_;
}

auto ChessBoard::Print() const -> void {
  const ChessLanguage language = ChessLanguage::kEnglish;
  std::cout << "White: ";
  for (const auto& piece : white_) {
    std::cout << ToString(piece, language) << ", ";
  }
  std::cout << '\n';

  std::cout << "Black: ";
  for (const auto& piece : black_) {
    std::cout << ToString(piece, language) << ", ";
  }
  std::cout << '\n';
  if (white_to_move_) {
    std::cout << "White to move\n";
  } else {
    std::cout << "Black to move\n";
  }

  if (!info_.empty()) {
    std::cout << "Info: " << info_ << '\n';
  }
  const std::string analysis_url =
      absl::StrFormat("https://lichess.org/analysis/%s?color=white",
                      absl::StrJoin(absl::StrSplit(fen_, ' '), "_"));
  std::cout << "Analysis: " << analysis_url << '\n';
}

}  // namespace chess
