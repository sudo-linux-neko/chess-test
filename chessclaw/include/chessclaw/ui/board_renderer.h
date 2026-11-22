// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/position.h"
#include "chessclaw/ui/double_buffer.h"
#include <string>
#include <vector>
namespace chessclaw::ui {
struct BoardTheme { bool unicode{true}; };
// Anime-night board: neon-edged Tokyo-night squares, glowing pieces.
// frame/t drive shimmer + highlight pulse. lastMoveFrom/To drive trail.
void renderBoard(DoubleBuffer& db, const Position& pos, int top, int left, Move highlightMove=MOVE_NONE, int highlightSq=-1, double animT=0.0, bool checkFlash=false);
void renderBoardAscii(DoubleBuffer& db, const Position& pos, int top, int left);
std::vector<std::string> boardCellGlyph(Piece p, bool dark, bool unicode);
// Live demo position: plays N plies of real engine self-play into outPos and
// returns UCI move list. Used by train-show so the board pieces MOVE live.
std::vector<std::string> demoLivePosition(Position& outPos, int plies, int depth, std::uint64_t seed);
} // namespace chessclaw::ui
