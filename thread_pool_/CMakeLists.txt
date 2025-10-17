cmake_minimum_required(VERSION 3.10)
project(thread_pool)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成共享库，加入yolo_pipeline.cc
add_library(thread_pool SHARED
    src/thread_pool.cc
)

# 包含头文件目录
target_include_directories(thread_pool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 查找OpenCV包
find_package(OpenCV REQUIRED)

# 包含OpenCV头文件目录
target_include_directories(thread_pool PUBLIC ${OpenCV_INCLUDE_DIRS})

# 链接OpenCV库
target_link_libraries(thread_pool ${OpenCV_LIBS})

# 设置输出路径
set_target_properties(thread_pool PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)