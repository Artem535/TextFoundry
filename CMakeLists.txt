cmake_minimum_required(VERSION 3.20)
project(TextFoundry VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Use a fixed FTXUI release directly from upstream instead of vcpkg.
FetchContent_Declare(
  ftxui
  URL https://github.com/ArthurSonzogni/FTXUI/archive/refs/tags/v6.1.9.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
FetchContent_MakeAvailable(ftxui)

add_subdirectory(src/textfoundry_engine)
add_subdirectory(src/text_foundry_cli)
add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)
