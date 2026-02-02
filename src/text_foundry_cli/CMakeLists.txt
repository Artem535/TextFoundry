find_package(CLI11 CONFIG REQUIRED)
find_package(reflectcpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(tf
        main.cc
        application.cc
        application.h
        applicationsettings.h
)

target_link_libraries(tf textfoundry_core CLI11::CLI11 reflectcpp::reflectcpp spdlog::spdlog)
