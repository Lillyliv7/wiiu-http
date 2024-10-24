
cmake_minimum_required(VERSION 3.2)
set(CMAKE_TOOLCHAIN_FILE $ENV{DEVKITPRO}/cmake/WiiU.cmake)
project(helloworld_cpp CXX)

add_executable(helloworld_cpp
   main.cpp
   local_ip.cpp
   split_string.cpp
   os_screen_help.cpp
   load_file.cpp
   timestamp.cpp)

wut_create_rpx(helloworld_cpp)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/helloworld_cpp.rpx"
        DESTINATION "${CMAKE_INSTALL_PREFIX}")