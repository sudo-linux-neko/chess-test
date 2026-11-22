// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/ui/charts.h"
#include "chessclaw/ui/terminal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace chessclaw::ui {
static const char* kBlocks[9]={" ","\u2581","\u2582","\u2583","\u2584","\u2585","\u2586","\u2587","\u2588"};
void renderBar(DoubleBuffer& db,int r,int c,int w,double frac,const std::string& label){
 if(frac<0)frac=0; if(frac>1)frac=1;
 int fill=(int)(frac*w);
 std::string s;
 for(int i=0;i<w;++i){
  double t=(double)i/(double)(w-1>0?w-1:1);
  if(i<fill) s+=gradientColor(t,80,140,255,255,80,80)+std::string(kBlocks[8]);
  else s+=std::string(kDim)+"-"+kReset;
 }
 s+=kReset;
 db.put(r,c,s);
 if(!label.empty()) db.put(r+1,c,label);
}
void renderCpuBars(DoubleBuffer& db,int r,int c,int n,const std::vector<double>& util){
 for(int i=0;i<n;++i){
  double u=(i<(int)util.size())?util[i]:0.0;
  int h=(int)(u*6); if(h<0)h=0; if(h>6)h=6;
  for(int k=0;k<6;++k){
   std::string ch=(k>=6-h)?kBlocks[8]:".";
   double t=(double)k/6.0;
   db.put(r+5-k,c+i*2,gradientColor(t,60,200,120,255,200,60)+ch+kReset);
  }
  char b[8]; std::snprintf(b,sizeof(b),"%d",i%10);
  db.put(r+6,c+i*2,b);
 }
}
static void drawAxes(DoubleBuffer& db,int top,int left,int h,int w,const std::string& title,double mn,double mx){
 db.box(top,left,h,w,title);
 char b[32]; std::snprintf(b,sizeof(b),"%.1f",mx); db.put(top+1,left+1,b);
 std::snprintf(b,sizeof(b),"%.1f",mn); db.put(top+h-2,left+1,b);
}
void renderLineChart(DoubleBuffer& db,int top,int left,int h,int w,const std::vector<double>& a,const std::vector<double>& b,const std::string& title,const std::string& la,const std::string& lb,bool logS){
 double mn=1e9,mx=-1e9;
 auto upd=[&](double v){ double vv=logS?(v>0?std::log10(v+1e-9):-9):v; if(vv<mn)mn=vv; if(vv>mx)mx=vv; };
 for(auto v:a) upd(v); for(auto v:b) upd(v);
 if(mx<=mn){mx=mn+1;}
 drawAxes(db,top,left,h,w,title,mn,mx);
 int pw=w-4, ph=h-4;
 if(pw<4)pw=4; if(ph<2)ph=2;
 auto plot=[&](const std::vector<double>& v,const std::string& col){
  if(v.empty()) return;
  for(int x=0;x<pw;++x){
   std::size_t idx=(std::size_t)((double)x/(pw-1)*(v.size()-1));
   double vv=logS?(v[idx]>0?std::log10(v[idx]+1e-9):-9):v[idx];
   double t=(vv-mn)/(mx-mn); int y=(int)((1-t)*(ph-1));
   int lvl=(int)(t*8); if(lvl<0)lvl=0; if(lvl>8)lvl=8;
   db.put(top+2+y,left+2+x,col+kBlocks[lvl]+kReset);
  }
 };
 plot(a,rgbFg(80,180,255));
 plot(b,rgbFg(255,120,80));
 db.put(top+h-1,left+2,rgbFg(80,180,255)+"-"+la+kReset+" "+rgbFg(255,120,80)+"-"+lb+kReset);
}
void renderEloChart(DoubleBuffer& db,int top,int left,int h,int w,const std::vector<double>& elo,const std::string& title){
 double mn=1e9,mx=-1e9; for(auto v:elo){if(v<mn)mn=v;if(v>mx)mx=v;}
 if(elo.empty()){mn=0;mx=100;}
 if(mx<=mn)mx=mn+50;
 drawAxes(db,top,left,h,w,title,mn,mx);
 int pw=w-4, ph=h-4; if(pw<4)pw=4; if(ph<2)ph=2;
 for(int x=0;x<pw;++x){
  std::size_t idx=elo.empty()?0:(std::size_t)((double)x/(pw-1)*(elo.size()-1));
  double v=elo.empty()?mn:elo[idx];
  double t=(v-mn)/(mx-mn); int y=(int)((1-t)*(ph-1));
  db.put(top+2+y,left+2+x,gradientColor(t,80,140,255,255,60,60)+kBlocks[8]+kReset);
 }
 db.put(top+h-1,left+2,std::string(kDim)+"ESTIMATE from self-play"+kReset);
}
} // namespace chessclaw::ui
