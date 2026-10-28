#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace external {
using Value = std::int32_t;
struct FileHeader {
    std::uint64_t max_runs = 0;
    std::uint64_t run_count = 0;
    std::uint64_t directory_offset = 48;
    std::uint64_t data_start_offset = 0;
    std::uint64_t data_end_offset = 0;
};
struct RunInfo {
    std::uint64_t id = 0;
    std::uint64_t record_count = 0;
    std::uint64_t data_offset = 0;
    std::uint64_t data_bytes = 0;
};

// 每个读者拥有独立文件游标；元数据检查在构造时完成。
class RunFile {
public:
    explicit RunFile(const std::filesystem::path& path);
    const FileHeader& header() const { return header_; }
    const std::vector<RunInfo>& runs() const { return runs_; }
    std::size_t read_block(std::size_t run_id, std::uint64_t record_offset,
                           Value* values, std::size_t capacity);
private:
    std::ifstream stream_;
    FileHeader header_;
    std::vector<RunInfo> runs_;
};

class RunWriter {
public:
    RunWriter(const std::filesystem::path& path, std::uint64_t max_runs);
    void begin_run();
    void append_block(const Value* values, std::size_t count);
    void end_run();
    void finish();
private:
    std::ofstream stream_;
    FileHeader header_;
    RunInfo current_run_;
    bool run_open_ = false;
    bool finished_ = false;
};
}
