find_package(CLI11 CONFIG REQUIRED)
find_package(reflectcpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(platform_folders CONFIG REQUIRED)


add_executable(tf
        main.cc
        application.cc
        application.h
        applicationsettings.h
        block_creation_controller.cc
        block_creation_controller.h
        tui.cc
        tui.h
)

target_include_directories(tf BEFORE PRIVATE
        ${ftxui_SOURCE_DIR}/include
)

target_link_libraries(tf
        textfoundry_core
        CLI11::CLI11
        reflectcpp::reflectcpp
        spdlog::spdlog
        sago::platform_folders
        ftxui::dom ftxui::screen ftxui::component
)
