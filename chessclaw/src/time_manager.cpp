// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/time_manager.h"
#include <algorithm>
namespace chessclaw {
TimeBudget allocateTime(const TimeControl& tc, int stmIsWhite, int overhead){
 int t = stmIsWhite ? tc.wtimeMs : tc.btimeMs;
 int inc = stmIsWhite ? tc.wincMs : tc.bincMs;
 if(tc.movetimeMs>=0){ std::int64_t m=(std::int64_t)tc.movetimeMs-overhead; if(m<10) m=10; return {m,m}; }
 if(t<0) return {1000,5000}; // no clock: default
 int mtg = tc.movestogo>0?tc.movestogo:30;
 // base fraction
 double frac = 1.0/std::max(1,mtg);
 // clamp sudden-death: don't use more than 1/4 of remaining
 double soft = t*frac + inc*0.6;
 double hard = t*0.25 + inc*0.3;
 // emergency protection: leave overhead + 100ms
 soft-=overhead; hard-=overhead;
 if(soft<10) soft=10; if(hard<20) hard=20;
 if(soft>hard) soft=hard;
 return {(std::int64_t)soft,(std::int64_t)hard};
}
} // namespace chessclaw
