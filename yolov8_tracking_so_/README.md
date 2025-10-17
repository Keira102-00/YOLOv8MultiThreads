# YOLOv8 Tracking SO库

这是一个封装了KalmanFilter、Track、Hungarian算法和BoTSORT算法的共享库项目，用于YOLOv8目标检测后的跟踪和计数。

## 功能特性

- **Kalman滤波器**: 用于目标状态预测和更新
- **Track管理**: 单个目标轨迹的完整生命周期管理
- **Hungarian算法**: 目标匹配算法
- **BoTSORT跟踪**: 多目标跟踪算法
- **计数功能**: 支持区域进出计数和方向判断

## 构建方法

### 依赖要求
- OpenCV 4.x
- Eigen3
- CMake 3.10+

### 构建步骤

```bash
# 创建构建目录
mkdir build && cd build

# 配置CMake
cmake ..

# 构建
make -j$(nproc)

# 安装（可选）
sudo make install
```

### 交叉编译示例

```bash
# ARM64交叉编译
export CC=aarch64-linux-gnu-gcc
export CXX=aarch64-linux-gnu-g++
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
make -j$(nproc)
```

## 使用方法

### 包含头文件
```cpp
#include "tracking.h"
```

### 基本使用示例

```cpp
// 创建跟踪器
BoTSORTTracker tracker;

// 定义检测区域
cv::Rect region(200, 30, 800, 950);

// 处理检测结果
std::vector<Eigen::VectorXd> detections;
std::vector<int> cls_ids;

// 填充检测结果...

// 更新跟踪器
tracker.update(detections, cls_ids, region);

// 获取统计信息
int total_count = tracker.get_object_count();
const int* class_counts = tracker.get_class_counts();
```

### API接口

#### BoTSORTTracker类

- `update()`: 更新跟踪状态
- `get_tracks()`: 获取所有跟踪轨迹
- `get_object_count()`: 获取总目标计数
- `get_class_counts()`: 获取各类别计数
- `reset_counters()`: 重置计数器

#### Track类

- `get_state()`: 获取当前状态
- `getMovementDirection()`: 获取移动方向
- 提供完整的轨迹信息访问

## 文件结构

```
yolov8_tracking_so/
├── include/
│   └── tracking.h          # 公共头文件
├── src/
│   ├── kalman_filter.cpp   # Kalman滤波器实现
│   ├── track.cpp          # Track类实现
│   ├── hungarian.cpp      # Hungarian算法实现
│   └── botsort_tracker.cpp # BoTSORT跟踪器实现
├── CMakeLists.txt         # CMake构建配置
└── README.md             # 本说明文档
```

## 注意事项

1. 确保系统已安装OpenCV和Eigen3开发包
2. 使用时需要链接 `libyolov8_tracking.so`
3. 支持2个类别（bottle和can）的跟踪和计数
4. 区域计数功能需要正确设置检测区域矩形