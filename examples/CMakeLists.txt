add_executable(example_main
        example_main.cc
)

add_executable(advanced_example
        advanced_example.cc
)

target_link_libraries(example_main textfoundry_core)
target_link_libraries(advanced_example textfoundry_core)
