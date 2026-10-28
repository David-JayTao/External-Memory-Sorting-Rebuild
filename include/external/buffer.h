#pragma once
#include "external/run_file.h"
#include <optional>

namespace external {
class InputBuffer {
public:
    InputBuffer(RunFile& file, std::size_t run_id, std::size_t capacity);
    bool Scan(Value& value);
private:
    RunFile& file_;
    std::size_t run_id_;
    std::vector<Value> values_;
    std::uint64_t next_record_ = 0;
    std::size_t position_ = 0;
    std::size_t buffered_count_ = 0;
};
class OutputBuffer {
public:
    OutputBuffer(RunWriter& writer, std::size_t capacity);
    void Put(Value value);
    void flush();
private:
    RunWriter& writer_;
    std::vector<Value> values_;
    std::size_t buffered_count_ = 0;
};
std::optional<std::uint64_t> Search(RunFile& file, std::size_t run_id,
                                   Value target, std::size_t buffer_capacity);
void display(RunFile& file, std::ostream& output, std::size_t buffer_capacity);
}
