# Week 4 验收

依据课件第 34–38、71、77 页。新增 [RecordStream / generate_runs](../../include/external/run_generation.h)、[merge_two / MergePlanNode / balanced_plan / execute_plan](../../include/external/merge.h)。复用 Week 3 文件与缓冲类。[Demo](../../apps/week4_demo.cpp)、[测试](../../tests/week4_tests.cpp)。

构建与完整回归：`./scripts/build.ps1 Debug` 和 `./scripts/build.ps1 Release`；单周测试：`ctest --test-dir build-Debug -R week4 --output-on-failure`。Demo：`./build-Debug/week4_demo week4-example`，目录必须不存在。多配置生成器按 README 调整路径。

手算：输入 [8,3,7,1,6,2,5,4]，M=3，得到 [3,7,8]、[1,2,6]、[4,5]。先合并前两段输出 6 条，再合并输出 8 条，累计成本 14。最终 [1,2,3,4,5,6,7,8]。

核心成员：MergePlanNode.left/right 指向两个孩子，source 定位叶子，output_path 定位中间结果，sequence_length 是结果长度，estimated_cost 是累计归并输出条数，status 是执行状态。RecordStream.next_run_ 指向下一待打开的 Run，input_ 管理当前 InputBuffer 的生命周期。

1. **初始 Run 和归并有什么区别？** 初始生成对最多 M 条记录内排；归并要求输入已有序，逐条选择较小值。
2. **为什么不能一次读完数据？** N 可能超过内存，记录工作区只允许 M 加固定数量的块缓冲。
3. **相等值会丢失吗？** 不会，选择左边后只推进左边，右边相等值仍待输出。
4. **树中的叶子和内部结点是什么？** 叶子是已有 Run，内部结点代表两个子结果的一次归并。
5. **奇数个 Run 怎么办？** 最后一段提升到下一层，不产生只有一个孩子的归并结点。
6. **什么时候删除临时文件？** 父结点成功合并后删除内部孩子输出；异常时清理本次工作目录；永不删除原始叶子文件。
7. **怎样证明结果完整？** 每次输出消耗一个输入元素；测试再逐条对比独立 std::sort 结果，同时验证有序性和重复次数。
8. **estimated_cost 是真实 I/O 吗？** 不是，是累计归并输出记录数，便于比较计划；未计元数据和缓存影响。

2026-10-29 实际验收：WSL GCC 15.2，Debug 和 Release 全部 CTest 2/2 通过，两种配置的 Demo 均打印成本 14 和正确排序结果。覆盖小于/等于/大于 M、升序、逆序、重复、空与不等长 Run、257 条固定种子随机输入、多阶段归并、损坏计划与无序输入。普通归并为串行实现，异步读算写安排在 Week 5 扩展。Windows 原生编译尚未验证。
