#include "external/merge.h"
#include <limits>
#include <stdexcept>

namespace external {
namespace {
bool scan_sorted(InputBuffer& input, Value& current, bool& has_previous, Value& previous) {
    if (!input.Scan(current)) return false;
    if (has_previous && current < previous) throw std::runtime_error("Merge input is not sorted");
    previous = current;
    has_previous = true;
    return true;
}
void copy_run(const RunLocation& source, const std::filesystem::path& output_path,
              std::size_t capacity) {
    RunFile file(source.path);
    InputBuffer input(file, source.run_id, capacity);
    RunWriter writer(output_path, 1);
    OutputBuffer output(writer, capacity);
    writer.begin_run();
    Value value = 0, previous = 0;
    bool has_previous = false;
    while (scan_sorted(input, value, has_previous, previous)) output.Put(value);
    output.flush();
    writer.end_run();
    writer.finish();
}
// 校验树形结构，防止公开计划对象被错误修改后递归循环或重复消费子树。
void validate_plan(const MergePlan& plan) {
    if (plan.root == -1) {
        if (!plan.nodes.empty()) throw std::invalid_argument("Missing root");
        return;
    }
    if (plan.root != static_cast<int>(plan.nodes.size()) - 1)
        throw std::invalid_argument("Root must be final node");
    std::vector<int> parents(plan.nodes.size(), 0);
    for (std::size_t index = 0; index < plan.nodes.size(); ++index) {
        const auto& node = plan.nodes[index];
        if (node.is_leaf()) continue;
        for (int child : {node.left, node.right}) {
            if (child < 0 || child >= static_cast<int>(index) || ++parents[child] != 1)
                throw std::invalid_argument("Invalid merge tree");
        }
        const auto left_length = plan.nodes[node.left].sequence_length;
        const auto right_length = plan.nodes[node.right].sequence_length;
        if (left_length > std::numeric_limits<std::uint64_t>::max() - right_length ||
            node.sequence_length != left_length + right_length)
            throw std::invalid_argument("Invalid plan length");
    }
    for (std::size_t index = 0; index + 1 < parents.size(); ++index)
        if (parents[index] != 1) throw std::invalid_argument("Disconnected plan");
}
RunLocation execute_node(MergePlan& plan, int index,
                         const std::filesystem::path& output_path,
                         const std::filesystem::path& work_directory, std::size_t capacity) {
    auto& node = plan.nodes.at(static_cast<std::size_t>(index));
    if (node.is_leaf()) {
        RunFile source(node.source.path);
        if (source.runs().at(node.source.run_id).record_count != node.sequence_length)
            throw std::runtime_error("Source length differs from plan");
        node.status = MergeStatus::Complete;
        return node.source;
    }
    const auto left = execute_node(plan, node.left, output_path, work_directory, capacity);
    const auto right = execute_node(plan, node.right, output_path, work_directory, capacity);
    node.output_path = index == plan.root ? output_path
        : work_directory / ("merge-" + std::to_string(index) + ".bin");
    merge_two(left, right, node.output_path, capacity);
    node.status = MergeStatus::Complete;
    // 只回收内部结点的输出；叶子所在的原始多 Run 文件始终保留。
    if (!plan.nodes[node.left].is_leaf()) std::filesystem::remove(left.path);
    if (!plan.nodes[node.right].is_leaf()) std::filesystem::remove(right.path);
    return {node.output_path, 0};
}
}

void merge_two(const RunLocation& left, const RunLocation& right,
               const std::filesystem::path& output_path, std::size_t buffer_capacity) {
    if (!buffer_capacity) throw std::invalid_argument("Zero buffer capacity");
    RunFile left_file(left.path), right_file(right.path);
    InputBuffer left_input(left_file, left.run_id, buffer_capacity);
    InputBuffer right_input(right_file, right.run_id, buffer_capacity);
    RunWriter writer(output_path, 1);
    OutputBuffer output(writer, buffer_capacity);
    writer.begin_run();
    Value left_value = 0, right_value = 0, left_previous = 0, right_previous = 0;
    bool left_seen = false, right_seen = false;
    bool has_left = scan_sorted(left_input, left_value, left_seen, left_previous);
    bool has_right = scan_sorted(right_input, right_value, right_seen, right_previous);
    while (has_left || has_right) {
        if (!has_right || (has_left && left_value <= right_value)) {
            output.Put(left_value);
            has_left = scan_sorted(left_input, left_value, left_seen, left_previous);
        } else {
            output.Put(right_value);
            has_right = scan_sorted(right_input, right_value, right_seen, right_previous);
        }
    }
    output.flush();
    writer.end_run();
    writer.finish();
}

MergePlan balanced_plan(const std::filesystem::path& source_path) {
    RunFile source(source_path);
    if (source.runs().size() > static_cast<std::size_t>(std::numeric_limits<int>::max() / 2))
        throw std::length_error("Too many plan nodes");
    MergePlan plan;
    std::vector<int> level;
    for (const auto& run : source.runs()) {
        MergePlanNode leaf;
        leaf.source = {source_path, static_cast<std::size_t>(run.id)};
        leaf.sequence_length = run.record_count;
        level.push_back(static_cast<int>(plan.nodes.size()));
        plan.nodes.push_back(leaf);
    }
    while (level.size() > 1) {
        std::vector<int> next_level;
        for (std::size_t index = 0; index < level.size(); index += 2) {
            if (index + 1 == level.size()) {
                next_level.push_back(level[index]);
                continue;
            }
            MergePlanNode parent;
            parent.left = level[index];
            parent.right = level[index + 1];
            const auto& left = plan.nodes[parent.left];
            const auto& right = plan.nodes[parent.right];
            parent.sequence_length = left.sequence_length + right.sequence_length;
            parent.estimated_cost = left.estimated_cost + right.estimated_cost + parent.sequence_length;
            next_level.push_back(static_cast<int>(plan.nodes.size()));
            plan.nodes.push_back(parent);
        }
        level = std::move(next_level);
    }
    if (!level.empty()) plan.root = level.front();
    return plan;
}

void execute_plan(MergePlan& plan, const std::filesystem::path& output_path,
                  const std::filesystem::path& work_directory, std::size_t buffer_capacity) {
    if (!buffer_capacity) throw std::invalid_argument("Zero buffer capacity");
    validate_plan(plan);
    if (std::filesystem::exists(output_path)) throw std::runtime_error("Output already exists");
    if (!std::filesystem::create_directory(work_directory))
        throw std::runtime_error("Work directory already exists");
    try {
        for (auto& node : plan.nodes) node.status = MergeStatus::Pending;
        if (plan.root == -1) {
            RunWriter empty(output_path, 1);
            empty.begin_run();
            empty.end_run();
            empty.finish();
        } else if (plan.nodes[plan.root].is_leaf()) {
            auto& root = plan.nodes[plan.root];
            copy_run(root.source, output_path, buffer_capacity);
            root.output_path = output_path;
            root.status = MergeStatus::Complete;
        } else {
            execute_node(plan, plan.root, output_path, work_directory, buffer_capacity);
        }
        std::filesystem::remove(work_directory);
    } catch (...) {
        // work_directory 是本次独占创建的目录，失败也不会清理用户目录。
        std::error_code error;
        std::filesystem::remove_all(work_directory, error);
        throw;
    }
}
}
