#include "external/run_generation.h"
#include <algorithm>
#include <stdexcept>

namespace external {
RecordStream::RecordStream(const std::filesystem::path& path, std::size_t buffer_capacity)
    : file_(path), buffer_capacity_(buffer_capacity) {
    if (!buffer_capacity) throw std::invalid_argument("Buffer capacity must be positive");
}
bool RecordStream::Scan(Value& value) {
    while (true) {
        if (input_ && input_->Scan(value)) return true;
        if (next_run_ == file_.runs().size()) return false;
        input_ = std::make_unique<InputBuffer>(file_, next_run_++, buffer_capacity_);
    }
}
std::uint64_t RecordStream::record_count() const {
    return (file_.header().data_end_offset - file_.header().data_start_offset) / 4;
}
void generate_runs(const std::filesystem::path& input_path,
                   const std::filesystem::path& output_path,
                   std::size_t memory_records, std::size_t buffer_capacity) {
    if (!memory_records || !buffer_capacity) throw std::invalid_argument("Zero capacity");
    RecordStream input(input_path, buffer_capacity);
    const auto count = input.record_count();
    const auto max_runs = count / memory_records + (count % memory_records != 0);
    RunWriter writer(output_path, max_runs);
    OutputBuffer output(writer, buffer_capacity);
    std::vector<Value> records;
    records.reserve(memory_records);
    Value value;
    while (input.Scan(value)) {
        records.push_back(value);
        if (records.size() == memory_records) {
            std::sort(records.begin(), records.end());
            writer.begin_run();
            for (Value sorted_value : records) output.Put(sorted_value);
            output.flush();
            writer.end_run();
            records.clear();
        }
    }
    if (!records.empty()) {
        std::sort(records.begin(), records.end());
        writer.begin_run();
        for (Value sorted_value : records) output.Put(sorted_value);
        output.flush();
        writer.end_run();
    }
    writer.finish();
}
}
