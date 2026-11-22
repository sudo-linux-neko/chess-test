// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/ui/double_buffer.h"
#include <string>
#include <vector>
namespace chessclaw::ui {
void renderLineChart(DoubleBuffer& db, int top,int left,int h,int w, const std::vector<double>& a, const std::vector<double>& b, const std::string& title, const std::string& labelA="loss", const std::string& labelB="val", bool logScale=false);
void renderEloChart(DoubleBuffer& db,int top,int left,int h,int w, const std::vector<double>& elo, const std::string& title="Elo estimate (self-play, ESTIMATE)");
void renderBar(DoubleBuffer& db,int r,int c,int w,double frac,const std::string& label="");
void renderCpuBars(DoubleBuffer& db,int r,int c,int n,const std::vector<double>& util);
} // namespace chessclaw::ui
