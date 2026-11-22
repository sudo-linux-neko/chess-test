// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include <string>
namespace chessclaw::ui {
// Screensaver-grade anime-night background, pure C++ / ANSI truecolor.
// Procedural layered art: deep-space gradient + moon + mountains + lake
// reflection + drifting sakura petals + twinkling stars + mist.
// Time parameter t (seconds) animates petals/stars/shimmer. Frame counter
// drives twinkle phases. All deterministic, no RNG allocation per frame.
void renderAnimeBackground(class DoubleBuffer& db, double t, int frame);
// Foreground vignette + scanline-free polish pass.
void renderVignette(class DoubleBuffer& db);
// Small neon title banner (Tokyo-night style).
void renderNeonTitle(class DoubleBuffer& db, int row, const std::string& text);
// Sakura petal particle field update helper (stateless hash).
struct Petal { double x, y, speed, drift, phase, size; };
Petal petalAt(int index, double t, int cols, int rows);
double starTwinkle(int index, double t);
} // namespace chessclaw::ui
