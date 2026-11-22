// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - CPU-only chess engine and NNUE self-training pipeline.
// Author: vaubhav
// Core chess types: colors, pieces, squares, moves.
#pragma once
#include <cstdint>
#include <string>

namespace chessclaw {

enum class Color : std::uint8_t { White = 0, Black = 1 };
enum class PieceType : std::uint8_t { None=0, Pawn=1, Knight=2, Bishop=3, Rook=4, Queen=5, King=6 };
enum class Piece : std::uint8_t {
  None=0,
  WP=1, WN=2, WB=3, WR=4, WQ=5, WK=6,
  BP=7, BN=8, BB=9, BR=10, BQ=11, BK=12
};

inline constexpr Color opposite(Color c) { return c==Color::White?Color::Black:Color::White; }
inline constexpr PieceType typeOf(Piece p) {
  switch(p){
    case Piece::WP: case Piece::BP: return PieceType::Pawn;
    case Piece::WN: case Piece::BN: return PieceType::Knight;
    case Piece::WB: case Piece::BB: return PieceType::Bishop;
    case Piece::WR: case Piece::BR: return PieceType::Rook;
    case Piece::WQ: case Piece::BQ: return PieceType::Queen;
    case Piece::WK: case Piece::BK: return PieceType::King;
    default: return PieceType::None;
  }
}
inline constexpr Color colorOf(Piece p) {
  if(p==Piece::None) return Color::White;
  return (static_cast<int>(p) <= 6) ? Color::White : Color::Black;
}
inline constexpr Piece makePiece(Color c, PieceType t) {
  if(t==PieceType::None) return Piece::None;
  int base = (c==Color::White)?0:6;
  return static_cast<Piece>(base + static_cast<int>(t));
}

using Square = std::uint8_t; // 0=a1 .. 63=h8
inline constexpr int fileOf(Square s){ return s & 7; }
inline constexpr int rankOf(Square s){ return s >> 3; }
inline constexpr Square makeSquare(int f,int r){ return static_cast<Square>(r*8+f); }
inline constexpr Square SQ_NONE = 64;
inline constexpr Square SQ_A1=0,SQ_H1=7,SQ_A8=56,SQ_H8=63;
inline constexpr Square SQ_E1=4,SQ_G1=6,SQ_C1=2,SQ_E8=60,SQ_G8=62,SQ_C8=58;

// Move: packed 32-bit: from(6) to(6) promo(3) flags(5) + score handled separately.
struct Move {
  std::uint16_t data{0}; // bits: from 0-5, to 6-11, promo 12-14, flags 15
  // flags in extra byte:
  std::uint8_t flags{0};
  std::int16_t score{0}; // ordering score (not part of identity)
  enum Flag : std::uint8_t {
    Quiet=0, Capture=1, DoublePush=2, EnPassant=3,
    CastleK=4, CastleQ=5, PromoN=6, PromoB=7, PromoR=8, PromoQ=9,
    CapPromoN=10, CapPromoB=11, CapPromoR=12, CapPromoQ=13
  };
  constexpr Move(){}
  constexpr Move(Square f,Square t,std::uint8_t fl=Quiet):data(static_cast<std::uint16_t>(f|(t<<6))),flags(fl){}
  constexpr Square from() const { return static_cast<Square>(data & 63); }
  constexpr Square to() const { return static_cast<Square>((data>>6)&63); }
  constexpr bool isNone() const { return data==0 && flags==0; }
  constexpr bool isCapture() const { return flags==Capture||flags==EnPassant||(flags>=CapPromoN&&flags<=CapPromoQ); }
  constexpr bool isPromo() const { return flags>=PromoN; }
  constexpr PieceType promoType() const {
    switch(flags){
      case PromoN: case CapPromoN: return PieceType::Knight;
      case PromoB: case CapPromoB: return PieceType::Bishop;
      case PromoR: case CapPromoR: return PieceType::Rook;
      case PromoQ: case CapPromoQ: return PieceType::Queen;
      default: return PieceType::None;
    }
  }
  constexpr bool isCastle() const { return flags==CastleK||flags==CastleQ; }
  constexpr bool operator==(const Move& o) const { return data==o.data && flags==o.flags; }
  constexpr bool operator!=(const Move& o) const { return !(*this==o); }
};
inline constexpr Move MOVE_NONE{};

std::string moveToUci(Move m);
Move uciToMove(const std::string& s); // from/to only; promo resolved by caller if needed
std::string moveToSan(const std::string& uciMove); // minimal helper (uci echo if unknown)

} // namespace chessclaw
