cmake_minimum_required(VERSION 3.20)
project(HashTable CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(HashTable src/main.cpp)
target_include_directories(HashTable PRIVATE include)
