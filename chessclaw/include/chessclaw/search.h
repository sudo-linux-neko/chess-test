// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/evaluate.h"
#include "chessclaw/movegen.h"
#include "chessclaw/nnue.h"
#include "chessclaw/position.h"
#include "chessclaw/transposition.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
namespace chessclaw {
struct SearchLimits {
  int maxDepth{64};
  std::int64_t maxNodes{INT64_MAX};
  int movetimeMs{-1};
  int wtimeMs{-1}, btimeMs{-1}, wincMs{0}, bincMs{0};
  int movestogo{0};
  bool infinite{false};
  std::vector<Move> searchMoves;
  int multiPv{1};
};
struct SearchStats {
  std::atomic<std::int64_t> nodes{0};
  std::atomic<int> seldepth{0};
  std::atomic<int> depth{0};
  std::atomic<int> scoreCp{0};
  std::atomic<bool> stopped{false};
  std::chrono::steady_clock::time_point start;
  std::int64_t softMs{1000}, hardMs{5000};
  std::vector<Move> pv;
  std::int64_t nps() const;
};
struct ThreadData {
  int id{0};
  std::uint64_t seed{0};
  std::int64_t nodes{0};
  int seldepth{0};
  std::array<std::array<Move,2>,64> killers{};
  std::array<std::array<int,64>,64> history{};
  std::array<std::array<Move,64>,64> counter{};
  std::array<Move,128> pvStack{};
};
class Searcher {
public:
  Searcher();
  void setThreads(int n);
  int threads() const { return numThreads_; }
  void setHashMb(std::size_t mb) { tt_.resize(mb); }
  TranspositionTable& tt() { return tt_; }
  SearchStats& stats() { return stats_; }
  void setNnue(const nnue::Network* net) { net_ = net; }
  void stop() { stats_.stopped.store(true); }
  bool shouldStop() const;
  Move search(Position& pos, const SearchLimits& lim);
  int searchSingle(Position& pos, const SearchLimits& lim, int threadId, std::vector<Move>& outPv);
  // exposed for tests/training
  int quiescence(Position& pos, int alpha, int beta, int ply, ThreadData& td, std::uint64_t* hist, int histLen);
  int negamax(Position& pos, int depth, int alpha, int beta, int ply, bool pvNode, ThreadData& td, std::uint64_t* hist, int histLen);
  int staticEval(const Position& pos);
  std::vector<ThreadData> threadData_;
private:
  int numThreads_{1};
  TranspositionTable tt_;
  SearchStats stats_;
  const nnue::Network* net_{nullptr};
  int lmrReduction(int depth, int moveIdx, bool pv) const;
  int timeFor(Position& pos, const SearchLimits& lim, Color stm);
  void computeTimeLimits(const Position& pos, const SearchLimits& lim);
};
int contemptValue();
} // namespace chessclaw
