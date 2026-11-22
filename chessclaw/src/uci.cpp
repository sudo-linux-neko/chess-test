// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/uci.h"
#include <iostream>
#include <sstream>
namespace chessclaw {
Uci::Uci(){ pos_.setStartpos(); searcher_.setThreads(1); searcher_.setHashMb(64); }
Move Uci::parseMove(const Position& p, const std::string& tok){
 if(tok.size()<4) return MOVE_NONE;
 int ff=tok[0]-'a', rf=tok[1]-'1', ft=tok[2]-'a', rt=tok[3]-'1';
 if(ff<0||ff>7||rf<0||rf>7||ft<0||ft>7||rt<0||rt>7) return MOVE_NONE;
 Square f=makeSquare(ff,rf), t=makeSquare(ft,rt);
 PieceType promo=PieceType::None;
 if(tok.size()>=5){ char c=tok[4]; if(c=='n')promo=PieceType::Knight; else if(c=='b')promo=PieceType::Bishop; else if(c=='r')promo=PieceType::Rook; else if(c=='q')promo=PieceType::Queen; }
 std::vector<Move> lm; generateMoves(p,lm,true,false);
 for(auto m:lm){ if(m.from()==f&&m.to()==t){ if(promo==PieceType::None && !m.isPromo()) return m; if(promo!=PieceType::None && m.isPromo() && m.promoType()==promo) return m; } }
 // fallback: construct
 Move m(f,t);
 if(promo!=PieceType::None){ if(p.pieceOn(t)!=Piece::None||m.flags==1) {} switch(promo){case PieceType::Knight:m.flags=Move::PromoN;break;case PieceType::Bishop:m.flags=Move::PromoB;break;case PieceType::Rook:m.flags=Move::PromoR;break;default:m.flags=Move::PromoQ;break;} }
 return m;
}
bool Uci::handlePosition(const std::string& line){
 std::istringstream ss(line);
 std::string tok; ss>>tok; // position
 std::string kind; ss>>kind;
 if(kind=="startpos"){
  pos_.setStartpos();
  std::string moves; ss>>moves;
  if(moves=="moves"){ std::string m; while(ss>>m){ Move mv=parseMove(pos_,m); if(mv.isNone()) break; UndoState u; if(!pos_.isLegal(mv)) break; pos_.doMove(mv,u); /* leak undo: keep history naive - store hash history in position? UCI keeps full fen stack simplified: we keep static history via vector */ } }
 } else if(kind=="fen"){
  std::string fen; std::string part;
  // FEN is 6 fields
  for(int i=0;i<6;++i){ std::string w; if(!(ss>>w)) break; fen+=(i?" ":"")+w; }
  pos_.setFen(fen);
  std::string moves; ss>>moves;
  if(moves=="moves"){ std::string m; while(ss>>m){ Move mv=parseMove(pos_,m); if(mv.isNone()) break; if(!pos_.isLegal(mv)) break; UndoState u; pos_.doMove(mv,u); } }
 } else return false;
 return true;
}
std::string Uci::handleGo(const std::string& line){
 SearchLimits lim;
 std::istringstream ss(line); std::string tok; ss>>tok; // go
 std::string w;
 while(ss>>w){
  if(w=="depth"&& (ss>>tok)) lim.maxDepth=std::stoi(tok);
  else if(w=="nodes"&&(ss>>tok)) lim.maxNodes=std::stoll(tok);
  else if(w=="movetime"&&(ss>>tok)) lim.movetimeMs=std::stoi(tok);
  else if(w=="wtime"&&(ss>>tok)) lim.wtimeMs=std::stoi(tok);
  else if(w=="btime"&&(ss>>tok)) lim.btimeMs=std::stoi(tok);
  else if(w=="winc"&&(ss>>tok)) lim.wincMs=std::stoi(tok);
  else if(w=="binc"&&(ss>>tok)) lim.bincMs=std::stoi(tok);
  else if(w=="movestogo"&&(ss>>tok)) lim.movestogo=std::stoi(tok);
  else if(w=="infinite") lim.infinite=true;
  else if(w=="searchmoves"){ std::string m; // consume rest as moves until end
   while(ss>>m){ if(m=="depth"||m=="nodes"||m=="movetime") break; lim.searchMoves.push_back(uciToMove(m)); } }
 }
 Move best=searcher_.search(pos_,lim);
 return std::string("bestmove ")+moveToUci(best);
}
std::string Uci::handleLine(const std::string& line, Position& pos, SearchLimits& pending){
 (void)pos; (void)pending;
 std::istringstream ss(line); std::string cmd; ss>>cmd;
 if(cmd=="uci"){
  return "id name chessclaw 1.0.0\nid author vaubhav\noption name Hash type spin default 64 min 1 max 4096\noption name Threads type spin default 1 min 1 max 64\noption name MoveOverhead type spin default 50 min 0 max 1000\noption name NNUEFile type string default <empty>\nuciok";
 } else if(cmd=="isready") return "readyok";
 else if(cmd=="ucinewgame"){ searcher_.tt().clear(); pos_.setStartpos(); return ""; }
 else if(cmd=="position"){ handlePosition(line); return ""; }
 else if(cmd.rfind("go",0)==0){ return handleGo(line); }
 else if(cmd=="setoption"){
  // setoption name X value Y
  auto p1=line.find("name "); auto p2=line.find(" value ");
  std::string name, value;
  if(p1!=std::string::npos){ name=line.substr(p1+5, (p2==std::string::npos?std::string::npos:p2-(p1+5))); value=(p2==std::string::npos?"":line.substr(p2+7)); }
  // trim
  auto trim=[](std::string s){ std::size_t a=s.find_first_not_of(" \t"); std::size_t b=s.find_last_not_of(" \t"); if(a==std::string::npos) return std::string(); return s.substr(a,b-a+1); };
  name=trim(name); value=trim(value);
  if(name=="Hash"){ hashMb_=std::stoi(value); searcher_.setHashMb((std::size_t)hashMb_); }
  else if(name=="Threads"){ threads_=std::stoi(value); searcher_.setThreads(threads_); }
  else if(name=="MoveOverhead"){ moveOverheadMs_=std::stoi(value); }
  else if(name=="NNUEFile"){ if(!value.empty()&&value!="<empty>"){ net_.load(value); searcher_.setNnue(&net_); nnue::setGlobalNet(&net_); } }
  return "";
 } else if(cmd=="stop"){ searcher_.stop(); return ""; }
 else if(cmd=="quit") return "quit";
 else if(cmd=="d"){ return pos_.fen(); }
 return "";
}
int Uci::run(){
 std::string line;
 pos_.setStartpos();
 while(std::getline(std::cin,line)){
  if(!line.empty()&&line.back()=='\r') line.pop_back();
  std::string out=handleLine(line,pos_,*(new SearchLimits()));
  if(out=="quit") break;
  if(!out.empty()) std::cout<<out<<std::endl;
  // go already returns bestmove string; print it
  // handleLine for go returns bestmove line: printed above. For async stop we keep sync (simple).
 }
 return 0;
}
} // namespace chessclaw
