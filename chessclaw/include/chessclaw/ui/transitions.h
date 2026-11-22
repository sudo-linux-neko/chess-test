// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/ui/double_buffer.h"
#include <string>
namespace chessclaw::ui {
void transitionFade(DoubleBuffer& db, const std::string& label);
void transitionWipe(DoubleBuffer& db, const std::string& label);
} // namespace chessclaw::ui
