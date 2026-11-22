// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/ui/transitions.h"
#include "chessclaw/ui/terminal.h"
#include <chrono>
#include <thread>
namespace chessclaw::ui {
void transitionFade(DoubleBuffer& db,const std::string& label){ for(int i=0;i<3;++i){ db.put(db.rows()/2,2,std::string(kDim)+"... "+label+kReset); db.render(); std::this_thread::sleep_for(std::chrono::milliseconds(90)); } }
void transitionWipe(DoubleBuffer& db,const std::string& label){ for(int i=0;i<(int)db.cols();i+=8){ db.put(db.rows()/2,i,rgbFg(120,180,255)+"\u2593"+kReset); db.render(); std::this_thread::sleep_for(std::chrono::milliseconds(10)); } (void)label; }
} // namespace chessclaw::ui
