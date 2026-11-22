// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/ui/double_buffer.h"
#include <string>
namespace chessclaw::ui {
void renderProgressBar(DoubleBuffer& db,int r,int c,int w,double frac,const std::string& label,double rate=0.0);
void renderAnimeProgressBar(DoubleBuffer& db,int r,int c,int w,double frac,const std::string& label,double rate,double t);
void renderAnimeProgressBar(DoubleBuffer& db,int r,int c,int w,double frac,const std::string& label,double rate,double t);
std::string formatEta(double seconds);
std::string formatCount(long long v);
} // namespace chessclaw::ui
