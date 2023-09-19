#最小版本，可以通过cmake --version查看
cmake_minimum_required(VERSION 3.6.2)


#项目名称，建议大写
project(WEBSERVER)



#搜索自定义的头文件目录,可以使用绝对路径${CMAKE_SOURCE_DIR}/include
include_directories(include)

#设置编译时的选项，如O2优化，开启警告，使用c++标准等
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -O0 -pthread")

#调试时使用该项
set(CMAKE_BUILD_TYPE Debug)

find_library(MYSQL_LIB libmysqlclient.so /home/mrlee/web_server/lib)
link_directories(
    ${PROJECT_SOURCE_DIR}/lib
)

#生成可执行文件命令,第一个参数是生成可执行文件的名称,后面的参数是源文件代码
add_executable(w_server 
main.cpp 
src/timer.cpp
src/log.cpp
src/wthread.cpp
src/net_init.cpp
src/http_session.cpp
src/http_net.cpp
src/http_management.cpp
src/http_handler.cpp

)



#指定生成可执行文件的输出目录
set(EXECUTABLE_OUTPUT_PATH "./out")
#光包含库目录是没用的，还必须使用TARGET_LINK_LIBRARIES链接该库
TARGET_LINK_LIBRARIES(w_server mysqlclient)
