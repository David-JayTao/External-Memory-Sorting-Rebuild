#include "external/merge.h"
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>

namespace external {
MergePlan optimal_plan(const std::filesystem::path& source_path) {
    RunFile source(source_path);
    if (source.runs().size() > static_cast<std::size_t>(std::numeric_limits<int>::max() / 2))
        throw std::length_error("Too many plan nodes");
    MergePlan plan;
    using WeightedNode = std::pair<std::uint64_t, int>;
    std::priority_queue<WeightedNode, std::vector<WeightedNode>, std::greater<WeightedNode>> pending;
    for (const auto& run : source.runs()) {
        MergePlanNode leaf;
        leaf.source = {source_path, static_cast<std::size_t>(run.id)};
        leaf.sequence_length = run.record_count;
        pending.push({leaf.sequence_length, static_cast<int>(plan.nodes.size())});
        plan.nodes.push_back(leaf);
    }
    while (pending.size() > 1) {
        const int left_index = pending.top().second;
        pending.pop();
        const int right_index = pending.top().second;
        pending.pop();
        const auto& left = plan.nodes[left_index];
        const auto& right = plan.nodes[right_index];
        MergePlanNode parent;
        parent.left = left_index;
        parent.right = right_index;
        parent.sequence_length = left.sequence_length + right.sequence_length;
        if (left.estimated_cost > std::numeric_limits<std::uint64_t>::max() - right.estimated_cost ||
            left.estimated_cost + right.estimated_cost >
                std::numeric_limits<std::uint64_t>::max() - parent.sequence_length)
            throw std::overflow_error("Merge cost overflow");
        parent.estimated_cost = left.estimated_cost + right.estimated_cost + parent.sequence_length;
        pending.push({parent.sequence_length, static_cast<int>(plan.nodes.size())});
        plan.nodes.push_back(parent);
    }
    if (!pending.empty()) plan.root = pending.top().second;
    return plan;
}
}
