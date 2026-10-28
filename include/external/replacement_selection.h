#pragma once
#include "external/loser_tree.h"
#include "external/run_generation.h"

namespace external {
struct TaggedValue {
    Value value;
    std::uint64_t run_number;
};
class ReplacementSelection {
public:
    explicit ReplacementSelection(const std::vector<SelectionEntry>& initial);
    bool empty() const;
    TaggedValue emit(std::optional<Value> incoming);
private:
    LoserTree tree_;
};
std::vector<SelectionEntry> load_selection(RecordStream& input, std::size_t memory_records);

// 将带 Run 编号的输出交给已有 RunWriter/OutputBuffer，串行和流水线共用。
class SelectionOutput {
public:
    SelectionOutput(RunWriter& writer, std::size_t buffer_capacity);
    void Put(const TaggedValue& record);
    void finish();
private:
    RunWriter& writer_;
    OutputBuffer output_;
    bool run_open_ = false;
    std::uint64_t current_run_ = 0;
};
void replacement_runs(const std::filesystem::path& input_path,
                      const std::filesystem::path& output_path,
                      std::size_t memory_records, std::size_t buffer_capacity);
}
