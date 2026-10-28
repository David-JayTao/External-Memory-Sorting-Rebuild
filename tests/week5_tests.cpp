#include "test_support.h"
#include "external/replacement_selection.h"
#include "external/merge.h"
#include "external/pipeline.h"
#include <algorithm>
#include <limits>
#include <random>
using namespace external;

// 小规模穷举所有二路合并顺序，独立验证贪心计划的最小成本。
std::uint64_t exhaustive_cost(const std::vector<std::uint64_t>& lengths) {
    if (lengths.size() < 2) return 0;
    auto best = std::numeric_limits<std::uint64_t>::max();
    for (std::size_t first = 0; first < lengths.size(); ++first) {
        for (std::size_t second = first + 1; second < lengths.size(); ++second) {
            std::vector<std::uint64_t> remaining;
            for (std::size_t index = 0; index < lengths.size(); ++index)
                if (index != first && index != second) remaining.push_back(lengths[index]);
            const auto merged = lengths[first] + lengths[second];
            remaining.push_back(merged);
            best = std::min(best, merged + exhaustive_cost(remaining));
        }
    }
    return best;
}
void test_optimal(const std::filesystem::path& directory) {
    std::size_t case_id = 0;
    for (const auto& lengths : std::vector<std::vector<std::uint64_t>>{
             {}, {0}, {1}, {0, 0, 1}, {1, 2, 3, 10}, {2, 2, 2, 2, 2}, {0, 3, 7, 1, 4}}) {
        const auto source = directory / ("optimal" + std::to_string(case_id++) + ".bin");
        RunWriter writer(source, lengths.size());
        std::vector<Value> expected;
        for (auto length : lengths) {
            writer.begin_run();
            for (std::uint64_t index = 0; index < length; ++index) {
                const auto value = static_cast<Value>(index);
                writer.append_block(&value, 1);
                expected.push_back(value);
            }
            writer.end_run();
        }
        writer.finish();
        auto optimal = optimal_plan(source);
        const auto balanced = balanced_plan(source);
        const auto cost = optimal.root == -1 ? 0 : optimal.nodes[optimal.root].estimated_cost;
        check(cost == exhaustive_cost(lengths), "Optimal cost vs exhaustive oracle");
        if (lengths == std::vector<std::uint64_t>({1, 2, 3, 10})) {
            check(cost == 25, "Huffman example cost");
            check(balanced.nodes[balanced.root].estimated_cost == 32, "Balanced example cost");
        }
        const auto output = source.string() + "-sorted.bin";
        execute_plan(optimal, output, source.string() + "-work", 2);
        std::sort(expected.begin(), expected.end());
        check(read_values(output) == expected, "Optimal execution result");
    }
}

void test_loser_tree() {
    LoserTree tree({{4, 0, true}, {1, 0, true}, {3, 0, true}, {2, 0, true}});
    check(tree.winner() == 1 && tree.top().value == 1, "Initial winner");
    tree.replace_winner({6, 0, true});
    for (Value expected : {2, 3, 4, 6}) {
        check(tree.top().value == expected, "Loser path replay");
        tree.replace_winner({});
    }
    check(tree.winner() == -1, "Exhausted loser tree");
    expect_error([&] { tree.top(); });
    std::mt19937 random(17);
    for (std::size_t slots : {1, 3, 5, 8, 17}) {
        std::vector<SelectionEntry> entries(slots);
        for (auto& entry : entries) entry = {static_cast<Value>(random() % 7), 0, true};
        LoserTree compared(entries);
        for (int step = 0; step < 100; ++step) {
            int expected = -1;
            for (std::size_t index = 0; index < slots; ++index) {
                if (!entries[index].valid) continue;
                if (expected == -1 || entries[index].run_number < entries[expected].run_number ||
                    (entries[index].run_number == entries[expected].run_number &&
                     entries[index].value < entries[expected].value)) expected = static_cast<int>(index);
            }
            check(compared.winner() == expected, "Loser tree vs linear oracle");
            if (expected == -1) break;
            const SelectionEntry next{static_cast<Value>(random() % 9), random() % 3, step < 60};
            entries[expected] = next;
            compared.replace_winner(next);
        }
    }
}
void test_pipeline_failures(const std::filesystem::path& directory) {
    const auto source = directory / "fault-input.bin";
    write_values(source, {9, 1, 8, 2, 7, 3, 6, 4, 5, 0, -1, 2, 3, 4, 5});
    std::size_t case_id = 0;
    for (auto task : {PipelineTask::Read, PipelineTask::Compute, PipelineTask::Write, PipelineTask::LaunchWriter}) {
        for (std::size_t stage : {0, 1, 3}) {
            const auto output = directory / ("fault" + std::to_string(case_id++) + ".bin");
            expect_error([&] { pipeline_runs(source, output, 3, 2, {task, stage}); });
            expect_error([&] { RunFile unfinished(output); });
            // 能立即重新打开并删除，说明异常路径已关闭文件并汇合线程。
            check(std::filesystem::remove(output), "Failed output can be removed");
        }
    }
    const auto recovered = directory / "recovered.bin";
    pipeline_runs(source, recovered, 3, 2);
    auto plan = optimal_plan(recovered);
    execute_plan(plan, directory / "recovered-sorted.bin", directory / "recovered-work", 1);
    auto expected = read_values(source);
    std::sort(expected.begin(), expected.end());
    check(read_values(directory / "recovered-sorted.bin") == expected, "Fresh call succeeds after faults");
    expect_error([&] { pipeline_runs(source, directory / "zero.bin", 0, 2); });
    expect_error([&] { pipeline_runs(source, directory / "zero.bin", 3, 0); });
    expect_error([&] { replacement_runs(source, directory / "zero.bin", 0, 2); });
    expect_error([&] { pipeline_runs(directory / "missing.bin", directory / "absent.bin", 3, 2); });
    expect_error([&] { pipeline_runs(source, directory / "missing-parent" / "out.bin", 3, 2); });
    const auto preserved = read_values(source);
    expect_error([&] { pipeline_runs(source, source, 3, 2); });
    check(read_values(source) == preserved, "Pipeline preserves existing output");
}
int main() {
    try {
        TestDirectory directory;
        test_loser_tree();
        test_optimal(directory.path);
        test_pipeline_failures(directory.path);
        std::vector<std::vector<Value>> cases = {
            {}, {1}, {1, 2, 3, 4, 5, 6, 7, 8}, {8, 7, 6, 5, 4, 3, 2, 1},
            {4, 1, 3, 2, 6, 0, 5, 2}, {2, 2, 2, 2, 2},
            {std::numeric_limits<Value>::max(), 0, std::numeric_limits<Value>::min(), -1}
        };
        std::mt19937 random(42);
        std::vector<Value> random_values(127);
        for (auto& value : random_values) value = static_cast<Value>(random() % 31) - 15;
        cases.push_back(random_values);
        std::size_t case_id = 0;
        for (const auto& values : cases) {
            for (std::size_t memory : {1, 3, 8}) {
                const auto base = directory.path / std::to_string(case_id++);
                const auto source = base.string() + "-input.bin";
                const auto result = base.string() + "-runs.bin";
                write_values(source, values);
                replacement_runs(source, result, memory, 2);
                RunFile file(result);
                check(file.runs().size() <= (values.size() + memory - 1) / memory, "Run count bound");
                for (std::size_t run = 0; run < file.runs().size(); ++run) {
                    InputBuffer input(file, run, 2);
                    Value previous = 0, value = 0;
                    bool first = true;
                    while (input.Scan(value)) {
                        check(first || previous <= value, "Replacement run sorted");
                        previous = value;
                        first = false;
                    }
                }
                auto actual = read_values(result), expected = values;
                std::sort(actual.begin(), actual.end());
                std::sort(expected.begin(), expected.end());
                check(actual == expected, "Replacement preserves all records");
                if (!values.empty() && std::is_sorted(values.begin(), values.end()))
                    check(file.runs().size() == 1, "Sorted input produces one long run");
                for (std::size_t buffer : {1, 2, 5}) {
                    const auto parallel = base.string() + "-pipeline" + std::to_string(buffer) + ".bin";
                    pipeline_runs(source, parallel, memory, buffer);
                    check(read_values(parallel) == read_values(result), "Pipeline equals serial sequence");
                    RunFile piped(parallel);
                    check(piped.runs().size() == file.runs().size(), "Pipeline run count");
                    for (std::size_t run = 0; run < file.runs().size(); ++run)
                        check(piped.runs()[run].record_count == file.runs()[run].record_count, "Pipeline run boundaries");
                    if (buffer == 2) {
                        auto plan = optimal_plan(parallel);
                        const auto final_path = parallel + "-sorted.bin";
                        execute_plan(plan, final_path, parallel + "-work", buffer);
                        check(read_values(final_path) == expected, "Pipeline plus optimal merge end-to-end");
                    }
                }
            }
        }
        std::cout << "Week 5 loser tree, replacement, optimal plan and pipeline tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
