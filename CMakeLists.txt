cmake_minimum_required(VERSION 3.20)

project(kepler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kepler
    src/main.cpp
    src/sgp4.cpp
)
