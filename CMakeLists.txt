cmake_minimum_required(VERSION 3.16)
project(QuickSoundSwitcher VERSION 1.2.0 LANGUAGES C CXX)

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation directory" FORCE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(QT_QML_GENERATE_QMLLS_INI ON)
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    Widgets
    LinguistTools
    Multimedia
)

qt_standard_project_setup(REQUIRES 6.9)

# Include the translation progress script
include(cmake/translation_progress.cmake)

set(SOURCES
    src/audiomanager.cpp
    src/quicksoundswitcher.cpp
    src/shortcutmanager.cpp
    src/soundpanelbridge.cpp
    src/utils.cpp
    src/main.cpp
    src/mediasessionmanager.cpp
)

set(HEADERS
    include/audiomanager.h
    include/policyconfig.h
    include/quicksoundswitcher.h
    include/shortcutmanager.h
    include/soundpanelbridge.h
    include/utils.h
    include/mediasessionmanager.h
)

# Get git commit hash
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(GIT_COMMIT_HASH "unknown")
endif()

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

# Generate version header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

# Generate translation progress header
generate_translation_progress()

# Configure Windows metadata
set(APP_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.ico")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/windows_metadata.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/windows_metadata.rc"
    @ONLY
)

qt_add_executable(QuickSoundSwitcher WIN32 MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    resources/resources.qrc
    #resources/appicon.rc
    "${CMAKE_CURRENT_BINARY_DIR}/windows_metadata.rc"
)

set(QML_FILES
    qml/SoundPanel.qml
    qml/SettingsWindow.qml
    qml/KeepAlive.qml
    qml/DevicesListView.qml
    qml/MediaFlyoutContent.qml
    qml/Card.qml
    qml/SettingsPane/AppearancePane.qml
    qml/SettingsPane/GeneralPane.qml
    qml/SettingsPane/LanguagePane.qml
    qml/SettingsPane/DebugPane.qml
)

set(QML_SINGLETONS
    qml/Singletons/UserSettings.qml
    qml/Singletons/Constants.qml
    qml/Singletons/Context.qml
)

set_source_files_properties(${QML_SINGLETONS}
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(QuickSoundSwitcher
    URI Odizinne.QuickSoundSwitcher
    VERSION 1.0
    QML_FILES ${QML_FILES} ${QML_SINGLETONS}
)

# Avoid collision between executable name and QML module directory (QTBUG-98568).
if(NOT WIN32 AND NOT APPLE)
    set_property(TARGET QuickSoundSwitcher PROPERTY OUTPUT_NAME QuickSoundSwitcher_app)
endif()

target_include_directories(QuickSoundSwitcher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(QuickSoundSwitcher PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    Qt::Widgets
    Qt::Multimedia
    gdi32
    winmm
    windowsapp
)

target_compile_definitions(QuickSoundSwitcher PRIVATE
    UNICODE
    WINRT_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

if(WIN32 AND MSVC)
    target_compile_options(QuickSoundSwitcher PRIVATE "/await")
endif()

qt_add_translations(${CMAKE_PROJECT_NAME}
    TS_FILES
        i18n/QuickSoundSwitcher_en.ts
        i18n/QuickSoundSwitcher_fr.ts
        i18n/QuickSoundSwitcher_de.ts
        i18n/QuickSoundSwitcher_ko.ts
        i18n/QuickSoundSwitcher_it.ts
        i18n/QuickSoundSwitcher_zh_CN.ts
    RESOURCE_PREFIX "/i18n"
    LUPDATE_OPTIONS "-no-obsolete"
)

add_dependencies(${CMAKE_PROJECT_NAME} update_translations)

install(TARGETS QuickSoundSwitcher
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET QuickSoundSwitcher
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
    DEPLOY_TOOL_OPTIONS --no-compiler-runtime --no-opengl-sw --no-system-dxc-compiler --no-system-d3d-compiler --skip-plugin-types designer,iconengines,qmllint,generic,networkinformation,help,qmltooling,sqldrivers,tls,qmlls
)
install(SCRIPT ${deploy_script})
