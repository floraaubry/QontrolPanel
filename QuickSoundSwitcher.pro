QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20 silent lrelease embed_translations

QM_FILES_RESOURCE_PREFIX = /translations

INCLUDEPATH +=                                      \
    QuickSoundSwitcher/                             \
    Utils/                                          \
    Panel/                                          \
    AudioManager/                                   \
    ShortcutManager/                                \
    OverlaySettings/                                \
    OverlayWidget/                                  \
    SoundOverlay/                                   \
    MediaFlyout/                                    \
    MediaSessionWorker/                             \

SOURCES +=                                          \
    MediaFlyout/MediaFlyout.cpp                     \
    MediaSessionWorker/MediaSessionWorker.cpp       \
    SoundOverlay/SoundOverlay.cpp                   \
    main.cpp                                        \
    Panel/Panel.cpp                                 \
    QuickSoundSwitcher/QuickSoundSwitcher.cpp       \
    Utils/Utils.cpp                                 \
    AudioManager/AudioManager.cpp                   \
    ShortcutManager/ShortcutManager.cpp             \
    OverlaySettings/OverlaySettings.cpp             \
    OverlayWidget/OverlayWidget.cpp                 \

HEADERS +=                                          \
    MediaFlyout/MediaFlyout.h                       \
    MediaSessionWorker/MediaSession.h               \
    MediaSessionWorker/MediaSessionWorker.h         \
    Panel/Panel.h                                   \
    QuickSoundSwitcher/QuickSoundSwitcher.h         \
    SoundOverlay/SoundOverlay.h                     \
    Utils/Utils.h                                   \
    AudioManager/AudioManager.h                     \
    AudioManager/PolicyConfig.h                     \
    ShortcutManager/ShortcutManager.h               \
    OverlaySettings/OverlaySettings.h               \
    OverlayWidget/OverlayWidget.h                   \

FORMS +=                                            \
    MediaFlyout/MediaFlyout.ui                      \
    Panel/Panel.ui                                  \
    OverlaySettings/OverlaySettings.ui              \
    SoundOverlay/SoundOverlay.ui                    \

TRANSLATIONS +=                                     \
    Resources/Tr/QuickSoundSwitcher_fr.ts           \
    Resources/Tr/QuickSoundSwitcher_en.ts           \

RESOURCES +=                                        \
    Resources/resources.qrc                         \

RC_FILE = Resources/appicon.rc

LIBS += -luser32 -ladvapi32 -lwinmm -lole32 -lgdi32 -lwindowsapp
