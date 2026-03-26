include(FetchContent)


# ObjectBox
FetchContent_Declare(
        objectbox
        GIT_REPOSITORY https://github.com/objectbox/objectbox-c.git
        GIT_TAG v5.1.0
)

FetchContent_MakeAvailable(objectbox)

# Other lib
find_package(ctre CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(reflectcpp CONFIG REQUIRED)


add_library(textfoundry_core STATIC
        tf/engine.cc
        tf/engine.h
        tf/block_generation.h
        tf/block.cc
        tf/block.h
        tf/block_ref.cc
        tf/block_ref.h
        tf/fragment.cc
        tf/fragment.h
        tf/composition.cc
        tf/composition.h
        tf/renderer.cc
        tf/renderer.h
        tf/block_type.hpp
        tf/version.h
        tf/error.h
        tf/logger.h
        tf/logger.cc
        tf/objectbox_repository.cc
        tf/objectbox_repository.h
        tf/repository_block_cache.cpp
        tf/types.h
        tf/objectbox-model.h
        tf/database_scheme.obx.hpp
        tf/database_scheme.obx.cpp
)

target_include_directories(textfoundry_core
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(textfoundry_core
        PUBLIC
            ctre::ctre
            spdlog::spdlog
            objectbox
            reflectcpp::reflectcpp
)

set_target_properties(textfoundry_core PROPERTIES EXPORT_NAME Core)

install(
        TARGETS textfoundry_core
        EXPORT TextFoundryTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tf
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
)
