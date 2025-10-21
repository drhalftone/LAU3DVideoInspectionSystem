QT = core gui widgets

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = LAURemoteToolsScheduler
TEMPLATE = app

# Version information
VERSION = 1.0.0
win32:QMAKE_TARGET_PRODUCT = "Remote Tools Scheduler"
win32:QMAKE_TARGET_DESCRIPTION = "System Recording Tools Installation and Configuration"
win32:QMAKE_TARGET_COMPANY = "Lau Consulting Inc"
win32:QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2025 Lau Consulting Inc"

# Windows icon and admin privileges
win32 {
    QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"
    LIBS += -lAdvapi32

    # Copy executable to install folder for testing (commented out for production builds)
    # CONFIG(release, debug|release) {
    #     QMAKE_POST_LINK += copy /y $$shell_quote($$shell_path($$OUT_PWD/$${TARGET}.exe)) $$shell_quote(C:/Program Files (x86)/RemoteRecordingTools/$${TARGET}.exe) $$escape_expand(\\n\\t)
    # }
}

# macOS icon
unix:macx {
}

SOURCES += \
    main.cpp \
    lausystemsetupwidget.cpp \
    lausystemcheckdialog.cpp

HEADERS += \
    lausystemsetupwidget.h \
    lausystemcheckdialog.h

RESOURCES += \
    LAURemoteToolsInstaller.qrc

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
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

