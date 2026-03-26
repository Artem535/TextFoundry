cmake_minimum_required(VERSION 3.20)
project(TextFoundry VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(FetchContent)
include(CMakePackageConfigHelpers)

set(TEXTFOUNDRY_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/TextFoundry")

# Use a fixed FTXUI release directly from upstream instead of vcpkg.
FetchContent_Declare(
  ftxui
  URL https://github.com/ArthurSonzogni/FTXUI/archive/refs/tags/v6.1.9.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ftxui)

add_subdirectory(src/textfoundry_engine)
add_subdirectory(src/textfoundry_ai)
add_subdirectory(src/text_foundry_cli)
option(TEXTFOUNDRY_BUILD_GUI "Build Qt/QML GUI frontend" ON)
if(TEXTFOUNDRY_BUILD_GUI)
  find_package(Qt6 6.8 QUIET COMPONENTS Core Qml Quick QuickControls2)
  if(Qt6_FOUND)
    add_subdirectory(src/text_foundry_gui)
  else()
    message(STATUS "Qt6 not found: skipping text_foundry_gui target")
  endif()
endif()
add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TextFoundryConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfig.cmake"
    INSTALL_DESTINATION "${TEXTFOUNDRY_INSTALL_CMAKEDIR}"
)

install(
    EXPORT TextFoundryTargets
    FILE TextFoundryTargets.cmake
    NAMESPACE TextFoundry::
    DESTINATION "${TEXTFOUNDRY_INSTALL_CMAKEDIR}"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfigVersion.cmake"
    DESTINATION "${TEXTFOUNDRY_INSTALL_CMAKEDIR}"
)
