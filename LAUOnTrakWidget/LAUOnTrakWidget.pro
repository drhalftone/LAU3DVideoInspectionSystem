QT       += core gui widgets network

CONFIG += c++11

# Version information
VERSION = 1.0.0
TARGET = LAUOnTrakWidget
win32:QMAKE_TARGET_PRODUCT = "OnTrak Control Widget"
win32:QMAKE_TARGET_DESCRIPTION = "OnTrak USB Relay Control Interface"
win32:QMAKE_TARGET_COMPANY = "Lau Consulting Inc"
win32:QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2025 Lau Consulting Inc"

SOURCES += main.cpp \
           lauontrakwidget.cpp

HEADERS += lauontrakwidget.h

RESOURCES += resources.qrc

# ============================================================================
# OnTrak ADU SDK Configuration
# ============================================================================
# IMPORTANT: This project requires the OnTrak ADU SDK to be installed.
#
# Download the SDK from: https://www.ontrak.net/
# Default installation path: C:/usr/OnTrak
#
# The SDK includes:
#   - AduHid.h (header file)
#   - AduHid64.lib (import library)
#   - AduHid64.dll (runtime library)
#
# If you installed to a different location, update ONTRAK_SDK_PATH below.
# ============================================================================

ONTRAK_SDK_PATH = C:/usr/OnTrak

win32 {
    # Include path for AduHid.h
    INCLUDEPATH += $$ONTRAK_SDK_PATH
    DEPENDPATH += $$ONTRAK_SDK_PATH

    # Link against AduHid64.lib
    LIBS += -L$$ONTRAK_SDK_PATH -lAduHid64

    message("OnTrak SDK path: $$ONTRAK_SDK_PATH")
    message("If build fails, ensure OnTrak SDK is installed to $$ONTRAK_SDK_PATH")
}

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

win32 {

    # Copy OnTrak DLL to output directory for runtime
    ONTRAK_DLL = $$ONTRAK_SDK_PATH/AduHid64.dll
    exists($$ONTRAK_DLL) {
        QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/AduHid64.dll) copy /y $$shell_path($$ONTRAK_DLL) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
    } else {
        warning("OnTrak DLL not found at $$ONTRAK_DLL - runtime may fail")
    }
}

