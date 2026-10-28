# 外存排序课程项目

用 C++17 逐步实现文件管理、归并计划树和置换选择。课程依据为本地课件第 75–78 页。

当前阶段：Week 3 文件管理与缓冲接口通过 Debug、Release 验收；Week 4–5 待实现。

Windows 安装或启用已有 C++17 编译器与 CMake 后运行：

```powershell
./scripts/build.ps1 Debug
./scripts/build.ps1 Release
```

课件保存在 `course_materials/`，不进入 Git。动态缓冲调度和并行 K 路归并是课件 Week 6–7 的内容，不纳入本项目的周验收。

Demo：`./build-Debug/week3_demo week3.bin`，输出路径必须不存在。Visual Studio 生成器在 `build-Debug/Debug/` 下查找可执行文件。

从[课程讲义](docs/课程讲义.md)开始学习，再读[Week 3 验收](docs/验收/Week3.md)。当前实际验证环境为 WSL Ubuntu / GCC 15.2；Windows 原生构建尚未验证。
