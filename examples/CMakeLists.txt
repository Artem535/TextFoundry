add_executable(example_main
        example_main.cc
)

add_executable(advanced_example
        advanced_example.cc
)

add_executable(ftxui_tabs_example
        ftxui_tabs_example.cc
)

target_link_libraries(example_main textfoundry_core)
target_link_libraries(advanced_example textfoundry_core)
target_link_libraries(ftxui_tabs_example ftxui::dom ftxui::screen ftxui::component)
