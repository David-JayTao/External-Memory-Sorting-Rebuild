#include "test_support.h"
#include "external/replacement_selection.h"
#include <algorithm>
#include <limits>
#include <random>
using namespace external;

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
int main() {
    try {
        TestDirectory directory;
        test_loser_tree();
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
            }
        }
        std::cout << "Week 5 loser tree and replacement tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
