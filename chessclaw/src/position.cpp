// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/position.h"
#include <sstream>
namespace chessclaw {
static std::uint64_t ZP[13][64];
static std::uint64_t ZS;
static std::uint64_t ZC[16];
static std::uint64_t ZE[8];
static bool zInit=false;
static std::uint64_t splitmix(std::uint64_t& s){ s+=0x9e3779b97f4a7c15ULL; std::uint64_t z=s; z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL; z=(z^(z>>27))*0x94d049bb133111ebULL; return z^(z>>31); }
void initZobrist(){ if(zInit) return; std::uint64_t st=0x123456789abcdefULL; for(int p=0;p<13;++p)for(int s=0;s<64;++s)ZP[p][s]=splitmix(st); ZS=splitmix(st); for(int i=0;i<16;++i)ZC[i]=splitmix(st); for(int i=0;i<8;++i)ZE[i]=splitmix(st); zInit=true; initAttacks(); }
std::uint64_t zobristPiece(Piece p,Square s){ return ZP[(int)p][s]; }
std::uint64_t zobristSide(){ return ZS; }
std::uint64_t zobristCastle(int c){ return ZC[c&15]; }
std::uint64_t zobristEp(int f){ return ZE[f&7]; }
Position::Position(){ initZobrist(); setStartpos(); }
void Position::clear(){ board_.fill(Piece::None); byColor_.fill(0); byType_.fill(0); stm_=Color::White; castling_=0; ep_=SQ_NONE; halfmove_=0; fullmove_=1; hash_=0; }
void Position::putPiece(Square s, Piece p){ board_[s]=p; byColor_[(int)colorOf(p)]|=bb(s); byType_[(int)typeOf(p)]|=bb(s); }
void Position::removePiece(Square s){ Piece p=board_[s]; if(p==Piece::None) return; board_[s]=Piece::None; byColor_[(int)colorOf(p)]&=~bb(s); byType_[(int)typeOf(p)]&=~bb(s); }
void Position::movePiece(Square f,Square t){ Piece p=board_[f]; board_[f]=Piece::None; board_[t]=p; Bitboard m=bb(f)|bb(t); byColor_[(int)colorOf(p)]^=m; byType_[(int)typeOf(p)]^=m; }
void Position::rebuildHash(){ std::uint64_t h=0; for(int s=0;s<64;++s){ Piece p=board_[s]; if(p!=Piece::None) h^=ZP[(int)p][s]; } if(stm_==Color::Black) h^=ZS; h^=ZC[castling_&15]; if(ep_!=SQ_NONE) h^=ZE[fileOf(ep_)]; hash_=h; }
void Position::setStartpos(){ setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }
bool Position::setFen(const std::string& fen){ initZobrist(); clear(); std::istringstream ss(fen); std::string board, stm, cast, ep, hm, fm; if(!(ss>>board)) return false; if(!(ss>>stm)) return false; if(!(ss>>cast)) return false; if(!(ss>>ep)) return false; if(!(ss>>hm)) hm="0"; if(!(ss>>fm)) fm="1";
 int r=7,f=0;
 for(char c:board){ if(c=='/'){ r--; f=0; } else if(c>='1'&&c<='8'){ f+=c-'0'; } else { if(f>7||r<0) return false; Piece p=Piece::None; switch(c){ case 'P':p=Piece::WP;break; case 'N':p=Piece::WN;break; case 'B':p=Piece::WB;break; case 'R':p=Piece::WR;break; case 'Q':p=Piece::WQ;break; case 'K':p=Piece::WK;break; case 'p':p=Piece::BP;break; case 'n':p=Piece::BN;break; case 'b':p=Piece::BB;break; case 'r':p=Piece::BR;break; case 'q':p=Piece::BQ;break; case 'k':p=Piece::BK;break; default: return false; }
  putPiece(makeSquare(f,r),p); f++; } }
 stm_=(stm=="b")?Color::Black:Color::White;
 castling_=0; if(cast.find('K')!=std::string::npos)castling_|=1; if(cast.find('Q')!=std::string::npos)castling_|=2; if(cast.find('k')!=std::string::npos)castling_|=4; if(cast.find('q')!=std::string::npos)castling_|=8;
 if(ep=="-"||ep.size()<2) ep_=SQ_NONE; else { int ff=ep[0]-'a', rr=ep[1]-'1'; if(ff<0||ff>7||rr<0||rr>7) ep_=SQ_NONE; else ep_=makeSquare(ff,rr); }
 halfmove_=std::stoi(hm); fullmove_=std::stoi(fm);
 rebuildHash(); return true; }
std::string Position::fen() const{ std::string s; for(int r=7;r>=0;--r){ int emp=0; for(int f=0;f<8;++f){ Piece p=board_[makeSquare(f,r)]; if(p==Piece::None){emp++;} else { if(emp){s.push_back('0'+emp);emp=0;} char c='?'; switch(p){case Piece::WP:c='P';break;case Piece::WN:c='N';break;case Piece::WB:c='B';break;case Piece::WR:c='R';break;case Piece::WQ:c='Q';break;case Piece::WK:c='K';break;case Piece::BP:c='p';break;case Piece::BN:c='n';break;case Piece::BB:c='b';break;case Piece::BR:c='r';break;case Piece::BQ:c='q';break;case Piece::BK:c='k';break;default:break;} s.push_back(c);} } if(emp) s.push_back('0'+emp); if(r) s.push_back('/'); }
 s+=(stm_==Color::White?" w ":" b ");
 if(!castling_) s+="-"; else { if(castling_&1)s+="K"; if(castling_&2)s+="Q"; if(castling_&4)s+="k"; if(castling_&8)s+="q"; }
 s+=" "; if(ep_==SQ_NONE) s+="-"; else { s.push_back('a'+fileOf(ep_)); s.push_back('1'+rankOf(ep_)); }
 s+=" "+std::to_string(halfmove_)+" "+std::to_string(fullmove_); return s; }
Bitboard Position::occupancy() const{ return byColor_[0]|byColor_[1]; }
Bitboard Position::pieces(Color c) const{ return byColor_[(int)c]; }
Bitboard Position::pieces(Color c, PieceType t) const{ return byColor_[(int)c]&byType_[(int)t]; }
Square Position::kingSquare(Color c) const{ Bitboard k=pieces(c,PieceType::King); if(!k) return SQ_NONE; return lsb(k); }
bool Position::isAttacked(Square s, Color by) const{
 Bitboard occ=occupancy();
 if(by==Color::White){ if(kPawnAttacks[1][s]&pieces(Color::White,PieceType::Pawn)) return true; }
 else { if(kPawnAttacks[0][s]&pieces(Color::Black,PieceType::Pawn)) return true; }
 if(kKnightAttacks[s]&pieces(by,PieceType::Knight)) return true;
 if(kKingAttacks[s]&pieces(by,PieceType::King)) return true;
 Bitboard bq=pieces(by,PieceType::Bishop)|pieces(by,PieceType::Queen);
 if(bishopAttacks(s,occ)&bq) return true;
 Bitboard rq=pieces(by,PieceType::Rook)|pieces(by,PieceType::Queen);
 if(rookAttacks(s,occ)&rq) return true;
 return false;
}
bool Position::inCheck() const{ Square k=kingSquare(stm_); if(k==SQ_NONE) return false; return isAttacked(k, opposite(stm_)); }
void Position::doNull(UndoState& u){ u.castling=castling_; u.ep=ep_; u.halfmove=halfmove_; u.captured=Piece::None; u.hash=hash_;
 if(ep_!=SQ_NONE){ hash_^=ZE[fileOf(ep_)]; ep_=SQ_NONE; }
 hash_^=ZS; stm_=opposite(stm_); if(stm_==Color::White) fullmove_++; }
void Position::undoNull(const UndoState& u){ stm_=opposite(stm_); if(stm_==Color::Black) fullmove_--; castling_=u.castling; ep_=u.ep; halfmove_=u.halfmove; hash_=u.hash; }
void Position::doMove(Move m, UndoState& u){
 u.castling=castling_; u.ep=ep_; u.halfmove=halfmove_; u.hash=hash_; u.captured=Piece::None;
 Square f=m.from(), t=m.to();
 Piece moving=board_[f];
 Color us=stm_, them=opposite(us);
 // hash out old ep + castle
 if(ep_!=SQ_NONE) hash_^=ZE[fileOf(ep_)];
 hash_^=ZC[castling_&15];
 // capture piece (incl ep)
 Piece cap=Piece::None;
 Square capSq=t;
 if(m.flags==Move::EnPassant){ capSq=makeSquare(fileOf(t),rankOf(f)); cap=board_[capSq]; }
 else cap=board_[t];
 u.captured=cap;
 // remove ep immediately
 ep_=SQ_NONE;
 bool isCastle=(m.flags==Move::CastleK||m.flags==Move::CastleQ);
 bool isPromo=m.isPromo();
 // update castling rights for king/rook moves and rook captures
 int newCast=castling_;
 if(typeOf(moving)==PieceType::King){ if(us==Color::White) newCast&=~(1|2); else newCast&=~(4|8); }
 if(f==makeSquare(0,0)||t==makeSquare(0,0)) newCast&=~2;
 if(f==makeSquare(7,0)||t==makeSquare(7,0)) newCast&=~1;
 if(f==makeSquare(0,7)||t==makeSquare(0,7)) newCast&=~8;
 if(f==makeSquare(7,7)||t==makeSquare(7,7)) newCast&=~4;
 // hash out moving piece from-square
 hash_^=ZP[(int)moving][f];
 if(cap!=Piece::None){ hash_^=ZP[(int)cap][capSq]; removePiece(capSq); }
 removePiece(f);
 // place moving / promo
 if(isCastle){
   // king
   putPiece(t,moving); hash_^=ZP[(int)moving][t];
   if(us==Color::White){ if(m.flags==Move::CastleK){ // H1->F1
       Piece r=board_[makeSquare(7,0)]; hash_^=ZP[(int)r][makeSquare(7,0)]; removePiece(makeSquare(7,0)); putPiece(makeSquare(5,0),r); hash_^=ZP[(int)r][makeSquare(5,0)];
     } else { Piece r=board_[makeSquare(0,0)]; hash_^=ZP[(int)r][makeSquare(0,0)]; removePiece(makeSquare(0,0)); putPiece(makeSquare(3,0),r); hash_^=ZP[(int)r][makeSquare(3,0)]; }
   } else { if(m.flags==Move::CastleK){ Piece r=board_[makeSquare(7,7)]; hash_^=ZP[(int)r][makeSquare(7,7)]; removePiece(makeSquare(7,7)); putPiece(makeSquare(5,7),r); hash_^=ZP[(int)r][makeSquare(5,7)];
     } else { Piece r=board_[makeSquare(0,7)]; hash_^=ZP[(int)r][makeSquare(0,7)]; removePiece(makeSquare(0,7)); putPiece(makeSquare(3,7),r); hash_^=ZP[(int)r][makeSquare(3,7)]; }
   }
 } else if(isPromo){
   Piece pp=makePiece(us,m.promoType()); putPiece(t,pp); hash_^=ZP[(int)pp][t];
 } else {
   putPiece(t,moving); hash_^=ZP[(int)moving][t];
   if(m.flags==Move::DoublePush){ Square epsq=makeSquare(fileOf(f),(rankOf(f)+rankOf(t))/2); ep_=epsq; hash_^=ZE[fileOf(epsq)]; }
 }
 castling_=newCast; hash_^=ZC[castling_&15];
 // halfmove
 if(typeOf(moving)==PieceType::Pawn||cap!=Piece::None) halfmove_=0; else halfmove_++;
 // side + fullmove
 hash_^=ZS; stm_=them; if(stm_==Color::White) fullmove_++;
}
void Position::undoMove(Move m, const UndoState& u){
 // revert side first
 stm_=opposite(stm_); if(stm_==Color::Black) fullmove_--;
 Square f=m.from(), t=m.to();
 Color us=stm_;
 bool isCastle=(m.flags==Move::CastleK||m.flags==Move::CastleQ);
 bool isPromo=m.isPromo();
 if(isCastle){
   removePiece(t);
   putPiece(f,makePiece(us,PieceType::King));
   if(us==Color::White){ if(m.flags==Move::CastleK){ removePiece(makeSquare(5,0)); putPiece(makeSquare(7,0),Piece::WR); } else { removePiece(makeSquare(3,0)); putPiece(makeSquare(0,0),Piece::WR); } }
   else { if(m.flags==Move::CastleK){ removePiece(makeSquare(5,7)); putPiece(makeSquare(7,7),Piece::BR); } else { removePiece(makeSquare(3,7)); putPiece(makeSquare(0,7),Piece::BR); } }
 } else if(isPromo){
   removePiece(t); putPiece(f,makePiece(us,PieceType::Pawn));
 } else {
   Piece moving=board_[t]; removePiece(t); putPiece(f,moving);
 }
 if(u.captured!=Piece::None){
   if(m.flags==Move::EnPassant){ Square capSq=makeSquare(fileOf(t),rankOf(f)); putPiece(capSq,u.captured); }
   else putPiece(t,u.captured);
 }
 castling_=u.castling; ep_=u.ep; halfmove_=u.halfmove; hash_=u.hash;
}
bool Position::isLegal(Move m) const{
 // quick sanity: from piece belongs to stm
 Piece p=board_[m.from()]; if(p==Piece::None||colorOf(p)!=stm_) return false;
 Position c=*this; UndoState u; c.doMove(m,u);
 Square k=c.kingSquare(stm_); // stm_ is mover (c side flipped)
 if(k==SQ_NONE) return false;
 return !c.isAttacked(k, c.stm_);
}
bool Position::isRepetition(int plyFromRoot, const std::uint64_t* hist, int histLen) const{
 int reps=0; for(int i=histLen-1;i>=0;--i){ if(hist[i]==hash_){ reps++; if(reps>=2) return true; } }
 (void)plyFromRoot; return reps>=1 && halfmove_<100 ? (reps>=1 ? false : false) : false;
 // Note: caller handles fifty-move; this helper reports at least one prior occurrence.
 // We return false for single rep (not yet triple); true only on second repeat (threefold).
}
} // namespace chessclaw
