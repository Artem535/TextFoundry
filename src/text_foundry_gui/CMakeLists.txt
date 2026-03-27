find_package(platform_folders CONFIG REQUIRED)
find_package(KF6SyntaxHighlighting QUIET)

qt_standard_project_setup(REQUIRES 6.8)

set_source_files_properties(qml/Constants/ColorPalette.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
set_source_files_properties(qml/Constants/General.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
set_source_files_properties(qml/Constants/Icons.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_executable(textfoundry_gui
    main.cc
)

if(KF6SyntaxHighlighting_FOUND AND NOT TARGET kquicksyntaxhighlightingplugin)
    find_library(KQUICKSYNTAXHIGHLIGHTINGPLUGIN_LIBRARY
        NAMES kquicksyntaxhighlightingplugin libkquicksyntaxhighlightingplugin
        HINTS
            /usr/lib64/qt6/qml/org/kde/syntaxhighlighting
            /usr/lib/qt6/qml/org/kde/syntaxhighlighting
    )

    if(KQUICKSYNTAXHIGHLIGHTINGPLUGIN_LIBRARY)
        add_library(kquicksyntaxhighlightingplugin SHARED IMPORTED)
        set_target_properties(kquicksyntaxhighlightingplugin PROPERTIES
            IMPORTED_LOCATION "${KQUICKSYNTAXHIGHLIGHTINGPLUGIN_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "KF6::SyntaxHighlighting"
        )
    endif()
endif()

set(TEXTFOUNDRY_GUI_RESOURCES
    "resource/app/textfoundry.svg"
    "resource/font/Font Awesome 7 Brands-Regular-400.otf"
    "resource/font/Font Awesome 7 Free-Regular-400.otf"
    "resource/font/Font Awesome 7 Free-Solid-900.otf"
    "resource/font/FiraCodeNerdFont-Regular.ttf"
    "resource/icons/solid/cubes.svg"
    "resource/icons/solid/layer-group.svg"
    "resource/icons/solid/eye.svg"
    "resource/icons/solid/gear.svg"
    "resource/icons/solid/arrow-left.svg"
    "resource/icons/solid/floppy-disk.svg"
    "resource/icons/solid/plus.svg"
    "resource/icons/solid/edit.svg"
    "resource/icons/solid/arrows-rotate.svg"
    "resource/icons/solid/ban.svg"
    "resource/icons/solid/copy.svg"
    "resource/icons/solid/eraser.svg"
    "resource/icons/solid/xmark.svg"
    "resource/icons/solid/font.svg"
    "resource/icons/solid/grip-lines.svg"
    "resource/icons/solid/check.svg"
    "resource/icons/solid/scissors.svg"
    "resource/icons/solid/paste.svg"
    "resource/icons/solid/trash.svg"
    "resource/icons/solid/broom.svg"
    "resource/icons/solid/wand-magic-sparkles.svg"
)

qt_add_qml_module(textfoundry_gui
    URI TextFoundry
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Constants/ColorPalette.qml
        qml/Constants/General.qml
        qml/Constants/Icons.qml
        qml/components/AppIcon.qml
        qml/components/DetailField.qml
        qml/components/IconToolButton.qml
        qml/components/SvgIcon.qml
        qml/components/SvgToolButton.qml
        qml/components/TagChip.qml
        qml/components/CodePreview.qml
        qml/components/DragHandle.qml
        qml/components/CompositionEditorWindow.qml
        qml/components/CompositionEditorSidebar.qml
        qml/components/CompositionFragmentCard.qml
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
        viewmodels/block_slice_view_model.cc
        viewmodels/block_slice_view_model.h
        viewmodels/composition_editor_view_model.cc
        viewmodels/composition_editor_view_model.h
        viewmodels/compositions_view_model.cc
        viewmodels/compositions_view_model.h
        viewmodels/render_view_model.cc
        viewmodels/render_view_model.h
    RESOURCES
        ${TEXTFOUNDRY_GUI_RESOURCES}
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
    textfoundry_ai_openai
    qt6keychain
    sago::platform_folders
)

if(KF6SyntaxHighlighting_FOUND)
    target_link_libraries(textfoundry_gui PRIVATE KF6::SyntaxHighlighting)
endif()

set_target_properties(textfoundry_gui PROPERTIES
    OUTPUT_NAME tf-gui
)

install(TARGETS textfoundry_gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(UNIX AND NOT APPLE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/dist/textfoundry.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/textfoundry.desktop
        @ONLY
    )

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/textfoundry.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )

    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/app/textfoundry.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    )
endif()
