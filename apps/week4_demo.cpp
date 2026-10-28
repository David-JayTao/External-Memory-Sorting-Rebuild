#include "external/run_generation.h"
#include "external/merge.h"
#include <iostream>
using namespace external;
int main(int argc, char** argv) {
    try {
        const std::filesystem::path directory = argc > 1 ? argv[1] : "week4-demo";
        if (!std::filesystem::create_directory(directory)) throw std::runtime_error("Demo directory exists");
        RunWriter writer(directory / "input.bin", 1);
        writer.begin_run();
        OutputBuffer output(writer, 2);
        for (Value value : {8, 3, 7, 1, 6, 2, 5, 4}) output.Put(value);
        output.flush();
        writer.end_run();
        writer.finish();
        generate_runs(directory / "input.bin", directory / "runs.bin", 3, 2);
        RunFile runs(directory / "runs.bin");
        std::cout << "Initial runs:\n";
        display(runs, std::cout, 2);
        auto plan = balanced_plan(directory / "runs.bin");
        std::cout << "Merge cost (records): " << plan.nodes[plan.root].estimated_cost << '\n';
        execute_plan(plan, directory / "sorted.bin", directory / "work", 2);
        RunFile sorted(directory / "sorted.bin");
        display(sorted, std::cout, 2);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
