# Week 3 验收

依据课件第 28 页文件布局、第 75–76 页任务。实现多个不等长 Run 共存于单一文件、文件生成与显示、InputBuffer、OutputBuffer、Scan、Put、Search。具体序列化和查找语义见[讲义](../课程讲义.md)。

代码：[文件类](../../include/external/run_file.h)、[缓冲区类](../../include/external/buffer.h)、[Demo](../../apps/week3_demo.cpp)、[测试](../../tests/week3_tests.cpp)。

构建：`./scripts/build.ps1 Debug` 和 `./scripts/build.ps1 Release`。通用方式：`cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`，`cmake --build build`，`ctest --test-dir build --output-on-failure`。Demo：`./build/week3_demo week3.bin`；Visual Studio 生成器使用 `build/Debug/week3_demo.exe`。

小例子：Run0=[1,4,4]、Run1=[]、Run2=[2]，缓冲容量 2。Put 先写出 [1,4]，flush 再写 [4]。Scan 不会把 Run2 的 2 当作 Run0 的数据。Search(Run0,4) 返回 1。

核心成员：next_record_ 是下次补读的记录下标；position_ 是当前缓存游标；buffered_count_ 是缓存有效条数；run_open_ 表示是否允许追加；data_end_offset 是最后记录之后的字节位置。

1. **为什么分 Header、Directory、Data？** 总体布局、逐 Run 元数据和记录分开管理，可直接定位任一 Run。
2. **空 Run 和没有 Run 一样吗？** 不一样。空 Run 有目录项而 record_count=0；没有 Run 时目录项数是 0。
3. **Scan 的 false 表示什么？** 指定 Run 已读完；I/O 错误通过异常报告。
4. **Put 会立即写磁盘吗？** 它先写算法缓冲，满块时写到文件流；末尾必须 flush。
5. **Search 找重复值怎么返回？** 返回首次命中的零基记录下标；不要求 Run 有序。
6. **为什么不能直接写 struct？** 编译器填充和字节序会使磁盘格式不稳定，这里按固定宽度小端编码。
7. **能检测所有损坏吗？** 不能。能检测结构损坏和截断，没有数据校验和。

2026-10-29 实际验收：WSL Ubuntu、GCC 15.2、CMake 4.4.4；Debug 和 Release 均构建成功、CTest 1/1 通过，两种配置 Demo 均输出上述三段和查找位置 1。测试覆盖缓冲容量 1/2/3/4/8、EOF、int32 极值、空文件、目录损坏、截断、未完成文件与非法状态。Windows 原生编译器尚未发现，提供的 PowerShell 入口待原生验证。
