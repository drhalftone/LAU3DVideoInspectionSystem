CONFIG  += cascade
CONFIG  += lucid
CONFIG  += orbbec

QT      += core gui serialport widgets opengl openglwidgets xml concurrent network

TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS EXCLUDE_LAU3DVIDEOWIDGET EXCLUDE_LAUSCANINSPECTOR #LUCID_SYNCBYGPIO #USE_GREENSCREEN_FILTER
DEFINES += SAVE_HEADER_FRAMES SHARED_CAMERA_THREAD LUCID_USEPTPCOMMANDS #RECORDRAWVIDEOTODISK

# Version information
VERSION = 1.0.0
TARGET = LAUMonitorLiveVideo
win32:QMAKE_TARGET_PRODUCT = "Monitor Live Video"
win32:QMAKE_TARGET_DESCRIPTION = "Automated Live Video Monitoring and Object Detection"
win32:QMAKE_TARGET_COMPANY = "Lau Consulting Inc"
win32:QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2025 Lau Consulting Inc"

HEADERS += ../LAUSupportFiles/Filters/lauabstractfilter.h \
           ../LAUSupportFiles/Filters/lausavetodiskfilter.h \
           ../LAUSupportFiles/Filters/laugreenscreenglfilter.h \
           ../LAUSupportFiles/Filters/laucolorizedepthglfilter.h \
           ../LAUSupportFiles/Classifiers/laucascadeclassifierfromlivevideo.h \
           ../LAUSupportFiles/Sources/lau3dcamera.h \
           ../LAUSupportFiles/Sources/lau3dcameras.h \
           ../LAUSupportFiles/Sources/lau3dmachinevisionscannerwidget.h \
           ../LAUSupportFiles/Sinks/lau3dscanglwidget.h \
           ../LAUSupportFiles/Sinks/lau3dvideoglwidget.h \
           ../LAUSupportFiles/Sinks/lau3dfiducialglwidget.h \
           ../LAUSupportFiles/Support/laucameraclassifierdialog.h \
           ../LAUSupportFiles/Support/laumemoryobject.h \
           ../LAUSupportFiles/Support/lauobjecthashtable.h \
           ../LAUSupportFiles/Support/laulookuptable.h \
           ../LAUSupportFiles/Support/laucontroller.h \
           ../LAUSupportFiles/Support/lauconstants.h \
           ../LAUSupportFiles/Support/lauglwidget.h \
           ../LAUSupportFiles/Support/sse2neon.h \
           ../LAUSupportFiles/Support/lauscan.h \
           ../LAUSupportFiles/Support/laurfidwidget.h


SOURCES += main.cpp \
           ../LAUSupportFiles/Filters/lauabstractfilter.cpp \
           ../LAUSupportFiles/Filters/lausavetodiskfilter.cpp \
           ../LAUSupportFiles/Filters/laugreenscreenglfilter.cpp \
           ../LAUSupportFiles/Filters/laucolorizedepthglfilter.cpp \
           ../LAUSupportFiles/Classifiers/laucascadeclassifierfromlivevideo.cpp \
           ../LAUSupportFiles/Sources/lau3dcamera.cpp \
           ../LAUSupportFiles/Sources/lau3dcameras.cpp \
           ../LAUSupportFiles/Sinks/lau3dscanglwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dvideoglwidget.cpp \
           ../LAUSupportFiles/Sinks/lau3dfiducialglwidget.cpp \
           ../LAUSupportFiles/Support/laucameraclassifierdialog.cpp \
           ../LAUSupportFiles/Support/lauobjecthashtable.cpp \
           ../LAUSupportFiles/Support/laumemoryobject.cpp \
           ../LAUSupportFiles/Support/laulookuptable.cpp \
           ../LAUSupportFiles/Support/lauglwidget.cpp \
           ../LAUSupportFiles/Support/lauscan.cpp \
           ../LAUSupportFiles/Support/laurfidwidget.cpp


RESOURCES += ../LAUSupportFiles/Images/lauvideoplayerlabel.qrc \
             ../LAUSupportFiles/Shaders/shaders.qrc

#CONFIG      += c++11
OTHER_FILES += ToDoList.txt
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

INCLUDEPATH += $$PWD/../LAUSupportFiles/Classifiers $$PWD/../LAUSupportFiles/Filters $$PWD/../LAUSupportFiles/Sources $$PWD/../LAUSupportFiles/Sinks $$PWD/../LAUSupportFiles/Support $$PWD/../LAUSupportFiles/User

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

cascade {
    DEFINES   += ENABLECASCADE ENABLEFILTERS
    HEADERS   += ../LAUSupportFiles/Classifiers/laucascadeclassifierglfilter.h
    SOURCES   += ../LAUSupportFiles/Classifiers/laucascadeclassifierglfilter.cpp
    RESOURCES += ../LAUSupportFiles/Classifiers/classifiers.qrc
}

lucid {
    DEFINES     += LUCID
    HEADERS     += ../LAUSupportFiles/Sources/laulucidcamera.h
    SOURCES     += ../LAUSupportFiles/Sources/laulucidcamera.cpp
}

orbbec {
    DEFINES     += ORBBEC
    HEADERS     += ../LAUSupportFiles/Sources/lauorbbeccamera.h
    SOURCES     += ../LAUSupportFiles/Sources/lauorbbeccamera.cpp
}

unix:macx {
    QMAKE_CXXFLAGS += -msse2 -msse3 -mssse3 -msse4.1
    QMAKE_CXXFLAGS_WARN_ON += -Wno-reorder
    INCLUDEPATH    += /usr/local/include/Tiff /usr/local/include/eigen3
    DEPENDPATH     += /usr/local/include/Tiff /usr/local/include/eigen3
    LIBS           += /usr/local/lib/libtiff.dylib

    cascade {
        INCLUDEPATH   += /usr/local/include/opencv4 /usr/local/include/openvino
        DEPENDPATH    += /usr/local/include/opencv4 /usr/local/include/openvino
        LIBS          += -L/usr/local/lib -lopencv_core -lopencv_objdetect -lopencv_imgproc \
                          -lopencv_calib3d -lopencv_highgui -lopencv_ml -lopencv_dnn -lopencv_imgcodecs \
                          -lopenvino
    }
}

win32 {
    INCLUDEPATH += $$quote(C:/usr/Tiff/include)
    DEPENDPATH  += $$quote(C:/usr/Tiff/include)
    LIBS        += -L$$quote(C:/usr/Tiff/lib) -ltiff -lopengl32
    
    # Copy Tiff DLL to output directory
    TIFF_BIN_PATH = $$quote(C:/usr/Tiff/bin)
    CONFIG(debug, debug|release) {
        QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/tiff.dll) copy /y $$shell_path($$TIFF_BIN_PATH/tiff.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
    }
    CONFIG(release, debug|release) {
        QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/tiff.dll) copy /y $$shell_path($$TIFF_BIN_PATH/tiff.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
    }

    lucid {
        INCLUDEPATH += $$quote(C:/Program Files/Lucid Vision Labs/Arena SDK/include/ArenaC)
        DEPENDPATH  += $$quote(C:/Program Files/Lucid Vision Labs/Arena SDK/include/ArenaC)
        LIBS        += -L$$quote(C:/Program Files/Lucid Vision Labs/Arena SDK/lib64/ArenaC) -lArenaC_v140
        
        # Arena SDK DLLs are found via PATH environment variable
        # (C:\Program Files\Lucid Vision Labs\Arena SDK\x64Release is in system PATH)
    }

    cascade {
        INCLUDEPATH   += $$quote(C:/usr/OpenCV/include)
        DEPENDPATH    += $$quote(C:/usr/OpenCV/include)
        LIBS          += -L$$quote(C:/usr/OpenCV/x64/vc17/lib)
        CONFIG(release, debug|release): LIBS += -lopencv_core490 -lopencv_objdetect490 -lopencv_imgproc490 -lopencv_calib3d490 \
                                                -lopencv_highgui490 -lopencv_ml490 -lopencv_face490 -lopencv_dnn490 -lopencv_imgcodecs490
        CONFIG(debug, debug|release):   LIBS += -lopencv_core490d -lopencv_objdetect490d -lopencv_imgproc490d -lopencv_calib3d490d \
                                                -lopencv_highgui490d -lopencv_ml490d -lopencv_face490d -lopencv_dnn490d -lopencv_imgcodecs490d
        
        # Copy OpenCV DLLs to output directory
        OPENCV_BIN_PATH = $$quote(C:/usr/OpenCV/x64/vc17/bin)
        
        # Copy all required OpenCV DLLs for release builds
        CONFIG(release, debug|release) {
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_core490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_core490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_objdetect490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_objdetect490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_imgproc490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_imgproc490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_calib3d490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_calib3d490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_highgui490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_highgui490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_ml490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_ml490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_face490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_face490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_dnn490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_dnn490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_imgcodecs490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_imgcodecs490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            # Copy additional OpenCV dependencies that might be needed
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_features2d490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_features2d490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_flann490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_flann490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_videoio490.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_videoio490.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
        }
        
        # Copy all required OpenCV DLLs for debug builds
        CONFIG(debug, debug|release) {
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_core490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_core490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_objdetect490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_objdetect490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_imgproc490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_imgproc490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_calib3d490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_calib3d490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_highgui490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_highgui490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_ml490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_ml490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_face490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_face490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_dnn490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_dnn490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_imgcodecs490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_imgcodecs490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            # Copy additional OpenCV dependencies that might be needed
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_features2d490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_features2d490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_flann490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_flann490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/opencv_videoio490d.dll) copy /y $$shell_path($$OPENCV_BIN_PATH/opencv_videoio490d.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
        }
    }

    orbbec {
        INCLUDEPATH += $$quote(C:/usr/OrbbecSDK/include)
        DEPENDPATH  += $$quote(C:/usr/OrbbecSDK/include)
        LIBS        += -L$$quote(C:/usr/OrbbecSDK/lib) -lOrbbecSDK
        
        # Copy OrbbecSDK DLLs to output directory
        ORBBEC_SDK_PATH = $$quote(C:/usr/OrbbecSDK/bin)
        
        # Copy all required Orbbec DLLs after build
        CONFIG(debug, debug|release) {
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/OrbbecSDK.dll) copy /y $$shell_path($$ORBBEC_SDK_PATH/OrbbecSDK.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            # Also copy configuration files if they exist
            exists($$ORBBEC_SDK_PATH/OrbbecSDKConfig_v1.0.xml) {
                QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/OrbbecSDKConfig_v1.0.xml) copy /y $$shell_path($$ORBBEC_SDK_PATH/OrbbecSDKConfig_v1.0.xml) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            }
        }
        CONFIG(release, debug|release) {
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/OrbbecSDK.dll) copy /y $$shell_path($$ORBBEC_SDK_PATH/OrbbecSDK.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            # Also copy configuration files if they exist
            exists($$ORBBEC_SDK_PATH/OrbbecSDKConfig_v1.0.xml) {
                QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/OrbbecSDKConfig_v1.0.xml) copy /y $$shell_path($$ORBBEC_SDK_PATH/OrbbecSDKConfig_v1.0.xml) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
            }
        }
    }

}

DISTFILES += toDoList

