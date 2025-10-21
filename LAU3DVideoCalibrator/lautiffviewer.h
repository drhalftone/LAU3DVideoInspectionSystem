/*********************************************************************************
 *                                                                               *
 * Copyright (C) 2025 Dr. Daniel L. Lau                                          *
 *                                                                               *
 * This file is part of LAU 3D Video Inspection System.                         *
 *                                                                               *
 * LAU 3D Video Inspection System is free software: you can redistribute it     *
 * and/or modify it under the terms of the GNU Lesser General Public License    *
 * as published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                              *
 *                                                                               *
 * LAU 3D Video Inspection System is distributed in the hope that it will be    *
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser      *
 * General Public License for more details.                                     *
 *                                                                               *
 * You should have received a copy of the GNU Lesser General Public License     *
 * along with LAU 3D Video Inspection System. If not, see                       *
 * <https://www.gnu.org/licenses/>.                                             *
 *                                                                               *
 *********************************************************************************/

#ifndef LAUTIFFVIEWER_H
#define LAUTIFFVIEWER_H

#define CAMERA_HEIGHT_PIXELS 480

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QProgressBar>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QSpacerItem>
#include <QTimer>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QSettings>
#include <QStandardPaths>
#include <QFileInfo>
#include <QGridLayout>

#include "laumemoryobject.h"
#include "laulookuptable.h"
#include "lauscan.h"

class LAUTiffViewerWorker;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUTiffViewer : public QWidget
{
    Q_OBJECT

public:
    explicit LAUTiffViewer(QWidget *parent = nullptr);
    explicit LAUTiffViewer(const QString &filename, QWidget *parent = nullptr);
    ~LAUTiffViewer();

    bool loadTiffFile(const QString &filename);
    bool loadLookupTables(const QString &lutxFilename);
    void setCurrentDirectory(int directory);
    int currentDirectory() const { return currentDir; }
    int totalDirectories() const { return numDirectories; }

    // Playback controls
    void play();
    void pause();
    void stop();
    bool isPlaying() const { return playing; }

    // Zoom and pan
    void setZoomFactor(double factor);
    double zoomFactor() const { return currentZoom; }
    void fitToWindow();
    void actualSize();

    // Display options
    void setAutoContrast(bool enable);
    bool autoContrast() const { return autoContrastEnabled; }

    // Camera selection
    void setCurrentCamera(int camera);
    int currentCamera() const { return currentCam; }
    int totalCameras() const { return numCameras; }

    // NEW: Bounding box access methods
    double getBoundingBoxXMin() const;
    double getBoundingBoxXMax() const;
    double getBoundingBoxYMin() const;
    double getBoundingBoxYMax() const;
    double getBoundingBoxZMin() const;
    double getBoundingBoxZMax() const;

    void setBoundingBoxValues(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
    LookUpTableBoundingBox getBoundingBox() const;
    void setBoundingBox(const LookUpTableBoundingBox &bbox);

    // Static utility methods
    static LAUScan convertMemoryObjectToScan(const LAUMemoryObject &memoryObject, const LAULookUpTable &lookupTable);
    static LAUScan convertMemoryObjectToScan(const LAUMemoryObject &memoryObject, const LAULookUpTable &lookupTable, bool applyTransform);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void onDirectoryChanged(int directory);
    void onPlayButtonClicked();
    void onStopButtonClicked();
    void onFrameRateChanged(int fps);
    void onZoomChanged();
    void onFitToWindowClicked();
    void onActualSizeClicked();
    void onAutoContrastToggled(bool enabled);
    void onPlaybackTimer();
    void onImageLoaded(const LAUMemoryObject &image, int directory);
    void onLoadingProgress(int directory);
    void onLoadingComplete();
    void onOpenFileClicked();
    void onCameraChanged(int camera);
    void onBoundingBoxChanged();

private:
    void setupUI();
    void updateImageDisplay();
    void updateControls();
    void loadImageAsync(int directory);
    QPixmap memoryObjectToPixmap(const LAUMemoryObject &memObj);
    void applyAutoContrast(QImage &image);
    void applyAutoContrastWithAlpha(QImage &image);
    void centerImage();
    void calculateCameraCount();
    LAUMemoryObject extractCameraROI(const LAUMemoryObject &fullImage, int cameraIndex);
    void generateCurrentScan();
    LAUScan getCurrentScan();
    QPixmap scanToPixmap(const LAUScan &scan);

    // Settings methods
    void saveSettings();
    void loadSettings();
    void saveViewerState();
    void loadViewerState();

    // UI Components
    QScrollArea *scrollArea;
    QLabel *imageLabel;
    QSlider *directorySlider;
    QSpinBox *directorySpinBox;
    QLabel *directoryLabel;
    QPushButton *playButton;
    QPushButton *stopButton;
    QSpinBox *frameRateSpinBox;
    QLabel *frameRateLabel;
    QComboBox *zoomComboBox;
    QPushButton *fitToWindowButton;
    QPushButton *actualSizeButton;
    QCheckBox *autoContrastCheckBox;
    QProgressBar *loadingProgressBar;
    QPushButton *openFileButton;
    QLabel *fileInfoLabel;
    QLabel *imageInfoLabel;
    QComboBox *cameraComboBox;
    QLabel *cameraLabel;

    // Bounding box controls
    QGroupBox *boundingBoxGroup;
    QSpinBox *xMinSpinBox;
    QSpinBox *xMaxSpinBox;
    QSpinBox *yMinSpinBox;
    QSpinBox *yMaxSpinBox;
    QSpinBox *zMinSpinBox;
    QSpinBox *zMaxSpinBox;

    // Layouts
    QVBoxLayout *mainLayout;
    QHBoxLayout *controlsLayout;
    QHBoxLayout *playbackLayout;
    QHBoxLayout *zoomLayout;
    QGroupBox *playbackGroup;
    QGroupBox *zoomGroup;
    QGroupBox *displayGroup;

    // Data members
    QString currentFilename;
    QString currentLutxFilename;
    int numDirectories;
    int currentDir;
    int numCameras;
    int currentCam;
    bool playing;
    QTimer *playbackTimer;
    double currentZoom;
    bool autoContrastEnabled;

    // Lookup tables and scan processing
    QList<LAULookUpTable> lookupTables;
    bool useLookupTables;

    // Threading
    LAUTiffViewerWorker *worker;
    QThread *workerThread;

    // Image cache
    QHash<int, LAUMemoryObject> imageCache;
    QMutex cacheMutex;
    static const int MAX_CACHE_SIZE = 50;

    // Scan cache for 3D processing
    QHash<QPair<int,int>, LAUScan> scanCache;  // Key: (directory, camera)
    QMutex scanCacheMutex;

signals:
    void directoryChanged(int directory);
    void fileLoaded(const QString &filename);
    void zoomChanged(double factor);
    void cameraChanged(int camera);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUTiffViewerWorker : public QObject
{
    Q_OBJECT

public:
    explicit LAUTiffViewerWorker(QObject *parent = nullptr);

    void setFilename(const QString &filename);
    void preloadImages(int startDir, int endDir);

public slots:
    void loadImage(int directory);
    void preloadRange(int start, int end);

private:
    QString filename;

signals:
    void imageLoaded(const LAUMemoryObject &image, int directory);
    void loadingProgress(int directory);
    void loadingComplete();
};

#endif // LAUTIFFVIEWER_H
