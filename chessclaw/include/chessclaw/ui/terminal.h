// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include <string>
namespace chessclaw::ui {
struct TermSize { int rows{24}; int cols{80}; };
TermSize termSize();
bool supportsUnicode();
bool supportsTruecolor();
void hideCursor();
void showCursor();
void enterAltScreen();
void exitAltScreen();
std::string rgbFg(int r,int g,int b);
std::string rgbBg(int r,int g,int b);
std::string ansi256Fg(int c);
std::string ansi256Bg(int c);
inline const char* kReset="\x1b[0m";
inline const char* kBold="\x1b[1m";
inline const char* kDim="\x1b[2m";
std::string gradientColor(double t, int r0,int g0,int b0, int r1,int g1,int b1);
// Background image support (optional, dark-theme):
// If CHESSCLAW_BG env points to a PPM/PGM image file, it is loaded once and
// drawn dimmed as the dashboard backdrop (half-block truecolor). Otherwise a
// procedural anime-night scene is rendered. Pure C++, no libraries.
bool hasBackgroundImage();
std::string backgroundImagePath();
} // namespace chessclaw::ui
