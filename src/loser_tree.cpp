#include "external/loser_tree.h"
#include <limits>
#include <stdexcept>

namespace external {
LoserTree::LoserTree(const std::vector<SelectionEntry>& entries) {
    if (entries.empty()) throw std::invalid_argument("Loser tree needs at least one slot");
    while (leaf_count_ < entries.size()) {
        if (leaf_count_ > static_cast<std::size_t>(std::numeric_limits<int>::max() / 2))
            throw std::length_error("Loser tree too large");
        leaf_count_ *= 2;
    }
    entries_ = entries;
    entries_.resize(leaf_count_); // 补齐的叶子无效，不借用 INT_MAX 当哨兵。
    losers_.resize(leaf_count_);
    std::vector<int> winners(2 * leaf_count_);
    for (std::size_t index = 0; index < leaf_count_; ++index)
        winners[leaf_count_ + index] = static_cast<int>(index);
    for (std::size_t node = leaf_count_ - 1; node > 0; --node) {
        const int left = winners[2 * node];
        const int right = winners[2 * node + 1];
        if (before(left, right)) {
            winners[node] = left;
            losers_[node] = right;
        } else {
            winners[node] = right;
            losers_[node] = left;
        }
    }
    champion_ = winners[1];
}
bool LoserTree::before(int left, int right) const {
    const auto& first = entries_[left];
    const auto& second = entries_[right];
    if (first.valid != second.valid) return first.valid;
    if (first.valid) {
        if (first.run_number != second.run_number) return first.run_number < second.run_number;
        if (first.value != second.value) return first.value < second.value;
    }
    return left < right; // 相等时用叶子下标确定唯一比赛结果。
}
int LoserTree::winner() const {
    return entries_[champion_].valid ? champion_ : -1;
}
const SelectionEntry& LoserTree::top() const {
    if (winner() == -1) throw std::logic_error("Loser tree exhausted");
    return entries_[champion_];
}
void LoserTree::replace_winner(const SelectionEntry& replacement) {
    if (winner() == -1) throw std::logic_error("Loser tree exhausted");
    int candidate = champion_;
    entries_[candidate] = replacement;
    std::size_t node = (leaf_count_ + static_cast<std::size_t>(candidate)) / 2;
    while (node > 0) {
        if (before(losers_[node], candidate)) std::swap(candidate, losers_[node]);
        node /= 2;
    }
    champion_ = candidate;
}
}
