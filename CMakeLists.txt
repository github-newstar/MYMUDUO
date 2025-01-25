cmake_minimum_required(VERSION 3.22)
project(mymuduo)
set(CMAKE_CXX_STANDARD 11)
#mymuduo库最终会编译成so动态库，这里指导动态库的路径，放在根目录的lib文件夹下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
#设置调试信息 启动c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fPIC")
#定义参与编译的文件
aux_source_directory(./src SRC_LIST)
#编译生成动态库mymuduo
add_library(mymuduo SHARED ${SRC_LIST})


