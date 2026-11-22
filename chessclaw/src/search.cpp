// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/search.h"
#include "chessclaw/time_manager.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
namespace chessclaw {
static const int kMate=32000;
static const int kInf=33000;
Searcher::Searcher(){ threadData_.resize(1); tt_.resize(64); stats_.start=std::chrono::steady_clock::now(); }
void Searcher::setThreads(int n){ if(n<1)n=1; if(n>64)n=64; numThreads_=n; threadData_.resize((std::size_t)n); for(int i=0;i<n;++i){ threadData_[i].id=i; threadData_[i].seed=0x9e3779b9ULL*(i+1); } }
std::int64_t SearchStats::nps() const{ auto now=std::chrono::steady_clock::now(); double s=std::chrono::duration<double>(now-start).count(); if(s<0.01) s=0.01; return (std::int64_t)(nodes.load()/s); }
bool Searcher::shouldStop() const{
 if(stats_.stopped.load()) return true;
 if(stats_.nodes.load()>=stats_.hardMs*1000000LL) return true; // misuse guard (not used)
 auto now=std::chrono::steady_clock::now();
 double el=std::chrono::duration<double,std::milli>(now-stats_.start).count();
 if(el>=stats_.hardMs) return true;
 return false;
}
int Searcher::staticEval(const Position& pos){
 int e=evaluate(pos);
 return (pos.sideToMove()==Color::White)?e:-e;
}
int Searcher::lmrReduction(int depth, int idx, bool pv) const{
 if(idx<3||depth<3) return 0;
 double r=std::log((double)depth)*std::log((double)(idx+1))/2.2;
 int red=(int)r;
 if(pv) red--; if(red<0) red=0; if(red>depth-1) red=depth-1;
 return red;
}
int Searcher::quiescence(Position& pos, int alpha, int beta, int ply, ThreadData& td, std::uint64_t* hist, int histLen){
 td.nodes++; stats_.nodes.fetch_add(1,std::memory_order_relaxed);
 if((td.nodes&1023)==0 && shouldStop()) return alpha;
 if(ply>=120) return staticEval(pos);
 // fifty / repetition
 if(pos.halfmove()>=100) return 0;
 bool inChk=pos.inCheck();
 int stand=0;
 if(!inChk){
  stand=staticEval(pos);
  if(stand>=beta) return beta;
  if(stand>alpha) alpha=stand;
  // delta pruning: even queen win can't beat alpha
  // futility margin
 }
 std::vector<Move> moves;
 if(inChk){ generateMoves(pos,moves,true,false); }
 else generateMoves(pos,moves,true,true);
 // order by SEE/MVV
 for(auto &m:moves){ m.score=(std::int16_t)std::clamp(moveOrderBaseScore(pos,m),-30000,30000); }
 std::sort(moves.begin(),moves.end(),[](const Move&a,const Move&b){return a.score>b.score;});
 int best= inChk ? -kInf : stand;
 int n=0;
 for(auto m:moves){
  // SEE pruning in quiescence (not when in check)
  if(!inChk){ int s=see(pos,m); if(s<-20) continue; }
  UndoState u; if(!pos.isLegal(m)) continue;
  pos.doMove(m,u);
  if(histLen<1024){ hist[histLen]=pos.hash(); }
  int sc=-quiescence(pos,-beta,-alpha,ply+1,td,hist,histLen+1);
  pos.undoMove(m,u);
  if(stats_.stopped.load()) return alpha;
  if((td.nodes&1023)==0 && shouldStop()) return alpha;
  if(sc>best){ best=sc; if(sc>alpha) alpha=sc; if(alpha>=beta) break; }
  n++;
 }
 if(inChk && n==0){
  // checkmate or stalemate: no legal moves while in check => mated; else (no moves but not in check handled above)
  return -kMate+ply;
 }
 if(!inChk && moves.empty()) return stand;
 // stalemate in quiescence? if no captures but has quiets, stand is fine.
 return best;
}
int Searcher::negamax(Position& pos, int depth, int alpha, int beta, int ply, bool pvNode, ThreadData& td, std::uint64_t* hist, int histLen){
 if(ply>=120) return staticEval(pos);
 td.nodes++; stats_.nodes.fetch_add(1,std::memory_order_relaxed);
 if((td.nodes&1023)==0){ if(shouldStop()) return alpha; }
 // fifty-move / repetition (twofold => draw-ish: return 0 if repeated once before? use simple: if hash seen in hist => 0)
 if(pos.halfmove()>=100) return 0;
 for(int i=histLen-2;i>=0&&i>=histLen-9;--i){ if(hist[i]==pos.hash()) return 0; }
 bool inChk=pos.inCheck();
 if(inChk) depth++; // check extension (capped by ply)
 if(depth<=0) return quiescence(pos,alpha,beta,ply,td,hist,histLen);
 // TT probe
 Move ttMove; int ttDepth=-1,ttFlag=0,ttScore=0,ttEval=0;
 bool ttHit=tt_.probe(pos.hash(),ttMove,ttDepth,ttFlag,ttScore,ttEval,ply);
 if(ttHit && ttDepth>=depth && !pvNode){
  if(ttFlag==0) return ttScore;
  else if(ttFlag==1 && ttScore>=beta) return ttScore;
  else if(ttFlag==2 && ttScore<=alpha) return ttScore;
 }
 int staticE=staticEval(pos);
 // reverse futility / static null move
 if(!pvNode && !inChk && depth<=6){
  int margin=110*depth;
  if(staticE-margin>=beta) return staticE-margin;
 }
 // null move pruning with verification
 if(!pvNode && !inChk && depth>=3 && ply>0){
  // need at least one non-pawn piece
  bool hasPiece=false;
  Color us=pos.sideToMove();
  if(pos.pieces(us,PieceType::Knight)|pos.pieces(us,PieceType::Bishop)|pos.pieces(us,PieceType::Rook)|pos.pieces(us,PieceType::Queen)) hasPiece=true;
  if(hasPiece && staticE>=beta){
   UndoState u; pos.doNull(u);
   if(histLen<1024) hist[histLen]=pos.hash();
   int R=3+depth/4;
   int sc=-negamax(pos,depth-1-R,-beta,-beta+1,ply+1,false,td,hist,histLen+1);
   pos.undoNull(u);
   if(sc>=beta) return sc;
  }
 }
 // futility at frontier
 bool futile=false;
 if(!pvNode && !inChk && depth==1){
  if(staticE+140<=alpha) futile=true;
 }
 // generate moves
 std::vector<Move> moves; generateMoves(pos,moves,true,false);
 if(moves.empty()){ if(inChk) return -kMate+ply; return 0; }
 // order: TT move, captures (MVV), killers, counter, history
 for(auto &m:moves){
  int s=moveOrderBaseScore(pos,m);
  if(ttHit && m==ttMove) s+=1000000;
  else {
   if(!m.isCapture()){
    if(ply<64){
     if(m==td.killers[ply][0]) s+=90000;
     else if(m==td.killers[ply][1]) s+=80000;
     if(m==td.counter[(int)pos.pieceOn(m.from())%64][m.to()]) s+=70000;
     s+=td.history[m.from()][m.to()]/16;
    }
   }
  }
  m.score=(std::int16_t)std::clamp(s,-30000,30000);
 }
 std::sort(moves.begin(),moves.end(),[](const Move&a,const Move&b){return a.score>b.score;});
 int bestScore=-kInf; Move bestMove=MOVE_NONE;
 int flag=2; // upper
 int moveIdx=0;
 // extended futility: skip quiet moves that can't raise alpha at low depth
 for(auto m:moves){
  bool isCap=m.isCapture(); bool isPromo=m.isPromo();
  // futility pruning
  if(!pvNode && !inChk && !isCap && !isPromo && depth<=3 && moveIdx>0){
   int margin = depth==1?140:depth==2?280:420;
   if(staticE+margin<=alpha) continue;
  }
  // SEE pruning for captures at low depth
  if(!pvNode && depth<=4 && isCap && moveIdx>0){
   int s=see(pos,m);
   if(s<-30*depth) continue;
  }
  UndoState u; pos.doMove(m,u);
  if(histLen<1024) hist[histLen]=pos.hash();
  int sc;
  if(moveIdx==0){
   sc=-negamax(pos,depth-1,-beta,-alpha,ply+1,pvNode,td,hist,histLen+1);
  } else {
   int R=lmrReduction(depth,moveIdx,pvNode);
   // late move pruning: skip very late quiets at low depth
   if(!pvNode && !inChk && !isCap && !isPromo && depth<=3 && moveIdx>8+depth*4) { pos.undoMove(m,u); moveIdx++; continue; }
   if(R>0){
    sc=-negamax(pos,depth-1-R,-alpha-1,-alpha,ply+1,false,td,hist,histLen+1);
    if(sc>alpha) sc=-negamax(pos,depth-1,-alpha-1,-alpha,ply+1,false,td,hist,histLen+1);
   } else {
    sc=-negamax(pos,depth-1,-alpha-1,-alpha,ply+1,false,td,hist,histLen+1);
   }
   if(sc>alpha && sc<beta){
    sc=-negamax(pos,depth-1,-beta,-alpha,ply+1,true,td,hist,histLen+1);
   }
  }
  pos.undoMove(m,u);
  if(stats_.stopped.load()) return alpha;
  if((td.nodes&511)==0 && shouldStop()) return alpha;
  moveIdx++;
  if(sc>bestScore){ bestScore=sc; bestMove=m; }
  if(sc>alpha){ alpha=sc; flag=0; }
  if(alpha>=beta){
   flag=1;
   // update killers/history/counter
   if(!isCap && ply<64){
    if(td.killers[ply][0]!=m){ td.killers[ply][1]=td.killers[ply][0]; td.killers[ply][0]=m; }
    td.history[m.from()][m.to()]+=depth*depth;
    // aging: decay occasionally
    if((td.nodes&4095)==0){ for(auto &r:td.history) for(auto &v:r) v/=2; }
    // countermove: store response to previous move (approx via from/to of beta move keyed by prev mover piece)
    // Simplified: key by moving piece type of current move's from-square piece before move is gone -> use to-square piece? Use best-effort.
   }
   break;
  }
 }
 tt_.store(pos.hash(),bestMove,depth,flag,bestScore,staticE,ply);
 return bestScore;
}
void Searcher::computeTimeLimits(const Position& pos, const SearchLimits& lim){
 using namespace std::chrono;
 stats_.start=steady_clock::now();
 stats_.stopped.store(false);
 if(lim.movetimeMs>=0){ stats_.softMs=lim.movetimeMs-30; stats_.hardMs=lim.movetimeMs-5; if(stats_.softMs<10)stats_.softMs=10; if(stats_.hardMs<20)stats_.hardMs=20; return; }
 if(lim.infinite){ stats_.softMs=(std::int64_t)9e12; stats_.hardMs=(std::int64_t)9e12; return; }
 if(lim.wtimeMs<0 && lim.btimeMs<0){ stats_.softMs=1000; stats_.hardMs=5000; if(lim.maxNodes!=INT64_MAX){stats_.softMs=(std::int64_t)9e12; stats_.hardMs=(std::int64_t)9e12;} return; }
 TimeControl tc{lim.wtimeMs,lim.btimeMs,lim.wincMs,lim.bincMs,lim.movestogo,lim.movetimeMs,lim.infinite};
 int isW=(pos.sideToMove()==Color::White)?1:0;
 auto b=allocateTime(tc,isW,50);
 stats_.softMs=b.softMs; stats_.hardMs=b.hardMs;
 if(lim.maxNodes!=INT64_MAX){ /* node limit handled in loop */ }
}
int Searcher::searchSingle(Position& pos, const SearchLimits& lim, int tid, std::vector<Move>& outPv){
 ThreadData& td=threadData_[(std::size_t)tid];
 td.nodes=0; td.seldepth=0;
 // clear killers/history for fresh search (keep across ID within this call? clear once)
 for(auto &k:td.killers){k[0]=MOVE_NONE;k[1]=MOVE_NONE;}
 for(auto &r:td.history) r.fill(0);
 for(auto &r:td.counter) r.fill(MOVE_NONE);
 std::uint64_t hist[1024]; int histLen=0;
 hist[0]=pos.hash(); histLen=1;
 int maxD=std::min(lim.maxDepth,64);
 // move limit set
 std::vector<Move> rootMoves; generateMoves(pos,rootMoves,true,false);
 if(!lim.searchMoves.empty()){
  std::vector<Move> f;
  for(auto m:rootMoves) for(auto s:lim.searchMoves) if(m.from()==s.from()&&m.to()==s.to()&&(s.promoType()==PieceType::None||m.promoType()==s.promoType())) f.push_back(m);
  rootMoves=std::move(f);
 }
 if(rootMoves.empty()) return 0;
 if(rootMoves.size()==1){ outPv=rootMoves; stats_.pv=rootMoves; stats_.scoreCp.store(staticEval(pos)); stats_.depth.store(1); return staticEval(pos); }
 int alpha=-kInf, beta=kInf;
 int prevScore=staticEval(pos);
 Move best=rootMoves[0];
 int bestScore=prevScore;
 // iterative deepening with aspiration
 for(int depth=1;depth<=maxD;++depth){
  if(shouldStop()) break;
  if(stats_.nodes.load()>=lim.maxNodes) break;
  int window=25;
  int a=prevScore-window, b=prevScore+window;
  if(depth<4){a=-kInf;b=kInf;}
  int sc=0;
  // order root moves by history/TT each iteration
  {
   Move ttM; int td2,tf,ts,te; bool hit=tt_.probe(pos.hash(),ttM,td2,tf,ts,te,0);
   for(auto &m:rootMoves){ int s=moveOrderBaseScore(pos,m); if(hit&&m==ttM) s+=1000000; s+=td.history[m.from()][m.to()]/8; m.score=(std::int16_t)std::clamp(s,-30000,30000); }
   std::sort(rootMoves.begin(),rootMoves.end(),[](const Move&a,const Move&b){return a.score>b.score;});
  }
  // search root moves with PVS (sequential within thread)
  int curBest=-kInf; Move curMove=rootMoves[0];
  int curA=a, curB=b;
  bool failed=false;
  for(std::size_t i=0;i<rootMoves.size();++i){
   Move m=rootMoves[i];
   UndoState u; pos.doMove(m,u); hist[histLen]=pos.hash();
   int s;
   if(i==0) s=-negamax(pos,depth-1,-curB,-curA,1,true,td,hist,histLen+1);
   else {
    int R=lmrReduction(depth,(int)i,true);
    if(R>0) s=-negamax(pos,depth-1-R,-curA-1,-curA,1,false,td,hist,histLen+1);
    else s=-negamax(pos,depth-1,-curA-1,-curA,1,false,td,hist,histLen+1);
    if(s>curA&&s<curB) s=-negamax(pos,depth-1,-curB,-curA,1,true,td,hist,histLen+1);
   }
   pos.undoMove(m,u);
   if(shouldStop()||stats_.stopped.load()){ failed=true; break; }
   // update ordering score
   rootMoves[i].score=(std::int16_t)std::clamp(s,-30000,30000);
   if(s>curBest){ curBest=s; curMove=m; }
   if(s>curA) curA=s;
   // time: soft cutoff after first root iterations? check
   auto now=std::chrono::steady_clock::now();
   double el=std::chrono::duration<double,std::milli>(now-stats_.start).count();
   if(el>=stats_.softMs && i>0) { /* allow finish current depth? break to respect soft */ }
   if(el>=stats_.hardMs){ stats_.stopped.store(true); failed=true; break; }
   if(stats_.nodes.load()>=lim.maxNodes){ stats_.stopped.store(true); failed=true; break; }
  }
  if(failed) break;
  // aspiration re-search
  if(curBest<=a||curBest>=b){
   // full window research at same depth
   curA=-kInf; curB=kInf; curBest=-kInf;
   for(std::size_t i=0;i<rootMoves.size();++i){
    Move m=rootMoves[i]; UndoState u; pos.doMove(m,u); hist[histLen]=pos.hash();
    int s = (i==0)? -negamax(pos,depth-1,-curB,-curA,1,true,td,hist,histLen+1)
                   : -negamax(pos,depth-1,-curA-1,-curA,1,false,td,hist,histLen+1);
    if(s>curA&&s<curB&&i>0) s=-negamax(pos,depth-1,-curB,-curA,1,true,td,hist,histLen+1);
    pos.undoMove(m,u);
    if(shouldStop()){failed=true;break;}
    if(s>curBest){curBest=s;curMove=m;}
    if(s>curA)curA=s;
   }
   if(failed) break;
  }
  bestScore=curBest; best=curMove; prevScore=bestScore;
  // move best to front
  for(std::size_t i=0;i<rootMoves.size();++i) if(rootMoves[i]==best){ std::swap(rootMoves[0],rootMoves[i]); break; }
  if(tid==0){
   stats_.depth.store(depth);
   stats_.scoreCp.store(bestScore);
   stats_.pv=rootMoves; // approx PV: best first
   // extract real PV from TT
   std::vector<Move> pv; Position tmp=pos; int ply=0;
   Move cur=best;
   pv.push_back(cur);
   UndoState u; tmp.doMove(cur,u);
   for(int d=0;d<depth&&ply<32;++d){ Move nm; int dd,ff,ss,ee; if(!tt_.probe(tmp.hash(),nm,dd,ff,ss,ee,ply+1)) break; if(nm.isNone()) break; // verify legal
    std::vector<Move> lm; generateMoves(tmp,lm,true,false); bool ok=false; for(auto m:lm) if(m==nm){ok=true;break;} if(!ok) break;
    pv.push_back(nm); UndoState u2; tmp.doMove(nm,u2); ply++; }
   stats_.pv=pv; outPv=pv;
  }
  // soft time cutoff: stop if elapsed > soft and depth >= 6
  {
   auto now=std::chrono::steady_clock::now(); double el=std::chrono::duration<double,std::milli>(now-stats_.start).count();
   if(el>=stats_.softMs && depth>=6) break;
  }
  // mate found: stop
  if(std::abs(bestScore)>kMate-100) break;
 }
 if(tid==0){ outPv=stats_.pv; }
 return bestScore;
}
Move Searcher::search(Position& pos, const SearchLimits& lim){
 computeTimeLimits(pos,lim);
 tt_.newSearch();
 stats_.nodes.store(0);
 stats_.depth.store(0);
 std::vector<Move> pv;
 if(numThreads_==1){ searchSingle(pos,lim,0,pv); }
 else {
  std::vector<std::thread> th;
  std::vector<std::vector<Move>> pvs((std::size_t)numThreads_);
  std::atomic<bool> done{false};
  for(int i=1;i<numThreads_;++i){
   th.emplace_back([this,&pos,&lim,i,&pvs](){
    Position p=pos; // copy (Position is copyable)
    SearchLimits l=lim; l.maxDepth=lim.maxDepth; // same depth; TT shared gives speedup
    // jitter: helpers start deeper aspiration to diversify
    searchSingle(p,l,i,pvs[(std::size_t)i]);
   });
  }
  searchSingle(pos,lim,0,pvs[0]);
  stats_.stopped.store(true);
  for(auto &t:th) t.join();
  pv=pvs[0];
 }
 if(pv.empty()){ std::vector<Move> lm; generateMoves(pos,lm,true,false); if(!lm.empty()) return lm[0]; return MOVE_NONE; }
 return pv[0];
}
int contemptValue(){ return 0; }
} // namespace chessclaw
