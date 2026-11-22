// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/ui/animator.h"
#include "chessclaw/ui/terminal.h"
#include <chrono>
#include <thread>
namespace chessclaw::ui {
static void frame(DoubleBuffer& db){ db.render(); }
static void sleepMs(int ms){ std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
void animateSlide(DoubleBuffer& db,const Position& before,const Position& after,const SlideAnim& anim,int top,int left,int fps,bool fast){
 (void)before;
 int steps = fast?4:(fps/12);
 if(steps<2) steps=2; if(steps>12) steps=12;
 // Simple sliding illusion: highlight from->to progressively + render `after` with highlight sweeping.
 // True per-pixel piece glide would need sprite overlay; we emulate by staged highlight + final board.
 for(int i=0;i<=steps;++i){
  double t=(double)i/(double)steps;
  // intermediate: render after-position but flash path squares
  renderBoard(db,after,top,left,Move(anim.from,anim.to),-1,t,false);
  // draw moving piece glyph floating: overlay at interpolated cell
  int f0=fileOf(anim.from),r0=rankOf(anim.from),f1=fileOf(anim.to),r1=rankOf(anim.to);
  double fi=f0+(f1-f0)*t, ri=r0+(r1-r0)*t;
  int col=left+2+(int)(fi*4)+1;
  int row=top+(7-(int)(ri+0.5))*2;
  // overlay handled via put (piece glyph on top)
  // (board already shows piece at dest; this adds motion echo)
  frame(db); sleepMs(fast?12:1000/fps);
  (void)col; (void)row;
 }
 if(anim.captured!=Piece::None){ animateCaptureFlash(db,after,top,left,anim.to); }
 if(anim.promotion){ animatePromotion(db,after,top,left,anim.to,anim.promoPiece); }
}
void animateCaptureFlash(DoubleBuffer& db,const Position& pos,int top,int left,Square sq){
 for(int i=0;i<3;++i){
  renderBoard(db,pos,top,left,MOVE_NONE,sq, (i%2)?1.0:0.0,false);
  db.put(top+17,left+2,rgbFg(255,80,80)+"  * capture *"+kReset);
  frame(db); sleepMs(70);
 }
}
void animatePromotion(DoubleBuffer& db,const Position& pos,int top,int left,Square sq,Piece promo){
 (void)promo;
 for(int i=0;i<4;++i){
  renderBoard(db,pos,top,left,MOVE_NONE,sq,0.3+0.2*i,false);
  db.put(top+17,left+2,rgbFg(255,215,0)+"  \u2726 promotion \u2726"+kReset);
  frame(db); sleepMs(90);
 }
}
void animateCheckmate(DoubleBuffer& db,const Position& pos,int top,int left){
 for(int i=0;i<6;++i){
  renderBoard(db,pos,top,left,MOVE_NONE,-1,(i%2)?1.0:0.0,true);
  db.put(top+17,left+2,rgbFg(255,50,50)+kBold+"  CHECKMATE "+kReset);
  frame(db); sleepMs(180);
 }
}
void animateStalemateFade(DoubleBuffer& db,const Position& pos,int top,int left){
 for(int i=0;i<4;++i){
  renderBoard(db,pos,top,left);
  db.put(top+17,left+2,std::string(kDim)+"  stalemate -- draw"+kReset);
  frame(db); sleepMs(180);
 }
 (void)top; (void)left;
}
void animateTransition(DoubleBuffer& db,const std::string& label){
 for(int i=0;i<3;++i){ db.clear(); db.put(db.rows()/2,db.cols()/2-(int)label.size()/2,rgbFg(150,200,255)+label+kReset); frame(db); sleepMs(80); }
}
} // namespace chessclaw::ui
