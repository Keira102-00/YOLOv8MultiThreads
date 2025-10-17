# YOLOv8MultiThreads

> **多线程 YOLOv8 推理（含旧版本 + C++ 多线程版本 + 依赖库源码）**
>
> Multi-threaded YOLOv8 inference — includes previous single-threaded experiments, new C++ multi-threaded implementation, and dependency library sources.

---

## 目录 | Table of Contents

- [项目概览 | Project Overview](#项目概览--project-overview)
- [仓库结构 | Repository Structure](#仓库结构--repository-structure)
- [亮点与功能 | Highlights & Features](#亮点与功能--highlights--features)
- [技术栈 | Tech Stack](#技术栈--tech-stack)
- [设计与架构说明 | Design & Architecture](#设计与架构说明--design--architecture)
- [快速开始 | Quick Start](#快速开始--quick-start)
  - [编译（C++）| Build (C++)](#编译c--build-c)
  - [运行 | Run](#运行--run)
- [配置项 | Configuration](#配置项--configuration)
- [性能与调优建议 | Performance & Tuning Tips](#性能与调优建议--performance--tuning-tips)
- [演进历史 | Changelog](#演进历史--changelog)
- [在简历中的一句话描述 | One-line Resume Blurb](#在简历中的一句话描述--one-line-resume-blurb)
- [贡献 | Contributing](#贡献--contributing)
- [许可 | License](#许可--license)
- [联系方式 | Contact](#联系方式--contact)

---

## 项目概览 | Project Overview

本仓库包含：
- `cpp_old/`：项目早期（单线程或试验性实现），用于功能验证与基线性能对比。
- `cpp/`：重构后的 C++ 多线程主工程，采用线程池/生产者-消费者模型实现视频/摄像头输入的并发推理。
- `lib/`：为项目提供支持的依赖库源码（例如 rknn 封装、通用线程池、工具函数等）。

This repository bundles:
- `cpp_old/`: legacy single-threaded/experimental implementation for baseline comparison.
- `cpp/`: main C++ multi-threaded refactor with thread pool and producer/consumer pattern for video/camera inference.
- `lib/`: supporting dependency source code (rknn wrappers, thread pool, utils).

目标：演示板端（RK3588）或类似 arm64 平台上，如何通过多线程组织把 NPU 与 CPU 协同推理并提升吞吐与实时性能。\
Goal: show how to organize multi-threaded inference on edge devices (RK3588/arm64) to coordinate NPU + CPU and improve throughput & responsiveness.

---

## 仓库结构 | Repository Structure

```text
YOLOv8MultiThreads/
├── cpp_old/           # 旧版本（Python/C++混合或单线程实现）
├── cpp/               # 新版本：C++ 多线程实现（主展示目录）
│   ├── include/       # 头文件
│   ├── src/           # 源码
│   ├── tests/         # 单元/集成测试 (如果有)
│   ├── CMakeLists.txt
│   └── README.md      # cpp 子项目的局部说明
├── lib/               # 依赖库源码（可单独编译或作为子模块）
│   ├── rknn_wrapper/
│   ├── threadpool/
│   └── utils/
├── docs/              # 架构图、设计说明、性能日志等文档
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
- 代码注释与模块化清晰，便于面试时讲解实现细节。

- Thread-pool based multi-threading to reduce latency and improve throughput.
- Support for camera and video file inputs.
- Modular post-processing & visualization (bbox, keypoints, etc.).
- Bundled dependency sources to simplify offline builds.
- Clean modular code with comments — easy to explain in interviews.

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
3. **资源隔离**：NPU 与 CPU 的内存/上下文需要妥善同步（rknn session/thread-safety 注意事项）。
4. **Batching（可选）**：如果 NPU 支持批推理，可以把多帧打包成 batch，以提高吞吐。

Key design notes: ThreadPool to reuse workers, blocking queues for producer-consumer, careful resource isolation for NPU sessions, optional batching when supported.

---

## 快速开始 | Quick Start

> 以下步骤假设你在 `cpp/` 子目录进行编译与运行。

### 依赖 | Dependencies

- C++17 编译器
- CMake >= 3.10
- OpenCV (编译时链接)
- RKNN SDK / 交叉编译工具链（仅在板端运行时）

请将 `lib/` 下依赖以合适方式（子模块或本地路径）添加到 CMake。\
Make sure to add `lib/` dependencies into your CMake configuration.

### 编译（C++）| Build (C++)

```bash
# 在仓库根目录
cd cpp
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

> 在交叉编译或 RK3588 上编译时，请使用相应工具链文件：

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/arm-toolchain.cmake ..
```

### 运行 | Run

```bash
# 使用摄像头（默认设备 0）
./yolo_multithread_demo --source 0 --model path/to/model.rknn

# 使用视频文件
./yolo_multithread_demo --source /path/to/video.mp4 --model path/to/model.rknn
```

常见命令行参数（示例）：
- `--source`：输入源（摄像头索引或视频路径）
- `--model`：rknn/onnx 模型路径
- `--threads`：推理工作线程数
- `--batch`：（可选）每次推理的 batch 大小

---

## 配置项 | Configuration

推荐把可配置项放在 `config/` 或命令行参数中：
- `THREADS`: 推理线程数量（默认：4）
- `QUEUE_MAX_SIZE`: 队列最大缓存帧数
- `MODEL_INPUT_SIZE`: 模型的输入尺寸（HxW）
- `NMS_THRESHOLD`, `SCORE_THRESHOLD`

---

## 性能与调优建议 | Performance & Tuning Tips

- **线程数调优**：推理线程数与 NPU 并行能力有关；在板端实验不同线程数，观察 CPU 占用与延迟。
- **合理队列长度**：避免过长队列导致内存激增，建议设置上限并在溢出时丢弃最早/最晚帧。
- **启用 batch（若支持）**：批处理通常能提升吞吐，但会增加单帧延迟。
- **减少拷贝**：尽量使用零拷贝或最小拷贝的帧缓冲管理。
- **序列化/反序列化优化**：如果有网络传输或保存结果，使用高效二进制格式。

---

## 演进历史 | Changelog

请在 `CHANGELOG.md` 记录每次重要更新。例如：

- `2025-10-16`：将 old + 新的 cpp 版本和 lib 源码合并入同一 repo；重构 `cpp/` 为多线程实现；补充 docs 与运行脚本。

---

## 在简历中的一句话描述 | One-line Resume Blurb

`YOLOv8 Multi-threaded Inference on RK3588 — Designed and implemented a C++ thread-pool architecture to coordinate NPU (RKNN) and CPU, achieving X% higher throughput vs single-thread baseline.`

（将 `X%` 替换为你对比测试得到的具体数据）

---

## 贡献 | Contributing

欢迎 issue/PR。建议流程：
1. 新增 issue 描述 bug / feature。
2. fork 并在 feature 分支中实现。
3. 提交 PR 并在 PR 描述中包含运行与测试说明。

---

## 许可 | License

默认 MIT（如有特殊许可需求，请根据实际情况修改）。

---

## 联系方式 | Contact

作者：Keira（GitHub: @Keira102-00）

---

> 如果你希望我：
> 1. 把 `README.md` 同步成 `README_zh-en.md`（文件名），或者
> 2. 生成 `cpp/README.md`（聚焦子工程的更详细使用手册），
> 3. 或者**把 README 转为一个 ready-to-upload 文件**（我已把当前版本放在画布中），
>
> 告诉我你希望我继续做哪一项，我可以直接生成对应文件并把内容写入仓库格式的文档。

