QT += core gui widgets

TEMPLATE = app
TARGET = QuickSoundSwitcher
CONFIG += c++20 lrelease embed_translations silent

QM_FILES_RESOURCE_PREFIX=/translations/tr

INCLUDEPATH += \
    AudioManager \
    SettingsPage \
    Panel \
    QuickSoundSwitcher \
    ShortcutManager \
    SoundOverlay \
    Utils

SOURCES += \
    AudioManager/AudioManager.cpp \
    SettingsPage/SettingsPage.cpp \
    Panel/Panel.cpp \
    QuickSoundSwitcher/QuickSoundSwitcher.cpp \
    ShortcutManager/ShortcutManager.cpp \
    SoundOverlay/SoundOverlay.cpp \
    Utils/Utils.cpp \
    main.cpp

HEADERS += \
    AudioManager/AudioManager.h \
    AudioManager/PolicyConfig.h \
    SettingsPage/SettingsPage.h \
    Panel/Panel.h \
    QuickSoundSwitcher/QuickSoundSwitcher.h \
    ShortcutManager/ShortcutManager.h \
    SoundOverlay/SoundOverlay.h \
    Utils/Utils.h

FORMS += \
    SettingsPage/SettingsPage.ui \
    Panel/Panel.ui \
    SoundOverlay/SoundOverlay.ui

RESOURCES += Resources/resources.qrc

LIBS += -lgdi32

RC_FILE = Resources/appicon.rc
