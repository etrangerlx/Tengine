# 代码结构重构变更摘要

- 日期：2026-10-16
- 范围：仅构建系统与文档，**不改任何运行时行为**
- 执行依据：`.trae/skills/tengine-structure-optimization/SKILL.md`

## 1. 变更规模

| 类型 | 文件 | 变更量 |
|---|---|---|
| 修改 | 10 个（根 CMakeLists、source/ 下 7 个 CMakeLists、cmake/utility.cmake、cmake/summary.cmake、README.md） | +360 / −268 |
| 新增 | `cmake/options.cmake`、`doc/docs_zh/develop_guides/code_structure.md`、skill 文件 | 3 个 |

## 2. 架构变化（Before → After）

| 项 | 重构前 | 重构后 |
|---|---|---|
| 选项管理 | 约 30 个 OPTION 平铺在根 CMakeLists | 集中到 [cmake/options.cmake](../../cmake/options.cmake)，按 7 组分类；根文件仅 INCLUDE |
| target 结构 | 全部源码汇入 `tengine-lite` / `tengine-lite-static` 两个单体 target | 13 个模块 OBJECT target（`tengine_core_*` + `tengine_device_cpu`）聚合而成，模块可独立管理 |
| 源文件收集 | `FILE(GLOB)` 与 `AUX_SOURCE_DIRECTORY` 静默收集 | 平铺模块改显式列表；算子插件目录保留 GLOB 但加 `CONFIGURE_DEPENDS` |
| 公共编译配置 | 全局变量合并后挂到单体 target | `tengine_build_config`（INTERFACE）承载，模块 PRIVATE 引用，不外泄给消费者 |
| 头文件路径 | `TENGINE_HEADER_PATH` 全局 union | `tengine_public_include`（INTERFACE）+ 模块 PRIVATE 路径；可见性与重构前一致 |
| 工具链 flag | 四段重复 FOREACH（TOOLCHAIN/ASM/C/CXX） | 合并为 `TENGINE_APPLY_TOOLCHAIN_FLAGS()`（cmake/utility.cmake） |
| 模块可见性 | 无从知晓 | `cmake/summary.cmake` 配置时打印全部模块 target 状态 |
| 兼容层 | — | 旧全局变量（`TENGINE_*_SOURCE` 等）全部保留导出，兼容未迁移后端插件与 standalone HCL |

关键修正（实施中发现并修复）：对象库补 `-fPIC`（共享库必需）、`-fvisibility=hidden`（恢复与基线一致的 87 个导出符号）、OpenMP 编译标志注入对象库（否则 `#pragma omp` 被静默忽略）、device 聚合层 LINK_PATH 合并补全。

## 3. 验证结果（与基线逐项对比）

| 验证项 | 基线 | 重构后 | 结论 |
|---|---|---|---|
| Debug 配置 + 构建 | OK | OK | 一致 |
| 共享库导出符号 | 87 | 87，`nm -D` diff 为空 | 动态 ABI 完全一致 |
| 静态库符号数 | 1222 | 1222 | 一致 |
| ctest（75 个 onnx op 测试） | 75/75 失败 | 相同失败、相同原因 | 一致（失败为测试数据缺失，预先存在） |
| install 产物 | lib + include/tengine/{c_api.h, defines.h} + bin | 相同 | 一致 |
| convert_tool（静态库消费者） | 构建通过 | 构建通过 | 一致 |

## 4. 文档产出

- 新增 [代码结构指南](code_structure.md)：模块地图、Mermaid 依赖图、目录树注释、CMake 选项速查、数据流、「新增算子 / 新增后端」上手路径、构建指南；
- 更新 [README.md](../../../README.md)：模块简介从 4 个核心模块补全至全部 12 个 source 子目录，并链接结构指南。

## 5. 已知说明与后续建议

1. 调试宏（`DEBUG_DATA` / `DEBUG_TIME` / `CONFIG_MEM_STAT`）现在同时作用于 static 与 shared（均默认关闭，默认构建无差异）；
2. tim-vx / cuda / opencl 等后端插件源码仍走变量注入，迁移为独立 target 需在对应硬件环境逐一验证；
3. 算子插件目录（`operator/prototype`、`tmfile/op`）的 GLOB 属于「一算子一文件」的插件设计，保留并加 `CONFIGURE_DEPENDS`；
4. `TENGINE_BUILD_CPP_API` 在当前源码树缺少 `cpp_api.cc/h`，开启会触发明确的 FATAL_ERROR（原为静默产出残缺库），属既有问题，未在本 PR 处理。
