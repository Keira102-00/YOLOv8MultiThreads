# YOLOv8 Multi-Threaded Inference on RK3588

### 🚀 Overview
This project implements a **multi-threaded YOLOv8 inference pipeline** on the **Rockchip RK3588 platform**, focusing on optimizing **CPU+NPU hybrid execution efficiency**.  
It is a **C++ engineering project** that rebuilds the YOLOv8 inference structure for better modularity, scalability, and real-time performance.

> ⚙️ Note: This repository demonstrates **multi-threaded system design** and **inference pipeline engineering**, not a plug-and-play demo.  
> It requires Rockchip-specific dependencies, prebuilt libraries, and model files to run properly.

---

## 🧩 Project Structure
```bash
.
├── CMakeLists.txt
├── CMakeLists.txt.user
├── include
│   ├── common_utils.h
│   ├── lib
│   │   ├── libthread_pool.so
│   │   ├── libyolov8_tracking.so -> libyolov8_tracking.so.1
│   │   ├── libyolov8_tracking.so.1 -> libyolov8_tracking.so.1.0.0
│   │   └── libyolov8_tracking.so.1.0.0
│   ├── model_wrapper.h
│   ├── result_processor.h
│   ├── src
│   │   ├── thread_pool.h
│   │   └── tracking.h
│   ├── tracker_wrapper.h
│   └── video_capture.h
├── main.cc
├── README.md
└── src
    ├── common_utils.cc
    ├── model_wrapper.cc
    ├── result_processor.cc
    ├── tracker_wrapper.cc
    └── video_capture.cc
```

## 🧠 Key Features

- **Thread Pool Design:**  
  Custom C++ thread pool (`thread_pool.h`) to manage multi-stream inference efficiently.  
  Tasks include model inference, postprocessing, and video I/O handling.

- **Modularized Architecture:**  
  Split into logical modules:  
  `model_wrapper`, `tracker_wrapper`, `result_processor`, and `video_capture`.

- **RK3588 NPU + CPU Hybrid Inference:**  
  Supports **RKNPU1** and **RKNPU2** modes (under `/rknpu1` and `/rknpu2/`)  
  for flexible deployment across Rockchip platforms.

- **Optimized Engineering Structure:**  
  Rewritten most of the original C++ directory to improve readability, maintainability, and reusability.

- **Precompiled Shared Libraries:**  
  `libthread_pool.so` and `libyolov8_tracking.so` are provided for linking.

---

## 🧍‍♂️ My Contribution

> This project was originally a single-threaded inference demo.  
> I **refactored and rebuilt most of the C++ directory**, introducing:
>
> - A complete **multi-threaded inference pipeline**
> - A **CMake-based modular project structure**
> - **Reusable utility layers** for data processing and result management
> - Integration of **custom thread pool** and **RKNN API calls**
>
> Except for `postprocess.cc/.h` and the existing NPU wrappers,  
> **almost all files in `src/` and `include/` were written and engineered by me.**

---

## 📸 Example Output 
TBD

- A **console output screenshot** showing thread scheduling logs  
- A **video frame or detection result** image



---

## ⚙️ Build Instructions 

> This section is for developers familiar with RK3588 SDK and RKNN toolkit.

```bash
# Clone repository
git clone git@github.com:Keira102-00/YOLOv8MultiThreads.git
cd /path/to/yolov8/
mv YOLOv8MultiThreads/ /path/to/yolov8/cpp/
#recompile program and rerun
```
## 🧱 Dependencies

| Dependency | Version / Description |
|-------------|-----------------------|
| **RKNN Toolkit**  | Rockchip model conversion tools |
| **OpenCV**        | Image processing and video I/O |
| **pthread**       | POSIX multithreading support |
| **rknpu_runtime** | Rockchip NPU runtime libraries |
| **C++17+**        | Required for thread pool and smart pointer usage |

---

## 🌍 中文介绍

### 🧭 项目概述
本项目实现了基于 **RK3588 平台的 YOLOv8 多线程推理框架**，重点在于利用 **CPU + NPU 混合架构** 提升推理性能与系统并行度。  
整个工程使用 **C++ 重构**，模块化、可扩展，并在工程化结构上做了全面优化。

---

### 🧠 我的主要贡献
- 从零构建 **多线程推理框架**（线程池、任务调度、模型封装等）  
- 完善工程结构并引入 **CMake 模块化构建体系**  
- 实现了 `model_wrapper`、`tracker_wrapper`、`result_processor` 等模块  
- 重写了项目主要的 C++ 文件（除原有的后处理部分）

---

### ⚙️ 项目特点
- 支持 **RKNPU1 / RKNPU2** 双版本推理  
- 支持 **自定义线程池** 实现并行推理  
- 模块化结构清晰，可复用性高  
- 适合展示嵌入式 AI 推理与系统工程化能力

---

**📁 Repository:** [YOLOv8MultiThreads](https://github.com/Keira102-00/YOLOv8MultiThreads)  
**🧑‍💻 Author:** Keira102  
**📅 Last Updated:** October 2025

