// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/ui/dashboard.h"
#include "chessclaw/ui/board_renderer.h"
#include "chessclaw/ui/charts.h"
#include "chessclaw/ui/terminal.h"
#include <cstdio>
namespace chessclaw::ui {
void renderMoveHistory(DoubleBuffer& db,int top,int left,int h,int w,const std::vector<std::string>& moves){
 db.box(top,left,h,w,"moves");
 int maxLines=h-2;
 int start=0; if((int)moves.size()>maxLines) start=(int)moves.size()-maxLines;
 for(int i=0;i<maxLines&&(start+i)<(int)moves.size();++i){
  int idx=start+i;
  bool cur=(idx==(int)moves.size()-1);
  double fade=(double)i/(maxLines>1?maxLines-1:1);
  std::string s=moves[(std::size_t)idx];
  char num[16]; std::snprintf(num,sizeof(num),"%2d.",idx/2+1);
  std::string line=(idx%2==0)?(std::string(num)+" "+s):(std::string("    ")+s);
  std::string col=cur?std::string(kBold)+rgbFg(255,255,160):gradientColor(fade,90,90,90,220,220,220);
  db.put(top+1+i,left+1,col+line+kReset);
 }
}
void renderDashboard(DoubleBuffer& db,const Position& pos,const std::vector<std::string>& hist,const std::vector<double>& elo,const DashStats& st,Move best){
 db.clear();
 int rows=db.rows(),cols=db.cols();
 int boardW=36, rightX=boardW+1;
 int rightW=cols-rightX-1; if(rightW<30) rightW=30;
 db.put(0,1,rgbFg(120,200,255)+kBold+"chessclaw 1.0.0 by vaubhav  --  CPU-only C++20"+kReset);
 renderBoard(db,pos,2,1,best);
 // right panels
 int rh=rows-1;
 int h1=8,h2=8;
 renderMoveHistory(db,1,rightX,h1,rightW,hist);
 renderEloChart(db,1+h1,rightX,h2,rightW,elo);
 int stTop=1+h1+h2;
 int stH=rh-stTop; if(stH<6) stH=6;
 db.box(stTop,rightX,stH,rightW,"engine");
 char b[128];
 std::snprintf(b,sizeof(b),"nps %.0f  depth %d  score %+dcp",st.nps,st.depth,st.scoreCp); db.put(stTop+1,rightX+1,b);
 renderBar(db,stTop+2,rightX+1,rightW-2,(st.scoreCp+1000)/2000.0,"eval");
 std::snprintf(b,sizeof(b),"hash %.0f%%  t %.1f/%.1fs",st.hashOcc*100,st.timeUsedS,st.timeLeftS); db.put(stTop+3,rightX+1,b);
 renderBar(db,stTop+4,rightX+1,rightW-2,st.hashOcc,"hash");
 std::snprintf(b,sizeof(b),"threads %d  best %s",st.threads,st.bestMove.c_str()); db.put(stTop+5,rightX+1,b);
 if(stH>8) renderCpuBars(db,stTop+6,rightX+1,st.threads,st.cpu);
 if(!st.pv.empty()&&stH>14){ std::string pvline="PV:"; for(std::size_t i=0;i<st.pv.size()&&i<4;++i) pvline+=" "+st.pv[i]; db.put(stTop+stH-2,rightX+1,pvline.substr(0,(std::size_t)(rightW-2))); }
}
} // namespace chessclaw::ui
