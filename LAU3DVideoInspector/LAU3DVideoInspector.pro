# ============================================================================
# LAU3DVideoRecorder - Main Video Recording Application
# ============================================================================
# Supported platforms: macOS, Windows
# Enabled features: cascade, orbbec, lucid (background filter only)
# ============================================================================

CONFIG  -= cascade
CONFIG  += orbbec
CONFIG  += lucid

DEFINES += LUCID_USEPTPCOMMANDS

QT      += core gui serialport widgets xml concurrent network

equals(QT_MAJOR_VERSION, 6){
   QT += openglwidgets opengl
}

equals(QT_MAJOR_VERSION, 5){
   QT += opengl
}

TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS EXCLUDE_LAUVELMEXWIDGET
CONFIG  += sdk_no_version_check c++17

# Version information
VERSION = 1.0.0
TARGET = LAU3DVideoInspector
win32:QMAKE_TARGET_PRODUCT = "3D Video Inspector"
win32:QMAKE_TARGET_DESCRIPTION = "3D Depth Camera Video Inspection Application"
win32:QMAKE_TARGET_COMPANY = "Lau Consulting Inc"
win32:QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2025 Lau Consulting Inc"

# ============================================================================
# CORE FILES (Always included)
# ============================================================================

HEADERS += ../LAUSupportFiles/Filters/lauabstractfilter.h \
           ../LAUSupportFiles/Filters/laucolorizedepthglfilter.h \
           ../LAUSupportFiles/Filters/lauprojectscantocameraglfilter.h \
           ../LAUSupportFiles/Filters/laumergedocumentswidget.h \
           ../LAUSupportFiles/Filters/laubackgroundglfilter.h \
           ../LAUSupportFiles/Filters/laugreenscreenglfilter.h \
           ../LAUSupportFiles/Sinks/lau3dvideorecordingwidget.h \
           ../LAUSupportFiles/Sinks/lau3dvideoplayerwidget.h \
           ../LAUSupportFiles/Sinks/lau3dmultiscanglwidget.h \
           ../LAUSupportFiles/Sinks/lau3dfiducialglwidget.h \
           ../LAUSupportFiles/Sinks/lau3dvideoglwidget.h \
           ../LAUSupportFiles/Sinks/lau3dscanglwidget.h \
           ../LAUSupportFiles/Sinks/lau3dvideowidget.h \
           ../LAUSupportFiles/Sources/lau3dcamera.h \
           ../LAUSupportFiles/Sources/lau3dcameras.h \
           ../LAUSupportFiles/Sources/lau3dmachinevisionscannerwidget.h \
           ../LAUSupportFiles/Support/laumachinelearningvideoframelabelerwidget.h \
           ../LAUSupportFiles/Support/laucameraclassifierdialog.h \
           ../LAUSupportFiles/Support/laucameraconnectiondialog.h \
           ../LAUSupportFiles/Support/lautransformeditorwidget.h \
           ../LAUSupportFiles/Support/laudepthlabelerwidget.h \
           ../LAUSupportFiles/Support/lauvideoplayerlabel.h \
           ../LAUSupportFiles/Support/laupalettewidget.h \
           ../LAUSupportFiles/Support/lauscaninspector.h \
           ../LAUSupportFiles/Support/laumemoryobject.h \
           ../LAUSupportFiles/Support/laulookuptable.h \
           ../LAUSupportFiles/Support/laucontroller.h \
           ../LAUSupportFiles/Support/lauconstants.h \
           ../LAUSupportFiles/Support/lauglwidget.h \
           ../LAUSupportFiles/Support/sse2neon.h \
           ../LAUSupportFiles/Support/lauscan.h \
           ../LAUSupportFiles/User/laudocument.h \
           ../LAUSupportFiles/User/laumenuwidget.h \
           ../LAUSupportFiles/User/laudocumentwidget.h \
           ../LAUSupportFiles/Support/laurfidwidget.h

SOURCES += main.cpp \
           ../LAUSupportFiles/Filters/laucolorizedepthglfilter.cpp \
           ../LAUSupportFiles/Filters/lauabstractfilter.cpp \
           ../LAUSupportFiles/Filters/lauprojectscantocameraglfilter.cpp \
           ../LAUSupportFiles/Filters/laumergedocumentswidget.cpp \
           ../LAUSupportFiles/Filters/laubackgroundglfilter.cpp \
           ../LAUSupportFiles/Filters/laugreenscreenglfilter.cpp \
           ../LAUSupportFiles/Sinks/lau3dvideorecordingwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dfiducialglwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dvideoplayerwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dvideoglwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dscanglwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dmultiscanglwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dvideowidget.cpp \
           ../LAUSupportFiles/Sources/lau3dcamera.cpp \
           ../LAUSupportFiles/Sources/lau3dcameras.cpp \
           ../LAUSupportFiles/Support/laucameraclassifierdialog.cpp \
           ../LAUSupportFiles/Support/laucameraconnectiondialog.cpp \
           ../LAUSupportFiles/Support/laumachinelearningvideoframelabelerwidget.cpp \
           ../LAUSupportFiles/Support/lautransformeditorwidget.cpp \
           ../LAUSupportFiles/Support/laudepthlabelerwidget.cpp \
           ../LAUSupportFiles/Support/lauvideoplayerlabel.cpp \
           ../LAUSupportFiles/Support/laupalettewidget.cpp \
           ../LAUSupportFiles/Support/lauscaninspector.cpp \
           ../LAUSupportFiles/Support/laumemoryobject.cpp \
           ../LAUSupportFiles/Support/laulookuptable.cpp \
           ../LAUSupportFiles/Support/lauglwidget.cpp \
           ../LAUSupportFiles/Support/lauscan.cpp \
           ../LAUSupportFiles/User/laudocument.cpp \
           ../LAUSupportFiles/User/laumenuwidget.cpp \
           ../LAUSupportFiles/User/laudocumentwidget.cpp \
           ../LAUSupportFiles/Support/laurfidwidget.cpp

RESOURCES += ../LAUSupportFiles/Images/lauvideoplayerlabel.qrc \
             ../LAUSupportFiles/Shaders/shaders.qrc

OTHER_FILES += ToDoList.txt
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

INCLUDEPATH += $$PWD/../LAUSupportFiles/Classifiers $$PWD/../LAUSupportFiles/Filters $$PWD/../LAUSupportFiles/Sources $$PWD/../LAUSupportFiles/Sinks $$PWD/../LAUSupportFiles/Support $$PWD/../LAUSupportFiles/User

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

# ============================================================================
# OPTIONAL FEATURES
# ============================================================================

cascade {
    DEFINES   += ENABLECASCADE
    HEADERS   += ../LAUSupportFiles/Classifiers/laucascadeclassifierglfilter.h \
                 ../LAUSupportFiles/Classifiers/laucascadeclassifierfromlivevideo.h \
                 ../LAUSupportFiles/Support/lauobjecthashtable.h \
                 ../LAUSupportFiles/Filters/lausavetodiskfilter.h
    SOURCES   += ../LAUSupportFiles/Classifiers/laucascadeclassifierglfilter.cpp \
                 ../LAUSupportFiles/Classifiers/laucascadeclassifierfromlivevideo.cpp \
                 ../LAUSupportFiles/Support/lauobjecthashtable.cpp \
                 ../LAUSupportFiles/Filters/lausavetodiskfilter.cpp
    RESOURCES += ../LAUSupportFiles/Classifiers/classifiers.qrc
}

orbbec {
    DEFINES     += ORBBEC ORBBEC_USE_RESOLUTION_DIALOG
    HEADERS     += ../LAUSupportFiles/Sources/lauorbbeccamera.h
    SOURCES     += ../LAUSupportFiles/Sources/lauorbbeccamera.cpp
}

lucid {
    DEFINES     += LUCID
    HEADERS     += ../LAUSupportFiles/Sources/laulucidcamera.h
    SOURCES     += ../LAUSupportFiles/Sources/laulucidcamera.cpp
}

# ============================================================================
# PLATFORM-SPECIFIC CONFIGURATION
# ============================================================================

unix:macx {
    QMAKE_CXXFLAGS += -msse2 -msse3 -mssse3 -msse4.1
    QMAKE_CXXFLAGS_WARN_ON += -Wno-reorder
    INCLUDEPATH    += /usr/local/include/Tiff /usr/local/include/eigen3
    DEPENDPATH     += /usr/local/include/Tiff /usr/local/include/eigen3
    LIBS           += /usr/local/lib/libtiff.dylib

    cascade {
        INCLUDEPATH   += /usr/local/include/opencv4
        DEPENDPATH    += /usr/local/include/opencv4
        LIBS          += -L/usr/local/lib -lopencv_core -lopencv_objdetect -lopencv_imgproc -lopencv_calib3d -lopencv_highgui -lopencv_ml
    }
}

win32 {
    INCLUDEPATH += $$quote(C:/usr/include) $$quote(C:/usr/Tiff/include/)
    DEPENDPATH  += $$quote(C:/usr/include) $$quote(C:/usr/Tiff/include/)
    LIBS        += -L$$quote(C:/usr/Tiff/lib) -ltiff -lopengl32

    # Copy Tiff DLL to output directory (required for all LAU applications)
    TIFF_BIN_PATH = $$quote(C:/usr/Tiff/bin)
    QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/tiff.dll) copy /y $$shell_path($$TIFF_BIN_PATH/tiff.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))

    # Copy OpenCV DLLs to output directory (if OpenCV is used by cascade feature)
    OPENCV_BIN_PATH = C:/usr/OpenCV/x64/vc17/bin
    QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_core490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_core490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
    QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_imgproc490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_imgproc490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
    QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_imgcodecs490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_imgcodecs490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
    QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_highgui490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_highgui490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))

    orbbec {
        INCLUDEPATH += $$quote(C:/usr/OrbbecSDK/include)
        DEPENDPATH  += $$quote(C:/usr/OrbbecSDK/include)
        LIBS        += -L$$quote(C:/usr/OrbbecSDK/lib) -lOrbbecSDK

        # Copy OrbbecSDK DLLs to output directory
        ORBBEC_SDK_PATH = C:/usr/OrbbecSDK/bin

        # Copy all required Orbbec DLLs after build (only if not already present)
        QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/OrbbecSDK.dll) copy /y $$shell_path($$ORBBEC_SDK_PATH/OrbbecSDK.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
        exists($$ORBBEC_SDK_PATH/OrbbecSDKConfig_v1.0.xml) {
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/OrbbecSDKConfig_v1.0.xml) copy /y $$shell_path($$ORBBEC_SDK_PATH/OrbbecSDKConfig_v1.0.xml) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
        }
    }

    lucid {
        INCLUDEPATH += $$quote(C:/Program Files/Lucid Vision Labs/Arena SDK/include/ArenaC)
        DEPENDPATH  += $$quote(C:/Program Files/Lucid Vision Labs/Arena SDK/include/ArenaC)
        LIBS        += -L$$quote(C:/Program Files/Lucid Vision Labs/Arena SDK/lib64/ArenaC) -lArenaC_v140
    }

    cascade {
        INCLUDEPATH   += $$quote(C:/usr/OpenCV/include)
        DEPENDPATH    += $$quote(C:/usr/OpenCV/include)
        LIBS          += -L$$quote(C:/usr/OpenCV/x64/vc17/lib)
        CONFIG(release, debug|release): LIBS += -lopencv_core490 -lopencv_objdetect490 -lopencv_imgproc490 -lopencv_calib3d490 -lopencv_highgui490 -lopencv_ml490 -lopencv_face490
        CONFIG(debug, debug|release):   LIBS += -lopencv_core490d -lopencv_objdetect490d -lopencv_imgproc490d -lopencv_calib3d490d -lopencv_highgui490d -lopencv_ml490d -lopencv_face490d
    }
}

DISTFILES += toDoList

