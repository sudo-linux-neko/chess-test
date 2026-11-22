// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/ui/progress_bar.h"
#include "chessclaw/ui/terminal.h"
#include <cmath>
#include <cstdio>
namespace chessclaw::ui {
void renderProgressBar(DoubleBuffer& db,int r,int c,int w,double frac,const std::string& label,double rate){
 renderAnimeProgressBar(db,r,c,w,frac,label,rate,0.0);
}
void renderAnimeProgressBar(DoubleBuffer& db,int r,int c,int w,double frac,const std::string& label,double rate,double t){
 if(frac<0)frac=0; if(frac>1)frac=1;
 int fill=(int)(frac*w);
 std::string s="\x1b[38;2;187;154;247m[\x1b[0m";
 for(int i=0;i<w;++i){
  double gt=(double)i/(double)(w>1?w-1:1);
  if(i<fill){
   int rr=(int)(125+(255-125)*gt), gg=(int)(207+(158-207)*gt), bb=(int)(255+(224-255)*gt);
   double shimmer=0.75+0.25*std::sin(t*4.0+i*0.35);
   rr=(int)(rr*shimmer); gg=(int)(gg*shimmer); bb=(int)(bb*shimmer);
   const char* gl=(i==fill-1)?"\u2593":"\u2588";
   char buf[48]; std::snprintf(buf,sizeof(buf),"\x1b[38;2;%d;%d;%dm%s\x1b[0m",rr,gg,bb,gl);
   s+=buf;
  } else {
   double drift=std::fmod(t*8.0-i, (double)(w*2));
   bool comet=(drift>=0&&drift<2.0);
   if(comet) s+="\x1b[38;2;80;120;180m\u2591\x1b[0m";
   else s+=std::string(kDim)+"\u2591"+kReset;
  }
 }
 s+="\x1b[38;2;187;154;247m]\x1b[0m";
 char b[128]; std::snprintf(b,sizeof(b)," \x1b[1m\x1b[38;2;255;255;180m%5.1f%%\x1b[0m",frac*100.0);
 s+=b;
 if(!label.empty()) s+=" \x1b[38;2;125;207;255m"+label+"\x1b[0m";
 if(rate>0){ char r2[64]; std::snprintf(r2,sizeof(r2)," \x1b[38;2;158;255;190m%.0f/s\x1b[0m",rate); s+=r2; }
 db.put(r,c,s);
}
std::string formatEta(double s){ if(s<0)s=0; int h=(int)(s/3600),m=(int)((s-h*3600)/60),ss=(int)(s-h*3600-m*60); char b[32]; std::snprintf(b,sizeof(b),"%02d:%02d:%02d",h,m,ss); return b; }
std::string formatCount(long long v){ char b[32]; if(v>1000000) std::snprintf(b,sizeof(b),"%.2fM",v/1e6); else if(v>1000) std::snprintf(b,sizeof(b),"%.1fk",v/1e3); else std::snprintf(b,sizeof(b),"%lld",v); return b; }
} // namespace chessclaw::ui
