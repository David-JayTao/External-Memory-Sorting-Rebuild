#include "external/replacement_selection.h"
#include <stdexcept>

namespace external {
ReplacementSelection::ReplacementSelection(const std::vector<SelectionEntry>& initial) : tree_(initial) {}
bool ReplacementSelection::empty() const { return tree_.winner() == -1; }
TaggedValue ReplacementSelection::emit(std::optional<Value> incoming) {
    const auto winner = tree_.top();
    SelectionEntry replacement;
    if (incoming) {
        replacement.valid = true;
        replacement.value = *incoming;
        replacement.run_number = winner.run_number + (*incoming < winner.value ? 1 : 0);
    }
    tree_.replace_winner(replacement);
    return {winner.value, winner.run_number};
}
std::vector<SelectionEntry> load_selection(RecordStream& input, std::size_t memory_records) {
    if (!memory_records) throw std::invalid_argument("Zero selection capacity");
    std::vector<SelectionEntry> initial(memory_records);
    for (auto& entry : initial) {
        if (!input.Scan(entry.value)) break;
        entry.valid = true;
    }
    return initial;
}
SelectionOutput::SelectionOutput(RunWriter& writer, std::size_t buffer_capacity)
    : writer_(writer), output_(writer, buffer_capacity) {}
void SelectionOutput::Put(const TaggedValue& record) {
    if (!run_open_ || record.run_number != current_run_) {
        if (run_open_) {
            if (record.run_number != current_run_ + 1) throw std::logic_error("Invalid run transition");
            output_.flush();
            writer_.end_run();
        } else if (record.run_number != 0) {
            throw std::logic_error("First run must be zero");
        }
        writer_.begin_run();
        current_run_ = record.run_number;
        run_open_ = true;
    }
    output_.Put(record.value);
}
void SelectionOutput::finish() {
    output_.flush();
    if (run_open_) {
        writer_.end_run();
        run_open_ = false;
    }
    writer_.finish();
}
void replacement_runs(const std::filesystem::path& input_path,
                      const std::filesystem::path& output_path,
                      std::size_t memory_records, std::size_t buffer_capacity) {
    if (!memory_records || !buffer_capacity) throw std::invalid_argument("Zero capacity");
    RecordStream input(input_path, buffer_capacity);
    // 除最后一段外，每段至少 M 条，逆序输入达到这个最坏上界。
    const auto count = input.record_count();
    const auto max_runs = count / memory_records + (count % memory_records != 0);
    ReplacementSelection selection(load_selection(input, memory_records));
    RunWriter writer(output_path, max_runs);
    SelectionOutput output(writer, buffer_capacity);
    while (!selection.empty()) {
        Value value;
        std::optional<Value> incoming;
        if (input.Scan(value)) incoming = value;
        output.Put(selection.emit(incoming));
    }
    output.finish();
}
}
