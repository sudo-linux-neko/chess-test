// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/benchmark.h"
#include "chessclaw/search.h"
#include <chrono>
#include <cstdio>
namespace chessclaw {
BenchResult runBenchmark(int depth, int threads, std::size_t hashMb){
 Searcher s; s.setThreads(threads); s.setHashMb(hashMb);
 Position p; p.setStartpos();
 SearchLimits lim; lim.maxDepth=depth;
 auto t0=std::chrono::steady_clock::now();
 Move b=s.search(p,lim);
 (void)b;
 auto t1=std::chrono::steady_clock::now();
 double sec=std::chrono::duration<double>(t1-t0).count();
 if(sec<0.01) sec=0.01;
 std::int64_t n=s.stats().nodes.load();
 BenchResult r; r.nps=n/sec; r.nodeCount=n; r.seconds=sec; r.depth=depth; return r;
}
void printBenchmark(const BenchResult& r){ std::printf("chessclaw bench: depth=%d nodes=%lld time=%.2fs nps=%.0f\n",r.depth,(long long)r.nodeCount,r.seconds,r.nps); }
} // namespace chessclaw
