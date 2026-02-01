include(FetchContent)


# ObjectBox
FetchContent_Declare(
        objectbox
        GIT_REPOSITORY https://github.com/objectbox/objectbox-c.git
        GIT_TAG v5.1.0
)

FetchContent_MakeAvailable(objectbox)
find_package(ObjectBoxGenerator 5.0.0 REQUIRED)

# Other lib
find_package(ctre CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(reflectcpp CONFIG REQUIRED)


add_library(textfoundry_core STATIC
        tf/engine.cc
        tf/engine.h
        tf/block.cc
        tf/block.h
        tf/blockref.cc
        tf/blockref.h
        tf/fragment.cc
        tf/fragment.h
        tf/composition.cc
        tf/composition.h
        tf/renderer.cc
        tf/renderer.h
        tf/blocktype.hpp
        tf/version.h
        tf/error.h
        tf/logger.h
        tf/logger.cc
        tf/objectboxrepository.cc
        tf/objectboxrepository.h
        tf/repository_block_cache.cpp
        tf/types.h
)

add_obx_schema(
        TARGET textfoundry_core
        SCHEMA_FILES tf/database_scheme.fbs
        INSOURCE # Opt-in: Generate in source directory
        CXX_STANDARD 14 # Defaults to C++14 otherwise
)

target_include_directories(textfoundry_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(textfoundry_core ctre::ctre spdlog::spdlog objectbox reflectcpp::reflectcpp)

