# LAU 3D Video Calibrator - Standalone Calibration Tool
# This is the standalone version for use at remote locations

QT += core widgets concurrent opengl openglwidgets xml

CONFIG += c++17
CONFIG += merging

TARGET = LAU3DVideoCalibrator
TEMPLATE = app

DEFINES += LAU_LOOKUP_TABLE_SUPPORT

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

INCLUDEPATH += $$PWD/../LAUSupportFiles/Support $$PWD/../LAUSupportFiles/Sinks $$PWD/../LAUSupportFiles/Filters $$PWD/../LAUSupportFiles/Calibration $$PWD/../LAUSupportFiles/Merge

# Merging functionality defines
merging {
    DEFINES += ENABLEPOINTMATCHER
    DEFINES += PCL_NO_PRECOMPILE
}

# Common source files
SOURCES += \
    ../LAUSupportFiles/Support/laulookuptable.cpp \
    ../LAUSupportFiles/Support/laumemoryobject.cpp \
    ../LAUSupportFiles/Support/lauscan.cpp \
    ../LAUSupportFiles/Support/lauscaninspector.cpp \
    ../LAUSupportFiles/Support/lauglwidget.cpp \
    ../LAUSupportFiles/Support/lautransformeditorwidget.cpp \
    ../LAUSupportFiles/Sinks/lau3dfiducialglwidget.cpp \
    ../LAUSupportFiles/Sinks/lau3dscanglwidget.cpp \
    ../LAUSupportFiles/Filters/lauproximityglfilter.cpp \
    ../LAUSupportFiles/Calibration/lausetxyplanewidget.cpp \
    laujetrwidget.cpp \
    laujetrdialog.cpp \
    laucamerainventorydialog.cpp \
    laucameraselectiondialog.cpp \
    laumatrixtable.cpp \
    lautiffviewerdialog.cpp \
    lautiffviewer.cpp \
    laujetrstandalonedialog.cpp \
    lauwelcomedialog.cpp \
    main_standalone.cpp

# Merge functionality (when enabled)
merging {
    SOURCES += \
        ../LAUSupportFiles/Sinks/lau3dmultiscanglwidget.cpp \
        ../LAUSupportFiles/Merge/lauiterativeclosestpointobject.cpp \
        ../LAUSupportFiles/Merge/laumergescanwidget.cpp
}

# Common header files
HEADERS += \
    ../LAUSupportFiles/Support/laulookuptable.h \
    ../LAUSupportFiles/Support/laumemoryobject.h \
    ../LAUSupportFiles/Support/lauscan.h \
    ../LAUSupportFiles/Support/lauscaninspector.h \
    ../LAUSupportFiles/Support/lauglwidget.h \
    ../LAUSupportFiles/Support/lautransformeditorwidget.h \
    ../LAUSupportFiles/Sinks/lau3dfiducialglwidget.h \
    ../LAUSupportFiles/Sinks/lau3dscanglwidget.h \
    ../LAUSupportFiles/Filters/lauproximityglfilter.h \
    ../LAUSupportFiles/Calibration/lausetxyplanewidget.h \
    laujetrwidget.h \
    laujetrdialog.h \
    laucamerainventorydialog.h \
    laucameraselectiondialog.h \
    laumatrixtable.h \
    lautiffviewerdialog.h \
    lautiffviewer.h \
    laujetrstandalonedialog.h \
    lauwelcomedialog.h

# Merge functionality headers (when enabled)
merging {
    HEADERS += \
        ../LAUSupportFiles/Support/laucontroller.h \
           ../LAUSupportFiles/Support/lauconstants.h \
        ../LAUSupportFiles/Sinks/lau3dmultiscanglwidget.h \
        ../LAUSupportFiles/Merge/lauiterativeclosestpointobject.h \
        ../LAUSupportFiles/Merge/laumergescanwidget.h
}

# Resource files
RESOURCES += \
    resources.qrc \
    ../LAUSupportFiles/Images/lauvideoplayerlabel.qrc \
    ../LAUSupportFiles/Shaders/shaders.qrc

# Platform-specific library configurations
win32 {
    INCLUDEPATH += $$quote(C:/usr/Tiff/include)
    DEPENDPATH  += $$quote(C:/usr/Tiff/include)
    LIBS        += -L$$quote(C:/usr/Tiff/lib) -ltiff

    # Eigen3 for linear algebra (calibration support)
    INCLUDEPATH += $$quote(C:/usr/Eigen3)
    DEPENDPATH += $$quote(C:/usr/Eigen3)

    # PCL Configuration for Windows (when merging is enabled)
    merging {
        message("Configuring PCL for Windows...")

        # Define vcpkg paths
        VCPKG_PATH = C:/usr/vcpkg/installed/x64-windows

        # PCL include paths
        INCLUDEPATH += $$quote($$VCPKG_PATH/include)
        INCLUDEPATH += $$quote($$VCPKG_PATH/include/eigen3)

        # PCL library path
        LIBS += -L$$quote($$VCPKG_PATH/lib)

        # Core PCL libraries
        LIBS += -lpcl_common \
                -lpcl_registration \
                -lpcl_filters \
                -lpcl_kdtree \
                -lpcl_search \
                -lpcl_features \
                -lpcl_sample_consensus \
                -lpcl_io

        # Essential dependencies for PCL (in dependency order)
        LIBS += -llz4 \
                -lflann_cpp \
                -lflann

        # Boost libraries (check actual names in your vcpkg installation)
        LIBS += -lboost_system-vc143-mt-x64-1_88 \
                -lboost_filesystem-vc143-mt-x64-1_88 \
                -lboost_thread-vc143-mt-x64-1_88 \
                -lboost_date_time-vc143-mt-x64-1_88 \
                -lboost_iostreams-vc143-mt-x64-1_88 \
                -lboost_serialization-vc143-mt-x64-1_88

        # Additional Windows libraries
        LIBS += -ladvapi32 -luser32 -lws2_32

        # PCL preprocessor definitions
        DEFINES += NOMINMAX WIN32_LEAN_AND_MEAN

        message("PCL configuration complete.")

        # Copy required DLLs to output directory
        VCPKG_BIN_PATH = $$quote($$VCPKG_PATH/bin)
        TIFF_BIN_PATH = $$quote(C:/usr/Tiff/bin)

        # Define list of required DLLs
        REQUIRED_DLLS = \
            lz4.dll \
            pcl_common.dll \
            pcl_registration.dll \
            pcl_filters.dll \
            pcl_kdtree.dll \
            pcl_search.dll \
            pcl_features.dll \
            pcl_sample_consensus.dll \
            pcl_io.dll \
            flann_cpp.dll \
            flann.dll \
            boost_system-vc143-mt-x64-1_88.dll \
            boost_filesystem-vc143-mt-x64-1_88.dll \
            boost_thread-vc143-mt-x64-1_88.dll \
            boost_date_time-vc143-mt-x64-1_88.dll \
            boost_iostreams-vc143-mt-x64-1_88.dll \
            boost_serialization-vc143-mt-x64-1_88.dll \
            tiff.dll

        # Copy DLLs from vcpkg bin directory to DESTDIR
        for(dll, REQUIRED_DLLS) {
            QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/$$dll) if exist $$shell_path($$VCPKG_BIN_PATH/$$dll) copy /y $$shell_path($$VCPKG_BIN_PATH/$$dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))
        }

        # Also copy from TIFF bin directory if it exists
        QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$DESTDIR/tiff.dll) if exist $$shell_path($$TIFF_BIN_PATH/tiff.dll) copy /y $$shell_path($$TIFF_BIN_PATH/tiff.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t))

        message("DLL copying configured for PCL dependencies")
    }
}

unix:macx {
    QMAKE_CXXFLAGS += -msse2 -msse3 -mssse3 -msse4.1
    QMAKE_CXXFLAGS_WARN_ON += -Wno-reorder
    INCLUDEPATH    += /usr/local/include/Tiff
    DEPENDPATH     += /usr/local/include/Tiff
    LIBS           += /usr/local/lib/libtiff.dylib

    # Eigen3 for linear algebra (calibration support)
    INCLUDEPATH += /usr/local/include/eigen3
    DEPENDPATH += /usr/local/include/eigen3

    # PCL Configuration for Mac (when merging is enabled)
    merging {
        # Add Boost include path (required by PCL)
        INCLUDEPATH += /usr/local/Cellar/boost/1.89.0/include

        # Add FLANN include path (required by PCL)
        INCLUDEPATH += /usr/local/Cellar/flann/1.9.2_3/include

        # Add LZ4 include path (required by FLANN)
        INCLUDEPATH += /usr/local/Cellar/lz4/1.10.0/include

        # Manual PCL installation configuration using isolated directory (first priority)
        exists(/Users/dllau/PCL/include) {
            message("Using custom PCL installation in /Users/dllau/PCL")
            INCLUDEPATH += /Users/dllau/PCL/include
            INCLUDEPATH += /Users/dllau/PCL/include/eigen3
            LIBS += -L/usr/local/lib
            LIBS += -llz4

            # PCL libraries for Mac (custom build)
            LIBS += -lpcl_common \
                    -lpcl_registration \
                    -lpcl_filters \
                    -lpcl_kdtree \
                    -lpcl_search \
                    -lpcl_features \
                    -lpcl_sample_consensus \
                    -lpcl_io
        } else:exists(/usr/local/pcl/include/pcl) {
            message("Using home-built PCL installation")
            INCLUDEPATH += /usr/local/pcl/include
            LIBS += -L/usr/local/pcl/lib -lpcl_common -lpcl_features -lpcl_filters -lpcl_io -lpcl_kdtree -lpcl_search -lpcl_registration -lpcl_sample_consensus
        } else:exists(/usr/local/include/pcl-1.15) {
            message("Using Homebrew PCL 1.15 installation")
            INCLUDEPATH += /usr/local/include/pcl-1.15
            LIBS += -L/usr/local/lib -lpcl_common -lpcl_features -lpcl_filters -lpcl_io -lpcl_kdtree -lpcl_search -lpcl_registration -lpcl_sample_consensus -llz4
        } else:exists(/usr/local/include/pcl-1.11) {
            message("Using Homebrew PCL 1.11 installation")
            INCLUDEPATH += /usr/local/include/pcl-1.11
            LIBS += -L/usr/local/lib -lpcl_common -lpcl_features -lpcl_filters -lpcl_io -lpcl_kdtree -lpcl_search -lpcl_registration -lpcl_sample_consensus
        } else:exists(/usr/local/include/pcl-1.12) {
            message("Using Homebrew PCL 1.12 installation")
            INCLUDEPATH += /usr/local/include/pcl-1.12
            LIBS += -L/usr/local/lib -lpcl_common -lpcl_features -lpcl_filters -lpcl_io -lpcl_kdtree -lpcl_search -lpcl_registration -lpcl_sample_consensus
        } else:exists(/usr/local/include/pcl) {
            message("Using generic Homebrew PCL installation")
            INCLUDEPATH += /usr/local/include/pcl
            LIBS += -L/usr/local/lib -lpcl_common -lpcl_features -lpcl_filters -lpcl_io -lpcl_kdtree -lpcl_search -lpcl_registration -lpcl_sample_consensus
        } else:exists(/opt/homebrew/include/pcl) {
            message("Using Apple Silicon Homebrew PCL installation")
            INCLUDEPATH += /opt/homebrew/include/pcl
            LIBS += -L/opt/homebrew/lib -lpcl_common -lpcl_features -lpcl_filters -lpcl_io -lpcl_kdtree -lpcl_search -lpcl_registration -lpcl_sample_consensus
        }
    }
}

unix:!macx {
    # Linux: use system libtiff
    LIBS += -ltiff

    # PCL Configuration for Linux (when merging is enabled)
    merging {
        # PCL on Linux (usually installed via package manager)
        LIBS += -lpcl_common \
                -lpcl_registration \
                -lpcl_filters \
                -lpcl_kdtree \
                -lpcl_search \
                -lpcl_features \
                -lpcl_sample_consensus \
                -lpcl_io

        # Eigen3 for linear algebra
        INCLUDEPATH += /usr/include/eigen3
        DEPENDPATH += /usr/include/eigen3
    }
}

# Compiler settings
win32-msvc* {
    QMAKE_CXXFLAGS += /W3
    LIBS += -lopengl32
}

win32-g++ {
    QMAKE_CXXFLAGS += -Wall -Wextra -msse4.1
    LIBS += -lopengl32
}

macx {
    QMAKE_CXXFLAGS += -std=c++17 -stdlib=libc++
    QMAKE_LFLAGS += -stdlib=libc++
}

unix:!macx {
    LIBS += -ltiff -lGL

    # Eigen3 for linear algebra (calibration support)
    INCLUDEPATH += /usr/include/eigen3
    DEPENDPATH += /usr/include/eigen3
}

# Version info
VERSION = 1.0.0
QMAKE_TARGET_COMPANY = "LAU Research Group"
QMAKE_TARGET_PRODUCT = "JETR Standalone Editor"
QMAKE_TARGET_DESCRIPTION = "Standalone JETR (Just-Enough-To-Reconstruct) vector editor for remote system use"
QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2025"

# Build configuration messages
merging {
    message("Building with Scan Merging/ICP support")
}
