# YOLOv8MultiThreads

> **多线程 YOLOv8 推理（最新 C++ 多线程版本 + 依赖库源码）**
>
> Multi-threaded YOLOv8 inference — includes the newest C++ multi-threaded implementation, and dependency library sources.

---

## 成果展示

https://github.com/user-attachments/assets/dd7e892e-ba70-4f49-b76a-b02a1ce0cd44

https://github.com/user-attachments/assets/14fff4bd-6dc4-4e1f-b032-339624e5f33f

<img width="591" height="134" alt="npu_load" src="https://github.com/user-attachments/assets/e91775f0-37dc-4bc8-95c7-72cab4426dbf" />

<img width="721" height="320" alt="fps_running" src="https://github.com/user-attachments/assets/10f734d4-0f81-4b0b-bc01-cd4735bf2ef9" />

---

## Table of Contents

- [项目概览 | Project Overview](#项目概览--project-overview)
- [仓库结构 | Repository Structure](#仓库结构--repository-structure)
- [亮点与功能 | Highlights & Features](#亮点与功能--highlights--features)
- [技术栈 | Tech Stack](#技术栈--tech-stack)
- [设计与架构说明 | Design & Architecture](#设计与架构说明--design--architecture)
  - [高层流程（ASCII 图）](#高层流程ascii-图)
  - [关键设计点](#关键设计点)
- [代码结构与集成说明 | Code Integration & Build Demo Context](#代码结构与集成说明--code-integration--build-demo-context)
  - [当前仓库定位 | Repository Purpose](#当前仓库定位--repository-purpose)
- [备注 | Notes](#备注--notes)
- [性能与调优建议 | Performance & Tuning Tips](#性能与调优建议--performance--tuning-tips)
- [演进历史 | Changelog](#演进历史--changelog)
- [贡献 | Contributing](#贡献--contributing)
- [许可 | License](#许可--license)
- [联系方式 | Contact](#联系方式--contact)

---

## 项目概览 | Project Overview

本仓库包含：
- `cpp_old_date/`：曾经更新的版本，但不是最新的。
- `cpp_newest_date/`：最新版本的代码。
- `thread_pool_/` & `yolov8_tracking_so_`：为项目提供支持的依赖库源码。

This repository bundles:
- `cpp_old/`: legacy single-threaded/experimental implementation for baseline comparison.
- `cpp/`: main C++ multi-threaded refactor with thread pool and producer/consumer pattern for video/camera inference.
- `thread_pool_/` & `yolov8_tracking_so_`: supporting dependency source code.

目标：演示板端（RK3588）或类似 arm64 平台上，如何通过多线程组织把 NPU 与 CPU 协同推理并提升吞吐与实时性能。\
Goal: show how to organize multi-threaded inference on edge devices (RK3588/arm64) to coordinate NPU + CPU and improve throughput & responsiveness.

---

## 仓库结构 | Repository Structure

```text
YOLOv8MultiThreads/
├── cpp_xxx/           # 旧版本（Python/C++混合或单线程实现）
├── cpp_newest/               # 新版本：C++ 多线程实现（主展示目录）
│   ├── include/       # 头文件
│   ├── src/           # 源码
│   ├── tests/         # 单元/集成测试 (如果有)
│   ├── CMakeLists.txt
│   └── README.md      # cpp 子项目的局部说明
├── thread_pool_/              # 依赖库源码（可单独编译或作为子模块）
├── threadpoyolov8_tracking_so_ol/  
├── CHANGELOG.md
└── README.md
```

注：如果你在不同平台（如 x86 与 arm）上编译，请在 docs 中写明交叉编译或依赖版本。\
Note: If you support multiple platforms (x86/arm), document cross-compile steps and dependency versions in `docs/`.

---

## 亮点与功能 | Highlights & Features

- 多线程/线程池架构，降低推理延迟并提高吞吐。
- 支持摄像头和视频文件输入。
- 可扩展的推理后处理和可视化 pipeline（支持绘制边界框、关键点等）。
- 包含依赖库源码，便于在无网络环境下复现编译。
- 代码注释与模块化清晰。

- Thread-pool based multi-threading to reduce latency and improve throughput.
- Support for camera and video file inputs.
- Modular post-processing & visualization (bbox, keypoints, etc.).
- Bundled dependency sources to simplify offline builds.
- Clean modular code with comments.

---

## 技术栈 | Tech Stack

- 语言：C++17
- 构建：CMake
- 图像处理：OpenCV
- 推理：RKNN（rknn），也可抽象为任意 NPU 后端
- 运行平台：RK3588/arm64 优先，x86 也支持（取决于 rknn/模拟方法）

---

## 设计与架构说明 | Design & Architecture

### 高层流程（ASCII 图）

```
[Video Capture] --> [Frame Queue] --> [ThreadPool: Inference Workers] --> [Post-processing Queue] --> [Visualizer/Output]
                  ^ producer                                          ^ consumer
```

- Producer（主线程）负责读取摄像头/视频帧并推入 `Frame Queue`。
- 多个 Inference Worker 从 `Frame Queue` 弹出帧，调用 rknn（或其他后端）进行推理，并将结果吐到 `Post-processing Queue`。
- Post-processing 线程负责 NMS、关键点后处理、结果合并与绘制。

Producer (main thread) reads frames -> pushes to Frame Queue. Multiple Inference Workers pop frames, run inference (rknn), push results to Post-processing Queue. Post-processing thread handles NMS, keypoint processing and visualization.

### 关键设计点

1. **线程池（ThreadPool）**：避免频繁创建/销毁线程，使用固定大小的 worker 池处理推理任务。
2. **队列（Blocking Queue）**：生产者-消费者模式，队列应支持阻塞和超时，防止内存无限增长。
3. **资源隔离**：NPU 与 CPU 的内存/上下文需要妥善同步（rknn session/thread-safety 注意事项）

Key design notes: ThreadPool to reuse workers, blocking queues for producer-consumer, careful resource isolation for NPU sessions.

---

## 代码结构与集成说明 | Code Integration & Build Demo Context

> ⚠️ 注意：本项目仅用于展示多线程架构与核心实现逻辑，并非完整可直接运行的 YOLOv8 推理 Demo。  
> 实际推理运行依赖于 Rockchip 官方 SDK（如 `rknn_model_zoo`）的编译与运行环境。

### 📂 当前仓库定位 | Repository Purpose

本仓库的 `cpp_version_name/` 目录展示了 YOLOv8 推理在 C++ 层的多线程实现方式，包括：
- Frame Producer / Consumer 队列设计；
- ThreadPool 任务调度逻辑；
- rknn 推理接口调用示意；
- 多核并行利用的同步与结果回传机制。

This repository showcases **the C++ multi-threaded implementation** of YOLOv8 inference, focusing on:
- Thread pool and task dispatching design,
- Frame queue management and synchronization,
- Integration points for rknn inference calls,
- Multi-core NPU coordination mechanisms.

---

## 🧩 备注 | Notes

- 本项目主要展示架构与思路，实际模型文件、SDK 接口和运行脚本为公司内部内容。

- 若无 SDK 环境，可单独阅读 cpp/ 中的多线程实现文件以理解架构设计。

This repo focuses on architecture demonstration, not full inference execution.
If SDK is unavailable, reviewing cpp/ code alone is sufficient to understand the threading and pipeline design.

---

## 可优化点 | Performance & Tuning Tips

- **线程数调优**：推理线程数与 NPU 并行能力有关；在板端实验不同线程数，观察 CPU 占用与延迟。
- **合理队列长度**：避免过长队列导致内存激增，建议设置上限并在溢出时丢弃最早/最晚帧。
- **启用 batch（若支持）**：批处理通常能提升吞吐，但会增加单帧延迟。
- **减少拷贝**：尽量使用零拷贝或最小拷贝的帧缓冲管理。
- **序列化/反序列化优化**：如果有网络传输或保存结果，使用高效二进制格式。

---

## 演进历史 | Changelog

see `CHANGELOG.md` for more

- `2025-10-17`：优化fps上限，新版本`cpp_0920Ver_highfps_load_20251017`

---

## 贡献 | Contributing

欢迎 issue/PR。建议流程：
1. 新增 issue 描述 bug / feature。
2. fork 并在 feature 分支中实现。
3. 提交 PR 并在 PR 描述中包含运行与测试说明。

---

## 许可 | License

默认 MIT

---

## 联系方式 | Contact

作者：Keira（GitHub: @Keira102-00）

---


