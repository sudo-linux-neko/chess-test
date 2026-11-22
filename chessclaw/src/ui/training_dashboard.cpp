// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/ui/training_dashboard.h"
#include "chessclaw/ui/anime_bg.h"
#include "chessclaw/ui/board_renderer.h"
#include "chessclaw/ui/charts.h"
#include "chessclaw/ui/progress_bar.h"
#include "chessclaw/ui/terminal.h"
#include <chrono>
#include <cstdio>
namespace chessclaw::ui {
static double nowSec(){
 using namespace std::chrono;
 static auto t0=steady_clock::now();
 return duration<double>(steady_clock::now()-t0).count();
}
static int sFrame=0;
void renderTrainingDashboard(DoubleBuffer& db,const TrainView& v){
 double t=nowSec(); int frame=sFrame++;
 renderAnimeBackground(db,t,frame);
 int cols=db.cols(), rows=db.rows();
 std::string phase=v.phase.empty()?"TRAINING":v.phase;
 char title[180]; std::snprintf(title,sizeof(title),"chessclaw \u2728 %s \u2728 by vaubhav \u2728 anime-night live",phase.c_str());
 renderNeonTitle(db,0,title);
 double epFrac=v.totalEpochs?(double)v.epoch/(double)v.totalEpochs:0;
 double bFrac=v.totalBatches?(double)v.batch/(double)v.totalBatches:0;
 if(epFrac<0)epFrac=0; if(epFrac>1)epFrac=1;
 if(bFrac<0)bFrac=0; if(bFrac>1)bFrac=1;
  // LEFT: live chess board with moving pieces (from TrainView.liveFen or startpos)
  Position bpos;
  if(!v.liveFen.empty()){ if(!bpos.setFen(v.liveFen)) bpos.setStartpos(); }
  else bpos.setStartpos();
  int boardTop=2, boardLeft=1;
  int boardW=34, boardH=17;
  int chartLeft=boardLeft+boardW+1;
  if(chartLeft+32 < cols && boardTop+boardH < rows){
   renderBoard(db,bpos,boardTop,boardLeft,v.liveLastMove,-1,t,false);
   if(boardTop+boardH-1<rows){
    std::string strip;
    int n=(int)v.liveMoves.size(); int s0=n>8?n-8:0;
    for(int i=s0;i<n;++i){ if(i>s0) strip+=" "; strip+=v.liveMoves[(std::size_t)i]; }
    if(strip.empty()) strip="e2e4 e7e5 g1f3 ...";
    db.put(boardTop+boardH-1,boardLeft,rgbFg(160,190,255)+strip.substr(0,(std::size_t)boardW)+kReset);
   }
  } else { chartLeft=1; }
 char ebuf[96]; std::snprintf(ebuf,sizeof(ebuf),"epoch %d/%d",v.epoch,v.totalEpochs);
 renderAnimeProgressBar(db,1,chartLeft,cols-chartLeft-4,epFrac,ebuf,v.posPerSec,t);
 char bbuf[96]; std::snprintf(bbuf,sizeof(bbuf),"batch %d/%d",v.batch,v.totalBatches);
 renderAnimeProgressBar(db,2,chartLeft,cols-chartLeft-4,bFrac,bbuf,0.0,t+1.7);
 int chartH=7, cTop=3;
 int halfW=(cols-chartLeft)/2;
 if(chartH+3<rows && halfW>10){
  renderLineChart(db,cTop,chartLeft,chartH,halfW-1,v.lossHist,v.valHist,"loss \u2726 (blue=train red=val)","train","val");
  renderLineChart(db,cTop,chartLeft+halfW,chartH,cols-chartLeft-halfW-1,v.lrHist,{},"lr (log)","lr","",true);
 }
 int infoRow=cTop+chartH+1;
 if(infoRow<rows){
  char st[256]; std::snprintf(st,sizeof(st),"\u25c6 train %.6f  \u25c6 val %.6f  \u25c6 best %.6f  lr %.2e",v.trainLoss,v.valLoss,v.bestVal,v.lr);
  db.put(infoRow,chartLeft,rgbFg(255,255,180)+kBold+st+kReset);
 }
 if(infoRow+1<rows){
  std::string eta=formatEta(v.etaSec);
  std::string etaCol=(v.etaSec>3600)?rgbFg(100,255,120):(v.etaSec>600?rgbFg(255,220,100):rgbFg(255,100,100));
  char st2[256]; std::snprintf(st2,sizeof(st2),"\u26a1 POS/S %.0f  \u23f3 ETA %s  %.0fs",v.posPerSec,eta.c_str(),v.elapsedSec);
  db.put(infoRow+1,chartLeft,etaCol+kBold+st2+kReset);
 }
 int eloTop=infoRow+2, eloH=6;
 if(eloTop+eloH+1<rows && cols-chartLeft>24){
  if(!v.eloHist.empty()){
   renderEloChart(db,eloTop,chartLeft,eloH,cols-chartLeft-1,v.eloHist,"Elo ESTIMATE \u2728 (real games)");
   std::string lab=v.eloLabel.empty()?"+0 ESTIMATE":v.eloLabel;
   char eb[192]; std::snprintf(eb,sizeof(eb),"ELO %s",lab.c_str());
   if(eloTop+eloH<rows) db.put(eloTop+eloH,chartLeft,rgbFg(255,200,120)+kBold+std::string(eb)+kReset);
  } else {
   db.box(eloTop,chartLeft,eloH,cols-chartLeft-1,"Elo ESTIMATE \u2728 (real games)");
   if(eloTop+2<rows) db.put(eloTop+2,chartLeft+2,std::string(kDim)+"Elo after epoch 1"+kReset);
   if(eloTop+3<rows) db.put(eloTop+3,chartLeft+2,std::string(kDim)+"--elo N for probe"+kReset);
  }
 }
 int cpuRow=eloTop+eloH+1;
 if(cpuRow+6<rows){
  db.put(cpuRow,chartLeft,rgbFg(140,255,170)+kBold+"CPU x12 \u26a1"+kReset);
  renderCpuBars(db,cpuRow+1,chartLeft,12,v.cpu);
 }
 renderVignette(db);
}
} // namespace chessclaw::ui
