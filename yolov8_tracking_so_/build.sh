#!/bin/bash

# YOLOv8 Tracking SO库构建脚本

set -e

echo "开始构建YOLOv8 Tracking SO库..."

# 创建构建目录
mkdir -p build
cd build

# 配置CMake
echo "配置CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# 构建库
echo "构建SO库..."
make -j$(nproc)

echo "构建完成！"
echo "SO库位置: $(pwd)/lib/libyolov8_tracking.so"
echo "头文件位置: $(pwd)/../include/tracking.h"

# 可选：安装到系统目录
# sudo make install

# 构建示例程序（可选）
if [ -d "../examples" ]; then
    echo "构建示例程序..."
    cd ../examples
    mkdir -p build
    cd build
    cmake ..
    make -j$(nproc)
    echo "示例程序位置: $(pwd)/bin/yolov8_tracking_example"
fi

echo "全部构建完成！"