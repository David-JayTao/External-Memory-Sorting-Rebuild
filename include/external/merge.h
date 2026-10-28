#pragma once
#include "external/buffer.h"
#include <string>

namespace external {
struct RunLocation {
    std::filesystem::path path;
    std::size_t run_id = 0;
};
enum class MergeStatus { Pending, Complete };
struct MergePlanNode {
    int left = -1;
    int right = -1;
    RunLocation source;
    std::filesystem::path output_path;
    std::uint64_t sequence_length = 0;
    std::uint64_t estimated_cost = 0; // 子树累计归并输出记录数
    MergeStatus status = MergeStatus::Pending;
    bool is_leaf() const { return left == -1 && right == -1; }
};
struct MergePlan {
    std::vector<MergePlanNode> nodes;
    int root = -1; // 没有初始 Run 时没有树根
};
void merge_two(const RunLocation& left, const RunLocation& right,
               const std::filesystem::path& output_path, std::size_t buffer_capacity);
MergePlan balanced_plan(const std::filesystem::path& source_path);
// work_directory 必须不存在。只删除本次创建的中间文件，不删除原始输入。
void execute_plan(MergePlan& plan, const std::filesystem::path& output_path,
                  const std::filesystem::path& work_directory, std::size_t buffer_capacity);
}
