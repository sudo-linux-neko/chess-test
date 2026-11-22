// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/search.h"
#include <string>
namespace chessclaw {
class Uci {
public:
  Uci();
  int run(); // stdin loop, returns exit code
  // testable entry
  std::string handleLine(const std::string& line, Position& pos, SearchLimits& pending);
  Searcher& searcher() { return searcher_; }
  Position& position() { return pos_; }
private:
  Searcher searcher_;
  Position pos_;
  nnue::Network net_;
  int hashMb_{64};
  int threads_{1};
  int moveOverheadMs_{50};
  bool handlePosition(const std::string& line);
  std::string handleGo(const std::string& line);
  Move parseMove(const Position& p, const std::string& tok);
};
} // namespace chessclaw
