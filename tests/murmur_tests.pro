QT += core gui quick quickcontrols2 multimedia testlib dbus
CONFIG += c++17 testcase
TARGET = murmur_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += \
    ../src/backend.h \
    ../src/soundengine.h \
    ../src/theme.h \
    ../src/translator.h

SOURCES += \
    murmur_tests.cpp \
    ../src/backend.cpp \
    ../src/soundengine.cpp \
    ../src/theme.cpp \
    ../src/translator.cpp
