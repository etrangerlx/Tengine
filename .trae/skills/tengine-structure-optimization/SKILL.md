---
name: tengine-structure-optimization
description: 优化 Tengine 项目的 CMake 构建架构与代码目录结构，并生成供开发者阅读的结构文档。当用户要求重构构建系统、整理目录、梳理模块架构、输出架构或结构文档时使用。不要用于普通功能开发或 bug 修复。
---

# Tengine 代码结构优化

按四个阶段推进：基线盘点 → CMake 构建架构优化 → 目录结构优化 → 结构文档输出。每个阶段结束必须验证构建可编译、可运行，再进入下一阶段。

## 硬约束

- 只做结构与构建重构，不修改任何运行时行为；`source/api/c_api.h` 的公开 C API 签名与语义保持不变。
- 现有 `OPTION(...)` 选项名向后兼容（如 `TENGINE_BUILD_BENCHMARK`、`TENGINE_ENABLE_TIM_VX`），只能新增不能删除；确需重命名时先征得用户确认。
- 目录移动一律使用 `git mv` 保留历史；大规模目录调整前先向用户展示方案并确认。
- 本项目禁止 in-source 构建（根 CMakeLists 有强制检查），一律在 `build/` 或独立目录中验证。
- CMake 最低版本 3.13，不得使用仅更高版本可用的命令，或先抬升 `cmake_minimum_required` 并在文档中说明。

## 项目事实速查

关键文件与现状（执行前先 Read 确认，行号可能变化）：

- 根 [CMakeLists.txt](file:///workspace/leixian_ws/Tengine/CMakeLists.txt) —— 约 30 个 `OPTION` 平铺；INCLUDE `cmake/check.cmake`、`cuda.cmake`、`torch.cmake`、`utility.cmake`；按开关 `ADD_SUBDIRECTORY` benchmark / examples / demos / tests / tools。
- [source/CMakeLists.txt](file:///workspace/leixian_ws/Tengine/source/CMakeLists.txt) —— 构建体系核心，存在以下已知问题：
  1. **全局变量累积式构建**：各模块 CMakeLists 向父作用域 `LIST(APPEND ...)` 十余组全局变量（`TENGINE_*_SOURCE`、`TENGINE_*_COMPILER_DEFINES`、`TENGINE_*_COMPILER_OPTIONS`、`TENGINE_*_LINKER_OPTIONS`、`TENGINE_*_LINK_LIBRARIES`），最后由 source/CMakeLists 汇总。
  2. **单一巨型 target**：所有模块源文件汇入 `tengine-lite` / `tengine-lite-static` 两个 target，模块间无依赖表达。
  3. **`FILE(GLOB)` 与 `AUX_SOURCE_DIRECTORY`** 收集源文件，新增/删除文件不会自动触发重新配置。
  4. **头文件路径全局污染**：`TENGINE_HEADER_PATH` 全量挂到统一 include。
  5. **重复代码**：`TENGINE_TOOLCHAIN_FLAG/_ASM_FLAG/_C_FLAG/_CXX_FLAG` 四段几乎相同的 `FOREACH`。
- `cmake/` 目录：`check/cuda/torch/utility/summary.cmake`、`generate_shader_spv_header.cmake`，子目录 `environments/`、`libraries/`（openmp、pthread、syslog）、`modules/`（FindCUDNN）。
- `source/` 子目录（12 个）：`api`、`device`（含 cpu、tim-vx、cuda、opencl、acl、tensorrt、vulkan、torch、opendla、common 后端）、`operator`、`scheduler`、`serializer`、`executer`、`graph`、`optimizer`、`system`、`utility`、`module`、`olreport`；另有 `defines.h.in` 模板。
- 顶层应用目录：`benchmark`、`examples`、`demos`、`tests`、`tools`（convert_tool、quantize、optimize、save_graph、align_tool）、`pytengine`、`toolchains`、`scripts`、`third_party`、`doc`。
- README「简介」只描述了 4 个核心模块（device / scheduler / operator / serializer），与 source 实际 12 个子目录不一致。

## 阶段 0：基线盘点

1. 在临时目录执行基线构建并记录结果（后续每阶段的对照标准）：

   ```
   cmake -B build -DTENGINE_BUILD_TESTS=ON && cmake --build build -j`nproc` && ctest --test-dir build
   ```

2. 记录基线：生成的库名、编译选项汇总（`cmake/summary.cmake` 输出）、ctest 用例数与结果。
3. 用 `Glob`/`Grep` 盘点所有 CMakeLists.txt 与 `.cmake` 文件，列出每个模块 CMakeLists 向父作用域导出的变量名清单，作为阶段 1 迁移对照表。

## 阶段 1：CMake 构建架构优化

目标：从「全局变量累积 + 单体 target」迁移到「每模块独立 target + 显式依赖图」。渐进式推进，每完成一小步就重新配置编译。

按顺序执行：

1. **选项收拢**：将根 CMakeLists 中约 30 个 `OPTION` 迁入新建的 `cmake/options.cmake`，按「构建开关 / 工具 / 后端插件 / 调试 / 实验」分组并加注释块；根 CMakeLists 仅保留 `INCLUDE`。选项名与默认值不变。
2. **消除 GLOB**：把 `FILE(GLOB ...)` 与 `AUX_SOURCE_DIRECTORY` 替换为显式源文件列表；文件过多时过渡期可用 `FILE(GLOB ... CONFIGURE_DEPENDS ...)`，并在文档中标注后续应显式化。
3. **模块 target 化**：为 `source/` 下每个模块创建独立库 target（建议 `tengine_core_<模块名>`，如 `tengine_core_scheduler`、`tengine_device_cpu`）：
   - 模块内源文件用 `TARGET_SOURCES(<target> PRIVATE ...)` 挂到本模块 target；
   - 模块特有宏/选项用 `TARGET_COMPILE_DEFINITIONS(... PRIVATE ...)`，不再向父作用域 `APPEND` 变量；
   - 模块间依赖用 `TARGET_LINK_LIBRARIES(<target> PUBLIC/PRIVATE <依赖 target>)` 显式表达（依据：scheduler/executer 依赖 graph 与 operator，operator 注册依赖 device 后端，api 是最外层门面；以代码实际 `#include` 关系为准逐一确认）；
   - 顶层 `tengine-lite` / `tengine-lite-static` 改为聚合链接各模块 target，导出的公开头路径用 `PUBLIC`，其余 `PRIVATE`。
4. **头文件路径收敛**：废弃全局 `TENGINE_HEADER_PATH` 拼接，改用各 target 的 `TARGET_INCLUDE_DIRECTORIES`，模块私有头目录一律 `PRIVATE`。
5. **去重工具链代码**：将四段重复的 `FOREACH` 工具链 flag 循环合并为 `cmake/utility.cmake` 中的一个函数（如 `TENGINE_APPLY_TOOLCHAIN_FLAGS(<target>)`）。
6. **安装规则随模块走**：`INSTALL(TARGETS ...)` 与头文件安装规则移到对应模块 CMakeLists；对外安装产物（`include/tengine/c_api.h`、`defines.h`、静态/动态库）保持不变。
7. 保留 `cmake/summary.cmake` 的汇总输出，在其中追加各模块 target 的使能状态打印，方便开发者确认配置。

可选增强（用户同意后）：添加 `CMakePresets.json` 提供 x86-linux / arm-linux / android 等常用 preset。

验证：重新执行阶段 0 的构建命令，库名、安装产物、ctest 结果与基线一致；`cmake --install build` 冒烟通过。

## 阶段 2：代码目录结构优化

目标：厘清职责边界，让目录结构自解释。不做大规模合并，除非用户明确要求。

1. **应用目录职责表**：在方案确认时向用户展示并达成一致——`examples/` 最小用法示例、`demos/` 完整应用、`tools/` 开发者工具（模型转换/量化/图优化）、`benchmark/` 性能评估、`tests/` 单元与集成测试。若发现文件放错位置（如 demo 级代码出现在 examples），列出迁移清单经用户确认后 `git mv`。
2. **source 模块对齐**：每个 `source/<模块>` 目录与阶段 1 建立的 target 一一对应；新增模块时同步创建目录 + CMakeLists + source/CMakeLists 注册，此规则写入文档。
3. **后端一致性**：检查 `source/device/` 各后端目录（cpu、tim-vx、cuda 等）的内部布局是否一致（CMakeLists、注册入口、kernel 目录），不一致的统一为相同骨架；后端开关在 `cmake/options.cmake` 中与目录一一对应。
4. **同步引用**：任何目录调整必须同步修改受影响的 CMake 路径、`#include` 相对路径与文档内链接；公共头文件移动时提供兼容转发头或与用户确认直接更新所有引用。

验证：全量构建 + ctest 通过；`git status` 无意外文件；`Grep` 检查无残留的旧路径引用。

## 阶段 3：输出结构文档

产出面向开发者的结构文档，写入 `doc/docs_zh/develop_guides/`（文件名如 `code_structure.md`；用户指定其他位置时从其要求），可选同步英文版到 `doc/docs_en/develop_guides/`。

文档必须包含以下章节：

1. **总览**：一段话说清 Tengine 分层（API 层 → 图/优化器 → 调度/执行器 → 算子前端 → 设备后端 → 序列化器）。
2. **模块地图**：表格列出 `source/` 全部子目录 —— 目录名、职责、对应 CMake target、关键入口文件、依赖哪些模块（纠正 README 只写 4 个模块的问题）。
3. **依赖关系图**：Mermaid `graph TD` 描述模块 target 间依赖，后端插件（tim-vx、cuda 等）单独分组。
4. **目录树注释版**：带行内注释的目录树，覆盖顶层目录与 `source/`、`tools/`。
5. **CMake 选项速查表**：按分组列出全部 `TENGINE_*` 选项、默认值、作用、影响哪些 target。
6. **数据流**：模型加载（tmfile 解析）→ 图构建 → 图优化 → 调度切图 → 执行器逐层执行 → 后端计算 的流程说明，标注每步对应的模块目录。
7. **上手路径**：两个典型任务的文件清单 —— 「新增/修改一个算子要动哪些文件」「新增一个硬件后端要动哪些文件」。
8. **构建指南**：常用配置组合的最小 cmake 命令（默认构建、开转换工具、开 TIM-VX 等）。

同时更新根 `README.md`「简介」中的模块列表，使其与实际目录和文档一致，并链接新文档。

## 产出物清单

- 重构后的 `CMakeLists.txt`、`cmake/*.cmake`（含新增 `cmake/options.cmake`）。
- 调整后的目录结构与迁移记录（哪些文件被 `git mv`）。
- `doc/docs_zh/develop_guides/` 下的结构文档（及可选英文版）。
- 更新后的 `README.md` 模块简介。
- 验证记录：基线与重构后构建、ctest、install 结果对比。
