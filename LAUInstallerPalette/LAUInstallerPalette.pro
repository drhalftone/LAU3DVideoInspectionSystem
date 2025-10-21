QT += core gui widgets

TARGET = LAUInstallerPalette
TEMPLATE = app

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    lauremotetoolspalette.cpp

HEADERS += \
    lauremotetoolspalette.h

RESOURCES += \
    resources.qrc

# Platform-specific settings
win32 {
    LIBS += -lUser32
    INCLUDEPATH += $$quote(C:/usr/Tiff/include)
    DEPENDPATH  += $$quote(C:/usr/Tiff/include)
    LIBS        += -L$$quote(C:/usr/Tiff/lib) -ltiff
}

macx {
    INCLUDEPATH    += /usr/local/include/Tiff
    DEPENDPATH     += /usr/local/include/Tiff
    LIBS           += /usr/local/lib/libtiff.dylib
}

unix:!macx {
    LIBS += -ltiff
}

# Install target
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/LAURemoteToolsPalette
INSTALLS += target

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
