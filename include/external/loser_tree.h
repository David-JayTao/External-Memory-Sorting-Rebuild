#pragma once
#include "external/run_file.h"

namespace external {
struct SelectionEntry {
    Value value = 0;
    std::uint64_t run_number = 0;
    bool valid = false;
};
// 内部结点只保留败者；输出冠军后只需沿冠军的路径重赛。
class LoserTree {
public:
    explicit LoserTree(const std::vector<SelectionEntry>& entries);
    int winner() const;
    const SelectionEntry& top() const;
    void replace_winner(const SelectionEntry& replacement);
private:
    bool before(int left, int right) const;
    std::vector<SelectionEntry> entries_;
    std::vector<int> losers_;
    std::size_t leaf_count_ = 1;
    int champion_ = 0;
};
}
