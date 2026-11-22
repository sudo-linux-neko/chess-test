// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/position.h"
#include "chessclaw/search.h"
#include "chessclaw/ui/double_buffer.h"
#include <string>
#include <vector>
namespace chessclaw::ui {
struct DashStats { double nps{0}; int depth{0}; int scoreCp{0}; double hashOcc{0}; double timeUsedS{0}; double timeLeftS{0}; int threads{1}; std::vector<double> cpu; std::string bestMove; std::vector<std::string> pv; };
void renderDashboard(DoubleBuffer& db, const Position& pos, const std::vector<std::string>& moveHist, const std::vector<double>& eloHist, const DashStats& st, Move bestMove=MOVE_NONE);
void renderMoveHistory(DoubleBuffer& db,int top,int left,int h,int w,const std::vector<std::string>& moves);
} // namespace chessclaw::ui
