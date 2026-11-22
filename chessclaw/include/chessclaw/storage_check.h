// SPDX-License-Identifier: GPL-3.0-only
// chessclaw - Author: vaubhav
#pragma once
#include <cstdint>
#include <string>
namespace chessclaw {
struct StorageReport { std::uint64_t totalBytes; std::string pretty; bool overBudget; };
StorageReport checkStorage(const std::string& root, std::uint64_t budgetBytes = 10ULL*1024*1024*1024);
std::string formatBytes(std::uint64_t b);
} // namespace chessclaw
