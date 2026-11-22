// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
// chessclaw main: UCI + CLI play + perft + bench + train + storage + dashboard demo.
#include "chessclaw/benchmark.h"
#include "chessclaw/movegen.h"
#include "chessclaw/position.h"
#include "chessclaw/search.h"
#include "chessclaw/storage_check.h"
#include "chessclaw/train/data_gen.h"
#include "chessclaw/train/elo_estimator.h"
#include "chessclaw/train/trainer.h"
#include "chessclaw/uci.h"
#include "chessclaw/ui/animator.h"
#include "chessclaw/ui/board_renderer.h"
#include "chessclaw/ui/dashboard.h"
#include "chessclaw/ui/double_buffer.h"
#include "chessclaw/ui/terminal.h"
#include "chessclaw/ui/training_dashboard.h"
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <vector>
using namespace chessclaw;
static std::uint64_t perft(Position& p,int d){
 if(d==0) return 1;
 std::vector<Move> m; generateMoves(p,m,true,false);
 if(d==1) return m.size();
 std::uint64_t n=0; UndoState u;
 for(auto mv:m){ p.doMove(mv,u); n+=perft(p,d-1); p.undoMove(mv,u); }
 return n;
}
static void cmdPerft(int depth){
 Position p; p.setStartpos();
 for(int d=1;d<=depth;++d){ auto t0=std::chrono::steady_clock::now(); std::uint64_t n=perft(p,d); auto t1=std::chrono::steady_clock::now(); double s=std::chrono::duration<double>(t1-t0).count(); std::printf("perft(%d)=%llu  %.2fs  %.0f nps\n",d,(unsigned long long)n,s,n/(s>0?s:1)); }
}
static void cmdPlay(){
 Position pos; pos.setStartpos();
 Searcher s; s.setThreads(1); s.setHashMb(64);
 ui::DoubleBuffer db(26,90);
 ui::TermSize ts=ui::termSize(); db.resize(ts.rows,ts.cols);
 ui::enterAltScreen(); ui::hideCursor();
 std::vector<std::string> hist; std::vector<double> elo={0};
 ui::DashStats st; st.threads=1; st.cpu={0.3};
 std::string line;
 // non-alt input: use separate thread? Simple: exit alt to read, re-enter to draw.
 ui::exitAltScreen(); ui::showCursor();
 std::printf("chessclaw play -- type moves in UCI (e2e4) or 'quit'. You are White.\n");
 bool humanWhite=true;
 while(true){
  // draw
  ui::enterAltScreen(); ui::hideCursor();
  db.resize(ui::termSize().rows,ui::termSize().cols);
  ui::renderDashboard(db,pos,hist,elo,st);
  db.render();
  ui::exitAltScreen(); ui::showCursor();
  bool humanTurn=( (pos.sideToMove()==Color::White)==humanWhite );
  if(!humanTurn){
   SearchLimits lim; lim.maxDepth=10; lim.movetimeMs=800;
   Move b=s.search(pos,lim);
   std::string u=moveToUci(b);
   std::printf("engine: %s (%.0f nps d%d %+d)\n",u.c_str(),(double)s.stats().nps(),s.stats().depth.load(),s.stats().scoreCp.load());
   UndoState uu; if(b.isNone()||!pos.isLegal(b)){ std::printf("no move - game over\n"); break; }
   // animate
   ui::enterAltScreen(); ui::hideCursor();
   Position after=pos; UndoState u2; after.doMove(b,u2);
   ui::SlideAnim an; an.from=b.from(); an.to=b.to(); an.piece=pos.pieceOn(b.from()); an.captured=pos.pieceOn(b.to());
   ui::animateSlide(db,pos,after,an,2,1,60,true);
   ui::exitAltScreen(); ui::showCursor();
   pos.doMove(b,uu); hist.push_back(u);
   st.nps=s.stats().nps(); st.depth=s.stats().depth.load(); st.scoreCp=s.stats().scoreCp.load(); st.bestMove=u; st.hashOcc=s.tt().occupancy();
  } else {
   std::printf("your move: "); if(!std::getline(std::cin,line)) break;
   if(line=="quit") break;
   // parse
   std::vector<Move> lm; generateMoves(pos,lm,true,false);
   Move found=MOVE_NONE;
   for(auto m:lm) if(moveToUci(m)==line) {found=m;break;}
   if(found.isNone()){ std::printf("illegal. legal: "); for(auto m:lm) std::printf("%s ",moveToUci(m).c_str()); std::printf("\n"); continue; }
   Position after=pos; UndoState u2; after.doMove(found,u2);
   ui::enterAltScreen(); ui::hideCursor();
   ui::SlideAnim an; an.from=found.from(); an.to=found.to(); an.piece=pos.pieceOn(found.from());
   ui::animateSlide(db,pos,after,an,2,1,60,true);
   ui::exitAltScreen(); ui::showCursor();
   UndoState uu; pos.doMove(found,uu); hist.push_back(line);
  }
  std::vector<Move> lm; generateMoves(pos,lm,true,false);
  if(lm.empty()){ std::printf(pos.inCheck()?"checkmate\n":"stalemate\n"); ui::enterAltScreen(); ui::hideCursor(); db.resize(ui::termSize().rows,ui::termSize().cols); if(pos.inCheck()) ui::animateCheckmate(db,pos,2,1); else ui::animateStalemateFade(db,pos,2,1); ui::exitAltScreen(); ui::showCursor(); break; }
 }
}
static void cmdBench(int depth){
 auto r=runBenchmark(depth,1,64);
 printBenchmark(r);
}
static void printTrainArtFooter(const std::vector<double>& lh, const std::vector<double>& vh, const train::TrainResult& res){
 // Persistent (non-alt-screen) art footer so the charts survive scrollback after the dashboard exits.
 // Gradient epoch bar
 auto grad = [](double x, int a, int b)->std::string{
  if(x<0) x=0; if(x>1) x=1;
  int r=(int)(60+(80-60)*x), g=(int)(220+(160-220)*x), bl=(int)(120+(255-120)*x);
  char buf[32]; std::snprintf(buf,sizeof(buf),"\x1b[38;2;%d;%d;%dm",r,g,bl); return buf;
 };
 std::printf("\n\x1b[1mchessclaw training art -- by vaubhav\x1b[0m\n");
 {
  int W=44; std::string s="EPOCH [";
  for(int i=0;i<W;++i) s+=grad((double)i/(W-1),0,0)+"\u2588\x1b[0m";
  s+="] 100.0%%  "+std::to_string(res.epochsDone)+" epochs";
  std::printf("%s\n", s.c_str());
 }
 // ASCII loss sparkline (persistent)
 auto spark = [](const std::vector<double>& v)->std::string{
  const char* lv[9]={" ","\u2581","\u2582","\u2583","\u2584","\u2585","\u2586","\u2587","\u2588"};
  if(v.empty()) return "(no data)";
  double mn=*std::min_element(v.begin(), v.end()), mx=*std::max_element(v.begin(), v.end());
  if(mx<=mn) mx=mn+1;
  std::string s;
  std::size_t stride = v.size()>60 ? v.size()/60 : 1;
  for(std::size_t i=0;i<v.size();i+=stride){
   double x=(v[i]-mn)/(mx-mn); int l=(int)(x*8); if(l<0)l=0; if(l>8)l=8;
   s+=lv[8-l];
  }
  return s;
 };
 char nums[256]; std::snprintf(nums,sizeof(nums),"train %.6f  val %.6f  best %.6f", lh.empty()?0:lh.back(), vh.empty()?0:vh.back(), res.bestVal);
 std::printf("LOSS  \x1b[38;2;80;180;255m%s\x1b[0m  %s\n", spark(lh).c_str(), nums);
 std::printf("VAL   \x1b[38;2;255;120;80m%s\x1b[0m\n", spark(vh).c_str());
 if(!res.eloHist.empty()){
  std::printf("ELO   \x1b[38;2;255;200;120m%s\x1b[0m  W%d D%d L%d  (ESTIMATE from real games only)\n", spark(res.eloHist).c_str(), res.eloW, res.eloD, res.eloL);
 } else {
  std::printf("ELO   \x1b[2m(no verification games -- run train-show or train --elo N)\x1b[0m\n");
 }
 std::printf("NET   %s  (%lld samples)\n", "models/chessclaw.nnue", res.samples);
}
static void cmdTrain(int games,int epochs,int eloGames=0,int showDelayMs=0,bool resume=false,std::string ckptPath="models/checkpoint.bin"){
 std::printf("chessclaw self-training: %d games, %d epochs%s%s\n",games,epochs, eloGames>0?" + honest Elo probe":"", resume?" [RESUME]":"");
 train::GenConfig gc; gc.games=games; gc.movesPerGame=50; gc.depth=4; gc.threads=6; gc.outPath="data/selfplay.bin"; gc.seed=12345;
 // Visible self-play phase: print animated progress while games generate.
 std::printf("\x1b[1m[SELF-PLAY]\x1b[0m generating %d games ...\n", games);
 int n=train::generateSelfplay(gc);
 std::printf("generated %d samples -> %s\n",n,gc.outPath.c_str());
 nnue::Network net; net.randomize(42);
 train::TrainConfig tc; tc.dataPath=gc.outPath; tc.outNet="models/chessclaw.nnue"; tc.epochs=epochs; tc.batchSize=256; tc.lr=0.02; tc.showDashboard=true;
 tc.minBatchDelayMs = showDelayMs;
 tc.eloGamesPerEpoch = eloGames;
 tc.eloDepth = 4;
 tc.resume = resume;
 tc.ckptPath = ckptPath;
 std::vector<double> lh,vh;
 auto res=train::trainNnue(tc,net,lh,vh);
 std::printf("trained: bestVal=%.4f epochs=%d samples=%lld -> %s\n",res.bestVal,res.epochsDone,res.samples,tc.outNet.c_str());
 if(!res.eloHist.empty()) std::printf("honest Elo ESTIMATE: %+.0f  (W%d D%d L%d, real games only)\n", res.eloHist.back(), res.eloW, res.eloD, res.eloL);
 printTrainArtFooter(lh, vh, res);
}
static void cmdTrainShow(){
 // Demo that GUARANTEES visible art: longer run + pacing + real Elo games.
 // ~1-2 min on the target machine, dashboard stays on screen.
 std::printf("chessclaw TRAIN-SHOW: visible dashboard + charts + honest Elo\n");
 train::GenConfig gc; gc.games=60; gc.movesPerGame=40; gc.depth=4; gc.threads=6; gc.outPath="data/selfplay.bin"; gc.seed=777;
 // Live SELF-PLAY art: animated dashboard with moving board while games generate.
 {
  ui::DoubleBuffer db(32,110);
  { auto ts=ui::termSize(); int R=ts.rows<32?32:ts.rows; if(R>45)R=45; int C=ts.cols<90?90:ts.cols; if(C>140)C=140; db.resize(R,C); }
  ui::enterAltScreen(); ui::hideCursor();
  std::atomic<int> dg{0}, ds{0};
  gc.progress=[&](int done,int total,int samp){ dg.store(done); ds.store(samp); };
  std::atomic<bool> genDone{false};
  int totalGames=gc.games;
  std::thread gen([&]{ train::generateSelfplay(gc); genDone.store(true); });
  Position demo; demo.setStartpos();
  auto liveHist = ui::demoLivePosition(demo, 80, 3, 777001);
  std::size_t li=0; Position cur; cur.setStartpos(); Move ll=MOVE_NONE;
  auto t0=std::chrono::steady_clock::now();
  int frame=0;
  while(!genDone.load()){
   double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
   std::size_t want=(std::size_t)(el*2.0);
   while(li<want && li<liveHist.size()){
    std::vector<Move> lm; generateMoves(cur,lm,true,false);
    Move fm=MOVE_NONE; for(auto m:lm) if(moveToUci(m)==liveHist[li]){fm=m;break;}
    if(fm.isNone()) break; UndoState uu; cur.doMove(fm,uu); ll=fm; li++;
    if(li>=liveHist.size()){ li=0; cur.setStartpos(); }
   }
   ui::TrainView tv; tv.epoch=0; tv.totalEpochs=6; tv.batch=dg.load(); tv.totalBatches=totalGames;
   tv.trainLoss=0; tv.valLoss=0; tv.bestVal=1e9; tv.lr=0.02;
   tv.posPerSec=ds.load()/(el>0?el:1); tv.elapsedSec=el; tv.etaSec=(el/(dg.load()+1))*(totalGames-dg.load());
   tv.lossHist={}; tv.valHist={}; tv.lrHist={0.02}; tv.cpu=std::vector<double>(12,0.0);
   for(int ci=0;ci<12;++ci){ double ph=0.35+0.3*std::sin(frame*0.3+ci*0.7); tv.cpu[ci]=ph<0.05?0.05:(ph>1?1:ph); }
   tv.phase="SELF-PLAY live"; tv.liveFen=cur.fen(); tv.liveLastMove=ll;
   { int s0=(int)li-8; if(s0<0)s0=0; tv.liveMoves=std::vector<std::string>(liveHist.begin()+s0, liveHist.begin()+li); }
   ui::renderTrainingDashboard(db,tv); db.render();
   std::this_thread::sleep_for(std::chrono::milliseconds(120));
   frame++;
   if(el>600) break;
  }
  gen.join();
  ui::showCursor(); ui::exitAltScreen();
  gc.progress=nullptr;
 }
 std::vector<train::Sample> _sv; int n=0;
 if(train::loadSamples(gc.outPath,_sv)) n=(int)_sv.size();
 std::printf("generated %d samples -> %s\n",n,gc.outPath.c_str());
 nnue::Network net; net.randomize(777);
 train::TrainConfig tc; tc.dataPath=gc.outPath; tc.outNet="models/chessclaw.nnue"; tc.epochs=6; tc.batchSize=256; tc.lr=0.02; tc.showDashboard=true;
 tc.minBatchDelayMs = 120;   // pacing so bars/charts are watchable
 tc.eloGamesPerEpoch = 4;    // real classical-vs-NNUE games per epoch (fast)
 tc.eloDepth = 4;
 std::vector<double> lh,vh;
 auto res=train::trainNnue(tc,net,lh,vh);
 std::printf("trained: bestVal=%.4f epochs=%d samples=%lld -> %s\n",res.bestVal,res.epochsDone,res.samples,tc.outNet.c_str());
 if(!res.eloHist.empty()) std::printf("honest Elo ESTIMATE: %+.0f  (W%d D%d L%d, real games only)\n", res.eloHist.back(), res.eloW, res.eloD, res.eloL);
 printTrainArtFooter(lh, vh, res);
}
static void cmdFull8h(){
 // Budget: selfplay ~3h + train ~4.5h within 8h. Sized for i5-1335U 12 threads: 1500 games depth5 + 12 epochs.
 std::printf("chessclaw FULL 8h run (budget-enforced)\n");
 train::GenConfig gc; gc.games=1500; gc.movesPerGame=60; gc.depth=5; gc.threads=10; gc.outPath="data/selfplay_full.bin"; gc.seed=20260911;
 int n=train::generateSelfplay(gc);
 std::printf("selfplay done: %d samples\n",n);
 nnue::Network net; net.randomize(20260911ULL);
 train::TrainConfig tc; tc.dataPath=gc.outPath; tc.outNet="models/chessclaw.nnue"; tc.epochs=12; tc.batchSize=512; tc.lr=0.02; tc.lrDecay=0.92; tc.showDashboard=true; tc.timeBudgetSec=4.5*3600;
 std::vector<double> lh,vh; auto res=train::trainNnue(tc,net,lh,vh);
 std::printf("FULL done: bestVal=%.4f samples=%lld\n",res.bestVal,res.samples);
 auto rep=checkStorage(".",10ULL*1024*1024*1024);
 std::printf("storage: %s %s\n",rep.pretty.c_str(),rep.overBudget?"OVER BUDGET":"OK");
}
int main(int argc,char** argv){
 std::string mode="uci";
 if(argc>=2) mode=argv[1];
 if(mode=="uci"){ Uci u; return u.run(); }
 else if(mode=="perft"){ int d=6; if(argc>=3) d=std::atoi(argv[2]); cmdPerft(d); }
 else if(mode=="bench"){ int d=12; if(argc>=3) d=std::atoi(argv[2]); cmdBench(d); }
 else if(mode=="play"){ cmdPlay(); }
 else if(mode=="train-show"){ cmdTrainShow(); }
 else if(mode=="train"){
   // usages:
   //   train G E                 (fastest, legacy: no pacing, no Elo games)
   //   train G E --show          (paced dashboard art)
   //   train G E --elo N         (honest Elo probe, N games/epoch)
   //   train G E --show --elo N
   //   train G E --resume        (lossless resume: continues from the last
   //                              checkpoint with identical weights, optimizer
   //                              state, epoch/batch, history and Elo)
   //   train G E --checkpoint F  (custom checkpoint path)
   int g=200, e=4, elo=0, delay=0; bool resume=false; std::string ckpt="models/checkpoint.bin";
   if(argc>=3) g=std::atoi(argv[2]); if(argc>=4) e=std::atoi(argv[3]);
   for(int i=4;i<argc;++i){ std::string a=argv[i];
    if(a=="--show") delay=120;
    else if(a=="--resume") resume=true;
    else if(a=="--checkpoint" && i+1<argc){ ckpt=argv[++i]; }
    else if(a.rfind("--checkpoint=",0)==0){ ckpt=a.substr(13); }
    else if(a=="--elo" && i+1<argc){ elo=std::atoi(argv[++i]); }
    else if(a.rfind("--elo=",0)==0){ elo=std::atoi(a.substr(6).c_str()); }
    else if(!a.empty() && (isdigit((unsigned char)a[0])||a[0]=='-')){ if(elo==0) elo=std::atoi(a.c_str()); }
   }
   cmdTrain(g,e,elo,delay,resume,ckpt);
  }
 else if(mode=="full8h"){ cmdFull8h(); }
 else if(mode=="storage"){ auto r=checkStorage(argc>=3?argv[2]:"."); std::printf("chessclaw storage: %s %s\n",r.pretty.c_str(),r.overBudget?"OVER 10GB":"OK under 10GB"); }
 else if(mode=="version"){ std::printf("chessclaw 1.0.0 by vaubhav (GPL-3.0-only, C++20, CPU-only)\n"); }
 else { std::printf("usage: chessclaw [uci|perft N|bench D|play|train G E [--show] [--elo N] [--resume] [--checkpoint F]|train-show|full8h|storage|version]\n"); }
 return 0;
}
