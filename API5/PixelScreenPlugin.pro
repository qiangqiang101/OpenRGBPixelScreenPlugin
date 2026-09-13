#-----------------------------------------------------------------------------------------------#
# OpenRGB Matrix Text Plugin QMake Project                                                      #
#-----------------------------------------------------------------------------------------------#

QT +=                                                                                           \
    core                                                                                        \
    gui                                                                                         \
    widgets

DEFINES += ORGBMATRIXTEXTPLUGIN_LIBRARY
TEMPLATE = lib

CONFIG +=                                                                                       \
    plugin                                                                                      \
    silent

MAJOR       = 1
MINOR       = 0
SUFFIX      = git

COMMITS     = $$system("git rev-list --count HEAD")

VERSION_NUM = $$MAJOR"."$$MINOR"."$$COMMITS
VERSION_STR = $$MAJOR"."$$MINOR

equals(SUFFIX, "git") {
    VERSION_STR = $$VERSION_STR"+ ("$$SUFFIX$$COMMITS")"
}

# Automatically generated build info
win32:BUILDDATE = $$system(date /t)
unix:BUILDDATE  = $$system(date -R)
GIT_COMMIT_ID   = $$system(git rev-parse HEAD)

# OpenRGB Plugin API version
API_VERSION = 5
message("Building for Plugin API Version: "$$API_VERSION)

PROJECT_DESC                = "Render custom scrolling text, clock, sensors, and pixel art on OpenRGB matrices"
PROJECT_NAME                = "Pixel Screen Plugin"
PROJECT_URL                 = "https://gitlab.com/OpenRGBDevelopers/OpenRGBPixelScreenPlugin"

DEFINES +=                                                                                      \
    VERSION_STRING=\\"\"\"$$VERSION_STR\\"\"\"                                                  \
    BUILDDATE_STRING=\\"\"\"$$BUILDDATE\\"\"\"                                                  \
    GIT_COMMIT_ID=\\"\"\"$$GIT_COMMIT_ID\\"\"\"                                                 \
    PROJECT_DESC=\\"\"\"$$PROJECT_DESC\\"\"\"                                                   \
    PROJECT_NAME=\\"\"\"$$PROJECT_NAME\\"\"\"                                                   \
    PROJECT_URL=\\"\"\"$$PROJECT_URL\\"\"\"                                                     \

# Update version in metadata json
JSON_FILE_IN            = $$PWD/PixelScreenPlugin.json.in
JSON_FILE_OUT           = $$PWD/PixelScreenPlugin.json

prebuild_json.target    = prebuild_json_target
prebuild_json.depends   = FORCE
prebuild_json.commands  = $$QMAKE_STREAM_EDITOR -e \"s|VERSION_NUM|$$VERSION_NUM|g\"            \
                                                -e \"s|VERSION_STR|$$VERSION_STR|g\"            \
                                                -e \"s|GIT_COMMIT_ID|$$GIT_COMMIT_ID|g\"        \
                                                -e \"s|PROJECT_DESC|$$PROJECT_DESC|g\"          \
                                                -e \"s|PROJECT_NAME|$$PROJECT_NAME|g\"          \
                                                -e \"s|PROJECT_URL|$$PROJECT_URL|g\"            \
                                                -e \"s|API_VERSION|$$API_VERSION|g\"            \
                                                $$JSON_FILE_IN > $$JSON_FILE_OUT                \

QMAKE_EXTRA_TARGETS    += prebuild_json
PRE_TARGETDEPS         += prebuild_json_target

# OpenRGB Plugin SDK
INCLUDEPATH +=                                                                                  \
    OpenRGB/                                                                                    \
    OpenRGB/dependencies/json                                                                   \
    OpenRGB/RGBController                                                                       \
    OpenRGB/qt                                                                                  \
    OpenRGB/i2c_smbus                                                                           \

SOURCES +=                                                                                      \
    OpenRGB/qt/hsv.cpp

# Header files
HEADERS +=                                                                                      \
    ../common/DeviceUpdateVTableHook.h                                                          \
    PixelScreenPlugin.h                                                                         \
    PixelScreenTab.h                                                                            \
    DeviceSettingsPage.h                                                                        \
    HardwareSensorManager.h                                                                     \
    HorizontalTabBar.h                                                                          \
    CustomTabWidget.h                                                                           \

# Source files
SOURCES +=                                                                                      \
    PixelScreenPlugin.cpp                                                                       \
    PixelScreenTab.cpp                                                                          \
    DeviceSettingsPage.cpp                                                                      \

# Forms
FORMS +=                                                                                        \
    PixelScreenTab.ui                                                                           \

# Platform configurations
CONFIG += c++17

CONFIG(debug, debug|release) {
    DESTDIR = debug
}
CONFIG(release, debug|release) {
    DESTDIR = release
}

win32:CONFIG += QTPLUGIN
win32:OBJECTS_DIR = _intermediate_$$DESTDIR/.obj
win32:MOC_DIR     = _intermediate_$$DESTDIR/.moc
win32:RCC_DIR     = _intermediate_$$DESTDIR/.qrc
win32:UI_DIR      = _intermediate_$$DESTDIR/.ui

win32:contains(QMAKE_TARGET.arch, x86_64) {
    LIBS += -lws2_32 -lole32
}
win32:contains(QMAKE_TARGET.arch, x86) {
    LIBS += -lws2_32 -lole32
}

win32:DEFINES +=                                                                                \
    _MBCS                                                                                       \
    WIN32                                                                                       \
    _CRT_SECURE_NO_WARNINGS                                                                     \
    _WINSOCK_DEPRECATED_NO_WARNINGS                                                             \
    WIN32_LEAN_AND_MEAN                                                                         \

unix:!macx {
    QMAKE_CXXFLAGS += -std=c++17 -Wno-psabi
    LIBS += -ldl
    target.path=$$PREFIX/lib/openrgb/plugins/
    INSTALLS += target
}

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15
macx:CONFIG += c++17

RESOURCES +=                                                                                    \
    resources.qrc
