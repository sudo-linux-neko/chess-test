// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/nnue.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#if defined(__AVX2__) && defined(__x86_64__)
#include <immintrin.h>
#endif
namespace chessclaw::nnue {
Network::Network(){ b1_.fill(0); w1_.fill(0); w2_.fill(0); b2_=0; }
int Network::featureIndex(Piece p, Square s, Color perspective){
 int pi=(int)p-1; // 0..11
 if(pi<0||pi>11) return -1;
 int sq=s;
 if(perspective==Color::Black){ // flip vertically + swap colors conceptually: mirror square
  sq = (7-rankOf(s))*8+fileOf(s);
  // map piece color: swap white<->black piece index
  int t=(int)typeOf(p); Color c=colorOf(p);
  Color nc=opposite(c);
  Piece np=makePiece(nc,(PieceType)t);
  pi=(int)np-1;
 }
 return pi*64+sq;
}
void Network::randomize(std::uint64_t seed){
 std::mt19937_64 rng(seed);
 std::normal_distribution<double> d(0.0, 8.0);
 for(auto &w:w1_) w=(std::int16_t)std::clamp((int)std::lround(d(rng)),-127,127);
 for(auto &b:b1_) b=(std::int16_t)std::clamp((int)std::lround(d(rng)/4.0),-64,64);
 std::normal_distribution<double> d2(0.0,8.0);
 for(auto &w:w2_) w=(std::int16_t)std::clamp((int)std::lround(d2(rng)),-127,127);
 b2_=0; loaded_=true;
}
void Network::refresh(const Position& pos, Accumulator& acc) const{
 acc.white.fill(0); acc.black.fill(0);
 for(int i=0;i<kHidden;++i){ acc.white[i]=b1_[i]; acc.black[i]=b1_[i]; }
 for(int s=0;s<64;++s){
  Piece p=pos.pieceOn((Square)s); if(p==Piece::None) continue;
  int fw=featureIndex(p,(Square)s,Color::White);
  int fb=featureIndex(p,(Square)s,Color::Black);
  const std::int16_t* rowW=&w1_[fw*kHidden];
  const std::int16_t* rowB=&w1_[fb*kHidden];
  for(int i=0;i<kHidden;++i){ acc.white[i]=(std::int16_t)(acc.white[i]+rowW[i]); acc.black[i]=(std::int16_t)(acc.black[i]+rowB[i]); }
 }
 // clamp to int16 sane range (avoid overflow wrap):
 for(int i=0;i<kHidden;++i){
  if(acc.white[i]>10000) acc.white[i]=10000; if(acc.white[i]<-10000) acc.white[i]=-10000;
  if(acc.black[i]>10000) acc.black[i]=10000; if(acc.black[i]<-10000) acc.black[i]=-10000;
 }
}
static inline std::int32_t crelu(std::int16_t x){ std::int32_t v=x; if(v<0)v=0; if(v>255)v=255; return v; }
int Network::evaluate(const Accumulator& acc, Color stm) const{
 const std::int16_t* first = (stm==Color::White)?acc.white.data():acc.black.data();
 const std::int16_t* second= (stm==Color::White)?acc.black.data():acc.white.data();
 std::int64_t sum=b2_;
#if defined(__AVX2__) && defined(__x86_64__) && defined(CHESSCLAW_HAS_AVX2)
 // Hand-vectorized CreLU + dot product: 16x int16 per AVX2 register.
 // Processes 128 hidden units in 8 iterations; fallback scalar on other arches.
 {
  __m256i vsum = _mm256_setzero_si256();
  __m256i vzero = _mm256_setzero_si256();
  __m256i vmax = _mm256_set1_epi16(255);
  for(int i=0;i<kHidden;i+=16){
   __m256i a = _mm256_loadu_si256((const __m256i*)(first+i));
   __m256i b = _mm256_loadu_si256((const __m256i*)(second+i));
   a = _mm256_max_epi16(_mm256_min_epi16(a, vmax), vzero);
   b = _mm256_max_epi16(_mm256_min_epi16(b, vmax), vzero);
   __m256i wa = _mm256_loadu_si256((const __m256i*)(w2_.data()+i));
   __m256i wb = _mm256_loadu_si256((const __m256i*)(w2_.data()+kHidden+i));
   __m256i pa = _mm256_madd_epi16(a, wa); // 8x int32
   __m256i pb = _mm256_madd_epi16(b, wb);
   vsum = _mm256_add_epi32(vsum, pa);
   vsum = _mm256_add_epi32(vsum, pb);
  }
  __m128i lo = _mm256_castsi256_si128(vsum);
  __m128i hi = _mm256_extracti128_si256(vsum, 1);
  __m128i s = _mm_add_epi32(lo, hi);
  s = _mm_hadd_epi32(s, s); s = _mm_hadd_epi32(s, s);
  sum += (std::int64_t)_mm_cvtsi128_si32(s);
 }
#else
 for(int i=0;i<kHidden;++i){
  std::int32_t a=crelu(first[i]);
  std::int32_t b=crelu(second[i]);
  sum += (std::int64_t)a * w2_[i];
  sum += (std::int64_t)b * w2_[kHidden+i];
 }
#endif
 // scale: acc is in kQIn units, w2 in kQOut-ish; divide to centipawns.
 // w1 scale 64, crelu cap 255 => sum scale ~64*64; map to cp by / (64*64/2)
 int out=(int)(sum / (kQIn*2));
 // White perspective: flip sign if stm black (since acc ordered stm-first but w2 symmetric? No: keep stm-relative then convert).
 // Our w2 is stm-first/second, so output is stm-relative. Convert to white perspective:
 int whiteCp = (stm==Color::White)?out:-out;
 return whiteCp;
}
int Network::evaluate(const Position& pos) const{
 if(!loaded_) return 0;
 Accumulator acc; refresh(pos,acc);
 return evaluate(acc,pos.sideToMove());
}
bool Network::save(const std::string& path) const{
 FILE* f=std::fopen(path.c_str(),"wb"); if(!f) return false;
 std::uint32_t magic=0x43434E55; // CCNU
 std::uint32_t ver=1;
 std::fwrite(&magic,4,1,f); std::fwrite(&ver,4,1,f);
 std::fwrite(w1_.data(),sizeof(std::int16_t),w1_.size(),f);
 std::fwrite(b1_.data(),sizeof(std::int16_t),b1_.size(),f);
 std::fwrite(w2_.data(),sizeof(std::int16_t),w2_.size(),f);
 std::fwrite(&b2_,sizeof(b2_),1,f);
 std::fclose(f); return true;
}
bool Network::load(const std::string& path){
 FILE* f=std::fopen(path.c_str(),"rb"); if(!f) return false;
 std::uint32_t magic=0,ver=0;
 if(std::fread(&magic,4,1,f)!=1||std::fread(&ver,4,1,f)!=1){std::fclose(f);return false;}
 if(magic!=0x43434E55||ver!=1){std::fclose(f);return false;}
 if(std::fread(w1_.data(),sizeof(std::int16_t),w1_.size(),f)!=w1_.size()){std::fclose(f);return false;}
 if(std::fread(b1_.data(),sizeof(std::int16_t),b1_.size(),f)!=b1_.size()){std::fclose(f);return false;}
 if(std::fread(w2_.data(),sizeof(std::int16_t),w2_.size(),f)!=w2_.size()){std::fclose(f);return false;}
 if(std::fread(&b2_,sizeof(b2_),1,f)!=1){std::fclose(f);return false;}
 std::fclose(f); loaded_=true; return true;
}
} // namespace chessclaw::nnue
