cmake_minimum_required(VERSION 3.20)
project(TextFoundry VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src/text_foundry_engine)

enable_testing()
add_subdirectory(tests)
