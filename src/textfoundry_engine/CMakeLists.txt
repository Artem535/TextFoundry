
find_package(ctre CONFIG REQUIRED)

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
)

target_link_libraries(textfoundry_core ctre::ctre)