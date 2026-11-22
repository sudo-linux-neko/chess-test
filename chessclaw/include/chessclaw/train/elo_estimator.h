// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include <string>
#include <vector>
namespace chessclaw::train {
struct EloPoint { double elo; double err; int wins, draws, losses; };
// Elo vs baseline from W/D/L using logistic (400 scale). Returns estimate labeled as estimate.
EloPoint eloFromScore(int w,int d,int l);
std::string eloLabel(const EloPoint& e);
} // namespace chessclaw::train
