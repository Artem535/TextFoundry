cmake_minimum_required(VERSION 3.20)
project(TextFoundry VERSION 0.2.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

include(GNUInstallDirs)
include(FetchContent)
include(CMakePackageConfigHelpers)

if(UNIX AND NOT APPLE)
  # Keep runtime lookup relative to the installed executable location so
  # linuxdeploy can resolve bundled shared libraries inside AppDir.
  set(CMAKE_INSTALL_RPATH
      "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR};\$ORIGIN/../lib")
endif()

set(TEXTFOUNDRY_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/TextFoundry")

# Use a fixed FTXUI release directly from upstream instead of vcpkg.
FetchContent_Declare(
  ftxui
  URL https://github.com/ArthurSonzogni/FTXUI/archive/refs/tags/v6.1.9.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ftxui)

add_subdirectory(src/textfoundry_engine)
set(OBJECTBOX_LICENSE_FILE "${objectbox_SOURCE_DIR}/LICENSE.txt")
add_subdirectory(src/textfoundry_ai)
add_subdirectory(src/text_foundry_cli)
option(TEXTFOUNDRY_BUILD_GUI "Build Qt/QML GUI frontend" ON)
option(TEXTFOUNDRY_VENDORED_KSYNTAXHIGHLIGHTING
  "Build KSyntaxHighlighting from source instead of using a system package"
  ON
)
if(TEXTFOUNDRY_BUILD_GUI)
  set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL "Release;RelWithDebInfo")
  find_package(Qt6 6.8 QUIET COMPONENTS Core Qml Quick QuickControls2)
  if(Qt6_FOUND)
    if(TEXTFOUNDRY_VENDORED_KSYNTAXHIGHLIGHTING AND NOT TARGET KF6::SyntaxHighlighting)
      set(TEXTFOUNDRY_SAVED_BUILD_TESTING "${BUILD_TESTING}")
      set(TEXTFOUNDRY_SAVED_KBUILD_TESTING "${KBUILD_TESTING}")
      set(TEXTFOUNDRY_SAVED_DISABLE_FIND_PACKAGE_PYTHON "${CMAKE_DISABLE_FIND_PACKAGE_Python}")
      set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
      set(KBUILD_TESTING OFF CACHE BOOL "" FORCE)
      set(BUILD_QCH OFF CACHE BOOL "" FORCE)
      set(CMAKE_DISABLE_FIND_PACKAGE_Python ON)
      FetchContent_Declare(
        syntax_highlighting
        GIT_REPOSITORY https://github.com/KDE/syntax-highlighting.git
        GIT_TAG v6.23.0
      )
      FetchContent_GetProperties(syntax_highlighting)
      if(NOT syntax_highlighting_POPULATED)
        FetchContent_Populate(syntax_highlighting)
        add_subdirectory(
          ${syntax_highlighting_SOURCE_DIR}
          ${syntax_highlighting_BINARY_DIR}
          EXCLUDE_FROM_ALL
        )
      endif()
      set(BUILD_TESTING "${TEXTFOUNDRY_SAVED_BUILD_TESTING}" CACHE BOOL "" FORCE)
      set(KBUILD_TESTING "${TEXTFOUNDRY_SAVED_KBUILD_TESTING}" CACHE BOOL "" FORCE)
      set(CMAKE_DISABLE_FIND_PACKAGE_Python "${TEXTFOUNDRY_SAVED_DISABLE_FIND_PACKAGE_PYTHON}")
    endif()
    if(NOT TARGET qt6keychain)
      find_package(Qt6Keychain CONFIG QUIET)
      if(TARGET qt6keychain)
        set_property(TARGET qt6keychain PROPERTY MAP_IMPORTED_CONFIG_MINSIZEREL Release)
      endif()
      if(NOT TARGET qt6keychain)
        set(BUILD_WITH_QT6 ON CACHE BOOL "" FORCE)
        set(BUILD_TEST_APPLICATION OFF CACHE BOOL "" FORCE)
        set(BUILD_TRANSLATIONS OFF CACHE BOOL "" FORCE)
        set(LIBSECRET_SUPPORT OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
          qtkeychain
          URL https://github.com/frankosterfeld/qtkeychain/archive/refs/tags/0.15.0.tar.gz
          DOWNLOAD_EXTRACT_TIMESTAMP FALSE
        )
        FetchContent_MakeAvailable(qtkeychain)
      endif()
    endif()
    add_subdirectory(src/text_foundry_gui)
  else()
    message(STATUS "Qt6 not found: skipping text_foundry_gui target")
  endif()
endif()
add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TextFoundryConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfig.cmake"
    INSTALL_DESTINATION "${TEXTFOUNDRY_INSTALL_CMAKEDIR}"
)

install(
    EXPORT TextFoundryTargets
    FILE TextFoundryTargets.cmake
    NAMESPACE TextFoundry::
    DESTINATION "${TEXTFOUNDRY_INSTALL_CMAKEDIR}"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfigVersion.cmake"
    DESTINATION "${TEXTFOUNDRY_INSTALL_CMAKEDIR}"
)

if(TARGET objectbox)
  install(FILES $<TARGET_FILE:objectbox>
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif()

if(EXISTS "${OBJECTBOX_LICENSE_FILE}")
  install(FILES "${OBJECTBOX_LICENSE_FILE}"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME}
    RENAME objectbox-LICENSE.txt
  )
endif()

set(CPACK_PACKAGE_NAME "textfoundry")
set(CPACK_PACKAGE_VENDOR "TextFoundry")
set(CPACK_PACKAGE_CONTACT "TextFoundry Maintainers")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Desktop workbench for authoring reusable prompt blocks, compositions, and renders")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "TextFoundry")
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RESOURCE_FILE_README
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/release_checklist.md")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

set(CPACK_GENERATOR "RPM")
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_PACKAGE_RELEASE 1)
set(CPACK_RPM_PACKAGE_LICENSE "Proprietary")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
set(CPACK_RPM_PACKAGE_AUTOREQ ON)
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /lib /lib64 /usr/lib /usr/lib64 /usr/bin /usr/sbin
    /usr/share /usr/share/applications /usr/share/icons /usr/share/metainfo
)

include(CPack)
