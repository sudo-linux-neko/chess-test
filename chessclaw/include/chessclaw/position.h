// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/types.h"
#include "chessclaw/bitboard.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace chessclaw {
struct UndoState {
  int castling{0};
  Square ep{SQ_NONE};
  int halfmove{0};
  Piece captured{Piece::None};
  std::uint64_t hash{0};
};
class Position {
public:
  Position();
  void setStartpos();
  bool setFen(const std::string& fen);
  std::string fen() const;
  Color sideToMove() const { return stm_; }
  Bitboard occupancy() const;
  Bitboard pieces(Color c) const;
  Bitboard pieces(Color c, PieceType t) const;
  Piece pieceOn(Square s) const { return board_[s]; }
  int castling() const { return castling_; }
  Square epSquare() const { return ep_; }
  int halfmove() const { return halfmove_; }
  int fullmove() const { return fullmove_; }
  std::uint64_t hash() const { return hash_; }
  Square kingSquare(Color c) const;
  bool inCheck() const;
  bool isAttacked(Square s, Color by) const;
  void doMove(Move m, UndoState& u);
  void undoMove(Move m, const UndoState& u);
  void doNull(UndoState& u);
  void undoNull(const UndoState& u);
  bool isLegal(Move m) const; // make + check king safety
  bool isRepetition(int plyFromRoot, const std::uint64_t* hist, int histLen) const;
private:
  std::array<Piece,64> board_{};
  std::array<Bitboard,2> byColor_{0,0};
  // by piece type index 1..6 combined both colors
  std::array<Bitboard,7> byType_{0,0,0,0,0,0,0};
  Color stm_{Color::White};
  int castling_{0}; // 1=WK 2=WQ 4=BK 8=BQ
  Square ep_{SQ_NONE};
  int halfmove_{0}, fullmove_{1};
  std::uint64_t hash_{0};
  void rebuildHash();
  void clear();
  void putPiece(Square s, Piece p);
  void removePiece(Square s);
  void movePiece(Square f, Square t);
};
void initZobrist();
std::uint64_t zobristPiece(Piece p, Square s);
std::uint64_t zobristSide();
std::uint64_t zobristCastle(int c);
std::uint64_t zobristEp(int f);
} // namespace chessclaw
