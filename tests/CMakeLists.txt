find_package(doctest CONFIG REQUIRED)

add_executable(core_tests
        test_main.cc
        ../examples/example_main.cc
)

target_link_libraries(core_tests textfoundry_core doctest::doctest)