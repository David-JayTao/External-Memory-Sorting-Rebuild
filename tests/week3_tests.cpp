#include "test_support.h"
#include <limits>
using namespace external;
int main() {
    try {
        TestDirectory directory;
        const auto path = directory.path / "runs.bin";
        {
            RunWriter writer(path, 3);
            expect_error([&] { writer.end_run(); });
            writer.begin_run();
            const Value data[] = {1, 4, 4};
            writer.append_block(data, 3);
            expect_error([&] { writer.finish(); });
            writer.end_run();
            writer.begin_run();
            writer.end_run();
            writer.begin_run();
            const Value limits[] = {std::numeric_limits<Value>::min(), std::numeric_limits<Value>::max()};
            writer.append_block(limits, 2);
            writer.end_run();
            expect_error([&] { writer.begin_run(); });
            writer.finish();
        }
        RunFile file(path);
        check(file.runs().size() == 3, "Run count");
        check(file.header().data_start_offset == 144, "Header layout");
        check(file.header().data_end_offset == 164, "Data end");
        Value values[4]{};
        check(file.read_block(0, 1, values, 4) == 2 && values[0] == 4 && values[1] == 4, "Read range");
        check(file.read_block(1, 0, values, 4) == 0, "Empty run");
        check(file.read_block(2, 0, values, 4) == 2 && values[0] == std::numeric_limits<Value>::min()
              && values[1] == std::numeric_limits<Value>::max(), "Integer limits");
        expect_error([&] { file.read_block(3, 0, values, 1); });
        expect_error([&] { file.read_block(0, 4, values, 1); });
        expect_error([&] { RunWriter duplicate(path, 1); });
        {
            RunWriter empty(directory.path / "empty.bin", 0);
            empty.finish();
        }
        check(RunFile(directory.path / "empty.bin").runs().empty(), "Empty file");
        {
            RunWriter unfinished(directory.path / "unfinished.bin", 1);
        }
        expect_error([&] { RunFile invalid(directory.path / "unfinished.bin"); });
        std::filesystem::copy_file(path, directory.path / "broken.bin");
        std::filesystem::resize_file(directory.path / "broken.bin", 160);
        expect_error([&] { RunFile invalid(directory.path / "broken.bin"); });
        std::cout << "Week 3 file tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
