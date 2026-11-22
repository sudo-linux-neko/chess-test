// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
// Procedural anime-night background: pure C++20, ANSI 24-bit truecolor.
// Tokyo-night palette. No downloads needed; works on xterm-256color + WSL2.
// Optional: CHESSCLAW_BG=/path/to.ppm dimmed photo backdrop (PPM P3/P6).
#include "chessclaw/ui/anime_bg.h"
#include "chessclaw/ui/double_buffer.h"
#include "chessclaw/ui/terminal.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
namespace chessclaw::ui {
static std::uint32_t hash32(std::uint32_t x){
 x^=x>>16; x*=0x7feb352d; x^=x>>15; x*=0x846ca68b; x^=x>>16; return x;
}
static double frand(std::uint32_t s){ return (hash32(s)%1000000)/1000000.0; }
Petal petalAt(int i, double t, int cols, int rows){
 double sp = 1.2 + frand(i*3+1)*2.2;
 double dr = 1.0 + frand(i*3+2)*2.5;
 double ph = frand(i*3+3)*6.28318;
 double y = std::fmod(frand(i*7+11)*rows + t*sp, (double)rows);
 double x = std::fmod(frand(i*13+5)*cols + t*dr + std::sin(t*0.9+ph)*3.0 + cols, (double)cols);
 return {x,y,sp,dr,ph,frand(i*17+9)};
}
double starTwinkle(int i, double t){ return 0.35+0.65*(0.5+0.5*std::sin(t*(0.6+frand(i)*2.0)+frand(i+999)*6.28318)); }
struct Img { int w{0},h{0}; std::vector<std::uint8_t> px; };
static Img gBg; static std::string gBgPath; static bool gBgTried=false;
static bool loadPpm(const std::string& path, Img& out){
 FILE* f=std::fopen(path.c_str(),"rb"); if(!f) return false;
 char magic[3]={0}; if(std::fscanf(f,"%2s",magic)!=1){std::fclose(f);return false;}
 auto nextInt=[&]()->int{ int c,v=0; bool neg=false;
  do{ c=std::fgetc(f); if(c=='#'){ while(c!='\n'&&c!=EOF) c=std::fgetc(f);} }while(c!=EOF&&(c<'0'||c>'9')&&c!='-');
  if(c==EOF) return -1; if(c=='-'){neg=true;c=std::fgetc(f);}
  for(;c>='0'&&c<='9';c=std::fgetc(f)) v=v*10+c-'0'; if(c!=EOF) std::ungetc(c,f); return neg?-v:v; };
 int w=nextInt(), h=nextInt(), mx=nextInt();
 if(w<=0||h<=0||w>4096||h>4096){std::fclose(f);return false;}
 std::fgetc(f);
 out.w=w; out.h=h; out.px.resize((std::size_t)w*h*3);
 if(magic[1]=='6'){ if(std::fread(out.px.data(),1,out.px.size(),f)!=out.px.size()){std::fclose(f);return false;} }
 else if(magic[1]=='3'){ for(std::size_t i=0;i<out.px.size();++i){ int v=nextInt(); if(v<0)v=0; if(v>255)v=255; out.px[i]=(std::uint8_t)(v*255/(mx>0?mx:255)); } }
 else {std::fclose(f);return false;}
 std::fclose(f); return true;
}
bool hasBackgroundImage(){ const char* p=std::getenv("CHESSCLAW_BG"); return p&&*p; }
std::string backgroundImagePath(){ const char* p=std::getenv("CHESSCLAW_BG"); return p?p:""; }
static void renderPhotoBg(DoubleBuffer& db){
 if(!gBgTried){ gBgTried=true; gBgPath=backgroundImagePath(); if(!gBgPath.empty()) loadPpm(gBgPath,gBg); }
 if(gBg.px.empty()) return;
 int rows=db.rows(), cols=db.cols();
 for(int r=0;r<rows;++r){
  int iy0=(r*2*gBg.h)/(rows*2), iy1=((r*2+1)*gBg.h)/(rows*2);
  if(iy0>=gBg.h) iy0=gBg.h-1; if(iy1>=gBg.h) iy1=gBg.h-1;
  std::string line; line.reserve((std::size_t)cols*24);
  for(int c=0;c<cols;++c){
   int ix=(c*gBg.w)/cols; if(ix>=gBg.w) ix=gBg.w-1;
   std::size_t o0=((std::size_t)iy0*gBg.w+ix)*3, o1=((std::size_t)iy1*gBg.w+ix)*3;
   int r0=(gBg.px[o0]*35)/100, g0=(gBg.px[o0+1]*35)/100+6, b0=(gBg.px[o0+2]*35)/100+14;
   int r1=(gBg.px[o1]*35)/100, g1=(gBg.px[o1+1]*35)/100+6, b1=(gBg.px[o1+2]*35)/100+14;
   if(r0>255)r0=255; if(g0>255)g0=255; if(b0>255)b0=255;
   if(r1>255)r1=255; if(g1>255)g1=255; if(b1>255)b1=255;
   char buf[64]; std::snprintf(buf,sizeof(buf),"\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm\xe2\x96\x84",r0,g0,b0,r1,g1,b1);
   line+=buf;
  }
  line+=kReset; db.put(r,0,line);
 }
}
void renderNeonTitle(DoubleBuffer& db, int row, const std::string& text){
 int c=(db.cols()-(int)text.size())/2; if(c<0)c=0;
 std::string s;
 for(std::size_t i=0;i<text.size();++i){
  double t=text.size()>1?(double)i/(text.size()-1):0;
  int r=(int)(125+(255-125)*t), g=(int)(207+(158-207)*t), b=(int)(255+(224-255)*t);
  char buf[40]; std::snprintf(buf,sizeof(buf),"\x1b[38;2;%d;%d;%dm",r,g,b);
  s+=buf; s+=text[i];
 }
 s+=kReset; db.put(row,c,s);
}
void renderVignette(DoubleBuffer& db){
 int rows=db.rows(), cols=db.cols();
 for(int r=0;r<rows;++r){
  double vr=std::min(1.0, std::min(r,rows-1-r)/(rows*0.18));
  if(vr>=1.0) continue;
  int dim=(int)((1.0-vr)*38);
  char buf[48]; std::snprintf(buf,sizeof(buf),"\x1b[48;2;%d;%d;%dm",26-dim/3,27-dim/3,38-dim/4);
  std::string edge=buf;
  db.put(r,0,edge+" "+kReset); db.put(r,cols-1,edge+" "+kReset);
 }
}
void renderAnimeBackground(DoubleBuffer& db, double t, int frame){
 (void)frame;
 if(hasBackgroundImage()){ renderPhotoBg(db); return; }
 int rows=db.rows(), cols=db.cols();
 if(rows<4||cols<8) return;
 double mx=cols*0.80, my=rows*0.18+std::sin(t*0.3)*0.6;
 double mr=std::min(cols,rows)*0.09+2.0;
 int horizon=(int)(rows*0.62);
 for(int r=0;r<rows;++r){
  std::string line; line.reserve((std::size_t)cols*22);
  double v=rows>1?(double)r/(rows-1):0;
  int br,bg2,bb2;
  if(v<0.62){ double k=v/0.62; br=(int)(26+(13-26)*k); bg2=(int)(27+(15-27)*k); bb2=(int)(38+(26-38)*k); }
  else { double k=(v-0.62)/0.38; br=(int)(13+(20-13)*k); bg2=(int)(15+(24-15)*k); bb2=(int)(26+(36-26)*k); }
  double band=std::sin(r*0.35+t*0.7)*3.0+std::sin(r*0.13-t*0.4)*3.0;
  int ab=(int)band;
  for(int c=0;c<cols;++c){
   int rr=br,gg=bg2,bb=bb2;
   if(r<horizon){
    std::uint32_t h=hash32((std::uint32_t)(r*7919+c*104729));
    if(h%89==0){ double tw=starTwinkle((int)(h%100000),t); int s=(int)(120+135*tw); rr=(rr+s)/2; gg=(gg+s)/2; bb=(bb+255)/2; }
    double dx=c-mx, dy=(r-my)*2.0;
    double d=std::sqrt(dx*dx+dy*dy);
    if(d<mr){ rr=240; gg=238; bb=230; }
    else if(d<mr+3){ double k=1.0-(d-mr)/3.0; rr=(int)(rr+(200-rr)*k*0.7); gg=(int)(gg+(195-gg)*k*0.7); bb=(int)(bb+(190-bb)*k*0.7); }
    else if(d<mr+8){ double k=1.0-(d-mr-3)/5.0; rr+=(int)(40*k); gg+=(int)(30*k); bb+=(int)(60*k); }
   }
   if(r>=horizon-6 && r<=horizon){
    double rg1=horizon-2.2-2.0*std::sin(c*0.11)-1.1*std::sin(c*0.031+2.0);
    double rg2=horizon-0.6-1.4*std::sin(c*0.21+1.0)-0.8*std::sin(c*0.07+4.0);
    if(r>=rg1 && r<rg2){ rr=36; gg=40; bb=62; }
    else if(r>=rg2){ rr=24; gg=28; bb=48; }
   }
   if(r>horizon){
    double sh=std::sin(c*0.5+t*2.0+r*0.8)*0.5+std::sin(c*0.17-t*1.2)*0.5;
    double mcol=1.0-std::min(1.0,std::abs(c-mx)/(cols*0.06));
    if(mcol>0){ int gl=(int)(mcol*(18+14*sh)); rr+=gl; gg+=gl; bb+=(int)(gl*1.2); }
    if(r==horizon+1){ std::uint32_t h2=hash32((std::uint32_t)(c*31+7));
     if(h2%23==0){ rr=255; gg=158; bb=100; } else if(h2%31==0){ rr=125; gg=207; bb=255; } else if(h2%41==0){ rr=187; gg=154; bb=247; } }
    rr+=ab/3; bb+=ab/2;
   }
   if(std::abs(r-horizon)<=1){ rr=(rr+90)/2; gg=(gg+100)/2; bb=(bb+140)/2; }
   if(rr<0)rr=0; if(rr>255)rr=255; if(gg<0)gg=0; if(gg>255)gg=255; if(bb<0)bb=0; if(bb>255)bb=255;
   char buf[32]; std::snprintf(buf,sizeof(buf),"\x1b[48;2;%d;%d;%dm ",rr,gg,bb);
   line+=buf;
  }
  line+=kReset; db.put(r,0,line);
 }
 int nPet=cols/14+4; if(nPet>20) nPet=20; // sparse petals: calmer, museum-grade
 for(int i=0;i<nPet;++i){
  Petal p=petalAt(i,t,cols,rows);
  int c=(int)p.x, r=(int)p.y;
  if(c<0||c>=cols||r<0||r>=rows) continue;
  double sway=0.5+0.5*std::sin(t*2.0+p.phase);
  const char* glyph=p.size>0.66?"\u2740":(p.size>0.33?"\u273f":"\u00b7");
  int pg=(int)(140+60*sway), pb=(int)(180+30*(1-sway));
  char buf[80]; std::snprintf(buf,sizeof(buf),"\x1b[38;2;%d;%d;%dm%s",247,pg,pb,glyph);
  db.put(r,c,std::string(buf)+kReset);
 }
}
} // namespace chessclaw::ui
