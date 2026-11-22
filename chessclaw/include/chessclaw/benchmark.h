// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
namespace chessclaw {
struct BenchResult { double nps{0}; std::int64_t nodeCount{0}; double seconds{0}; int depth{0}; };
BenchResult runBenchmark(int depth=12, int threads=1, std::size_t hashMb=64);
void printBenchmark(const BenchResult& r);
} // namespace chessclaw
