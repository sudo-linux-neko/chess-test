// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/ui/terminal.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif
namespace chessclaw::ui {
TermSize termSize(){
 TermSize t;
#if defined(_WIN32)
 CONSOLE_SCREEN_BUFFER_INFO info; if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&info)){ t.cols=info.srWindow.Right-info.srWindow.Left+1; t.rows=info.srWindow.Bottom-info.srWindow.Top+1; }
#else
 struct winsize w{}; if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&w)==0 && w.ws_col>0){ t.cols=w.ws_col; t.rows=w.ws_row; }
#endif
 if(t.cols<40) t.cols=80; if(t.rows<10) t.rows=24;
 return t;
}
bool supportsUnicode(){
 const char* lang=std::getenv("LANG"); const char* lc=std::getenv("LC_ALL"); const char* lcc=std::getenv("LC_CTYPE");
 std::string s; if(lc) s+=lc; if(lang) s+=lang; if(lcc) s+=lcc;
 for(auto &c:s) c=(char)tolower(c);
 if(s.find("utf")!=std::string::npos) return true;
#if defined(_WIN32)
 return true;
#else
 return false;
#endif
}
bool supportsTruecolor(){
 const char* ct=std::getenv("COLORTERM"); if(ct&&(std::string(ct).find("truecolor")!=std::string::npos||std::string(ct).find("24bit")!=std::string::npos)) return true;
 const char* t=std::getenv("TERM"); if(t&&std::string(t).find("256")!=std::string::npos) return true;
 return true;
}
void hideCursor(){ std::fputs("\x1b[?25l",stdout); }
void showCursor(){ std::fputs("\x1b[?25h",stdout); }
void enterAltScreen(){ std::fputs("\x1b[?1049h",stdout); }
void exitAltScreen(){ std::fputs("\x1b[?1049l",stdout); }
std::string rgbFg(int r,int g,int b){ char buf[32]; std::snprintf(buf,sizeof(buf),"\x1b[38;2;%d;%d;%dm",r,g,b); return buf; }
std::string rgbBg(int r,int g,int b){ char buf[32]; std::snprintf(buf,sizeof(buf),"\x1b[48;2;%d;%d;%dm",r,g,b); return buf; }
std::string ansi256Fg(int c){ char buf[32]; std::snprintf(buf,sizeof(buf),"\x1b[38;5;%dm",c); return buf; }
std::string ansi256Bg(int c){ char buf[32]; std::snprintf(buf,sizeof(buf),"\x1b[48;5;%dm",c); return buf; }
std::string gradientColor(double t,int r0,int g0,int b0,int r1,int g1,int b1){ if(t<0)t=0; if(t>1)t=1; int r=(int)(r0+(r1-r0)*t),g=(int)(g0+(g1-g0)*t),b=(int)(b0+(b1-b0)*t); return rgbFg(r,g,b); }

} // namespace chessclaw::ui
