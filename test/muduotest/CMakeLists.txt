# 最低版本要求
cmake_minimum_required(VERSION 3.0)
# 工程名字
project(main)

# 指定搜索的子目录
add_subdirectory(muduotest)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g) # -g 编译出来的是可调试文件

# 配置头文件搜索路径 -I
# include_directories(/usr/include)

# 配置库文件搜索路径 -L
# link_directories(/usr/lib)

# 设置需要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)

# 设置可执行文件最终存储的文件路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 设置生成库最终存储的文件路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 把.指定路径下的所有源文件名字放入变量名SRC_LIST里面
# aux_source_directory(. SRC_LIST)

# 通过 muduo_server.cpp 生成可执行文件，由SRC_LIST变量所定义的源文件编译而来
add_executable(server ${SRC_LIST})

# 表示server这个目标程序需要链接 这三个库文件
target_link_libraries(server muduo_net muduo_base pthread)
