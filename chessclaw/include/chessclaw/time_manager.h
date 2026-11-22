// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include <cstdint>
namespace chessclaw {
struct TimeControl { int wtimeMs{-1}, btimeMs{-1}, wincMs{0}, bincMs{0}, movestogo{0}, movetimeMs{-1}; bool infinite{false}; };
// Returns {softMs, hardMs}.
struct TimeBudget { std::int64_t softMs; std::int64_t hardMs; };
TimeBudget allocateTime(const TimeControl& tc, int stmIsWhite, int moveOverheadMs=50);
} // namespace chessclaw
