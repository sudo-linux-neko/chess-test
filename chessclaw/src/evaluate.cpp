// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/evaluate.h"
#include "chessclaw/nnue.h"
namespace chessclaw {
int pieceValueMg(PieceType t){ switch(t){case PieceType::Pawn:return 100;case PieceType::Knight:return 320;case PieceType::Bishop:return 330;case PieceType::Rook:return 500;case PieceType::Queen:return 900;case PieceType::King:return 20000;default:return 0;} }
int pieceValueEg(PieceType t){ switch(t){case PieceType::Pawn:return 110;case PieceType::Knight:return 310;case PieceType::Bishop:return 320;case PieceType::Rook:return 520;case PieceType::Queen:return 960;case PieceType::King:return 20000;default:return 0;} }
// Simple PSTs (mg), from white perspective a1=0.
static const int kPstP[64]={0,0,0,0,0,0,0,0, 50,50,50,50,50,50,50,50, 10,10,20,30,30,20,10,10, 5,5,10,25,25,10,5,5, 0,0,0,20,20,0,0,0, 5,-5,-10,0,0,-10,-5,5, 5,10,10,-20,-20,10,10,5, 0,0,0,0,0,0,0,0};
static const int kPstN[64]={-50,-40,-30,-30,-30,-30,-40,-50, -40,-20,0,0,0,0,-20,-40, -30,0,10,15,15,10,0,-30, -30,5,15,20,20,15,5,-30, -30,0,15,20,20,15,0,-30, -30,5,10,15,15,10,5,-30, -40,-20,0,5,5,0,-20,-40, -50,-40,-30,-30,-30,-30,-40,-50};
static const int kPstB[64]={-20,-10,-10,-10,-10,-10,-10,-20, -10,0,0,0,0,0,0,-10, -10,0,5,10,10,5,0,-10, -10,5,5,10,10,5,5,-10, -10,0,10,10,10,10,0,-10, -10,10,10,10,10,10,10,-10, -10,5,0,0,0,0,5,-10, -20,-10,-10,-10,-10,-10,-10,-20};
static const int kPstR[64]={0,0,0,0,0,0,0,0, 5,10,10,10,10,10,10,5, -5,0,0,0,0,0,0,-5, -5,0,0,0,0,0,0,-5, -5,0,0,0,0,0,0,-5, -5,0,0,0,0,0,0,-5, -5,0,0,0,0,0,0,-5, 0,0,0,5,5,0,0,0};
static const int kPstQ[64]={-20,-10,-10,-5,-5,-10,-10,-20, -10,0,0,0,0,0,0,-10, -10,0,5,5,5,5,0,-10, -5,0,5,5,5,5,0,-5, 0,0,5,5,5,5,0,-5, -10,5,5,5,5,5,0,-10, -10,0,5,0,0,0,0,-10, -20,-10,-10,-5,-5,-10,-10,-20};
static const int kPstK[64]={-30,-40,-40,-50,-50,-40,-40,-30, -30,-40,-40,-50,-50,-40,-40,-30, -30,-40,-40,-50,-50,-40,-40,-30, -30,-40,-40,-50,-50,-40,-40,-30, -20,-30,-30,-40,-40,-30,-30,-20, -10,-20,-20,-20,-20,-20,-20,-10, 20,20,0,0,0,0,20,20, 20,30,10,0,0,10,30,20};
static int mirror(Square s){ return makeSquare(fileOf(s),7-rankOf(s)); }
bool isEndgame(const Position& pos){
 int nonPawn=0;
 for(int s=0;s<64;++s){ Piece p=pos.pieceOn((Square)s); auto t=typeOf(p); if(t==PieceType::Queen||t==PieceType::Rook||t==PieceType::Bishop||t==PieceType::Knight) nonPawn+=pieceValueMg(t); }
 return nonPawn < 1300;
}
int evaluateClassical(const Position& pos){
 int mgW=0,mgB=0;
 int pawnFilesW[8]={0},pawnFilesB[8]={0};
 int pawnsW=0,pawnsB=0;
 // mobility counts
 int mob=0;
 for(int s=0;s<64;++s){
  Piece p=pos.pieceOn((Square)s); if(p==Piece::None) continue;
  auto t=typeOf(p); auto c=colorOf(p);
  int v=pieceValueMg(t);
  const int* pst=nullptr;
  switch(t){case PieceType::Pawn:pst=kPstP;break;case PieceType::Knight:pst=kPstN;break;case PieceType::Bishop:pst=kPstB;break;case PieceType::Rook:pst=kPstR;break;case PieceType::Queen:pst=kPstQ;break;case PieceType::King:pst=kPstK;break;default:break;}
  int pv = pst ? (c==Color::White ? pst[s] : pst[mirror((Square)s)]) : 0;
  if(t==PieceType::Pawn){ if(c==Color::White){pawnFilesW[fileOf((Square)s)]++;pawnsW++;} else {pawnFilesB[fileOf((Square)s)]++;pawnsB++;} }
  if(c==Color::White) mgW+=v+pv; else mgB+=v+pv;
 }
 // pawn structure: doubled + isolated
 for(int f=0;f<8;++f){
  if(pawnFilesW[f]>1) mgW-=12*(pawnFilesW[f]-1);
  if(pawnFilesB[f]>1) mgB-=12*(pawnFilesB[f]-1);
  bool isoW=(pawnFilesW[f]>0)&&((f==0||pawnFilesW[f-1]==0)&&(f==7||pawnFilesW[f+1]==0));
  bool isoB=(pawnFilesB[f]>0)&&((f==0||pawnFilesB[f-1]==0)&&(f==7||pawnFilesB[f+1]==0));
  if(isoW) mgW-=10; if(isoB) mgB-=10;
 }
 // passed pawns + king safety (pawn shield) + mobility (cheap)
 Bitboard occ=pos.occupancy();
 for(int s=0;s<64;++s){
  Piece p=pos.pieceOn((Square)s); if(typeOf(p)!=PieceType::Pawn) continue;
  bool w=(colorOf(p)==Color::White); int f=fileOf((Square)s), r=rankOf((Square)s);
  bool passed=true;
  for(int df=-1;df<=1;++df){ int ff=f+df; if(ff<0||ff>7) continue;
   for(int rr=0;rr<8;++rr){ if(w){ if(rr>r){ Piece q=pos.pieceOn(makeSquare(ff,rr)); if(q==Piece::BP) {passed=false;break;} } } else { if(rr<r){ Piece q=pos.pieceOn(makeSquare(ff,rr)); if(q==Piece::WP){passed=false;break;} } } }
   if(!passed) break;
  }
  if(passed){ int adv=w?r:(7-r); int bonus=(adv>=4)?(adv*12):(adv*5); if(w) mgW+=bonus; else mgB+=bonus; }
 }
 // king pawn shield
 {
  Square kw=pos.kingSquare(Color::White), kb=pos.kingSquare(Color::Black);
  if(kw!=SQ_NONE){ int kf=fileOf(kw),kr=rankOf(kw); if(kr<2){ int shield=0; for(int df=-1;df<=1;++df){int ff=kf+df; if(ff<0||ff>7)continue; for(int rr=kr+1;rr<=kr+2&&rr<8;++rr){ if(pos.pieceOn(makeSquare(ff,rr))==Piece::WP) shield++; }} mgW+=shield*8 - (3-shield)*10; } }
  if(kb!=SQ_NONE){ int kf=fileOf(kb),kr=rankOf(kb); if(kr>5){ int shield=0; for(int df=-1;df<=1;++df){int ff=kf+df; if(ff<0||ff>7)continue; for(int rr=kr-1;rr>=kr-2&&rr>=0;--rr){ if(pos.pieceOn(makeSquare(ff,rr))==Piece::BP) shield++; }} mgB+=shield*8 - (3-shield)*10; } }
 }
 // mobility: count pseudo attacks for knights/bishops/rooks/queens (cheap, capped)
 {
  int mW=0,mB=0;
  for(int s=0;s<64;++s){ Piece p=pos.pieceOn((Square)s); if(p==Piece::None) continue; auto t=typeOf(p); Bitboard a=0;
   if(t==PieceType::Knight) a=kKnightAttacks[s]; else if(t==PieceType::Bishop) a=bishopAttacks((Square)s,occ); else if(t==PieceType::Rook) a=rookAttacks((Square)s,occ); else if(t==PieceType::Queen) a=queenAttacks((Square)s,occ); else continue;
   int c=popcount(a); if(colorOf(p)==Color::White) mW+=c; else mB+=c;
  }
  mob=(mW-mB)*2;
 }
 int score=(mgW-mgB)+mob;
 // tempo
 score += (pos.sideToMove()==Color::White?10:-10);
 // endgame scaling: if endgame, reduce shield over-weight already included; nothing extra.
 (void)pawnsW; (void)pawnsB;
 return score;
}
// NNUE hook: mixed hybrid. evaluate() blends classical + NNUE when a net is registered.
namespace nnue { const Network* gNet=nullptr; void setGlobalNet(const Network* n){ gNet=n; } }
int evaluate(const Position& pos){
 int cl=evaluateClassical(pos);
 if(nnue::gNet && nnue::gNet->loaded()){
  int nn=nnue::gNet->evaluate(pos);
  // Blend: 60% NNUE + 40% classical (keeps sane when net untrained).
  int mixed=(nn*3+cl*2)/5;
  // Clamp to sane range.
  if(mixed>15000) mixed=15000; if(mixed<-15000) mixed=-15000;
  return mixed;
 }
 return cl;
}
} // namespace chessclaw
