// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#include "chessclaw/storage_check.h"
#include <cstdio>
#include <filesystem>
namespace chessclaw {
std::string formatBytes(std::uint64_t b){ char buf[64]; double v=(double)b; const char* u="B"; if(v>1024){v/=1024;u="KiB";} if(v>1024){v/=1024;u="MiB";} if(v>1024){v/=1024;u="GiB";} std::snprintf(buf,sizeof(buf),"%.2f %s (%llu bytes)",v,u,(unsigned long long)b); return buf; }
StorageReport checkStorage(const std::string& root, std::uint64_t budget){
 std::uint64_t total=0;
 try{ for(auto &e:std::filesystem::recursive_directory_iterator(root)) if(e.is_regular_file()) total+=e.file_size(); }catch(...){}
 return {total,formatBytes(total),total>budget};
}
} // namespace chessclaw
