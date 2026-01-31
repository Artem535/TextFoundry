cmake_minimum_required(VERSION 3.20)
project(TextFoundry VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src/textfoundry_engine)
add_subdirectory(src/text_foundry_cli)

enable_testing()
add_subdirectory(tests)
