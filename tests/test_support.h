#pragma once
#include "external/run_file.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include "external/buffer.h"

inline void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
template<class Action>
void expect_error(Action action) {
    bool caught = false;
    try { action(); } catch (const std::exception&) { caught = true; }
    check(caught, "Expected exception");
}
// CTest 为各周提供独立工作目录，只清理由测试自己建立的子目录。
class TestDirectory {
public:
    TestDirectory() {
        check(std::filesystem::create_directory(path), "Test scratch directory already exists");
    }
    ~TestDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
    const std::filesystem::path path = "scratch";
};

inline void write_values(const std::filesystem::path& path, const std::vector<external::Value>& values) {
    external::RunWriter writer(path, 1);
    writer.begin_run();
    external::OutputBuffer output(writer, 3);
    for (auto value : values) output.Put(value);
    output.flush();
    writer.end_run();
    writer.finish();
}
inline std::vector<external::Value> read_values(const std::filesystem::path& path) {
    external::RunFile file(path);
    std::vector<external::Value> values;
    for (std::size_t run = 0; run < file.runs().size(); ++run) {
        external::InputBuffer input(file, run, 3);
        external::Value value;
        while (input.Scan(value)) values.push_back(value);
    }
    return values;
}
