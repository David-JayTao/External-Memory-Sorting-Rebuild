# 外存排序课程项目

用 C++17 逐步实现文件管理、归并计划树和置换选择。课程依据为本地课件第 75–78 页。

| 阶段 | 已实现 |
|---|---|
| Week 3 | 单文件多 Run、Header/Directory/Data、输入输出缓冲、Scan/Put/Search |
| Week 4 | 有限内存分批内排、两路归并、归并计划树与临时文件清理 |
| Week 5 | 败者树、置换选择、最优归并树、双缓冲读算写流水线与异常处理 |

三个阶段均通过 WSL 中的 Debug、Release 构建、测试和 Demo。每周一个正式标签，工作区只保留一套共享代码。

Windows 安装或启用已有 C++17 编译器与 CMake 后运行：

```powershell
./scripts/build.ps1 Debug
./scripts/build.ps1 Release
```

也可使用通用 CMake 命令（单配置生成器示例）：

```sh
cmake -S . -B build-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-Debug --parallel
ctest --test-dir build-Debug --output-on-failure
```

将 Debug 替换为 Release 可构建另一配置。Visual Studio 生成器需给构建和 CTest 传 `--config Release`、`-C Release`；上面的 PowerShell 脚本已处理它们。WSL 使用自己的 Linux CMake 和 GCC，不能直接把 Windows CMake 当作 Linux 工具。

课件保存在 `course_materials/`，不进入 Git。动态缓冲调度和并行 K 路归并是课件 Week 6–7 的内容，不纳入本项目的周验收。

Demo：`./build-Debug/week3_demo week3.bin`，输出路径必须不存在。Visual Studio 生成器在 `build-Debug/Debug/` 下查找可执行文件。

```sh
./build-Debug/week3_demo week3.bin
./build-Debug/week4_demo week4-example
./build-Debug/week5_demo week5-example
```

Week 4、5 参数为新目录。Week 5 会显示败者树更新、串行与流水线置换选择、完整排序，以及普通与最优计划的成本比较。三份测试可分别用 `ctest --test-dir build-Debug -R week3`（或 week4/week5）运行；默认执行全部回归。测试只使用 CTest 与标准 C++，不依赖大型测试框架。

从[课程讲义](docs/课程讲义.md)开始学习，再读[验收手册](docs/验收手册.md)及 [Week 3](docs/验收/Week3.md)、[Week 4](docs/验收/Week4.md)、[Week 5](docs/验收/Week5.md) 问答。[开发历程](docs/开发历程.md)记录阶段提交、命令与验证结果。可选运行 `python scripts/check_docs.py`（Python 3.9+）检查文档链接。

当前实际验证环境为 WSL Ubuntu / GCC 15.2 / CMake 4.4.4；Windows 原生构建尚未验证。M 表示排序/选择工作区的记录容量，B 表示每块记录容量，另有 O(R) 目录与计划元数据，不宣称整个进程只使用 M 条记录的内存。

```text
include/external/   共享文件、缓冲、排序、计划与流水线接口
src/               上述接口的一套实现
apps/              week3_demo.cpp、week4_demo.cpp、week5_demo.cpp
tests/             三周测试和小型公共验证助手
scripts/           build.ps1、check_docs.py
docs/              课程讲义、验收手册、开发历程、验收/Week3–5
course_materials/  本地课件及阅读材料（Git 忽略）
```

查看历史：`git log --graph --decorate --oneline --all`、`git tag -n`。需要学习旧版本时，可用 `git worktree add ../sorting-week3 week3-v1.0` 创建独立学习目录，不必切换当前工作区。
