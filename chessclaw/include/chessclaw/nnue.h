// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/position.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace chessclaw::nnue {
inline constexpr int kNumFeatures = 768;
inline constexpr int kHidden = 128;
inline constexpr int kQIn = 64;
inline constexpr int kQOut = 64;
struct Accumulator { std::array<std::int16_t, kHidden> white{}; std::array<std::int16_t, kHidden> black{}; };
class Network {
public:
  Network();
  bool loaded() const { return loaded_; }
  bool load(const std::string& path);
  bool save(const std::string& path) const;
  void randomize(std::uint64_t seed);
  void refresh(const Position& pos, Accumulator& acc) const;
  int evaluate(const Position& pos) const;
  int evaluate(const Accumulator& acc, Color stm) const;
  std::array<std::int16_t, kNumFeatures*kHidden>& w1() { return w1_; }
  const std::array<std::int16_t, kNumFeatures*kHidden>& w1() const { return w1_; }
  std::array<std::int16_t, kHidden>& b1() { return b1_; }
  const std::array<std::int16_t, kHidden>& b1() const { return b1_; }
  std::array<std::int16_t, 2*kHidden>& w2() { return w2_; }
  const std::array<std::int16_t, 2*kHidden>& w2() const { return w2_; }
  std::int32_t& b2ref() { return b2_; }
  std::int32_t b2val() const { return b2_; }
  static int featureIndex(Piece p, Square s, Color perspective);
private:
  std::array<std::int16_t, kNumFeatures*kHidden> w1_{};
  std::array<std::int16_t, kHidden> b1_{};
  std::array<std::int16_t, 2*kHidden> w2_{};
  std::int32_t b2_{0};
  bool loaded_{false};
};
extern const Network* gNet;
void setGlobalNet(const Network* n);
} // namespace chessclaw::nnue
