#include "external/pipeline.h"
#include "external/merge.h"
#include <iostream>
using namespace external;
int main(int argc, char** argv) {
    try {
        const std::filesystem::path directory = argc > 1 ? argv[1] : "week5-demo";
        if (!std::filesystem::create_directory(directory)) throw std::runtime_error("Demo directory exists");
        RunWriter writer(directory / "input.bin", 1);
        writer.begin_run();
        const Value values[] = {4, 1, 3, 2, 6, 0, 5, 2};
        writer.append_block(values, 8);
        writer.end_run();
        writer.finish();
        LoserTree tree({{4, 0, true}, {1, 0, true}, {3, 0, true}, {2, 0, true}});
        std::cout << "Loser tree winner: " << tree.top().value;
        tree.replace_winner({6, 0, true});
        std::cout << ", after replacement: " << tree.top().value << '\n';
        replacement_runs(directory / "input.bin", directory / "serial.bin", 3, 2);
        pipeline_runs(directory / "input.bin", directory / "pipeline.bin", 3, 2);
        std::cout << "Serial replacement runs:\n";
        RunFile serial(directory / "serial.bin");
        display(serial, std::cout, 2);
        std::cout << "Pipeline replacement runs:\n";
        RunFile parallel(directory / "pipeline.bin");
        display(parallel, std::cout, 2);
        auto plan = optimal_plan(directory / "pipeline.bin");
        execute_plan(plan, directory / "sorted.bin", directory / "work", 2);
        std::cout << "Final result:\n";
        RunFile sorted(directory / "sorted.bin");
        display(sorted, std::cout, 2);

        RunWriter weighted(directory / "weighted.bin", 4);
        for (int length : {1, 2, 3, 10}) {
            weighted.begin_run();
            for (Value value = 0; value < length; ++value) weighted.append_block(&value, 1);
            weighted.end_run();
        }
        weighted.finish();
        const auto ordinary = balanced_plan(directory / "weighted.bin");
        const auto optimal = optimal_plan(directory / "weighted.bin");
        std::cout << "Lengths [1,2,3,10], balanced cost: " << ordinary.nodes[ordinary.root].estimated_cost
                  << ", optimal cost: " << optimal.nodes[optimal.root].estimated_cost << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
