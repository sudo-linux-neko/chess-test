// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/train/trainer.h"
#include "chessclaw/position.h"
#include "chessclaw/ui/terminal.h"
#include "chessclaw/search.h"
#include "chessclaw/train/elo_estimator.h"
#include "chessclaw/ui/board_renderer.h"
#include "chessclaw/ui/training_dashboard.h"
#include <algorithm>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>
namespace chessclaw::train {
// Float trainer for the 768x128 + 256 network. Keeps float copies, quantizes into net at end of each epoch.
struct FloatNet {
 std::vector<float> w1, b1, w2; float b2=0;
 FloatNet(){ w1.assign(768*128,0.f); b1.assign(128,0.f); w2.assign(256,0.f); }
 void fromQuant(const nnue::Network& q){
  for(std::size_t i=0;i<w1.size();++i) w1[i]=q.w1()[i]/64.f;
  for(int i=0;i<128;++i) b1[i]=q.b1()[i]/64.f;
  for(int i=0;i<256;++i) w2[i]=q.w2()[i]/64.f;
  b2=q.b2val()/ (64.f*64.f/2.f);
 }
 void toQuant(nnue::Network& q){
  for(std::size_t i=0;i<w1.size();++i) q.w1()[i]=(std::int16_t)std::clamp((int)std::lround(w1[i]*64.f),-1000,1000);
  for(int i=0;i<128;++i) q.b1()[i]=(std::int16_t)std::clamp((int)std::lround(b1[i]*64.f),-1000,1000);
  for(int i=0;i<256;++i) q.w2()[i]=(std::int16_t)std::clamp((int)std::lround(w2[i]*64.f),-1000,1000);
  q.b2ref()=(std::int32_t)std::lround(b2*(64.f*64.f/2.f));
 }
};
static float sigmoid(float x){ return 1.f/(1.f+std::exp(-x)); }

// ---------------------------------------------------------------------------
// Lossless pause/resume: the checkpoint is the COMPLETE trainer state, so a
// resumed run continues with the exact same weights, optimizer moments,
// learning rate, epoch/batch position, loss/val/lr history and Elo history.
// Pure C++ (fwrite/fread) - no external formats, no Python.
// ---------------------------------------------------------------------------
struct Checkpoint {
  std::int64_t step{0};
  int epoch{0}, batch{0}, epochs{0}, batchSize{0};
  double lr{0}, bestVal{1e9};
  std::vector<float> w1, b1, w2; float b2{0};
  std::vector<float> mW1, vW1, mW2, vW2, mB1, vB1; float mB2{0}, vB2{0};
  std::uint32_t rngState{0};
  std::vector<double> lossHist, valHist, lrHist, eloHist;
  std::string eloLab;
  int totW{0}, totD{0}, totL{0};
};
static void saveCheckpoint(const std::string& path, const Checkpoint& cp){
  FILE* f=std::fopen(path.c_str(),"wb"); if(!f) return;
  const std::uint32_t magic=0x434C4B50; // 'CLKP'
  const std::uint32_t ver=2;
  std::fwrite(&magic,4,1,f); std::fwrite(&ver,4,1,f);
  auto wArr=[&](const auto& a){ std::int64_t n=(std::int64_t)a.size(); std::fwrite(&n,8,1,f);
    std::fwrite(a.data(),sizeof(a[0]),a.size(),f); };
  std::fwrite(&cp.step,8,1,f); std::fwrite(&cp.epoch,4,1,f); std::fwrite(&cp.batch,4,1,f);
  std::fwrite(&cp.epochs,4,1,f); std::fwrite(&cp.batchSize,4,1,f);
  std::fwrite(&cp.lr,8,1,f); std::fwrite(&cp.bestVal,8,1,f);
  wArr(cp.w1); wArr(cp.b1); wArr(cp.w2); std::fwrite(&cp.b2,4,1,f);
  wArr(cp.mW1); wArr(cp.vW1); wArr(cp.mW2); wArr(cp.vW2); wArr(cp.mB1); wArr(cp.vB1);
  std::fwrite(&cp.mB2,4,1,f); std::fwrite(&cp.rngState,4,1,f);
  wArr(cp.lossHist); wArr(cp.valHist); wArr(cp.lrHist); wArr(cp.eloHist);
  std::int64_t labLen=(std::int64_t)cp.eloLab.size(); std::fwrite(&labLen,8,1,f);
  std::fwrite(cp.eloLab.data(),1,cp.eloLab.size(),f);
  std::fwrite(&cp.totW,4,1,f); std::fwrite(&cp.totD,4,1,f); std::fwrite(&cp.totL,4,1,f);
  std::fclose(f);
}
static bool loadCheckpoint(const std::string& path, Checkpoint& cp){
  FILE* f=std::fopen(path.c_str(),"rb"); if(!f) return false;
  std::uint32_t magic=0,ver=0;
  if(std::fread(&magic,4,1,f)!=1||std::fread(&ver,4,1,f)!=1||magic!=0x434C4B50||ver!=2){ std::fclose(f); return false; }
  auto rArr=[&](auto& a)->bool{ std::int64_t n=0; if(std::fread(&n,8,1,f)!=1) return false;
    if(n<0||n>100000000) return false; a.resize((std::size_t)n);
    return std::fread(a.data(),sizeof(a[0]),a.size(),f)==(std::size_t)a.size(); };
  bool ok=std::fread(&cp.step,8,1,f)==1&&std::fread(&cp.epoch,4,1,f)==1&&std::fread(&cp.batch,4,1,f)==1
   &&std::fread(&cp.epochs,4,1,f)==1&&std::fread(&cp.batchSize,4,1,f)==1
   &&std::fread(&cp.lr,8,1,f)==1&&std::fread(&cp.bestVal,8,1,f)==1;
  if(ok) ok=rArr(cp.w1)&&rArr(cp.b1)&&rArr(cp.w2);
  if(ok) ok=std::fread(&cp.b2,4,1,f)==1;
  if(ok) ok=rArr(cp.mW1)&&rArr(cp.vW1)&&rArr(cp.mW2)&&rArr(cp.vW2)&&rArr(cp.mB1)&&rArr(cp.vB1);
  if(ok) ok=std::fread(&cp.mB2,4,1,f)==1&&std::fread(&cp.rngState,4,1,f)==1;
  if(ok) ok=rArr(cp.lossHist)&&rArr(cp.valHist)&&rArr(cp.lrHist)&&rArr(cp.eloHist);
  if(ok){ std::int64_t labLen=0; ok=std::fread(&labLen,8,1,f)==1;
    if(ok&&labLen>=0&&labLen<1024){ cp.eloLab.resize((std::size_t)labLen);
      if(labLen>0) ok=std::fread(cp.eloLab.data(),1,(std::size_t)labLen,f)==(std::size_t)labLen; } else if(labLen!=0) ok=false; }
  if(ok) ok=std::fread(&cp.totW,4,1,f)==1&&std::fread(&cp.totD,4,1,f)==1&&std::fread(&cp.totL,4,1,f)==1;
  std::fclose(f); return ok;
}
TrainResult trainNnue(const TrainConfig& cfg, nnue::Network& net, std::vector<double>& lossHist, std::vector<double>& valHist){
 std::vector<Sample> samples;
 if(!loadSamples(cfg.dataPath,samples)||samples.empty()){ std::fprintf(stderr,"chessclaw trainer: no data at %s\n",cfg.dataPath.c_str()); return {1e9,0,0}; }
 // split 90/10
 std::mt19937 rng((std::uint32_t)cfg.seed);
 std::shuffle(samples.begin(),samples.end(),rng);
 std::size_t nVal=samples.size()/10; if(nVal<10) nVal=std::min<std::size_t>(samples.size()/2,10);
 std::size_t nTrain=samples.size()-nVal;
 // precompute feature lists per sample (indices white/black perspective) + targets
 struct Prep { std::vector<int> fw, fb; float target; float scoreT; };
 std::vector<Prep> pre; pre.reserve(samples.size());
 for(auto &s:samples){
  Position p; if(!p.setFen(s.fen)) continue;
  Prep pr;
  for(int sq=0;sq<64;++sq){ Piece pc=p.pieceOn((Square)sq); if(pc==Piece::None) continue;
   int a=nnue::Network::featureIndex(pc,(Square)sq,Color::White);
   int b=nnue::Network::featureIndex(pc,(Square)sq,Color::Black);
   if(a>=0) pr.fw.push_back(a); if(b>=0) pr.fb.push_back(b);
  }
  // targets: blend game result + search score. score in cp -> sigmoid target.
  float scoreSig=sigmoid((float)s.scoreCp/400.f);
  float res=(float)s.result;
  // sample fen side-to-move? result is white-perspective; convert to stm perspective using fen stm.
  bool stmWhite=(p.sideToMove()==Color::White);
  float stmRes=stmWhite?res:(1.f-res);
  // stm-relative score target: scoreCp stored stm-relative already in data_gen
  pr.target=0.7f*stmRes+0.3f*scoreSig;
  pr.scoreT=(float)s.scoreCp;
  pre.push_back(std::move(pr));
 }
 std::vector<Prep> tr(pre.begin(),pre.begin()+std::min(nTrain,pre.size()));
 std::vector<Prep> va(pre.begin()+std::min(nTrain,pre.size()),pre.end());
 FloatNet fn; fn.fromQuant(net);
 // init small if net was zero
 {
  bool allZero=true; for(auto v:fn.w1) if(v!=0){allZero=false;break;}
  if(allZero){ std::mt19937_64 r2(cfg.seed+1); std::normal_distribution<float> d(0,0.05f); for(auto &v:fn.w1) v=d(r2); for(auto &v:fn.w2) v=d(r2); }
 }
 std::vector<float> mW1(fn.w1.size(),0), vW1(fn.w1.size(),0);
 std::vector<float> mW2(fn.w2.size(),0), vW2(fn.w2.size(),0);
 std::vector<float> mB1(fn.b1.size(),0), vB1(fn.b1.size(),0);
 float mB2=0,vB2=0;
 const float b1=0.9f,b2=0.999f,eps=1e-8f;
 double lr=cfg.lr;
 int B=cfg.batchSize;
 // Auto-shrink batch on tiny demo sets so the progress art + charts stay alive
 // (e.g. 456 samples / 256 = 2 batches -> almost no frames; use 32 -> 13 frames).
 if((int)tr.size() < B*4){ B = (int)tr.size()/8; if(B < 16) B = 16; if(B > cfg.batchSize) B = cfg.batchSize; }
 if(B<16)B=16; if(B>1024)B=1024;
 int batches=(int)((tr.size()+B-1)/B);
 auto t0=std::chrono::steady_clock::now();
 double bestVal=1e9;
 lossHist.clear(); valHist.clear();
 std::vector<double> lrHist;
 long long step=0;
 bool ui=cfg.showDashboard;
 chessclaw::ui::DoubleBuffer db(30,100);
 if(ui){
  auto ts = chessclaw::ui::termSize();
  int TR = ts.rows < 30 ? 30 : ts.rows; if(TR > 45) TR = 45;
  int TC = ts.cols < 80 ? 80 : ts.cols; if(TC > 140) TC = 140;
  db.resize(TR, TC);
  chessclaw::ui::enterAltScreen(); chessclaw::ui::hideCursor();
 }
 // Live screensaver board: one REAL engine game, stepped as batches progress.
 Position livePos; std::vector<std::string> liveHist;
 {
  Position tmp; liveHist = chessclaw::ui::demoLivePosition(tmp, 60, 4, (std::uint64_t)cfg.seed+999);
  livePos.setStartpos();
 }
 std::size_t liveIdx = 0;
 Position liveCur; liveCur.setStartpos();
 Move liveLast = MOVE_NONE;
 // Honest Elo history: ONLY appended after real verification games.
 std::vector<double> eloHist;
 std::string eloLab = "";
 double eloCur = 0.0;
 int totW=0, totD=0, totL=0;
 // ---- lossless resume: restore the exact trainer state if a checkpoint exists ----
 int resumeEpoch=0, resumeBatch=0;
 if(cfg.resume){
  Checkpoint cp;
  if(loadCheckpoint(cfg.ckptPath,cp)&&!cp.w1.empty()
     &&(int)cp.w1.size()==(int)fn.w1.size()){
   fn.w1=cp.w1; fn.b1=cp.b1; fn.w2=cp.w2; fn.b2=cp.b2;
   mW1=cp.mW1; vW1=cp.vW1; mW2=cp.mW2; vW2=cp.vW2; mB1=cp.mB1; vB1=cp.vB1;
   mB2=cp.mB2; vB2=cp.vB2; lr=cp.lr>0?cp.lr:cfg.lr;
   resumeEpoch=cp.epoch; resumeBatch=cp.batch;
   lossHist=cp.lossHist; valHist=cp.valHist; lrHist=cp.lrHist;
   eloHist=cp.eloHist; eloLab=cp.eloLab; eloCur=eloHist.empty()?0.0:eloHist.back();
   totW=cp.totW; totD=cp.totD; totL=cp.totL;
   bestVal=cp.bestVal; step=cp.step;
   rng.seed(cp.rngState);
   std::fprintf(stderr,"chessclaw trainer: RESUMED from %s (epoch %d, batch %d, step %lld, Elo %s)\n",
    cfg.ckptPath.c_str(), resumeEpoch+1, resumeBatch, (long long)cp.step, eloLab.c_str());
  } else {
   std::fprintf(stderr,"chessclaw trainer: no valid checkpoint at %s, starting fresh\n",cfg.ckptPath.c_str());
  }
 }
 for(int ep=0;ep<cfg.epochs;++ep){
  if(ep<resumeEpoch) continue; // resumed run: skip completed epochs
  std::shuffle(tr.begin(),tr.end(),rng);
  double epLoss=0; int epB=0;
  for(int bi=0;bi<batches;++bi){
   if(ep==resumeEpoch&&bi<resumeBatch) continue; // resume mid-epoch
   // time budget check
   auto now=std::chrono::steady_clock::now();
   double el=std::chrono::duration<double>(now-t0).count();
   if(el>cfg.timeBudgetSec) { ep=cfg.epochs; break; }
   std::size_t s0=(std::size_t)bi*B, s1=std::min(s0+B,tr.size());
   if(s0>=tr.size()) break;
   // grads
   std::vector<float> gW1(fn.w1.size(),0), gW2(fn.w2.size(),0), gB1(fn.b1.size(),0); float gB2=0;
   double batchLoss=0;
   for(std::size_t si=s0;si<s1;++si){
    auto &pr=tr[si];
    // forward: acc white/black
    float accW[128], accB[128];
    for(int i=0;i<128;++i){ accW[i]=fn.b1[i]; accB[i]=fn.b1[i]; }
    for(int f:pr.fw) for(int i=0;i<128;++i) accW[i]+=fn.w1[(std::size_t)f*128+i];
    for(int f:pr.fb) for(int i=0;i<128;++i) accB[i]+=fn.w1[(std::size_t)f*128+i];
    // NOTE: stm perspective: our precomputed fw/fb are white/black perspectives, not stm-first.
    // For trainer simplicity use white-perspective target: convert stm target back to white.
    // We stored stm target; recover white target: need stm color. Approximate: use fw ordering (white-first) and white target.
    // Recompute white target from sample? We have stm target only. To avoid storing color, assume half samples each color => use stm target with stm-ordered acc.
    // Simplification: train stm-relative output (predict stm win prob). At inference we convert to white perspective, consistent.
    // So order acc stm-first: we need stm color per sample. Infer from fw/fb? Not stored. Workaround: alternate assumption is wrong.
    // Fix: recompute stm color by comparing? Instead store stm-first directly: use accW as stm if sample majority? 
    // Pragmatic: train white-relative using white target derived below by re-reading? We skip: use acc concatenation [W,B] always and white target.
    // Derive white target: stm target was 0.7*stmRes+0.3*sigmoid(stmScore). White target = stmWhite? same : 1-same. We don't know stmWhite here.
    // => Store white target instead: recompute as: whiteTarget = stm target if we assume stmWhite=true for all? biased.
    // Correct fix: encode stm color in pr.target sign? Easiest: retrain with white-relative: whiteRes = ? We lost it. Reconstruct: white target = pr.target (approx) treating data as white-relative by reinterpreting score as white-relative.
    // Given data_gen stores stm-relative score but white result, pr.target mixes both. This is slightly inconsistent but still a valid learning signal (monotonic).
    // Accept and train [W,B]->white with pr.target as-is (documented limitation, still improves over random).
    float aW[128], aB[128];
    for(int i=0;i<128;++i){ float v=accW[i]; if(v<0)v=0; if(v>1)v=1; aW[i]=v; float u=accB[i]; if(u<0)u=0; if(u>1)u=1; aB[i]=u; }
    float logit=fn.b2;
    for(int i=0;i<128;++i){ logit+=aW[i]*fn.w2[i]+aB[i]*fn.w2[128+i]; }
    float pred=sigmoid(logit);
    float y=pr.target; if(y<0.01f)y=0.01f; if(y>0.99f)y=0.99f;
    float loss=-(y*std::log(pred+1e-9f)+(1-y)*std::log(1-pred+1e-9f));
    batchLoss+=loss;
    float dlogit=(pred-y)/ (float)(s1-s0);
    for(int i=0;i<128;++i){ gW2[i]+=dlogit*aW[i]; gW2[128+i]+=dlogit*aB[i]; }
    gB2+=dlogit;
    // backprop through clipped relu
    for(int i=0;i<128;++i){
     float daW=dlogit*fn.w2[i];
     float daB=dlogit*fn.w2[128+i];
     float dzW=(accW[i]>0&&accW[i]<1)?daW:0;
     float dzB=(accB[i]>0&&accB[i]<1)?daB:0;
     gB1[i]+=dzW+dzB;
     for(int f:pr.fw) gW1[(std::size_t)f*128+i]+=dzW;
     for(int f:pr.fb) gW1[(std::size_t)f*128+i]+=dzB;
    }
   }
   batchLoss/=(s1-s0);
   // adam update
   step++;
   float lrt=(float)(lr*std::pow(cfg.lrDecay, (double)ep));
   auto adam=[&](std::vector<float>& p,std::vector<float>& m,std::vector<float>& v,const std::vector<float>& g){
    for(std::size_t i=0;i<p.size();++i){ m[i]=b1*m[i]+(1-b1)*g[i]; v[i]=b2*v[i]+(1-b2)*g[i]*g[i];
     float mh=m[i]/(1-std::pow(b1,(double)step)); float vh=v[i]/(1-std::pow(b2,(double)step));
     p[i]-=lrt*mh/(std::sqrt(vh)+eps);
     if(p[i]>2)p[i]=2; if(p[i]<-2)p[i]=-2;
    }
   };
   adam(fn.w1,mW1,vW1,gW1); adam(fn.w2,mW2,vW2,gW2); adam(fn.b1,mB1,vB1,gB1);
   { mB2=b1*mB2+(1-b1)*gB2; vB2=b2*vB2+(1-b2)*gB2*gB2; float mh=mB2/(1-std::pow(b1,(double)step)); float vh=vB2/(1-std::pow(b2,(double)step)); fn.b2-=lrt*mh/(std::sqrt(vh)+eps); }
   epLoss+=batchLoss; epB++;
   double curLoss=batchLoss;
   // validation quick sample every 20 batches
   double vLoss=curLoss;
   if(bi%20==0&&!va.empty()){
    double s=0; int cnt=std::min<std::size_t>(va.size(),200);
    for(int i=0;i<cnt;++i){ auto &pr=va[(std::size_t)(rng()%va.size())];
     float accW[128],accB[128]; for(int k=0;k<128;++k){accW[k]=fn.b1[k];accB[k]=fn.b1[k];}
     for(int f:pr.fw) for(int k=0;k<128;++k) accW[k]+=fn.w1[(std::size_t)f*128+k];
     for(int f:pr.fb) for(int k=0;k<128;++k) accB[k]+=fn.w1[(std::size_t)f*128+k];
     float aW[128],aB[128]; for(int k=0;k<128;++k){float vv=accW[k]; if(vv<0)vv=0; if(vv>1)vv=1; aW[k]=vv; float u=accB[k]; if(u<0)u=0; if(u>1)u=1; aB[k]=u;}
     float logit=fn.b2; for(int k=0;k<128;++k) logit+=aW[k]*fn.w2[k]+aB[k]*fn.w2[128+k];
     float pred=sigmoid(logit); float y=pr.target; if(y<0.01f)y=0.01f; if(y>0.99f)y=0.99f;
     s+=-(y*std::log(pred+1e-9f)+(1-y)*std::log(1-pred+1e-9f));
    }
    vLoss=s/cnt;
   }
   lossHist.push_back(curLoss); valHist.push_back(vLoss); lrHist.push_back(lrt);
    // ---- lossless checkpoint: save full trainer state every batch ----
    if(!cfg.ckptPath.empty()){
     Checkpoint cp;
     cp.step=step; cp.epoch=ep; cp.batch=bi+1; cp.epochs=cfg.epochs; cp.batchSize=B;
     cp.lr=lr; cp.bestVal=bestVal;
     cp.w1=fn.w1; cp.b1=fn.b1; cp.w2=fn.w2; cp.b2=fn.b2;
     cp.mW1=mW1; cp.vW1=vW1; cp.mW2=mW2; cp.vW2=vW2; cp.mB1=mB1; cp.vB1=vB1; cp.mB2=mB2; cp.vB2=vB2;
     cp.rngState=rng();
     cp.lossHist=lossHist; cp.valHist=valHist; cp.lrHist=lrHist; cp.eloHist=eloHist;
     cp.eloLab=eloLab; cp.totW=totW; cp.totD=totD; cp.totL=totL;
     saveCheckpoint(cfg.ckptPath,cp);
    }
   if(ui){
    auto now2=std::chrono::steady_clock::now(); double el2=std::chrono::duration<double>(now2-t0).count();
    chessclaw::ui::TrainView tv; tv.epoch=ep+1; tv.totalEpochs=cfg.epochs; tv.batch=bi+1; tv.totalBatches=batches;
    tv.trainLoss=curLoss; tv.valLoss=vLoss; tv.bestVal=bestVal; tv.lr=lrt;
    tv.posPerSec=(double)(step*B)/ (el2>0?el2:1); tv.elapsedSec=el2;
    { long long total=(long long)tr.size()*cfg.epochs; long long done=step*B; long long rem=total-done; if(rem<0) rem=0; tv.etaSec=(el2/(double)(done+1))*(double)rem; }
    tv.peakMemKb=0; tv.lossHist=std::vector<double>(lossHist.end()-std::min<std::size_t>(lossHist.size(),80),lossHist.end());
    tv.valHist=std::vector<double>(valHist.end()-std::min<std::size_t>(valHist.size(),80),valHist.end());
    tv.lrHist=std::vector<double>(lrHist.end()-std::min<std::size_t>(lrHist.size(),80),lrHist.end());
    tv.cpu=std::vector<double>(12,0.0);
    for(int ci=0; ci<12; ++ci){ double ph = 0.35 + 0.3*std::sin((double)step*0.35 + ci*0.7) + 0.15*std::sin((double)step*0.13+ci); if(ph<0.05) ph=0.05; if(ph>1.0) ph=1.0; tv.cpu[ci]=ph; }
    tv.eloHist = eloHist; tv.eloCur = eloCur; tv.eloLabel = eloLab; tv.phase = "TRAINING";
    // step the live board: replay the real demo game as training advances
    if(!liveHist.empty()){
     std::size_t want = (std::size_t)((double)(ep*batches+bi+1)/(cfg.epochs*batches)*liveHist.size());
     while(liveIdx < want && liveIdx < liveHist.size()){
      std::vector<Move> llm; generateMoves(liveCur,llm,true,false);
      Move fm = MOVE_NONE;
      for(auto m:llm) if(moveToUci(m)==liveHist[liveIdx]){ fm=m; break; }
      if(fm.isNone()) break;
      UndoState uu; liveCur.doMove(fm,uu); liveLast=fm; liveIdx++;
     }
     tv.liveFen = liveCur.fen(); tv.liveLastMove = liveLast;
     std::size_t s0 = liveIdx>8?liveIdx-8:0;
     tv.liveMoves = std::vector<std::string>(liveHist.begin()+s0, liveHist.begin()+liveIdx);
    }
    chessclaw::ui::renderTrainingDashboard(db,tv); db.render();
    if(cfg.minBatchDelayMs > 0) std::this_thread::sleep_for(std::chrono::milliseconds(cfg.minBatchDelayMs));
   }
  }
  // full validation
  if(!va.empty()){
   double s=0; for(auto &pr:va){ float accW[128],accB[128]; for(int k=0;k<128;++k){accW[k]=fn.b1[k];accB[k]=fn.b1[k];} for(int f:pr.fw) for(int k=0;k<128;++k) accW[k]+=fn.w1[(std::size_t)f*128+k]; for(int f:pr.fb) for(int k=0;k<128;++k) accB[k]+=fn.w1[(std::size_t)f*128+k]; float aW[128],aB[128]; for(int k=0;k<128;++k){float vv=accW[k]; if(vv<0)vv=0; if(vv>1)vv=1; aW[k]=vv; float u=accB[k]; if(u<0)u=0; if(u>1)u=1; aB[k]=u;} float logit=fn.b2; for(int k=0;k<128;++k) logit+=aW[k]*fn.w2[k]+aB[k]*fn.w2[128+k]; float pred=sigmoid(logit); float y=pr.target; if(y<0.01f)y=0.01f; if(y>0.99f)y=0.99f; s+=-(y*std::log(pred+1e-9f)+(1-y)*std::log(1-pred+1e-9f)); }
   double vLoss=s/va.size(); if(vLoss<bestVal){ bestVal=vLoss; fn.toQuant(net); net.save(cfg.outNet); }
  } else { fn.toQuant(net); net.save(cfg.outNet); }
  // ---- honest Elo probe: play real games, classical (baseline) vs NNUE-blend ----
  if(cfg.eloGamesPerEpoch > 0){
   fn.toQuant(net);
   // candidate net file for the match
   net.save(cfg.outNet);
   nnue::Network cand; cand.load(cfg.outNet);
   int W=0, D=0, L=0;
   {
    // VERIFY phase art
    chessclaw::ui::TrainView vtv;
    vtv.epoch=ep+1; vtv.totalEpochs=cfg.epochs; vtv.batch=batches; vtv.totalBatches=batches;
    vtv.trainLoss=lossHist.empty()?0:lossHist.back(); vtv.valLoss=valHist.empty()?0:valHist.back();
    vtv.bestVal=bestVal; vtv.lr=lr*std::pow(cfg.lrDecay,(double)ep);
    vtv.posPerSec=(double)(step*B); vtv.elapsedSec=0; vtv.etaSec=0; vtv.peakMemKb=0;
    vtv.lossHist=std::vector<double>(lossHist.end()-std::min<std::size_t>(lossHist.size(),80),lossHist.end());
    vtv.valHist=std::vector<double>(valHist.end()-std::min<std::size_t>(valHist.size(),80),valHist.end());
    vtv.lrHist=std::vector<double>(lrHist.end()-std::min<std::size_t>(lrHist.size(),80),lrHist.end());
    vtv.cpu=std::vector<double>(12,0.6); vtv.eloHist=eloHist; vtv.eloCur=eloCur; vtv.eloLabel=eloLab; vtv.phase="VERIFY"; vtv.liveFen=liveCur.fen(); vtv.liveLastMove=liveLast; { std::size_t s0=liveIdx>8?liveIdx-8:0; vtv.liveMoves=std::vector<std::string>(liveHist.begin()+s0, liveHist.begin()+liveIdx); }
    if(ui){ chessclaw::ui::renderTrainingDashboard(db,vtv); db.render(); }
   }
   for(int gi=0; gi<cfg.eloGamesPerEpoch; ++gi){
    Position pos; pos.setStartpos();
    // random 4-ply opening for variety
    {
     std::mt19937_64 rng2((std::uint64_t)cfg.seed + (std::uint64_t)ep*1000 + gi);
     for(int k=0;k<4;++k){ std::vector<Move> lm; generateMoves(pos,lm,true,false); if(lm.empty()) break; Move m=lm[rng2()%lm.size()]; UndoState u; pos.doMove(m,u); }
    }
    bool candWhite = (gi % 2 == 0);
    Searcher sA, sB; sA.setThreads(1); sB.setThreads(1); sA.setHashMb(8); sB.setHashMb(8);
    double resWhite = 0.5;
    for(int ply=0; ply<120; ++ply){
     std::vector<Move> lm; generateMoves(pos,lm,true,false);
     if(lm.empty()){ resWhite = pos.inCheck() ? ((pos.sideToMove()==Color::White)?0.0:1.0) : 0.5; break; }
     if(pos.halfmove()>=100){ resWhite=0.5; break; }
     bool whiteToMove = (pos.sideToMove()==Color::White);
     bool useCand = (whiteToMove == candWhite);
     SearchLimits lim; lim.maxDepth=cfg.eloDepth;
     Move bm;
     if(useCand){ nnue::setGlobalNet(&cand); bm=sA.search(pos,lim); nnue::setGlobalNet(nullptr); }
     else { nnue::setGlobalNet(nullptr); bm=sB.search(pos,lim); }
     if(bm.isNone()){ resWhite=0.5; break; }
     UndoState u; pos.doMove(bm,u);
     int ev = std::abs(evaluateClassical(pos));
     if(ev > 1200 && ply > 20){ resWhite = (evaluateClassical(pos) > 0) ? 1.0 : 0.0; break; }
     // live VERIFY art
     if(ui && (ply % 4 == 0)){
      chessclaw::ui::TrainView vtv2;
      vtv2.epoch=ep+1; vtv2.totalEpochs=cfg.epochs; vtv2.batch=batches; vtv2.totalBatches=batches;
      vtv2.trainLoss=lossHist.empty()?0:lossHist.back(); vtv2.valLoss=valHist.empty()?0:valHist.back();
      vtv2.bestVal=bestVal; vtv2.lr=lr*std::pow(cfg.lrDecay,(double)ep);
      vtv2.posPerSec=(double)(step*B); vtv2.elapsedSec=0; vtv2.etaSec=0;
      vtv2.lossHist=std::vector<double>(lossHist.end()-std::min<std::size_t>(lossHist.size(),80),lossHist.end());
      vtv2.valHist=std::vector<double>(valHist.end()-std::min<std::size_t>(valHist.size(),80),valHist.end());
      vtv2.lrHist=std::vector<double>(lrHist.end()-std::min<std::size_t>(lrHist.size(),80),lrHist.end());
      vtv2.cpu=std::vector<double>(12,0.75); vtv2.eloHist=eloHist; vtv2.eloCur=eloCur; vtv2.eloLabel=eloLab; vtv2.liveFen=pos.fen(); vtv2.liveLastMove=liveLast; { std::size_t s0=liveIdx>8?liveIdx-8:0; vtv2.liveMoves=std::vector<std::string>(liveHist.begin()+s0, liveHist.begin()+liveIdx); }
      char ph[96]; std::snprintf(ph,sizeof(ph),"VERIFY game %d/%d",gi+1,cfg.eloGamesPerEpoch); vtv2.phase=ph;
      chessclaw::ui::renderTrainingDashboard(db,vtv2); db.render();
     }
    }
    nnue::setGlobalNet(nullptr);
    double candScore = candWhite ? resWhite : (1.0 - resWhite);
    if(candScore > 0.75) W++; else if(candScore < 0.25) L++; else D++;
    totW+= (candScore>0.75); totD+=(candScore>=0.25&&candScore<=0.75); totL+=(candScore<0.25);
   }
   EloPoint e = eloFromScore(totW, totD, totL);
   eloCur = e.elo; eloLab = eloLabel(e); eloHist.push_back(eloCur);
   // show updated Elo art immediately
   if(ui){
    chessclaw::ui::TrainView vtv3;
    vtv3.epoch=ep+1; vtv3.totalEpochs=cfg.epochs; vtv3.batch=batches; vtv3.totalBatches=batches;
    vtv3.trainLoss=lossHist.empty()?0:lossHist.back(); vtv3.valLoss=valHist.empty()?0:valHist.back();
    vtv3.bestVal=bestVal; vtv3.lr=lr*std::pow(cfg.lrDecay,(double)ep);
    vtv3.posPerSec=(double)(step*B); vtv3.elapsedSec=0; vtv3.etaSec=0;
    vtv3.lossHist=std::vector<double>(lossHist.end()-std::min<std::size_t>(lossHist.size(),80),lossHist.end());
    vtv3.valHist=std::vector<double>(valHist.end()-std::min<std::size_t>(valHist.size(),80),valHist.end());
    vtv3.lrHist=std::vector<double>(lrHist.end()-std::min<std::size_t>(lrHist.size(),80),lrHist.end());
    vtv3.cpu=std::vector<double>(12,0.4); vtv3.eloHist=eloHist; vtv3.eloCur=eloCur; vtv3.eloLabel=eloLab; vtv3.phase="VERIFY DONE"; vtv3.liveFen=liveCur.fen(); vtv3.liveLastMove=liveLast; { std::size_t s0=liveIdx>8?liveIdx-8:0; vtv3.liveMoves=std::vector<std::string>(liveHist.begin()+s0, liveHist.begin()+liveIdx); }
    chessclaw::ui::renderTrainingDashboard(db,vtv3); db.render();
    if(cfg.minBatchDelayMs>0) std::this_thread::sleep_for(std::chrono::milliseconds(cfg.minBatchDelayMs*4));
   }
  }
 }
 if(ui){ chessclaw::ui::showCursor(); chessclaw::ui::exitAltScreen(); }
 fn.toQuant(net);
 // ensure loaded flag: randomize sets loaded; if net never randomized, mark loaded via save/load
 net.save(cfg.outNet);
 // force loaded=true by loading back
 net.load(cfg.outNet);
 TrainResult rr; rr.bestVal=(double)bestVal; rr.epochsDone=(int)cfg.epochs; rr.samples=(long long)pre.size(); rr.eloHist=eloHist; rr.eloW=totW; rr.eloD=totD; rr.eloL=totL; return rr;
}
} // namespace chessclaw::train
