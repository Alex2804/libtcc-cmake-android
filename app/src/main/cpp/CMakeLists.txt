cmake_minimum_required(VERSION 3.4.1)

find_library(log-lib log)
add_subdirectory(libs/libtcc)

add_library(native-lib SHARED native-lib.cpp)

target_link_libraries(native-lib ${log-lib} libtcc)