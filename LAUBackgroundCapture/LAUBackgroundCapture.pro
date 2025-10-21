# ============================================================================
# LAUBackgroundFilter - Standalone Background Filtering Application
# ============================================================================
# Supported platforms: macOS, Windows, Linux
# Enabled features: orbbec, lucid
# ============================================================================

CONFIG  -= cascade
CONFIG  += orbbec
CONFIG  += lucid

DEFINES += LUCID_USEPTPCOMMANDS

# Uncomment the following line to view raw NIR video without background filters
# DEFINES += RAW_NIR_VIDEO

QT      += core gui serialport widgets xml concurrent network

# ============================================================================
# Build Directory Configuration - MUST BE DEFINED EARLY
# ============================================================================
# Define build directory outside the repository
# This is needed early so DESTDIR can be used in platform-specific DLL copy commands

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

equals(QT_MAJOR_VERSION, 6){
   QT += openglwidgets opengl
}

equals(QT_MAJOR_VERSION, 5){
   QT += opengl
}

TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS
CONFIG  += sdk_no_version_check c++17

# Enable console output on Windows for --check-cameras mode
win32:CONFIG += console

# Version information
VERSION = 1.0.0
TARGET = LAUBackgroundCapture
win32:QMAKE_TARGET_PRODUCT = "Background Capture"
win32:QMAKE_TARGET_DESCRIPTION = "Background Capture and Depth Processing Tool"
win32:QMAKE_TARGET_COMPANY = "Lau Consulting Inc"
win32:QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2025 Lau Consulting Inc"

# ============================================================================
# CORE FILES (Always included)
# ============================================================================

HEADERS += laubackgroundfiltermainwindow.h \
           ../LAUSupportFiles/Filters/lauabstractfilter.h \
           ../LAUSupportFiles/Filters/laubackgroundglfilter.h \
           ../LAUSupportFiles/Sinks/lau3dvideowidget.h \
           ../LAUSupportFiles/Sinks/lau3dvideoglwidget.h \
           ../LAUSupportFiles/Sinks/lau3dfiducialglwidget.h \
           ../LAUSupportFiles/Sinks/lau3dvideoplayerwidget.h \
           ../LAUSupportFiles/Sinks/lau3dscanglwidget.h \
           ../LAUSupportFiles/Sinks/lau3dmultisensorvideowidget.h \
           ../LAUSupportFiles/Sources/lau3dcamera.h \
           ../LAUSupportFiles/Sources/lau3dcameras.h \
           ../LAUSupportFiles/Support/laumemoryobject.h \
           ../LAUSupportFiles/Support/laulookuptable.h \
           ../LAUSupportFiles/Support/lauglwidget.h \
           ../LAUSupportFiles/Support/lauscan.h \
           ../LAUSupportFiles/Support/laucontroller.h \
           ../LAUSupportFiles/Support/lauconstants.h \
           ../LAUSupportFiles/Support/lauvideoplayerlabel.h \
           ../LAUSupportFiles/Support/lauscaninspector.h \
           ../LAUSupportFiles/Support/laumachinelearningvideoframelabelerwidget.h \
           ../LAUSupportFiles/Support/laucameraclassifierdialog.h \
           ../LAUSupportFiles/Support/laupalettewidget.h \
           ../LAUSupportFiles/Support/laudepthlabelerwidget.h \
           ../LAUSupportFiles/Support/laucameraconnectiondialog.h \
           ../LAUSupportFiles/Support/sse2neon.h

SOURCES += main.cpp \
           laubackgroundfiltermainwindow.cpp \
           ../LAUSupportFiles/Filters/lauabstractfilter.cpp \
           ../LAUSupportFiles/Filters/laubackgroundglfilter.cpp \
           ../LAUSupportFiles/Sinks/lau3dvideowidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dvideoglwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dfiducialglwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dvideoplayerwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dscanglwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dmultisensorvideowidget.cpp \
           ../LAUSupportFiles/Sources/lau3dcamera.cpp \
           ../LAUSupportFiles/Sources/lau3dcameras.cpp \
           ../LAUSupportFiles/Support/laumemoryobject.cpp \
           ../LAUSupportFiles/Support/laulookuptable.cpp \
           ../LAUSupportFiles/Support/lauglwidget.cpp \
           ../LAUSupportFiles/Support/lauscan.cpp \
           ../LAUSupportFiles/Support/lauvideoplayerlabel.cpp \
           ../LAUSupportFiles/Support/lauscaninspector.cpp \
           ../LAUSupportFiles/Support/laumachinelearningvideoframelabelerwidget.cpp \
           ../LAUSupportFiles/Support/laucameraclassifierdialog.cpp \
           ../LAUSupportFiles/Support/laupalettewidget.cpp \
           ../LAUSupportFiles/Support/laudepthlabelerwidget.cpp \
           ../LAUSupportFiles/Support/laucameraconnectiondialog.cpp

RESOURCES += ../LAUSupportFiles/Images/lauvideoplayerlabel.qrc \
             ../LAUSupportFiles/Shaders/shaders.qrc

OTHER_FILES += TODO.md

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

INCLUDEPATH += $$PWD/../LAUSupportFiles/Filters $$PWD/../LAUSupportFiles/Sources $$PWD/../LAUSupportFiles/Sinks $$PWD/../LAUSupportFiles/Support $$PWD/../LAUSupportFiles/User

# ============================================================================
# PLATFORM-SPECIFIC CONFIGURATION
# ============================================================================

unix:macx {
    QMAKE_CXXFLAGS += -msse2 -msse3 -mssse3 -msse4.1
    QMAKE_CXXFLAGS_WARN_ON += -Wno-reorder
    INCLUDEPATH    += /usr/local/include/Tiff /usr/local/include/eigen3
    DEPENDPATH     += /usr/local/include/Tiff /usr/local/include/eigen3
    LIBS           += /usr/local/lib/libtiff.dylib
}

unix:!macx {
    QMAKE_CXXFLAGS += -msse2 -msse3 -mssse3 -msse4.1
    QMAKE_CXXFLAGS_WARN_ON += -Wno-reorder
    INCLUDEPATH    += /usr/local/include /usr/include/eigen3
    DEPENDPATH     += /usr/local/include /usr/include/eigen3
    LIBS           += -ltiff
}

win32 {
    INCLUDEPATH += $$quote(C:/usr/include) $$quote(C:/usr/Tiff/include/)
    DEPENDPATH  += $$quote(C:/usr/include) $$quote(C:/usr/Tiff/include/)
    LIBS        += -L$$quote(C:/usr/Tiff/lib) -ltiff -lopengl32

    # Copy Tiff DLL and dependencies to output directory
    TIFF_BIN_PATH = $$quote(C:/usr/Tiff/bin)

    # Copy TIFF DLL (required)
    QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/tiff.dll) copy /y $$shell_path($$TIFF_BIN_PATH/tiff.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))

    # Copy dependency DLLs if they exist
    QMAKE_POST_LINK += $$quote(if exist $$shell_path($$TIFF_BIN_PATH/zlib.dll) (if not exist $$shell_path($$DESTDIR/zlib.dll) copy /y $$shell_path($$TIFF_BIN_PATH/zlib.dll) $$shell_path($$DESTDIR/)) $$escape_expand(\\n\\t))
    QMAKE_POST_LINK += $$quote(if exist $$shell_path($$TIFF_BIN_PATH/zlib1.dll) (if not exist $$shell_path($$DESTDIR/zlib1.dll) copy /y $$shell_path($$TIFF_BIN_PATH/zlib1.dll) $$shell_path($$DESTDIR/)) $$escape_expand(\\n\\t))
    QMAKE_POST_LINK += $$quote(if exist $$shell_path($$TIFF_BIN_PATH/jpeg.dll) (if not exist $$shell_path($$DESTDIR/jpeg.dll) copy /y $$shell_path($$TIFF_BIN_PATH/jpeg.dll) $$shell_path($$DESTDIR/)) $$escape_expand(\\n\\t))
    QMAKE_POST_LINK += $$quote(if exist $$shell_path($$TIFF_BIN_PATH/libjpeg.dll) (if not exist $$shell_path($$DESTDIR/libjpeg.dll) copy /y $$shell_path($$TIFF_BIN_PATH/libjpeg.dll) $$shell_path($$DESTDIR/)) $$escape_expand(\\n\\t))
    QMAKE_POST_LINK += $$quote(if exist $$shell_path($$TIFF_BIN_PATH/liblzma.dll) (if not exist $$shell_path($$DESTDIR/liblzma.dll) copy /y $$shell_path($$TIFF_BIN_PATH/liblzma.dll) $$shell_path($$DESTDIR/)) $$escape_expand(\\n\\t))
    QMAKE_POST_LINK += $$quote(if exist $$shell_path($$TIFF_BIN_PATH/lzma.dll) (if not exist $$shell_path($$DESTDIR/lzma.dll) copy /y $$shell_path($$TIFF_BIN_PATH/lzma.dll) $$shell_path($$DESTDIR/)) $$escape_expand(\\n\\t))

    orbbec {
        INCLUDEPATH += $$quote(C:/usr/OrbbecSDK/include)
        DEPENDPATH  += $$quote(C:/usr/OrbbecSDK/include)
        LIBS        += -L$$quote(C:/usr/OrbbecSDK/lib) -lOrbbecSDK

        # Copy OrbbecSDK DLLs to output directory
        ORBBEC_SDK_PATH = C:/usr/OrbbecSDK/bin

        CONFIG(debug, debug|release) {
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/OrbbecSDK.dll) copy /y $$shell_path($$ORBBEC_SDK_PATH/OrbbecSDK.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            exists($$ORBBEC_SDK_PATH/OrbbecSDKConfig_v1.0.xml) {
                QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/OrbbecSDKConfig_v1.0.xml) copy /y $$shell_path($$ORBBEC_SDK_PATH/OrbbecSDKConfig_v1.0.xml) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            }
        }
        CONFIG(release, debug|release) {
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/OrbbecSDK.dll) copy /y $$shell_path($$ORBBEC_SDK_PATH/OrbbecSDK.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            exists($$ORBBEC_SDK_PATH/OrbbecSDKConfig_v1.0.xml) {
                QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/OrbbecSDKConfig_v1.0.xml) copy /y $$shell_path($$ORBBEC_SDK_PATH/OrbbecSDKConfig_v1.0.xml) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            }
        }
    }

    lucid {
        INCLUDEPATH += $$quote(C:/Program Files/Lucid Vision Labs/Arena SDK/include/ArenaC)
        DEPENDPATH  += $$quote(C:/Program Files/Lucid Vision Labs/Arena SDK/include/ArenaC)
        LIBS        += -L$$quote(C:/Program Files/Lucid Vision Labs/Arena SDK/lib64/ArenaC) -lArenaC_v140
    }
}

# ============================================================================
# OPTIONAL FEATURES
# ============================================================================

orbbec {
    DEFINES     += ORBBEC
    HEADERS     += ../LAUSupportFiles/Sources/lauorbbeccamera.h
    SOURCES     += ../LAUSupportFiles/Sources/lauorbbeccamera.cpp
}

lucid {
    DEFINES     += LUCID
    HEADERS     += ../LAUSupportFiles/Sources/laulucidcamera.h
    SOURCES     += ../LAUSupportFiles/Sources/laulucidcamera.cpp
}
