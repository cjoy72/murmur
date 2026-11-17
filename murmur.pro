QT += core gui qml quick quickcontrols2 multimedia dbus

CONFIG += c++17 release
TARGET = murmur
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/soundengine.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/soundengine.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
