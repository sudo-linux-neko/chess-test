// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/position.h"
namespace chessclaw {
// Classical evaluation (centipawns, White perspective positive).
int evaluateClassical(const Position& pos);
// Full hybrid evaluation: classical + NNUE if loaded (side-to-move perspective? No: white perspective).
int evaluate(const Position& pos);
// Endgame scale helper exposed for tests.
bool isEndgame(const Position& pos);
int pieceValueMg(PieceType t);
int pieceValueEg(PieceType t);
} // namespace chessclaw
