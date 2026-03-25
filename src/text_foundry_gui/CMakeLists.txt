find_package(platform_folders CONFIG REQUIRED)
find_package(KF6SyntaxHighlighting QUIET)

qt_standard_project_setup(REQUIRES 6.8)

set_source_files_properties(qml/Constants/ColorPalette.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
set_source_files_properties(qml/Constants/General.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_executable(textfoundry_gui
    main.cc
)

qt_add_qml_module(textfoundry_gui
    URI TextFoundry
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Constants/ColorPalette.qml
        qml/Constants/General.qml
        qml/components/DetailField.qml
        qml/components/TagChip.qml
        qml/components/CodePreview.qml
        qml/pages/BlocksPage.qml
        qml/pages/CompositionsPage.qml
        qml/pages/RenderPage.qml
        qml/pages/SettingsPage.qml
        qml/components/InfoCard.qml
    SOURCES
        app/session_view_model.cc
        app/session_view_model.h
        models/blocks_model.cc
        models/blocks_model.h
        viewmodels/block_editor_view_model.cc
        viewmodels/block_editor_view_model.h
        viewmodels/compositions_view_model.cc
        viewmodels/compositions_view_model.h
        viewmodels/render_view_model.cc
        viewmodels/render_view_model.h
)

target_include_directories(textfoundry_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/viewmodels
)

target_link_libraries(textfoundry_gui PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    textfoundry_core
    sago::platform_folders
)

if(KF6SyntaxHighlighting_FOUND)
    target_link_libraries(textfoundry_gui PRIVATE KF6::SyntaxHighlighting)
endif()

set_target_properties(textfoundry_gui PROPERTIES
    OUTPUT_NAME tf-gui
)

install(TARGETS textfoundry_gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
