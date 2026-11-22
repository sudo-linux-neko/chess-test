// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/nnue.h"
#include "chessclaw/train/data_gen.h"
#include <string>
#include <vector>
namespace chessclaw::train {
struct TrainConfig {
  std::string dataPath{"data/selfplay.bin"};
  std::string outNet{"models/chessclaw.nnue"};
  int epochs{8}; int batchSize{256}; double lr{0.01}; double lrDecay{0.9};
  int hidden{128}; bool showDashboard{true}; int seed{42};
  double timeBudgetSec{6*3600}; // 6h default within 8h total incl. selfplay
  // NEW: controls for visible training art + honest Elo tracking.
  // minBatchDelayMs: artificial pacing per batch so the dashboard stays
  // visible on tiny demo runs (0 = fastest, no delay).
  int minBatchDelayMs{0};
  // eloGamesPerEpoch: number of verification games (classical vs NNUE-blend)
  // played at the end of each epoch to build a REAL Elo history.
  // 0 = disabled (fastest).
  int eloGamesPerEpoch{0};
  int eloDepth{4};
  // Lossless pause/resume: every batch the full trainer state (weights, Adam
  // moments, lr, epoch/batch, loss/val/elo history) is written to ckptPath.
  // resume=true continues exactly from the last checkpoint.
  bool resume{false};
  std::string ckptPath{"models/checkpoint.bin"};
};
struct TrainResult { double bestVal{1e9}; int epochsDone{0}; long long samples{0}; std::vector<double> eloHist; int eloW{0}, eloD{0}, eloL{0}; };
TrainResult trainNnue(const TrainConfig& cfg, nnue::Network& net, std::vector<double>& lossHist, std::vector<double>& valHist);
} // namespace chessclaw::train
