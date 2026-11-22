// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/types.h"
#include <atomic>
#include <cstdint>
#include <vector>
namespace chessclaw {
struct TTEntry { std::atomic<std::uint64_t> key{0}; std::atomic<std::uint64_t> data{0}; };
class TranspositionTable {
public:
  TranspositionTable();
  void resize(std::size_t megabytes);
  void clear();
  std::size_t size() const { return table_.size(); }
  double occupancy() const; // fraction 0..1 (sampled)
  // data packing: move(16: from6 to6 promo2 flag4) depth(8) flag(2: exact/lower/upper) age(8) score(16) eval(16)
  bool probe(std::uint64_t key, Move& best, int& depth, int& flag, int& score, int& eval, int ply) const;
  void store(std::uint64_t key, Move best, int depth, int flag, int score, int eval, int ply);
  void newSearch() { ++age_; }
private:
  std::vector<TTEntry> table_;
  std::uint8_t age_{0};
  static std::uint16_t packMove(Move m);
  static Move unpackMove(std::uint16_t v);
};
} // namespace chessclaw
