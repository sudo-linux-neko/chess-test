// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/movegen.h"
namespace chessclaw {
static void push(std::vector<Move>& o, Square f, Square t, std::uint8_t fl){ o.emplace_back(f,t,fl); }
static void genPawn(const Position& p, std::vector<Move>& o, bool capsOnly){
 Color us=p.sideToMove(); int dir=(us==Color::White)?8:-8;
 int startRank=(us==Color::White)?1:6; int promoRank=(us==Color::White)?6:1;
 Bitboard pawns=p.pieces(us,PieceType::Pawn); Bitboard occ=p.occupancy(); Bitboard them=p.pieces(opposite(us));
 Bitboard b=pawns;
 while(b){ Square f=poplsb(b); int fr=rankOf(f), ff=fileOf(f);
  Square one=static_cast<Square>(f+dir);
  bool isPromo=(fr==promoRank);
  // pushes
  if(!capsOnly){
   if(!(occ&bb(one))){
    if(isPromo){ push(o,f,one,Move::PromoN); push(o,f,one,Move::PromoB); push(o,f,one,Move::PromoR); push(o,f,one,Move::PromoQ); }
    else push(o,f,one,Move::Quiet);
    if(fr==startRank){ Square two=static_cast<Square>(f+2*dir); if(!(occ&bb(two))&&!(occ&bb(one))) push(o,f,two,Move::DoublePush); }
   }
  }
  // captures
  for(int df=-1;df<=1;df+=2){ int tf=ff+df; if(tf<0||tf>7) continue; int tr=fr+((us==Color::White)?1:-1); if(tr<0||tr>7) continue; Square t=makeSquare(tf,tr);
   bool isCap=(them&bb(t))!=0;
   bool isEp=(t==p.epSquare());
   if(isCap||isEp){
    std::uint8_t fl = isEp?Move::EnPassant:Move::Capture;
    if(isPromo){
     if(isEp){ push(o,f,t,Move::EnPassant); } // ep never promo (rank mismatch) but keep legal path
     else { push(o,f,t,Move::CapPromoN); push(o,f,t,Move::CapPromoB); push(o,f,t,Move::CapPromoR); push(o,f,t,Move::CapPromoQ); (void)fl; }
    } else push(o,f,t,fl);
   }
  }
 }
}
static void genPiece(const Position& p, std::vector<Move>& o, PieceType t, bool capsOnly){
 Color us=p.sideToMove(); Bitboard occ=p.occupancy(); Bitboard own=p.pieces(us); Bitboard them=p.pieces(opposite(us));
 Bitboard bbpc=p.pieces(us,t); 
 while(bbpc){ Square f=poplsb(bbpc); Bitboard att=0;
  if(t==PieceType::Knight) att=kKnightAttacks[f];
  else if(t==PieceType::King) att=kKingAttacks[f];
  else if(t==PieceType::Bishop) att=bishopAttacks(f,occ);
  else if(t==PieceType::Rook) att=rookAttacks(f,occ);
  else att=queenAttacks(f,occ);
  att&=~own;
  if(capsOnly) att&=them;
  while(att){ Square t2=poplsb(att); bool cap=(them&bb(t2))!=0; push(o,f,t2,cap?Move::Capture:Move::Quiet); }
 }
}
static void genCastles(const Position& p, std::vector<Move>& o){
 if(p.inCheck()) { /* still may castle out? No: illegal, but filter later */ }
 Color us=p.sideToMove(); Bitboard occ=p.occupancy(); Color them=opposite(us);
 if(us==Color::White){
  if(p.castling()&1){
   if(!(occ&(bb(makeSquare(5,0))|bb(makeSquare(6,0)))) && p.pieceOn(makeSquare(7,0))==Piece::WR){
    if(!p.isAttacked(makeSquare(4,0),them)&&!p.isAttacked(makeSquare(5,0),them)&&!p.isAttacked(makeSquare(6,0),them))
     push(o,makeSquare(4,0),makeSquare(6,0),Move::CastleK);
   }
  }
  if(p.castling()&2){
   if(!(occ&(bb(makeSquare(1,0))|bb(makeSquare(2,0))|bb(makeSquare(3,0)))) && p.pieceOn(makeSquare(0,0))==Piece::WR){
    if(!p.isAttacked(makeSquare(4,0),them)&&!p.isAttacked(makeSquare(3,0),them)&&!p.isAttacked(makeSquare(2,0),them))
     push(o,makeSquare(4,0),makeSquare(2,0),Move::CastleQ);
   }
  }
 } else {
  if(p.castling()&4){
   if(!(occ&(bb(makeSquare(5,7))|bb(makeSquare(6,7)))) && p.pieceOn(makeSquare(7,7))==Piece::BR){
    if(!p.isAttacked(makeSquare(4,7),them)&&!p.isAttacked(makeSquare(5,7),them)&&!p.isAttacked(makeSquare(6,7),them))
     push(o,makeSquare(4,7),makeSquare(6,7),Move::CastleK);
   }
  }
  if(p.castling()&8){
   if(!(occ&(bb(makeSquare(1,7))|bb(makeSquare(2,7))|bb(makeSquare(3,7)))) && p.pieceOn(makeSquare(0,7))==Piece::BR){
    if(!p.isAttacked(makeSquare(4,7),them)&&!p.isAttacked(makeSquare(3,7),them)&&!p.isAttacked(makeSquare(2,7),them))
     push(o,makeSquare(4,7),makeSquare(2,7),Move::CastleQ);
   }
  }
 }
}
void generateMoves(const Position& pos, std::vector<Move>& out, bool legalOnly, bool capturesOnly){
 out.clear(); out.reserve(64);
 std::vector<Move> pseudo; pseudo.reserve(64);
 genPawn(pos,pseudo,capturesOnly);
 genPiece(pos,pseudo,PieceType::Knight,capturesOnly);
 genPiece(pos,pseudo,PieceType::Bishop,capturesOnly);
 genPiece(pos,pseudo,PieceType::Rook,capturesOnly);
 genPiece(pos,pseudo,PieceType::Queen,capturesOnly);
 genPiece(pos,pseudo,PieceType::King,capturesOnly);
 if(!capturesOnly) genCastles(pos,pseudo);
 if(!legalOnly){ out=std::move(pseudo); return; }
 for(auto m:pseudo) if(pos.isLegal(m)) out.push_back(m);
}
bool hasLegalMove(const Position& pos){ std::vector<Move> v; generateMoves(pos,v,true,false); return !v.empty(); }
std::vector<Move> legalMoves(const Position& pos){ std::vector<Move> v; generateMoves(pos,v,true,false); return v; }
int moveOrderBaseScore(const Position& pos, Move m){
 int s=0;
 if(m.isCapture()){
  Piece cap=pos.pieceOn(m.to()); PieceType ct=typeOf(cap);
  if(m.flags==Move::EnPassant) ct=PieceType::Pawn;
  PieceType at=typeOf(pos.pieceOn(m.from()));
  int cv=(ct==PieceType::Pawn?100:ct==PieceType::Knight?320:ct==PieceType::Bishop?330:ct==PieceType::Rook?500:ct==PieceType::Queen?900:0);
  int av=(at==PieceType::Pawn?100:at==PieceType::Knight?320:at==PieceType::Bishop?330:at==PieceType::Rook?500:at==PieceType::Queen?900:20000);
  s=100000+cv*10-av/10;
 }
 if(m.isPromo()) s+=80000;
 return s;
}
int see(const Position& pos, Move m){
 // Simplified SEE: MVV-LVA material diff with recapture approximation.
 if(!m.isCapture()&&!m.isPromo()) return 0;
 static const int V[7]={0,100,320,330,500,900,20000};
 Square to=m.to();
 Piece cap = (m.flags==Move::EnPassant)?makePiece(opposite(pos.sideToMove()),PieceType::Pawn):pos.pieceOn(to);
 int gain=V[(int)typeOf(cap)];
 if(m.isPromo()) gain+=V[(int)m.promoType()]-V[(int)PieceType::Pawn];
 PieceType att=typeOf(pos.pieceOn(m.from()));
 // If smallest defender is worth less than gain, assume recapture loses attacker.
 // Cheap approx: gain - attackerValue/4 if square defended.
 if(pos.isAttacked(to, opposite(pos.sideToMove()))) gain-=V[(int)att]/4;
 return gain;
}
} // namespace chessclaw
