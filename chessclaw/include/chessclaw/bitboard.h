// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/types.h"
#include <cstdint>
namespace chessclaw {
using Bitboard = std::uint64_t;
inline constexpr Bitboard bb(Square s){ return 1ULL << s; }
inline int popcount(Bitboard b){ return __builtin_popcountll(b); }
inline Square poplsb(Bitboard& b){ Square s=static_cast<Square>(__builtin_ctzll(b)); b&=b-1; return s; }
inline Square lsb(Bitboard b){ return static_cast<Square>(__builtin_ctzll(b)); }
extern Bitboard kKnightAttacks[64];
extern Bitboard kKingAttacks[64];
extern Bitboard kPawnAttacks[2][64];
// Magic bitboards: O(1) sliding attacks. Tables built once at init.
struct Magic { std::uint64_t mask, magic; std::uint32_t offset; int shift; };
extern Magic kBishopMagics[64];
extern Magic kRookMagics[64];
extern Bitboard gBishopAttacks[];   // flattened: sum of bishop table sizes
extern Bitboard gRookAttacks[];     // flattened: sum of rook table sizes
void initAttacks();                 // builds magics + tables
inline Bitboard bishopAttacks(Square s, Bitboard occ){
 const Magic& m=kBishopMagics[s];
 return gBishopAttacks[m.offset + ((((occ&m.mask)*m.magic) >> m.shift))];
}
inline Bitboard rookAttacks(Square s, Bitboard occ){
 const Magic& m=kRookMagics[s];
 return gRookAttacks[m.offset + ((((occ&m.mask)*m.magic) >> m.shift))];
}
inline Bitboard queenAttacks(Square s, Bitboard occ){ return bishopAttacks(s,occ)|rookAttacks(s,occ); }
} // namespace chessclaw
