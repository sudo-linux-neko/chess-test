// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/movegen.h"
#include "chessclaw/position.h"
#include <cstdio>
#include <string>
#include <vector>
using namespace chessclaw;
static std::uint64_t perft(Position& p,int d){ if(d==0) return 1; std::vector<Move> m; generateMoves(p,m,true,false); if(d==1) return m.size(); std::uint64_t n=0; UndoState u; for(auto mv:m){ p.doMove(mv,u); n+=perft(p,d-1); p.undoMove(mv,u);} return n; }
static int check(const std::string& fen,int depth,std::uint64_t expect){
 Position p; p.setFen(fen); std::uint64_t n=perft(p,depth); bool ok=(n==expect);
 std::printf("%s perft(%d)=%llu expect %llu\n",ok?"PASS":"FAIL",depth,(unsigned long long)n,(unsigned long long)expect);
 return ok?0:1;
}
int main(){
 int fails=0;
 fails+=check("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",1,20);
 fails+=check("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",2,400);
 fails+=check("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",3,8902);
 fails+=check("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",4,197281);
 fails+=check("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",1,48);
 fails+=check("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",2,2039);
 fails+=check("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",3,97862);
 fails+=check("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",3,2812);
 fails+=check("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",4,43238);
 fails+=check("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",3,9467);
 fails+=check("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",3,62379);
 if(fails==0) std::printf("ALL PERFT PASS\n"); else std::printf("%d FAILURES\n",fails);
 return fails;
}
