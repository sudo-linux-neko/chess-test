// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/train/data_gen.h"
#include "chessclaw/evaluate.h"
#include "chessclaw/search.h"
#include <cstdio>
#include <random>
#include <thread>
namespace chessclaw::train {
bool saveSamples(const std::string& path, const std::vector<Sample>& v){
 FILE* f=std::fopen(path.c_str(),"wb"); if(!f) return false;
 std::uint32_t n=(std::uint32_t)v.size(); std::fwrite(&n,4,1,f);
 for(auto &s:v){ std::uint8_t L=(std::uint8_t)std::min<std::size_t>(255,s.fen.size()); std::fwrite(&L,1,1,f); std::fwrite(s.fen.data(),1,L,f); std::int16_t sc=(std::int16_t)s.scoreCp; std::fwrite(&sc,2,1,f); std::uint8_t r=(std::uint8_t)(s.result*100); std::fwrite(&r,1,1,f); }
 std::fclose(f); return true;
}
bool loadSamples(const std::string& path, std::vector<Sample>& out){
 FILE* f=std::fopen(path.c_str(),"rb"); if(!f) return false;
 std::uint32_t n=0; if(std::fread(&n,4,1,f)!=1){std::fclose(f);return false;}
 out.clear(); out.reserve(n);
 for(std::uint32_t i=0;i<n;++i){ std::uint8_t L=0; if(std::fread(&L,1,1,f)!=1) break; std::string fen(L,' '); if(L&&std::fread(fen.data(),1,L,f)!=L) break; std::int16_t sc=0; std::uint8_t r=50; if(std::fread(&sc,2,1,f)!=1) break; if(std::fread(&r,1,1,f)!=1) break; out.push_back({fen,sc,r/100.0}); }
 std::fclose(f); return true;
}
int generateSelfplay(const GenConfig& cfg){
 // Fast self-play: random openings + shallow search labels, game result by final eval.
 std::vector<Sample> all; all.reserve((std::size_t)cfg.games*20);
 int perThread=(cfg.games+cfg.threads-1)/cfg.threads;
 std::vector<std::vector<Sample>> outs((std::size_t)cfg.threads);
 std::atomic<int> doneGames{0};
 std::atomic<int> doneSamples{0};
 std::vector<std::thread> th;
 for(int t=0;t<cfg.threads;++t){
  th.emplace_back([&,t]{
   std::mt19937_64 rng(cfg.seed+t*10007);
   Searcher s; s.setThreads(1); s.setHashMb(16);
   for(int g=0;g<perThread;++g){
    int gi=t*perThread+g; if(gi>=cfg.games) break;
    Position pos; pos.setStartpos();
    // random opening: 4-8 ply random legal moves
    int openPly=4+(rng()%5);
    for(int i=0;i<openPly;++i){ std::vector<Move> lm; generateMoves(pos,lm,true,false); if(lm.empty()) break; Move m=lm[rng()%lm.size()]; UndoState u; pos.doMove(m,u); if(pos.halfmove()>=100) break; }
    std::vector<std::pair<std::string,int>> gamePos;
    for(int ply=0;ply<cfg.movesPerGame;++ply){
     std::vector<Move> lm; generateMoves(pos,lm,true,false);
     if(lm.empty()) break;
     if(pos.halfmove()>=100) break;
     // shallow search for move choice + label
     SearchLimits lim; lim.maxDepth=cfg.depth;
     Move best=s.search(pos,lim);
     int sc=s.stats().scoreCp.load();
     // store position before move (fen + stm-relative score)
     int stmScore=(pos.sideToMove()==Color::White)?sc:-sc;
     gamePos.emplace_back(pos.fen(),stmScore);
     // epsilon-greedy: 15% random to diversify
     Move chosen=best;
     if((rng()%100)<15) chosen=lm[rng()%lm.size()];
     else { bool ok=false; for(auto m:lm) if(m==chosen){ok=true;break;} if(!ok) chosen=lm[rng()%lm.size()]; }
     UndoState u; pos.doMove(chosen,u);
     // adjudicate: big eval = resign
     if(std::abs(s.staticEval(pos))>1200 && ply>20) break;
    }
    int finalE=evaluateClassical(pos);
    double res=0.5; if(finalE>150) res=1.0; else if(finalE<-150) res=0.0;
    // assign result from white perspective -> convert to stm perspective per sample? Store white-perspective result; trainer converts.
    // Determine white result: if stm black to move positions, keep white result; trainer flips.
    for(auto &gp:gamePos){
     Position p; p.setFen(gp.first);
     double r = res; // white perspective
     outs[(std::size_t)t].push_back({gp.first,gp.second,r});
    }
    doneGames.fetch_add(1);
    doneSamples.fetch_add((int)gamePos.size());
    if(cfg.progress) cfg.progress(doneGames.load(), cfg.games, doneSamples.load());
    // cap per-thread memory
    if(outs[(std::size_t)t].size()>80000) break;
   }
  });
 }
 for(auto &t:th) t.join();
 for(auto &o:outs) all.insert(all.end(),o.begin(),o.end());
 if(!saveSamples(cfg.outPath,all)) return -1;
 return (int)all.size();
}
} // namespace chessclaw::train
