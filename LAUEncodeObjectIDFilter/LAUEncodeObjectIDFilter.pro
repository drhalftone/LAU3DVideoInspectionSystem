QT = core xml

# Removed gui, widgets, opengl, openglwidgets
# This is a console application and doesn't need GUI components

CONFIG += c++17 console

# Define HEADLESS to disable QImage and other GUI dependencies in LAU support libraries
DEFINES += HEADLESS

TARGET = LAUEncodeObjectIDFilter
TEMPLATE = app

# Version information
VERSION = 1.0.0
win32:QMAKE_TARGET_PRODUCT = "Encode Object ID Filter"
win32:QMAKE_TARGET_DESCRIPTION = "Object ID Encoding and Filtering Utility"
win32:QMAKE_TARGET_COMPANY = "Lau Consulting Inc"
win32:QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2025 Lau Consulting Inc"

# Include path for Support library
INCLUDEPATH += ../LAUSupportFiles/Support

# TIFF library for LUT support
win32 {
    INCLUDEPATH += $$quote(C:/usr/Tiff/include)
    DEPENDPATH  += $$quote(C:/usr/Tiff/include)
    LIBS        += -L$$quote(C:/usr/Tiff/lib) -ltiff
}
unix:macx {
    CONFIG += sdk_no_version_check console
    QMAKE_CXXFLAGS += -msse2 -msse3 -mssse3 -msse4.1
    QMAKE_CXXFLAGS_WARN_ON += -Wno-reorder
    INCLUDEPATH    += /usr/local/include/Tiff /usr/local/include/eigen3
    DEPENDPATH     += /usr/local/include/Tiff /usr/local/include/eigen3
    LIBS           += /usr/local/lib/libtiff.dylib
}

win32 {
    CONFIG += console
    CONFIG -= app_bundle
}

SOURCES += \
    main.cpp \
    ../LAUSupportFiles/Support/lauobjecthashtable.cpp \
    ../LAUSupportFiles/Support/laumemoryobject.cpp

HEADERS += \
    ../LAUSupportFiles/Support/laumemoryobject.h \
    ../LAUSupportFiles/Support/lauobjecthashtable.h \
    ../LAUSupportFiles/Support/lauconstants.h

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
# ============================================================================
# Build Directory Configuration - Place build artifacts outside repository
# ============================================================================
# This prevents build files from being tracked by git and keeps repo clean

# Define build directory outside the repository
BUILD_ROOT = $$PWD/../../build

CONFIG(debug, debug|release) {
    DESTDIR = $$BUILD_ROOT/$$TARGET-debug
    OBJECTS_DIR = $$DESTDIR/.obj
    MOC_DIR = $$DESTDIR/.moc
    RCC_DIR = $$DESTDIR/.rcc
    UI_DIR = $$DESTDIR/.ui
}

CONFIG(release, debug|release) {
    DESTDIR = $$BUILD_ROOT/$$TARGET-release
    OBJECTS_DIR = $$DESTDIR/.obj
    MOC_DIR = $$DESTDIR/.moc
    RCC_DIR = $$DESTDIR/.rcc
    UI_DIR = $$DESTDIR/.ui
}

