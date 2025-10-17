cmake_minimum_required(VERSION 3.10)
project(yolov8_tracking_so)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找依赖
find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)

# 源文件
set(SOURCES
    src/kalman_filter.cpp
    src/track.cpp
    src/hungarian.cpp
    src/botsort_tracker.cpp
)

# 创建共享库
add_library(yolov8_tracking SHARED ${SOURCES})

# 链接库
target_link_libraries(yolov8_tracking ${OpenCV_LIBS})

# 设置版本信息
set_target_properties(yolov8_tracking PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

# 安装规则
install(TARGETS yolov8_tracking
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)