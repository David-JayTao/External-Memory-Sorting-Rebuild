#include "test_support.h"
#include "external/buffer.h"
#include <limits>
#include <sstream>
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
        for (std::size_t capacity : {1, 2, 3, 4, 8}) {
            const auto buffered_path = directory.path / ("buffer" + std::to_string(capacity) + ".bin");
            RunWriter writer(buffered_path, 1);
            expect_error([&] { OutputBuffer invalid(writer, 0); });
            OutputBuffer output(writer, capacity);
            writer.begin_run();
            for (Value value : {-3, 1, 1, 7, 9}) output.Put(value);
            output.flush();
            output.flush();
            writer.end_run();
            writer.finish();
            RunFile buffered(buffered_path);
            InputBuffer input(buffered, 0, capacity);
            Value value = 0;
            for (Value expected : {-3, 1, 1, 7, 9})
                check(input.Scan(value) && value == expected, "Buffered round trip");
            check(!input.Scan(value) && !input.Scan(value), "Repeated EOF");
            check(Search(buffered, 0, 1, capacity) == 1, "First duplicate position");
            check(!Search(buffered, 0, 8, capacity), "Search miss");
            std::ostringstream text;
            display(buffered, text, capacity);
            check(text.str() == "Run 0: -3 1 1 7 9\n", "Display");
            expect_error([&] { InputBuffer invalid(buffered, 0, 0); });
        }
        check(!Search(file, 1, 4, 2), "Search empty run");
        // 字节级检查独立于本项目读者，防止写者与读者共享同一种错误。
        std::ifstream raw(path, std::ios::binary);
        raw.seekg(144);
        check(raw.get() == 1 && raw.get() == 0 && raw.get() == 0 && raw.get() == 0, "Little endian record");
        for (std::streamoff offset : {0, 8, 16, 24, 32, 40, 48, 56, 64, 72}) {
            const auto corrupt_path = directory.path / ("corrupt" + std::to_string(offset) + ".bin");
            std::filesystem::copy_file(path, corrupt_path);
            std::fstream corrupt(corrupt_path, std::ios::binary | std::ios::in | std::ios::out);
            corrupt.seekp(offset);
            corrupt.put(static_cast<char>(255));
            corrupt.close();
            expect_error([&] { RunFile invalid(corrupt_path); });
        }
        std::cout << "Week 3 file, buffer, Scan/Put/Search and corruption tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
