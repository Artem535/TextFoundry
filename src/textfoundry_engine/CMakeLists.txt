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
        engine.cc
        engine.h
        block.cc
        block.h
        blockref.cc
        blockref.h
        fragment.cc
        fragment.h
        composition.cc
        composition.h
        renderer.cc
        renderer.h
        blocktype.hpp
        version.h
        error.h
        logger.h
        logger.cc
        objectboxrepository.cc
        objectboxrepository.h
        repository_block_cache.cpp
        ../text_foundry_cli/main.cc
        types.h
)

add_obx_schema(
        TARGET textfoundry_core
        SCHEMA_FILES database_scheme.fbs
        INSOURCE # Opt-in: Generate in source directory
        CXX_STANDARD 14 # Defaults to C++14 otherwise
)

target_link_libraries(textfoundry_core ctre::ctre spdlog::spdlog objectbox reflectcpp::reflectcpp)

