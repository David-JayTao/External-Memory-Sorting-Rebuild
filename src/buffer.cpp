#include "external/buffer.h"
#include <stdexcept>

namespace external {
InputBuffer::InputBuffer(RunFile& file, std::size_t run_id, std::size_t capacity)
    : file_(file), run_id_(run_id), values_(capacity) {
    if (!capacity) throw std::invalid_argument("Buffer capacity must be positive");
    if (run_id >= file_.runs().size()) throw std::out_of_range("Run ID");
}
bool InputBuffer::Scan(Value& value) {
    if (position_ == buffered_count_) {
        buffered_count_ = file_.read_block(run_id_, next_record_, values_.data(), values_.size());
        next_record_ += buffered_count_;
        position_ = 0;
        if (!buffered_count_) return false;
    }
    value = values_[position_++];
    return true;
}
OutputBuffer::OutputBuffer(RunWriter& writer, std::size_t capacity)
    : writer_(writer), values_(capacity) {
    if (!capacity) throw std::invalid_argument("Buffer capacity must be positive");
}
void OutputBuffer::Put(Value value) {
    values_[buffered_count_++] = value;
    if (buffered_count_ == values_.size()) flush();
}
void OutputBuffer::flush() {
    if (buffered_count_) {
        writer_.append_block(values_.data(), buffered_count_);
        buffered_count_ = 0;
    }
}
std::optional<std::uint64_t> Search(RunFile& file, std::size_t run_id,
                                   Value target, std::size_t buffer_capacity) {
    InputBuffer input(file, run_id, buffer_capacity);
    Value value;
    std::uint64_t index = 0;
    while (input.Scan(value)) {
        if (value == target) return index;
        ++index;
    }
    return std::nullopt;
}
void display(RunFile& file, std::ostream& output, std::size_t buffer_capacity) {
    for (std::size_t run_id = 0; run_id < file.runs().size(); ++run_id) {
        output << "Run " << run_id << ':';
        InputBuffer input(file, run_id, buffer_capacity);
        Value value;
        while (input.Scan(value)) output << ' ' << value;
        output << '\n';
    }
}
}
