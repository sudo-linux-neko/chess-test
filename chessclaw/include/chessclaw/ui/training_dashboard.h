// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/ui/double_buffer.h"
#include "chessclaw/types.h"
#include <string>
#include <vector>
namespace chessclaw::ui {
struct TrainView {
  int epoch{0}, totalEpochs{1}; int batch{0}, totalBatches{1};
  double trainLoss{0}, valLoss{0}, bestVal{1e9}, lr{1e-3};
  double posPerSec{0}, etaSec{0}, elapsedSec{0};
  long long peakMemKb{0};
  std::vector<double> lossHist, valHist, lrHist, cpu;
  std::vector<double> eloHist;
  double eloCur{0.0};
  std::string eloLabel;
  std::string phase;
  std::string liveFen;
  Move liveLastMove{MOVE_NONE};
  std::vector<std::string> liveMoves;
};
void renderTrainingDashboard(DoubleBuffer& db, const TrainView& v);
} // namespace chessclaw::ui
