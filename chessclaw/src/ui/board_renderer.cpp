// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/ui/board_renderer.h"
#include "chessclaw/ui/terminal.h"
#include "chessclaw/movegen.h"
#include "chessclaw/search.h"
#include <cmath>
#include <random>
namespace chessclaw::ui {
static std::string pieceGlyph(Piece p, bool uni){
 if(uni){ switch(p){case Piece::WK:return "\u2654";case Piece::WQ:return "\u2655";case Piece::WR:return "\u2656";case Piece::WB:return "\u2657";case Piece::WN:return "\u2658";case Piece::WP:return "\u2659";case Piece::BK:return "\u265A";case Piece::BQ:return "\u265B";case Piece::BR:return "\u265C";case Piece::BB:return "\u265D";case Piece::BN:return "\u265E";case Piece::BP:return "\u265F";default:return " ";} }
 else { switch(p){case Piece::WK:return "K";case Piece::WQ:return "Q";case Piece::WR:return "R";case Piece::WB:return "B";case Piece::WN:return "N";case Piece::WP:return "P";case Piece::BK:return "k";case Piece::BQ:return "q";case Piece::BR:return "r";case Piece::BB:return "b";case Piece::BN:return "n";case Piece::BP:return "p";default:return " ";} }
}
std::vector<std::string> boardCellGlyph(Piece p,bool dark,bool unicode){ (void)dark; return {pieceGlyph(p,unicode)}; }
void renderBoardAscii(DoubleBuffer& db,const Position& pos,int top,int left){
 for(int r=7;r>=0;--r){
  int row=top+(7-r)*2;
  db.put(row,left,std::string(1,(char)('1'+r))+" ");
  for(int f=0;f<8;++f){
   Piece p=pos.pieceOn(makeSquare(f,r));
   bool dark=(f+r)%2==0;
   std::string bg=dark?ansi256Bg(94):ansi256Bg(230);
   std::string fg=(colorOf(p)==Color::White&&p!=Piece::None)?rgbFg(255,255,255):rgbFg(20,20,20);
   std::string cell=" "+pieceGlyph(p,false)+" ";
   db.put(row,left+2+f*3,bg+fg+cell+kReset);
   db.put(row+1,left+2+f*3,bg+"   "+kReset);
  }
 }
 db.put(top+16,left,"  a  b  c  d  e  f  g  h");
}
std::vector<std::string> demoLivePosition(Position& outPos, int plies, int depth, std::uint64_t seed){
 outPos.setStartpos();
 std::vector<std::string> hist;
 std::mt19937_64 rng(seed);
 Searcher s; s.setThreads(1); s.setHashMb(16);
 for(int i=0;i<plies;++i){
  std::vector<Move> lm; generateMoves(outPos,lm,true,false);
  if(lm.empty()) break;
  SearchLimits lim; lim.maxDepth=depth;
  Move best=s.search(outPos,lim);
  bool ok=false; for(auto m:lm) if(m==best){ok=true;break;}
  Move chosen = ok?best:lm[rng()%lm.size()];
  if((rng()%100)<12) chosen=lm[rng()%lm.size()];
  hist.push_back(moveToUci(chosen));
  UndoState u; outPos.doMove(chosen,u);
  if(outPos.halfmove()>=100) break;
 }
 return hist;
}
void renderBoard(DoubleBuffer& db,const Position& pos,int top,int left,Move hl,int hsq,double animT,bool checkFlash){
 bool uni=supportsUnicode();
 if(!uni){ renderBoardAscii(db,pos,top,left); return; }
 // ANIME-NIGHT board: deep indigo + ice-cyan neon squares, pink last-move trail.
 double pulse=0.5+0.5*std::sin(animT*3.0);
 for(int r=7;r>=0;--r){
  int brow=(7-r);
  int row=top+brow*2;
  db.put(row,left,rgbFg(150,170,220)+std::string(1,(char)('1'+r))+" "+kReset);
  db.put(row+1,left,"  ");
  for(int f=0;f<8;++f){
   bool dark=(f+r)%2==0;
   double t=(double)(f+r)/14.0;
   int br,bg2,bb2;
   if(dark){ br=(int)(30+(22-30)*t); bg2=(int)(32+(26-32)*t); bb2=(int)(58+(46-58)*t); }
   else { br=(int)(122+(100-122)*t); bg2=(int)(180+(160-180)*t); bb2=(int)(235+(220-235)*t); }
   bool isHl=false;
   if(!hl.isNone()&&(makeSquare(f,r)==hl.from()||makeSquare(f,r)==hl.to())) isHl=true;
   if(hsq>=0&&makeSquare(f,r)==(Square)hsq) isHl=true;
   if(isHl){ double k=0.55+0.35*pulse; br=(int)(br+(247-br)*k); bg2=(int)(bg2+(140-bg2)*k); bb2=(int)(bb2+(180-bb2)*k); }
   if(checkFlash){
    Square kw=pos.kingSquare(pos.sideToMove());
    if(makeSquare(f,r)==kw){ double pl=0.5+0.5*animT; br=(int)(200+55*pl); bg2=60; bb2=90; }
   }
   // neon edge shimmer on light squares
   if(!dark){ bb2+=(int)(10*std::sin(animT*2.0+f*0.8+r*0.5)); if(bb2>255)bb2=255; if(bb2<0)bb2=0; }
   std::string bg=rgbBg(br,bg2,bb2);
   Piece p=pos.pieceOn(makeSquare(f,r));
   std::string fg;
   if(p==Piece::None) fg=rgbFg(br,bg2,bb2);
   else if(colorOf(p)==Color::White) fg="\x1b[38;2;255;250;235m\x1b[1m";
   else fg="\x1b[38;2;12;10;22m\x1b[1m";
   std::string glyph=pieceGlyph(p,true);
   std::string top_cell=bg+fg+" "+glyph+" "+kReset;
   std::string sh=(p==Piece::None)?(bg+"    "+kReset):(bg+rgbFg(br/2,bg2/2,bb2/2)+" \u2584\u2584 "+kReset);
   db.put(row,left+2+f*4,top_cell);
   db.put(row+1,left+2+f*4,sh);
  }
 }
 db.put(top+16,left,rgbFg(150,170,220)+"   a   b   c   d   e   f   g   h"+kReset);
}
} // namespace chessclaw::ui
