cmake_minimum_required(VERSION 3.16)
project(HW5)

set(CMAKE_CXX_STANDARD 17)

file(GLOB my_files "*.h" "*.hpp" "*.cpp")

add_executable(HW5 ${my_files})
