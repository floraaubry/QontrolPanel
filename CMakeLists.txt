cmake_minimum_required(VERSION 3.30)
project(QontrolPanel VERSION 1.9.8 LANGUAGES C CXX)

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
    Network
)

qt_standard_project_setup(REQUIRES 6.9)

include(cmake/languages.cmake)

set(SOURCES
    src/audiomanager.cpp
    src/audiobridge.cpp
    src/QontrolPanel.cpp
    src/shortcutmanager.cpp
    src/soundpanelbridge.cpp
    src/main.cpp
    src/mediasessionmanager.cpp
    src/windowfocusmanager.cpp
    src/updater.cpp
    src/headsetcontrolmonitor.cpp
    src/headsetcontrolbridge.cpp
    src/monitormanager.cpp
    src/monitormanagerimpl.cpp
    src/logmanager.cpp
)

set(HEADERS
    include/audiomanager.h
    include/audiobridge.h
    include/policyconfig.h
    include/qontrolpanel.h
    include/shortcutmanager.h
    include/soundpanelbridge.h
    include/mediasessionmanager.h
    include/windowfocusmanager.h
    include/updater.h
    include/headsetcontrolmonitor.h
    include/headsetcontrolbridge.h
    include/monitormanager.h
    include/monitormanagerimpl.h
    include/logmanager.h
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

# Generate languages header
set(LANGUAGE_DATA "")
foreach(LANG_INFO ${SUPPORTED_LANGUAGES})
    string(REPLACE "|" ";" LANG_LIST "${LANG_INFO}")
    list(GET LANG_LIST 0 CODE)
    list(GET LANG_LIST 1 ENGLISH_NAME)
    list(GET LANG_LIST 2 NATIVE_NAME)

    string(APPEND LANGUAGE_DATA "        {\"${CODE}\", \"${ENGLISH_NAME}\", \"${NATIVE_NAME}\"},\n")
endforeach()

# Remove trailing comma and newline
string(REGEX REPLACE ",\n$" "\n" LANGUAGE_DATA "${LANGUAGE_DATA}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/languages.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/languages.h"
    @ONLY
)

# Configure Windows metadata
set(APP_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.ico")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/windows_metadata.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/windows_metadata.rc"
    @ONLY
)

qt_add_executable(QontrolPanel WIN32
    ${SOURCES}
    ${HEADERS}
    resources/icons/icons.qrc
    resources/sounds/sounds.qrc
    resources/videos/videos.qrc
    "${CMAKE_CURRENT_BINARY_DIR}/windows_metadata.rc"
)

set(QML_FILES
    qml/Main.qml
    qml/PowerMenu.qml
    qml/PowerConfirmationWindow.qml
    qml/SettingsWindow.qml
    qml/KeepAlive.qml
    qml/MediaFlyoutContent.qml
    qml/IntroWindow.qml
    qml/DonatePopup.qml
    qml/ConfirmPowerActionDialog.qml
    qml/SystemTray.qml
    qml/ChatMixNotification.qml
    qml/ExecutableRenameContextMenu.qml
    qml/ExecutableRenameDialog.qml
    qml/PanelSpacer.qml
    qml/PanelFooter.qml
    qml/Common/ApplicationsListView.qml
    qml/Common/NFToolButton.qml
    qml/Common/NFSlider.qml
    qml/Common/NFButton.qml
    qml/Common/DevicesListView.qml
    qml/Common/Card.qml
    qml/Common/LabeledSwitch.qml
    qml/Common/ProgressSlider.qml
    qml/Common/CustomComboBox.qml

    qml/Common/CustomScrollView.qml
    qml/SettingsPane/AppearancePane.qml
    qml/SettingsPane/ComponentsPane.qml
    qml/SettingsPane/GeneralPane.qml
    qml/SettingsPane/LanguagePane.qml
    qml/SettingsPane/DebugPane.qml
    qml/SettingsPane/CommAppsPane.qml
    qml/SettingsPane/ShortcutsPane.qml
    qml/SettingsPane/HeadsetControlPane.qml
    qml/SettingsPane/ConsolePane.qml
    qml/SettingsPane/DeviceRenamingPane.qml
    qml/Debug/DebugRectangle.qml
)

set(QML_SINGLETONS
    qml/Singletons/UserSettings.qml
    qml/Singletons/Constants.qml
    qml/Singletons/Context.qml
    qml/Singletons/LogBridge.qml
)

set_source_files_properties(${QML_SINGLETONS}
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(QontrolPanel
    URI Odizinne.QontrolPanel
    VERSION 1.0
    QML_FILES ${QML_FILES} ${QML_SINGLETONS}
    QML_FILES qml/LoggingConfDialog.qml
)

target_include_directories(QontrolPanel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(QontrolPanel PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    Qt::Widgets
    Qt::Multimedia
    Qt::Network
    gdi32
    winmm
    windowsapp
)

target_compile_definitions(QontrolPanel PRIVATE
    UNICODE
    WINRT_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

if(WIN32 AND MSVC)
    target_compile_options(QontrolPanel PRIVATE "/await")
endif()

get_ts_files(TS_FILES ${CMAKE_PROJECT_NAME})
get_qm_files(QM_FILES ${CMAKE_PROJECT_NAME})

qt_add_translations(${CMAKE_PROJECT_NAME}
    TS_FILES ${TS_FILES}
    LUPDATE_OPTIONS "-locations none"
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/i18n
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/i18n/compiled
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QM_FILES} ${CMAKE_BINARY_DIR}/i18n/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QM_FILES} ${PROJECT_SOURCE_DIR}/i18n/compiled/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/dependencies ${CMAKE_BINARY_DIR}/dependencies
    COMMENT "Copying translations and dependencies"
)

add_dependencies(${CMAKE_PROJECT_NAME} update_translations)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/dependencies/
    DESTINATION ${CMAKE_INSTALL_BINDIR}/dependencies
    COMPONENT Runtime
)

install(TARGETS QontrolPanel
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET QontrolPanel
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
    DEPLOY_TOOL_OPTIONS --no-compiler-runtime --no-opengl-sw --no-system-dxc-compiler --no-system-d3d-compiler --skip-plugin-types designer,iconengines,qmllint,generic,networkinformation,help,qmltooling,sqldrivers,qmlls
)
install(SCRIPT ${deploy_script})

install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR}/i18n)
