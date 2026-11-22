// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/transposition.h"
namespace chessclaw {
TranspositionTable::TranspositionTable(){ resize(64); }
void TranspositionTable::resize(std::size_t mb){
 std::size_t n=(mb*1024*1024)/16;
 if(n<1024) n=1024;
 // power of two for fast mask
 std::size_t p=1; while(p<n) p<<=1;
 table_=std::vector<TTEntry>(p);
 clear();
}
void TranspositionTable::clear(){ for(auto &e:table_){ e.key.store(0,std::memory_order_relaxed); e.data.store(0,std::memory_order_relaxed);} }
double TranspositionTable::occupancy() const{
 if(table_.empty()) return 0.0;
 std::size_t samp=std::min<std::size_t>(1000,table_.size());
 std::size_t used=0;
 for(std::size_t i=0;i<samp;++i){ std::size_t idx=(i*table_.size())/samp; if(table_[idx].key.load(std::memory_order_relaxed)!=0) used++; }
 return (double)used/(double)samp;
}
std::uint16_t TranspositionTable::packMove(Move m){ return (std::uint16_t)(m.data | ((std::uint16_t)m.flags<<12)); }
Move TranspositionTable::unpackMove(std::uint16_t v){ Move m; m.data=(std::uint16_t)(v&0x0FFF); m.flags=(std::uint8_t)((v>>12)&0xF); return m; }
bool TranspositionTable::probe(std::uint64_t key, Move& best, int& depth, int& flag, int& score, int& eval, int ply) const{
 if(table_.empty()) return false;
 std::size_t idx=(key^(key>>33))&(table_.size()-1);
 std::uint64_t sk=table_[idx].key.load(std::memory_order_relaxed);
 std::uint64_t sd=table_[idx].data.load(std::memory_order_relaxed);
 if((sk^sd)!=key) return false;
 std::uint16_t pm=(std::uint16_t)(sd&0xFFFF);
 int d=(int)((sd>>16)&0xFF); int fl=(int)((sd>>24)&0x3); int sc=(std::int16_t)((sd>>32)&0xFFFF); int ev=(std::int16_t)((sd>>48)&0xFFFF);
 if(sc>29000) sc-=ply; else if(sc<-29000) sc+=ply;
 best=unpackMove(pm); depth=d; flag=fl; score=sc; eval=ev;
 return true;
}
void TranspositionTable::store(std::uint64_t key, Move best, int depth, int flag, int score, int eval, int ply){
 if(table_.empty()) return;
 int sc=score; if(sc>29000) sc+=ply; else if(sc<-29000) sc-=ply;
 std::uint16_t pm=packMove(best);
 std::uint64_t sd=(std::uint64_t)pm | ((std::uint64_t)(depth&0xFF)<<16) | ((std::uint64_t)(flag&0x3)<<24) | ((std::uint64_t)(std::uint16_t)sc<<32) | ((std::uint64_t)(std::uint16_t)eval<<48);
 // mix age into key xor? store key^data for lockless validation
 std::size_t idx=(key^(key>>33))&(table_.size()-1);
 // replace if deeper or different key (always-replace with depth preference via sampling single slot: keep deeper)
 std::uint64_t oldKey=table_[idx].key.load(std::memory_order_relaxed);
 std::uint64_t oldData=table_[idx].data.load(std::memory_order_relaxed);
 int oldDepth=(int)((oldData>>16)&0xFF);
 if((oldKey^oldData)==key || depth>=oldDepth-2 || age_%4==0){
  table_[idx].data.store(sd,std::memory_order_relaxed);
  table_[idx].key.store(key^sd,std::memory_order_relaxed);
 }
 (void)eval;
}
} // namespace chessclaw
