// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/position.h"
#include <vector>
namespace chessclaw {
// Generate pseudo-legal moves; legalOnly filters via Position::isLegal.
void generateMoves(const Position& pos, std::vector<Move>& out, bool legalOnly=true, bool capturesOnly=false);
bool hasLegalMove(const Position& pos);
std::vector<Move> legalMoves(const Position& pos);
// SEE (static exchange evaluation), returns material gain estimate for capture on 'to'.
int see(const Position& pos, Move m);
// MVV-LVA + promo scoring helper.
int moveOrderBaseScore(const Position& pos, Move m);
} // namespace chessclaw
