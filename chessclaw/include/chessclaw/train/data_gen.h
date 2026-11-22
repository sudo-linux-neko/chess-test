// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/position.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace chessclaw::train {
struct Sample { std::string fen; int scoreCp{0}; double result{0.5}; };
// progress callback: doneGames, totalGames, samplesSoFar
using SelfplayProgress = std::function<void(int,int,int)>;
struct GenConfig { int games{200}; int movesPerGame{60}; int depth{4}; int threads{4}; std::string outPath{"data/selfplay.bin"}; std::uint64_t seed{12345}; SelfplayProgress progress{}; };
int generateSelfplay(const GenConfig& cfg); // returns samples written; binary format below
// Binary sample file: u32 count, then per sample: u8 fenLen, fen bytes, i16 score, u8 result*100
bool loadSamples(const std::string& path, std::vector<Sample>& out);
bool saveSamples(const std::string& path, const std::vector<Sample>& v);
} // namespace chessclaw::train
