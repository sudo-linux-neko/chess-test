// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/train/elo_estimator.h"
#include <cmath>
#include <cstdio>
namespace chessclaw::train {
EloPoint eloFromScore(int w,int d,int l){
 int n=w+d+l; if(n==0) return {0,0,0,0,0};
 double s=(w+0.5*d)/n;
 if(s<=0.001) s=0.001; if(s>=0.999) s=0.999;
 double elo=400*std::log10(s/(1-s));
 double var=(s*(1-s)/n); double err=400*std::sqrt(var)/(s*(1-s)*std::log(10.0));
 return {elo,err,w,d,l};
}
std::string eloLabel(const EloPoint& e){ char b[128]; std::snprintf(b,sizeof(b),"%+.0f +/- %.0f (W%d D%d L%d) ESTIMATE",e.elo,e.err,e.wins,e.draws,e.losses); return b; }
} // namespace chessclaw::train
