find_package(platform_folders CONFIG REQUIRED)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml Quick QuickControls2 QuickEffects Svg)

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
    "resource/icons/solid/folder.svg"
    "resource/icons/solid/folder-open.svg"
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
        qml/components/SvgToolButton.qml
        qml/components/TagChip.qml
        qml/components/CodePreview.qml
        qml/components/BlockSliceDialog.qml
        qml/components/BlockEditorWorkspace.qml
        qml/components/CompositionBlockRewriteDialog.qml
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
        components/svg_icon.cc
        components/svg_icon.h
        components/syntax_highlighter.cc
        components/syntax_highlighter.h
        app/session_view_model.cc
        app/session_view_model.h
        models/blocks_model.cc
        models/blocks_model.h
        viewmodels/block_editor_view_model.cc
        viewmodels/block_editor_view_model.h
        viewmodels/block_slice_view_model.cc
        viewmodels/block_slice_view_model.h
        viewmodels/composition_block_rewrite_view_model.cc
        viewmodels/composition_block_rewrite_view_model.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/components
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/viewmodels
)

target_link_libraries(textfoundry_gui PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickEffects
    Qt6::Svg
    textfoundry_core
    textfoundry_ai_openai
    qt6keychain
    sago::platform_folders
)

target_compile_definitions(textfoundry_gui PRIVATE
    TEXTFOUNDRY_APP_VERSION="${PROJECT_VERSION}"
)

if(TARGET KF6::SyntaxHighlighting)
    target_link_libraries(textfoundry_gui PRIVATE KF6::SyntaxHighlighting)
    target_compile_definitions(textfoundry_gui PRIVATE
        TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING=1
    )
    if(DEFINED syntax_highlighting_SOURCE_DIR)
        target_compile_definitions(textfoundry_gui PRIVATE
            TEXTFOUNDRY_KSYNTAX_DATA_DIR="${syntax_highlighting_SOURCE_DIR}/data"
        )
    endif()
elseif(TARGET KF6SyntaxHighlighting)
    target_link_libraries(textfoundry_gui PRIVATE KF6SyntaxHighlighting)
    target_compile_definitions(textfoundry_gui PRIVATE
        TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING=1
    )
    if(DEFINED syntax_highlighting_SOURCE_DIR)
        target_compile_definitions(textfoundry_gui PRIVATE
            TEXTFOUNDRY_KSYNTAX_DATA_DIR="${syntax_highlighting_SOURCE_DIR}/data"
        )
    endif()
endif()

set_target_properties(textfoundry_gui PROPERTIES
    MACOSX_BUNDLE TRUE
    OUTPUT_NAME tf-gui
)

if(WIN32)
    set_target_properties(textfoundry_gui PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

install(TARGETS textfoundry_gui
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(DEFINED syntax_highlighting_SOURCE_DIR)
    if(EXISTS "${syntax_highlighting_SOURCE_DIR}/data/syntax")
        install(
            DIRECTORY "${syntax_highlighting_SOURCE_DIR}/data/syntax"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/org.kde.syntax-highlighting"
        )
    endif()
    if(EXISTS "${syntax_highlighting_SOURCE_DIR}/data/themes")
        install(
            DIRECTORY "${syntax_highlighting_SOURCE_DIR}/data/themes"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/org.kde.syntax-highlighting"
        )
    endif()
endif()

if(UNIX AND NOT APPLE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/dist/textfoundry.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/textfoundry.desktop
        @ONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/dist/textfoundry.metainfo.xml.in
        ${CMAKE_CURRENT_BINARY_DIR}/textfoundry.metainfo.xml
        @ONLY
    )

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/textfoundry.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/textfoundry.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
    )

    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/app/textfoundry.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    )
endif()
