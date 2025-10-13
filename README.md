# 🧩 YOLOv8 Multi-Threads Inference on RK3588
### Overview

This project implements a multi-threaded inference pipeline for the YOLOv8 model on Rockchip RK3588 devices.
The main goal is to improve frame processing efficiency by leveraging parallel video capture, inference, and postprocessing threads.

Unlike a typical YOLOv8 demo, this version has been engineered for scalability and modularity — the C++ codebase has been restructured for multi-threaded workloads and better integration with external tracking and inference libraries.

⚠️ Due to dependencies on proprietary SDKs and hardware-accelerated libraries, this repository cannot run standalone.
It serves primarily as an engineering reference for multi-thread design and RKNN-based deployment structure.

## 🧱 Project Structure
```bash
.
├── CMakeLists.txt
├── include/
│   ├── common_utils.h
│   ├── model_wrapper.h
│   ├── result_processor.h
│   ├── tracker_wrapper.h
│   ├── video_capture.h
│   ├── src/
│   │   ├── thread_pool.h
│   │   └── tracking.h
│   ├── lib/
│   │   ├── libthread_pool.so
│   │   ├── libyolov8_tracking.so -> libyolov8_tracking.so.1.0.0
│   │   └── libyolov8_tracking.so.1.0.0
│   └── ...
├── src/
│   ├── common_utils.cc
│   ├── model_wrapper.cc
│   ├── result_processor.cc
│   ├── tracker_wrapper.cc
│   ├── video_capture.cc
├── main.cc
├── postprocess.cc
├── yolov8.h
├── rknpu1/
│   └── yolov8.cc
└── rknpu2/
    ├── yolov8.cc
    ├── yolov8_rv1106_1103.cc
    └── yolov8_zero_copy.cc
```

## ✨ Key Features

### Multi-threaded Inference Framework

Independent threads for video capture, model inference, and result visualization.

Thread synchronization through a custom thread pool implementation.

### Modular Design

model_wrapper.cc abstracts RKNN model handling.

video_capture.cc supports decoupled input frame pipeline.

result_processor.cc handles postprocessing and visualization logic.

### Lightweight Tracking Integration

Supports real-time tracking integration via libyolov8_tracking.so.

### Optimized for Embedded AI

Specifically designed for RK3588 NPU + CPU hybrid architecture.

## 🧠 Engineering Highlights 

The entire C++ refactor in the src/ and include/ directories was independently implemented,
transforming the initial demo into a modular, multi-threaded inference framework.

### My contributions include:

Refactoring single-thread YOLOv8 demo → multi-threaded structure.

Designing reusable thread pool and model wrapper modules.

Implementing modularized input/output abstraction (video_capture, result_processor).

Engineering project build system via customized CMakeLists.

Integrating performance-oriented synchronization and error handling mechanisms.

## ⚙️ Dependencies

This project depends on several closed-source or device-specific SDKs and is not directly runnable without the following:

Rockchip RKNPU Toolkit (v1 or v2)

OpenCV (≥ 4.5)

RKNN runtime libraries

Pre-compiled .so libraries for tracking and threading

## 📘 Usage (Reference Only)
```bash
mkdir build && cd build
cmake ..
make -j8
./yolov8_app [model_path] [video_source/camera id/netcamera ippwd]
```

## 📄 License

This repository is released for research and educational reference only.
Please do not distribute the prebuilt libraries (.so files) without permission.

## 🌏 中文说明
项目简介

本项目为 YOLOv8 模型在 RK3588 平台上的多线程推理框架。
旨在通过 视频采集、推理、结果处理线程的并行化 提升实时处理效率。

与传统 YOLOv8 Demo 不同，本版本针对嵌入式 AI 环境进行了系统级工程化设计，
在保证结构清晰的同时，实现了多模块解耦、线程池复用与推理流程可扩展。

### 我的主要工作

重新设计并重写了 src/ 与 include/ 目录下的大部分核心逻辑；

实现了完整的 线程池架构 与任务分配机制；

将原始 Demo 工程化、模块化，使其可扩展至复杂实时任务；

编写 CMake 构建脚本，实现灵活模块编译；

在推理与结果处理之间增加同步优化，提升帧处理稳定性。

### 注意事项

本项目包含对 Rockchip 平台依赖的 .so 库文件及 SDK 接口，
并非可独立运行的 Demo，主要用于展示多线程推理架构设计思路。