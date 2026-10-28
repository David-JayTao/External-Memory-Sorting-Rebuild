#include "test_support.h"
#include "external/run_generation.h"
#include "external/merge.h"
#include <algorithm>
#include <random>
using namespace external;
int main() {
    try {
        TestDirectory directory;
        std::vector<std::vector<Value>> cases = {
            {}, {1}, {3, 2}, {3, 2, 1}, {4, 3, 2, 1},
            {1, 2, 3, 4, 5, 6, 7}, {7, 6, 5, 4, 3, 2, 1},
            {8, 3, 7, 1, 6, 2, 5, 4}, {2, 2, -1, 2, 0, -1, 2}
        };
        std::mt19937 random(20261029);
        std::vector<Value> random_values(257);
        for (auto& value : random_values) value = static_cast<Value>(random() % 101) - 50;
        cases.push_back(random_values);
        std::size_t case_id = 0;
        for (const auto& values : cases) {
            const auto base = directory.path / std::to_string(case_id++);
            const auto source = base.string() + "-input.bin";
            const auto generated = base.string() + "-runs.bin";
            write_values(source, values);
            generate_runs(source, generated, 3, 2);
            RunFile runs(generated);
            check(runs.runs().size() == (values.size() + 2) / 3, "Initial run count");
            for (std::size_t run_id = 0; run_id < runs.runs().size(); ++run_id) {
                std::vector<Value> chunk(3);
                chunk.resize(runs.read_block(run_id, 0, chunk.data(), chunk.size()));
                check(std::is_sorted(chunk.begin(), chunk.end()), "Run sorted");
            }
            auto actual = read_values(generated);
            auto expected = values;
            std::sort(actual.begin(), actual.end());
            std::sort(expected.begin(), expected.end());
            check(actual == expected, "Initial runs preserve multiset");
            auto plan = balanced_plan(generated);
            const auto sorted = base.string() + "-sorted.bin";
            const auto work = base.string() + "-work";
            execute_plan(plan, sorted, work, 2);
            check(read_values(sorted) == expected, "Full merge equals independent std::sort");
            check(RunFile(sorted).runs().size() == 1, "Exactly one final run");
            check(!std::filesystem::exists(work), "Temporary directory cleaned");
            for (const auto& node : plan.nodes) check(node.status == MergeStatus::Complete, "Plan status");
            expect_error([&] { generate_runs(source, generated, 0, 2); });
        }
        const auto mixed = directory.path / "mixed.bin";
        {
            RunWriter writer(mixed, 4);
            for (const std::vector<Value>& values : std::vector<std::vector<Value>>{{}, {1, 1, 4}, {}, {-2, 3}}) {
                writer.begin_run();
                writer.append_block(values.data(), values.size());
                writer.end_run();
            }
            writer.finish();
        }
        auto plan = balanced_plan(mixed);
        execute_plan(plan, directory.path / "mixed-sorted.bin", directory.path / "mixed-work", 1);
        check(read_values(directory.path / "mixed-sorted.bin") == std::vector<Value>({-2, 1, 1, 3, 4}), "Unequal and empty runs");
        plan.nodes[plan.root].left = plan.root;
        expect_error([&] { execute_plan(plan, directory.path / "invalid.bin", directory.path / "invalid-work", 2); });
        write_values(directory.path / "unsorted.bin", {2, 1});
        expect_error([&] { merge_two({directory.path / "unsorted.bin", 0}, {mixed, 1}, directory.path / "bad.bin", 1); });
        expect_error([&] { merge_two({mixed, 1}, {mixed, 3}, mixed, 1); });
        check(read_values(mixed) == std::vector<Value>({1, 1, 4, -2, 3}), "Input preserved");
        std::cout << "Week 4 generation, plan and merge tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
