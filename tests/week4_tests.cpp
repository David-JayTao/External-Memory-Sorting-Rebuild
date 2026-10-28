#include "test_support.h"
#include "external/run_generation.h"
#include <algorithm>
using namespace external;
int main() {
    try {
        TestDirectory directory;
        const std::vector<std::vector<Value>> cases = {
            {}, {1}, {3, 2}, {3, 2, 1}, {4, 3, 2, 1},
            {1, 2, 3, 4, 5, 6, 7}, {7, 6, 5, 4, 3, 2, 1},
            {8, 3, 7, 1, 6, 2, 5, 4}, {2, 2, -1, 2, 0, -1, 2}
        };
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
            expect_error([&] { generate_runs(source, generated, 0, 2); });
        }
        std::cout << "Week 4 generation tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
