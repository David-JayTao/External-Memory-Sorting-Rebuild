#pragma once
#include "external/replacement_selection.h"

namespace external {
// 仅测试使用：确定性模拟任务或第二个线程启动失败。默认不注入。
enum class PipelineTask { None, Read, Compute, Write, LaunchWriter };
struct PipelineTestFault {
    PipelineTask task = PipelineTask::None;
    std::size_t stage = 0;
};
void pipeline_runs(const std::filesystem::path& input_path,
                   const std::filesystem::path& output_path,
                   std::size_t memory_records, std::size_t buffer_capacity,
                   PipelineTestFault fault = {});
}
