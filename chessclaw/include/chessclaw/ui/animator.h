// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include "chessclaw/position.h"
#include "chessclaw/ui/board_renderer.h"
#include "chessclaw/ui/double_buffer.h"
namespace chessclaw::ui {
struct SlideAnim { Square from{0}; Square to{0}; Piece piece{Piece::None}; Piece captured{Piece::None}; bool castle{false}; Square rookFrom{64}; Square rookTo{64}; bool promotion{false}; Piece promoPiece{Piece::None}; };
void animateSlide(DoubleBuffer& db, const Position& before, const Position& after, const SlideAnim& anim, int top, int left, int fps=60, bool fast=false);
void animateCaptureFlash(DoubleBuffer& db, const Position& pos, int top, int left, Square sq);
void animatePromotion(DoubleBuffer& db, const Position& pos, int top, int left, Square sq, Piece promo);
void animateCheckmate(DoubleBuffer& db, const Position& pos, int top, int left);
void animateStalemateFade(DoubleBuffer& db, const Position& pos, int top, int left);
void animateTransition(DoubleBuffer& db, const std::string& label);
} // namespace chessclaw::ui
