#pragma once
#include "external/buffer.h"
#include <memory>

namespace external {
// 将文件内所有 Run 按目录顺序看作一条输入流，允许它们尚未排序。
class RecordStream {
public:
    RecordStream(const std::filesystem::path& path, std::size_t buffer_capacity);
    bool Scan(Value& value);
    std::uint64_t record_count() const;
private:
    RunFile file_;
    std::size_t buffer_capacity_;
    std::size_t next_run_ = 0;
    std::unique_ptr<InputBuffer> input_;
};
void generate_runs(const std::filesystem::path& input_path,
                   const std::filesystem::path& output_path,
                   std::size_t memory_records, std::size_t buffer_capacity);
}
