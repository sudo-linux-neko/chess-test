// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/ui/double_buffer.h"
#include <cstdio>
namespace chessclaw::ui {
DoubleBuffer::DoubleBuffer(int r,int c):rows_(r),cols_(c){ cur_.assign((std::size_t)r,std::string((std::size_t)c,' ')); prev_.assign((std::size_t)r,std::string((std::size_t)c,' ')); }
void DoubleBuffer::resize(int r,int c){ rows_=r; cols_=c; cur_.assign((std::size_t)r,std::string((std::size_t)c,' ')); prev_.assign((std::size_t)r,std::string((std::size_t)c,' ')); first_=true; }
void DoubleBuffer::clear(const std::string& f){ char ch=f.empty()?' ':f[0]; for(auto &row:cur_) row.assign((std::size_t)cols_,ch); }
static std::size_t utf8len(unsigned char c){ if((c&0x80)==0) return 1; if((c&0xE0)==0xC0) return 2; if((c&0xF0)==0xE0) return 3; if((c&0xF8)==0xF0) return 4; return 1; }
void DoubleBuffer::put(int r,int c,const std::string& s){
 if(r<0||r>=rows_||c>=cols_) return;
 // Write visible cells; ANSI escapes are zero-width styling: we keep them inline by inserting into row string.
 // Strategy: build new row: prefix bytes [0,c) + styled content + suffix after c+cells.
 // Since rows_ are plain ASCII spaces initially, byte offset == column. After inserting multi-byte/ANSI, offsets drift.
 // To stay robust, we maintain rows as byte strings and map visible-column->byte-offset by scanning.
 std::string &row=cur_[r];
 // find byte offset of visible column c
 std::size_t boff=0; int vis=0;
 while(boff<row.size()&&vis<c){
  if(row[boff]=='\x1b'&&boff+1<row.size()&&row[boff+1]=='['){ std::size_t j=boff+2; while(j<row.size()&&(row[j]<'@'||row[j]>'~')) j++; if(j<row.size()) j++; boff=j; continue; }
  unsigned char ch=(unsigned char)row[boff]; boff+=utf8len(ch); vis++;
 }
 if(vis<c){ // pad
  row.append((std::size_t)(c-vis),' '); boff=row.size();
 }
 // find byte offset of visible column c+cells(s)
 int cells=0;
 for(std::size_t i=0;i<s.size();){ if(s[i]=='\x1b'&&i+1<s.size()&&s[i+1]=='['){ std::size_t j=i+2; while(j<s.size()&&(s[j]<'@'||s[j]>'~')) j++; if(j<s.size()) j++; i=j; continue;} unsigned char ch=(unsigned char)s[i]; i+=utf8len(ch); cells++; }
 std::size_t eoff=boff; int v2=0;
 // count cells from boff to cover `cells` visible cells
 while(eoff<row.size()&&v2<cells){
  if(row[eoff]=='\x1b'&&eoff+1<row.size()&&row[eoff+1]=='['){ std::size_t j=eoff+2; while(j<row.size()&&(row[j]<'@'||row[j]>'~')) j++; if(j<row.size()) j++; eoff=j; continue; }
  unsigned char ch=(unsigned char)row[eoff]; eoff+=utf8len(ch); v2++;
 }
 // if fewer cells remain, extend
 std::string tail;
 if(v2<cells){ /* eoff==row.size(): nothing to erase beyond */ }
 else { tail=row.substr(eoff); }
 std::string head=row.substr(0,boff);
 row=head+s+tail;
 // truncate visible width to cols_
 int vc=0; std::size_t t=0;
 while(t<row.size()&&vc<cols_){
  if(row[t]=='\x1b'&&t+1<row.size()&&row[t+1]=='['){ std::size_t j=t+2; while(j<row.size()&&(row[j]<'@'||row[j]>'~')) j++; if(j<row.size()) j++; t=j; continue; }
  unsigned char ch=(unsigned char)row[t]; t+=utf8len(ch); vc++;
 }
 if(vc>=cols_){ // cut at t, keep trailing reset? just cut
  row=row.substr(0,t);
 }
}
void DoubleBuffer::putChar(int r,int c,char ch){ if(r<0||r>=rows_||c<0||c>=cols_) return; put(r,c,std::string(1,ch)); }
void DoubleBuffer::hline(int r,int c,int n,const std::string& s){ for(int i=0;i<n;++i) put(r,c+i,s); }
void DoubleBuffer::box(int r,int c,int h,int w,const std::string& title){
 if(h<2||w<2) return;
 put(r,c,"+"); for(int i=1;i<w-1;++i) put(r,c+i,"-"); put(r,c+w-1,"+");
 for(int i=1;i<h-1;++i){ put(r+i,c,"|"); put(r+i,c+w-1,"|"); }
 put(r+h-1,c,"+"); for(int i=1;i<w-1;++i) put(r+h-1,c+i,"-"); put(r+h-1,c+w-1,"+");
 if(!title.empty()) put(r,c+2," "+title+" ");
}
void DoubleBuffer::render(bool force){
 (void)force;
 std::string out; out.reserve(8192);
 out+="\x1b[H";
 for(int r=0;r<rows_;++r){ out+=cur_[r]; out+="\x1b[0m"; if(r+1<rows_) out+="\r\n"; }
 std::fputs(out.c_str(),stdout); std::fflush(stdout);
 prev_=cur_; first_=false;
}
std::string DoubleBuffer::snapshot() const{ std::string s; for(int r=0;r<rows_;++r){ s+=cur_[r]; s+="\n"; } return s; }
} // namespace chessclaw::ui
