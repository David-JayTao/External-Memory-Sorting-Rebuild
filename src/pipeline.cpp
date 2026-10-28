#include "external/pipeline.h"
#include <array>
#include <exception>
#include <stdexcept>
#include <thread>

namespace external {
namespace {
void inject_fault(PipelineTestFault fault, PipelineTask task, std::size_t stage) {
    if (fault.task == task && fault.stage == stage)
        throw std::runtime_error("Injected pipeline failure");
}
void read_chunk(RecordStream& input, std::vector<Value>& block, std::size_t capacity) {
    block.clear();
    Value value;
    while (block.size() < capacity && input.Scan(value)) block.push_back(value);
}
void compute_chunk(ReplacementSelection& selection, const std::vector<Value>& input,
                   std::vector<TaggedValue>& output, std::size_t capacity) {
    output.clear();
    std::size_t position = 0;
    while (output.size() < capacity && !selection.empty()) {
        std::optional<Value> incoming;
        if (position < input.size()) incoming = input[position++];
        output.push_back(selection.emit(incoming));
    }
    if (position != input.size()) throw std::logic_error("Unconsumed input block");
}
void write_chunk(SelectionOutput& output, std::vector<TaggedValue>& block) {
    for (const auto& record : block) output.Put(record);
    block.clear();
}
}

void pipeline_runs(const std::filesystem::path& input_path,
                   const std::filesystem::path& output_path,
                   std::size_t memory_records, std::size_t buffer_capacity,
                   PipelineTestFault fault) {
    if (!memory_records || !buffer_capacity) throw std::invalid_argument("Zero capacity");
    RecordStream input(input_path, buffer_capacity);
    const auto count = input.record_count();
    ReplacementSelection selection(load_selection(input, memory_records));
    RunWriter writer(output_path, count / memory_records + (count % memory_records != 0));
    SelectionOutput output(writer, buffer_capacity);
    std::array<std::vector<Value>, 2> input_blocks;
    std::array<std::vector<TaggedValue>, 2> output_blocks;
    for (auto& block : input_blocks) block.reserve(buffer_capacity);
    for (auto& block : output_blocks) block.reserve(buffer_capacity);
    int active_input = 0;
    int active_output = 0;
    read_chunk(input, input_blocks[active_input], buffer_capacity);
    std::size_t stage = 0;

    // 主线程承担计算，另两个线程读下一块、写上一块，阶段末 join 后交换。
    while (!selection.empty() || !output_blocks[1 - active_output].empty()) {
        std::exception_ptr read_error, write_error, compute_error;
        std::thread reader;
        std::thread output_thread;
        try {
            reader = std::thread([&] {
                try {
                    inject_fault(fault, PipelineTask::Read, stage);
                    read_chunk(input, input_blocks[1 - active_input], buffer_capacity);
                } catch (...) { read_error = std::current_exception(); }
            });
            inject_fault(fault, PipelineTask::LaunchWriter, stage);
            output_thread = std::thread([&] {
                try {
                    inject_fault(fault, PipelineTask::Write, stage);
                    write_chunk(output, output_blocks[1 - active_output]);
                } catch (...) { write_error = std::current_exception(); }
            });
            inject_fault(fault, PipelineTask::Compute, stage);
            compute_chunk(selection, input_blocks[active_input], output_blocks[active_output], buffer_capacity);
        } catch (...) { compute_error = std::current_exception(); }

        // 即使第二个线程创建失败或计算抛异常，也先收回已启动的线程。
        if (reader.joinable()) reader.join();
        if (output_thread.joinable()) output_thread.join();
        if (read_error) std::rethrow_exception(read_error);
        if (write_error) std::rethrow_exception(write_error);
        if (compute_error) std::rethrow_exception(compute_error);
        active_input = 1 - active_input;
        active_output = 1 - active_output;
        ++stage;
    }
    // 最后一轮只排空输出；所有线程退出后才刷尾块和提交文件头。
    output.finish();
}
}
