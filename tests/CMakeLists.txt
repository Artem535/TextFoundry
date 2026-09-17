find_package(doctest CONFIG REQUIRED)

add_executable(ai_tests
        ai_test_main.cc
)

target_link_libraries(ai_tests textfoundry_core textfoundry_ai_openai doctest::doctest)

if(UNIX AND TARGET objectbox)
  set_property(TARGET ai_tests PROPERTY BUILD_WITH_INSTALL_RPATH OFF)
  set_property(TARGET ai_tests APPEND PROPERTY
    BUILD_RPATH "$<TARGET_FILE_DIR:objectbox>"
  )
endif()

add_test(NAME ai_tests COMMAND ai_tests)

if(WIN32 AND TARGET objectbox)
  set_tests_properties(ai_tests PROPERTIES
    ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:$<TARGET_FILE_DIR:objectbox>;PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin"
  )
endif()
