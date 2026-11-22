// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include <string>
#include <vector>
namespace chessclaw::ui {
class DoubleBuffer {
public:
  DoubleBuffer(int rows=24,int cols=80);
  void resize(int rows,int cols);
  int rows() const { return rows_; }
  int cols() const { return cols_; }
  void clear(const std::string& fill=" ");
  void put(int r,int c,const std::string& s);
  void putChar(int r,int c,char ch);
  void hline(int r,int c,int n,const std::string& s);
  void box(int r,int c,int h,int w,const std::string& title="");
  void render(bool force=false); // diff + flush
  std::string snapshot() const;  // for tests
private:
  int rows_, cols_;
  std::vector<std::string> cur_, prev_;
  bool first_{true};
};
} // namespace chessclaw::ui
