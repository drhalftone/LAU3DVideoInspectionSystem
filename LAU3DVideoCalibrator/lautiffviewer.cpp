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

#include "LAUTiffViewer.h"
#include <QDebug>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUTiffViewer::LAUTiffViewer(QWidget *parent)
    : QWidget(parent)
    , numDirectories(0)
    , currentDir(0)
    , numCameras(0)
    , currentCam(0)
    , playing(false)
    , currentZoom(1.0)
    , autoContrastEnabled(false)
    , useLookupTables(false)
    , worker(nullptr)
    , workerThread(nullptr)
{
    setupUI();

    // Initialize worker thread
    worker = new LAUTiffViewerWorker();
    workerThread = new QThread(this);
    worker->moveToThread(workerThread);

    // Connect worker signals
    connect(worker, &LAUTiffViewerWorker::imageLoaded, this, &LAUTiffViewer::onImageLoaded);
    connect(worker, &LAUTiffViewerWorker::loadingProgress, this, &LAUTiffViewer::onLoadingProgress);
    connect(worker, &LAUTiffViewerWorker::loadingComplete, this, &LAUTiffViewer::onLoadingComplete);
    connect(this, &LAUTiffViewer::destroyed, worker, &LAUTiffViewerWorker::deleteLater);

    workerThread->start();

    // Setup playback timer
    playbackTimer = new QTimer(this);
    connect(playbackTimer, &QTimer::timeout, this, &LAUTiffViewer::onPlaybackTimer);

    setFocusPolicy(Qt::StrongFocus);

    // Load settings and previous state
    loadSettings();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUTiffViewer::LAUTiffViewer(const QString &filename, QWidget *parent)
    : LAUTiffViewer(parent)
{
    loadTiffFile(filename);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUTiffViewer::~LAUTiffViewer()
{
    // Save settings before destroying
    saveSettings();

    if (workerThread && workerThread->isRunning()) {
        workerThread->quit();
        workerThread->wait(3000);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
double LAUTiffViewer::getBoundingBoxXMin() const
{
    return xMinSpinBox ? xMinSpinBox->value() : -1000.0;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
double LAUTiffViewer::getBoundingBoxXMax() const
{
    return xMaxSpinBox ? xMaxSpinBox->value() : 1000.0;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
double LAUTiffViewer::getBoundingBoxYMin() const
{
    return yMinSpinBox ? yMinSpinBox->value() : -1000.0;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
double LAUTiffViewer::getBoundingBoxYMax() const
{
    return yMaxSpinBox ? yMaxSpinBox->value() : 1000.0;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
double LAUTiffViewer::getBoundingBoxZMin() const
{
    return zMinSpinBox ? zMinSpinBox->value() : 500.0;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
double LAUTiffViewer::getBoundingBoxZMax() const
{
    return zMaxSpinBox ? zMaxSpinBox->value() : 3000.0;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::setBoundingBoxValues(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
    // Check for NaN or infinity values and substitute defaults for UI display
    if (xMinSpinBox) {
        int value = (std::isnan(xMin) || std::isinf(xMin)) ? -1000 : (int)xMin;
        xMinSpinBox->setValue(value);
    }
    if (xMaxSpinBox) {
        int value = (std::isnan(xMax) || std::isinf(xMax)) ? 1000 : (int)xMax;
        xMaxSpinBox->setValue(value);
    }
    if (yMinSpinBox) {
        int value = (std::isnan(yMin) || std::isinf(yMin)) ? -1000 : (int)yMin;
        yMinSpinBox->setValue(value);
    }
    if (yMaxSpinBox) {
        int value = (std::isnan(yMax) || std::isinf(yMax)) ? 1000 : (int)yMax;
        yMaxSpinBox->setValue(value);
    }
    if (zMinSpinBox) {
        int value = (std::isnan(zMin) || std::isinf(zMin)) ? -1000 : (int)zMin;
        zMinSpinBox->setValue(value);
    }
    if (zMaxSpinBox) {
        int value = (std::isnan(zMax) || std::isinf(zMax)) ? 1000 : (int)zMax;
        zMaxSpinBox->setValue(value);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LookUpTableBoundingBox LAUTiffViewer::getBoundingBox() const
{
    LookUpTableBoundingBox bbox;
    bbox.xMin = getBoundingBoxXMin();
    bbox.xMax = getBoundingBoxXMax();
    bbox.yMin = getBoundingBoxYMin();
    bbox.yMax = getBoundingBoxYMax();
    bbox.zMin = getBoundingBoxZMin();
    bbox.zMax = getBoundingBoxZMax();
    return bbox;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::setBoundingBox(const LookUpTableBoundingBox &bbox)
{
    setBoundingBoxValues(bbox.xMin, bbox.xMax, bbox.yMin, bbox.yMax, bbox.zMin, bbox.zMax);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::setupUI()
{
    // Create main layout
    mainLayout = new QVBoxLayout(this);

    // Create scroll area for image display
    scrollArea = new QScrollArea();
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setWidgetResizable(true);

    imageLabel = new QLabel();
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(200, 200);
    imageLabel->setStyleSheet("QLabel { background-color: #2b2b2b; color: white; }");
    imageLabel->setText("No image loaded");
    scrollArea->setWidget(imageLabel);

    // File controls
    QHBoxLayout *fileLayout = new QHBoxLayout();
    openFileButton = new QPushButton("Open TIFF File");
    fileInfoLabel = new QLabel("No file loaded");
    fileInfoLabel->setStyleSheet("QLabel { color: #666; }");

    fileLayout->addWidget(openFileButton);
    fileLayout->addWidget(fileInfoLabel);
    fileLayout->addStretch();

    // Directory navigation controls
    QGroupBox *navGroup = new QGroupBox("Directory Navigation");
    QHBoxLayout *navLayout = new QHBoxLayout(navGroup);

    directoryLabel = new QLabel("Directory:");
    directorySlider = new QSlider(Qt::Horizontal);
    directorySlider->setMinimum(0);
    directorySlider->setMaximum(0);
    directorySlider->setValue(0);

    directorySpinBox = new QSpinBox();
    directorySpinBox->setMinimum(0);
    directorySpinBox->setMaximum(0);
    directorySpinBox->setValue(0);
    directorySpinBox->setSuffix(" / 0");

    navLayout->addWidget(directoryLabel);
    navLayout->addWidget(directorySlider, 1);
    navLayout->addWidget(directorySpinBox);

    // Playback controls
    playbackGroup = new QGroupBox("Playback Controls");
    playbackLayout = new QHBoxLayout(playbackGroup);

    playButton = new QPushButton("Play");
    stopButton = new QPushButton("Stop");
    frameRateLabel = new QLabel("FPS:");
    frameRateSpinBox = new QSpinBox();
    frameRateSpinBox->setMinimum(1);
    frameRateSpinBox->setMaximum(60);
    frameRateSpinBox->setValue(10);

    playbackLayout->addWidget(playButton);
    playbackLayout->addWidget(stopButton);
    playbackLayout->addWidget(frameRateLabel);
    playbackLayout->addWidget(frameRateSpinBox);
    playbackLayout->addStretch();

    // Zoom controls
    zoomGroup = new QGroupBox("Zoom & View");
    zoomLayout = new QHBoxLayout(zoomGroup);

    zoomComboBox = new QComboBox();
    zoomComboBox->addItems({"25%", "50%", "75%", "100%", "125%", "150%", "200%", "400%", "Fit to Window"});
    zoomComboBox->setCurrentText("100%");

    fitToWindowButton = new QPushButton("Fit to Window");
    actualSizeButton = new QPushButton("Actual Size");

    zoomLayout->addWidget(new QLabel("Zoom:"));
    zoomLayout->addWidget(zoomComboBox);
    zoomLayout->addWidget(fitToWindowButton);
    zoomLayout->addWidget(actualSizeButton);
    zoomLayout->addStretch();

    // Display options
    displayGroup = new QGroupBox("Display Options");
    QHBoxLayout *displayLayout = new QHBoxLayout(displayGroup);

    autoContrastCheckBox = new QCheckBox("Auto Contrast");

    // Camera selection
    cameraLabel = new QLabel("Camera:");
    cameraComboBox = new QComboBox();
    cameraComboBox->setEnabled(false);

    imageInfoLabel = new QLabel("");
    imageInfoLabel->setStyleSheet("QLabel { color: #666; }");

    displayLayout->addWidget(autoContrastCheckBox);
    displayLayout->addWidget(cameraLabel);
    displayLayout->addWidget(cameraComboBox);
    displayLayout->addWidget(imageInfoLabel);
    displayLayout->addStretch();

    // Bounding box controls
    boundingBoxGroup = new QGroupBox("Bounding Box");
    QGridLayout *bboxLayout = new QGridLayout(boundingBoxGroup);

    // Create spinboxes for bounding box
    xMinSpinBox = new QSpinBox();
    xMinSpinBox->setRange(-10000, 10000);
    xMinSpinBox->setValue(-1000);
    xMinSpinBox->setSuffix(" mm");

    xMaxSpinBox = new QSpinBox();
    xMaxSpinBox->setRange(-10000, 10000);
    xMaxSpinBox->setValue(1000);
    xMaxSpinBox->setSuffix(" mm");

    yMinSpinBox = new QSpinBox();
    yMinSpinBox->setRange(-10000, 10000);
    yMinSpinBox->setValue(-1000);
    yMinSpinBox->setSuffix(" mm");

    yMaxSpinBox = new QSpinBox();
    yMaxSpinBox->setRange(-10000, 10000);
    yMaxSpinBox->setValue(1000);
    yMaxSpinBox->setSuffix(" mm");

    zMinSpinBox = new QSpinBox();
    zMinSpinBox->setRange(-10000, 10000);
    zMinSpinBox->setValue(500);
    zMinSpinBox->setSuffix(" mm");

    zMaxSpinBox = new QSpinBox();
    zMaxSpinBox->setRange(-10000, 10000);
    zMaxSpinBox->setValue(3000);
    zMaxSpinBox->setSuffix(" mm");

    // Add to grid layout
    bboxLayout->addWidget(new QLabel("X Min:"), 0, 0);
    bboxLayout->addWidget(xMinSpinBox, 0, 1);
    bboxLayout->addWidget(new QLabel("X Max:"), 0, 2);
    bboxLayout->addWidget(xMaxSpinBox, 0, 3);

    bboxLayout->addWidget(new QLabel("Y Min:"), 1, 0);
    bboxLayout->addWidget(yMinSpinBox, 1, 1);
    bboxLayout->addWidget(new QLabel("Y Max:"), 1, 2);
    bboxLayout->addWidget(yMaxSpinBox, 1, 3);

    bboxLayout->addWidget(new QLabel("Z Min:"), 2, 0);
    bboxLayout->addWidget(zMinSpinBox, 2, 1);
    bboxLayout->addWidget(new QLabel("Z Max:"), 2, 2);
    bboxLayout->addWidget(zMaxSpinBox, 2, 3);

    // Initially disable bounding box controls
    boundingBoxGroup->setEnabled(false);

    // Progress bar
    loadingProgressBar = new QProgressBar();
    loadingProgressBar->setVisible(false);

    // Add everything to main layout
    mainLayout->addLayout(fileLayout);
    mainLayout->addWidget(scrollArea, 1);
    mainLayout->addWidget(navGroup);
    mainLayout->addWidget(playbackGroup);
    mainLayout->addWidget(zoomGroup);
    mainLayout->addWidget(displayGroup);
    mainLayout->addWidget(boundingBoxGroup);
    mainLayout->addWidget(loadingProgressBar);

    // Connect signals
    connect(openFileButton, &QPushButton::clicked, this, &LAUTiffViewer::onOpenFileClicked);
    connect(directorySlider, &QSlider::valueChanged, this, &LAUTiffViewer::onDirectoryChanged);
    connect(directorySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &LAUTiffViewer::onDirectoryChanged);
    connect(playButton, &QPushButton::clicked, this, &LAUTiffViewer::onPlayButtonClicked);
    connect(stopButton, &QPushButton::clicked, this, &LAUTiffViewer::onStopButtonClicked);
    connect(frameRateSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &LAUTiffViewer::onFrameRateChanged);
    connect(zoomComboBox, &QComboBox::currentTextChanged, this, &LAUTiffViewer::onZoomChanged);
    connect(fitToWindowButton, &QPushButton::clicked, this, &LAUTiffViewer::onFitToWindowClicked);
    connect(actualSizeButton, &QPushButton::clicked, this, &LAUTiffViewer::onActualSizeClicked);
    connect(autoContrastCheckBox, &QCheckBox::toggled, this, &LAUTiffViewer::onAutoContrastToggled);
    connect(cameraComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LAUTiffViewer::onCameraChanged);

    // Connect bounding box controls
    connect(xMinSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &LAUTiffViewer::onBoundingBoxChanged);
    connect(xMaxSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &LAUTiffViewer::onBoundingBoxChanged);
    connect(yMinSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &LAUTiffViewer::onBoundingBoxChanged);
    connect(yMaxSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &LAUTiffViewer::onBoundingBoxChanged);
    connect(zMinSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &LAUTiffViewer::onBoundingBoxChanged);
    connect(zMaxSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &LAUTiffViewer::onBoundingBoxChanged);

    // Initially disable controls
    updateControls();
}

// Rest of the implementation remains the same as the original,
// just copy all the other methods from the original lautiffviewer.cpp
// The key additions are the new bounding box getter/setter methods above

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUTiffViewer::loadTiffFile(const QString &filename)
{
    if (filename.isEmpty()) {
        return false;
    }

    // Check if file exists
    if (!QFile::exists(filename)) {
        QMessageBox::warning(this, "Error", "File does not exist: " + filename);
        return false;
    }

    // Get number of directories
    int dirs = LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(filename);
    if (dirs <= 0) {
        QMessageBox::warning(this, "Error", "Could not read TIFF file or file has no directories: " + filename);
        return false;
    }

    // Stop any current playback
    stop();

    // Clear cache
    QMutexLocker locker(&cacheMutex);
    imageCache.clear();
    locker.unlock();

    // Clear scan cache
    QMutexLocker scanLocker(&scanCacheMutex);
    scanCache.clear();
    scanLocker.unlock();

    // Update member variables
    currentFilename = filename;
    numDirectories = dirs;
    currentDir = 0;
    currentCam = 0;

    // Calculate number of cameras
    calculateCameraCount();

    // Update UI
    directorySlider->setMaximum(numDirectories - 1);
    directorySpinBox->setMaximum(numDirectories - 1);
    directorySpinBox->setSuffix(QString(" / %1").arg(numDirectories - 1));

    // Update file info
    QFileInfo fileInfo(filename);
    fileInfoLabel->setText(QString("%1 (%2 directories, %3 cameras)").arg(fileInfo.fileName()).arg(numDirectories).arg(numCameras));

    // Setup worker with new filename
    worker->setFilename(filename);

    // Load first image
    loadImageAsync(0);

    // Enable controls
    updateControls();

    // Save the newly loaded file to settings
    QSettings settings;
    settings.setValue("LAUTiffViewer/lastOpenedFile", filename);

    emit fileLoaded(filename);
    return true;
}

// Continue with the rest of the implementation...
// I'll add the remaining methods from the original file with minimal changes
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUTiffViewer::loadLookupTables(const QString &lutxFilename)
{
    try {
        // Load lookup tables from LUTX file
        lookupTables = LAULookUpTable::LAULookUpTableX(lutxFilename);

        if (lookupTables.isEmpty()) {
            QMessageBox::warning(this, "Error", "No lookup tables found in file: " + lutxFilename);
            return false;
        }

        // Verify we have enough lookup tables for our cameras
        if (lookupTables.size() < numCameras) {
            QMessageBox::warning(this, "Warning",
                                 QString("Lookup table file contains %1 tables but %2 cameras detected. Using available tables.")
                                     .arg(lookupTables.size()).arg(numCameras));
        }

        currentLutxFilename = lutxFilename;
        useLookupTables = true;

        // Clear scan cache as lookup tables have changed
        QMutexLocker scanLocker(&scanCacheMutex);
        scanCache.clear();
        scanLocker.unlock();

        // Enable bounding box controls
        boundingBoxGroup->setEnabled(true);

        // Refresh current display
        updateImageDisplay();

        qDebug() << "Loaded" << lookupTables.size() << "lookup tables from" << lutxFilename;
        return true;

    } catch (...) {
        QMessageBox::warning(this, "Error", "Failed to load lookup tables from: " + lutxFilename);
        return false;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUTiffViewer::convertMemoryObjectToScan(const LAUMemoryObject &object, const LAULookUpTable &table)
{
    // Default behavior: apply transform
    return convertMemoryObjectToScan(object, table, true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUTiffViewer::convertMemoryObjectToScan(const LAUMemoryObject &object, const LAULookUpTable &table, bool applyTransform)
{
    LAUScan scan(object.width(), object.height(), LAU3DVideoParameters::ColorXYZG);
    float zMin = qMin(table.zLimits().x(), table.zLimits().y());
    float zMax = qMax(table.zLimits().x(), table.zLimits().y());
    for (unsigned int row = 0; row < object.height(); row++){
        unsigned short *inBuffer = (unsigned short*)object.constScanLine(row);
        float *otBuffer = (float*)scan.scanLine(row);
        for (unsigned int col = 0; col < object.width(); col++){
            float *lutVector = (float*)table.constScanLine(row) + (table.colors() * col);
            float pixel = inBuffer[col] / 65535.0f;
            float z = (lutVector[4] * pow(pixel, 4.0)) + (lutVector[5] * pow(pixel, 3.0)) + (lutVector[6] * pow(pixel, 2.0)) + (lutVector[7] * pixel) + lutVector[8];
            if (z <= zMin || z >= zMax){
                z = NAN;
            }
            float x = (lutVector[0] * z) + lutVector[1];
            float y = (lutVector[2] * z) + lutVector[3];
            otBuffer[4*col + 0] = x;
            otBuffer[4*col + 1] = y;
            otBuffer[4*col + 2] = z;
            otBuffer[4*col + 3] = pixel;
        }
    }
    // Apply transform only if requested
    if (applyTransform) {
        scan.transformScanInPlace(table.transform());
    }
    scan.updateLimits();
    return(scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::generateCurrentScan()
{
    // Check if we have lookup tables and valid indices
    if (!useLookupTables || lookupTables.isEmpty() || currentCam >= lookupTables.size()) {
        return;
    }

    QPair<int,int> cacheKey(currentDir, currentCam);

    // Check if scan already exists in cache
    QMutexLocker scanLocker(&scanCacheMutex);
    if (scanCache.contains(cacheKey)) {
        scanLocker.unlock();
        return; // Already generated
    }
    scanLocker.unlock();

    // Get the full image from image cache
    QMutexLocker locker(&cacheMutex);
    if (!imageCache.contains(currentDir)) {
        locker.unlock();
        return; // No image available
    }

    LAUMemoryObject fullImage = imageCache[currentDir];
    locker.unlock();

    // Extract camera ROI
    LAUMemoryObject cameraImage = extractCameraROI(fullImage, currentCam);
    if (!cameraImage.isValid()) {
        return;
    }

    // Convert to scan using lookup table
    LAUScan scan = convertMemoryObjectToScan(cameraImage, lookupTables[currentCam]);

    // Cache the generated scan
    scanLocker.relock();
    scanCache[cacheKey] = scan;
    scanLocker.unlock();

    qDebug() << "Generated scan for directory" << currentDir << "camera" << currentCam;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUTiffViewer::getCurrentScan()
{
    QPair<int,int> cacheKey(currentDir, currentCam);

    QMutexLocker scanLocker(&scanCacheMutex);
    if (scanCache.contains(cacheKey)) {
        return scanCache[cacheKey];
    }
    scanLocker.unlock();

    // Generate scan if not in cache
    generateCurrentScan();

    // Try again
    scanLocker.relock();
    if (scanCache.contains(cacheKey)) {
        return scanCache[cacheKey];
    }

    // Return empty scan if generation failed
    return LAUScan();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QPixmap LAUTiffViewer::scanToPixmap(const LAUScan &scan)
{
    if (!scan.isValid()) {
        return QPixmap();
    }

    // Get the original camera image for display
    QMutexLocker locker(&cacheMutex);
    if (!imageCache.contains(currentDir)) {
        locker.unlock();
        return QPixmap();
    }

    LAUMemoryObject fullImage = imageCache[currentDir];
    locker.unlock();

    // Extract camera ROI from original image
    LAUMemoryObject cameraImage = extractCameraROI(fullImage, currentCam);
    if (!cameraImage.isValid()) {
        return QPixmap();
    }

    // Convert original camera image to QImage
    QImage originalImage = cameraImage.toImage();
    if (originalImage.isNull()) {
        return QPixmap();
    }

    // Convert to ARGB32 format to add alpha channel
    QImage imageWithAlpha = originalImage.convertToFormat(QImage::Format_ARGB32);

    // Get bounding box values
    float xMin = (float)xMinSpinBox->value();
    float xMax = (float)xMaxSpinBox->value();
    float yMin = (float)yMinSpinBox->value();
    float yMax = (float)yMaxSpinBox->value();
    float zMin = (float)zMinSpinBox->value();
    float zMax = (float)zMaxSpinBox->value();

    // Apply alpha channel based on 3D scan bounding box
    if (scan.color() == ColorXYZ || scan.color() == ColorXYZW || scan.color() == ColorXYZG) {
        for (unsigned int row = 0; row < scan.height() && row < (unsigned int)imageWithAlpha.height(); row++) {
            float *scanLine = (float*)scan.constScanLine(row);
            unsigned char *imageLine = imageWithAlpha.scanLine(row);

            for (unsigned int col = 0; col < scan.width() && col < (unsigned int)imageWithAlpha.width(); col++) {
                float x = scanLine[col * scan.colors() + 0];
                float y = scanLine[col * scan.colors() + 1];
                float z = scanLine[col * scan.colors() + 2];

                // Check if point is inside bounding box
                if (x >= xMin && x <= xMax && y >= yMin && y <= yMax && z >= zMin && z <= zMax) {
                    // Point is inside bounding box - keep original alpha (or set to full opacity)
                    imageLine[4*col+3] = 0xFF;  // Set alpha to 255 (fully opaque)
                } else {
                    // Point is outside bounding box - set alpha to 0 (fully transparent)
                    imageLine[4*col+3] = 0x00;  // Set alpha to 0 (fully transparent)
                }
            }
        }
    }

    // Apply auto contrast to original image if enabled
    if (autoContrastEnabled) {
        applyAutoContrast(imageWithAlpha);
    }

    return QPixmap::fromImage(imageWithAlpha);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onBoundingBoxChanged()
{
    // Save bounding box settings
    QSettings settings;
    settings.setValue("LAUTiffViewer/xMin", xMinSpinBox->value());
    settings.setValue("LAUTiffViewer/xMax", xMaxSpinBox->value());
    settings.setValue("LAUTiffViewer/yMin", yMinSpinBox->value());
    settings.setValue("LAUTiffViewer/yMax", yMaxSpinBox->value());
    settings.setValue("LAUTiffViewer/zMin", zMinSpinBox->value());
    settings.setValue("LAUTiffViewer/zMax", zMaxSpinBox->value());

    // Refresh display with new bounding box
    updateImageDisplay();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::calculateCameraCount()
{
    if (currentFilename.isEmpty()) {
        numCameras = 0;
        return;
    }

    // Get image dimensions from first directory
    int imageHeight = LAUMemoryObject::howManyRowsDoesThisTiffFileHave(currentFilename, 0);
    if (imageHeight <= 0) {
        numCameras = 0;
        return;
    }

    // Calculate number of cameras (ignore excess rows)
    numCameras = imageHeight / CAMERA_HEIGHT_PIXELS;

    // Update camera combo box
    cameraComboBox->blockSignals(true);
    cameraComboBox->clear();

    for (int i = 0; i < numCameras; i++) {
        cameraComboBox->addItem(QString("Camera %1").arg(i));
    }

    cameraComboBox->setCurrentIndex(0);
    cameraComboBox->setEnabled(numCameras > 1);
    cameraComboBox->blockSignals(false);

    qDebug() << "Detected" << numCameras << "cameras in TIFF file (image height:" << imageHeight << ")";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUTiffViewer::extractCameraROI(const LAUMemoryObject &fullImage, int cameraIndex)
{
    if (!fullImage.isValid() || cameraIndex < 0 || cameraIndex >= numCameras) {
        return LAUMemoryObject();
    }

    // Calculate ROI for the specified camera
    int startRow = cameraIndex * CAMERA_HEIGHT_PIXELS;
    QRect cameraROI(0, startRow, fullImage.width(), CAMERA_HEIGHT_PIXELS);

    // Extract the camera region
    LAUMemoryObject cameraImage = fullImage.crop(cameraROI);

    // Copy metadata from original image
    cameraImage.setXML(fullImage.xml());
    cameraImage.setRFID(fullImage.rfid());
    cameraImage.setTransform(fullImage.transform());
    cameraImage.setProjection(fullImage.projection());
    cameraImage.setAnchor(fullImage.anchor());
    cameraImage.setElapsed(fullImage.elapsed());
    cameraImage.setJetr(fullImage.jetr());

    return cameraImage;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::setCurrentCamera(int camera)
{
    if (camera < 0 || camera >= numCameras) {
        return;
    }

    if (camera != currentCam) {
        currentCam = camera;

        // Update UI control without triggering signals
        cameraComboBox->blockSignals(true);
        cameraComboBox->setCurrentIndex(camera);
        cameraComboBox->blockSignals(false);

        // Refresh display with new camera
        updateImageDisplay();

        // Save camera selection
        QSettings settings;
        settings.setValue("LAUTiffViewer/lastCamera", camera);

        emit cameraChanged(camera);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::setCurrentDirectory(int directory)
{
    if (directory < 0 || directory >= numDirectories) {
        return;
    }

    if (directory != currentDir) {
        currentDir = directory;

        // Update UI controls without triggering signals
        directorySlider->blockSignals(true);
        directorySpinBox->blockSignals(true);

        directorySlider->setValue(directory);
        directorySpinBox->setValue(directory);

        directorySlider->blockSignals(false);
        directorySpinBox->blockSignals(false);

        // Load new image
        loadImageAsync(directory);

        // Save directory selection
        QSettings settings;
        settings.setValue("LAUTiffViewer/lastDirectory", directory);

        emit directoryChanged(directory);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::loadImageAsync(int directory)
{
    if (currentFilename.isEmpty() || directory < 0 || directory >= numDirectories) {
        return;
    }

    // Check cache first
    QMutexLocker locker(&cacheMutex);
    if (imageCache.contains(directory)) {
        LAUMemoryObject cachedImage = imageCache[directory];
        locker.unlock();
        onImageLoaded(cachedImage, directory);
        return;
    }
    locker.unlock();

    // Load asynchronously using worker
    QMetaObject::invokeMethod(worker, "loadImage", Qt::QueuedConnection, Q_ARG(int, directory));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onImageLoaded(const LAUMemoryObject &image, int directory)
{
    if (directory != currentDir) {
        return; // Image is for a different directory
    }

    if (!image.isValid()) {
        imageLabel->setText("Failed to load image");
        imageInfoLabel->setText("");
        return;
    }

    // Extract camera ROI from the full image
    LAUMemoryObject cameraImage = extractCameraROI(image, currentCam);
    if (!cameraImage.isValid()) {
        imageLabel->setText("Failed to extract camera ROI");
        imageInfoLabel->setText("");
        return;
    }

    // Cache the full image (not just the camera ROI)
    QMutexLocker locker(&cacheMutex);
    if (imageCache.size() >= MAX_CACHE_SIZE) {
        // Remove oldest entry (simple strategy)
        auto it = imageCache.begin();
        imageCache.erase(it);
    }
    imageCache[directory] = image;
    locker.unlock();

    // Convert camera image to pixmap and display
    QPixmap pixmap = memoryObjectToPixmap(cameraImage);
    if (!pixmap.isNull()) {
        updateImageDisplay();

        // Update image info (show both full image and camera info)
        QString info = QString("Full: %1x%2, Camera %3: %4x%5, Channels: %6, Depth: %7 bytes")
                           .arg(image.width())
                           .arg(image.height())
                           .arg(currentCam)
                           .arg(cameraImage.width())
                           .arg(cameraImage.height())
                           .arg(cameraImage.colors())
                           .arg(cameraImage.depth());
        imageInfoLabel->setText(info);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QPixmap LAUTiffViewer::memoryObjectToPixmap(const LAUMemoryObject &memObj)
{
    if (!memObj.isValid()) {
        return QPixmap();
    }

    QImage image = memObj.toImage();
    if (image.isNull()) {
        return QPixmap();
    }

    // Always apply inverted contrast for better visibility
    applyAutoContrast(image);

    return QPixmap::fromImage(image);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::applyAutoContrast(QImage &image)
{
    // Simple auto contrast implementation
    if (image.format() != QImage::Format_RGB888 && image.format() != QImage::Format_Grayscale8) {
        return;
    }

    int minVal = 255, maxVal = 0;

    // Find min and max values
    for (int y = 0; y < image.height(); ++y) {
        const uchar *line = image.constScanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            if (image.format() == QImage::Format_Grayscale8) {
                int val = line[x];
                minVal = qMin(minVal, val);
                maxVal = qMax(maxVal, val);
            } else {
                for (int c = 0; c < 3; ++c) {
                    int val = line[x * 3 + c];
                    minVal = qMin(minVal, val);
                    maxVal = qMax(maxVal, val);
                }
            }
        }
    }

    if (maxVal <= minVal) return;

    // Apply inverted contrast stretch with 95% gray maximum (242 instead of 255)
    double scale = 242.0 / (maxVal - minVal);  // 95% of 255 is ~242
    for (int y = 0; y < image.height(); ++y) {
        uchar *line = image.scanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            if (image.format() == QImage::Format_Grayscale8) {
                // Invert: high values become low, scale to max 242
                int inverted = 242 - (int)((line[x] - minVal) * scale);
                line[x] = qBound(0, inverted, 242);
            } else {
                for (int c = 0; c < 3; ++c) {
                    // Invert: high values become low, scale to max 242
                    int inverted = 242 - (int)((line[x * 3 + c] - minVal) * scale);
                    line[x * 3 + c] = qBound(0, inverted, 242);
                }
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::applyAutoContrastWithAlpha(QImage &image)
{
    // Auto contrast implementation for ARGB32 format that only processes visible pixels
    if (image.format() != QImage::Format_ARGB32) {
        return;
    }

    int minVal = 255, maxVal = 0;

    // Find min and max values only for visible (non-transparent) pixels
    for (int y = 0; y < image.height(); ++y) {
        const unsigned char *line = image.constScanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            // Check if pixel is visible (alpha > 0)
            if (line[x * 4 + 3] > 0) {
                // Process RGB channels
                for (int c = 0; c < 3; ++c) {
                    int val = line[x * 4 + c];
                    minVal = qMin(minVal, val);
                    maxVal = qMax(maxVal, val);
                }
            }
        }
    }

    if (maxVal <= minVal) return;

    // Apply inverted contrast stretch with 95% gray maximum only to visible pixels
    double scale = 242.0 / (maxVal - minVal);  // 95% of 255 is ~242
    for (int y = 0; y < image.height(); ++y) {
        unsigned char *line = image.scanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            // Only process visible pixels (alpha > 0)
            if (line[x * 4 + 3] > 0) {
                // Apply inverted contrast stretch to RGB channels, leave alpha unchanged
                for (int c = 0; c < 3; ++c) {
                    // Invert: high values become low, scale to max 242
                    int inverted = 242 - (int)((line[x * 4 + c] - minVal) * scale);
                    line[x * 4 + c] = qBound(0, inverted, 242);
                }
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::updateImageDisplay()
{
    if (useLookupTables && !lookupTables.isEmpty() && currentCam < lookupTables.size()) {
        // Use scan-based rendering
        generateCurrentScan();
        LAUScan currentScan = getCurrentScan();

        if (currentScan.isValid()) {
            QPixmap pixmap = scanToPixmap(currentScan);
            if (!pixmap.isNull()) {
                // Apply zoom
                if (currentZoom != 1.0) {
                    QSize scaledSize = pixmap.size() * currentZoom;
                    pixmap = pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }

                imageLabel->setPixmap(pixmap);
                imageLabel->resize(pixmap.size());
                return;
            }
        }
    }

    // Fall back to memory object rendering
    QMutexLocker locker(&cacheMutex);
    if (!imageCache.contains(currentDir)) {
        locker.unlock();
        return;
    }

    LAUMemoryObject fullImage = imageCache[currentDir];
    locker.unlock();

    // Extract camera ROI
    LAUMemoryObject cameraImage = extractCameraROI(fullImage, currentCam);
    if (!cameraImage.isValid()) {
        return;
    }

    QPixmap pixmap = memoryObjectToPixmap(cameraImage);
    if (pixmap.isNull()) {
        return;
    }

    // Apply zoom
    if (currentZoom != 1.0) {
        QSize scaledSize = pixmap.size() * currentZoom;
        pixmap = pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    imageLabel->setPixmap(pixmap);
    imageLabel->resize(pixmap.size());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::updateControls()
{
    bool hasFile = !currentFilename.isEmpty() && numDirectories > 0;

    directorySlider->setEnabled(hasFile);
    directorySpinBox->setEnabled(hasFile);
    playButton->setEnabled(hasFile && numDirectories > 1);
    stopButton->setEnabled(hasFile);
    frameRateSpinBox->setEnabled(hasFile);
    zoomComboBox->setEnabled(hasFile);
    fitToWindowButton->setEnabled(hasFile);
    actualSizeButton->setEnabled(hasFile);
    autoContrastCheckBox->setEnabled(hasFile);
    cameraComboBox->setEnabled(hasFile && numCameras > 1);
    boundingBoxGroup->setEnabled(hasFile && useLookupTables);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onDirectoryChanged(int directory)
{
    setCurrentDirectory(directory);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onCameraChanged(int camera)
{
    setCurrentCamera(camera);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onPlayButtonClicked()
{
    if (playing) {
        pause();
    } else {
        play();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onStopButtonClicked()
{
    stop();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::play()
{
    if (numDirectories <= 1) return;

    playing = true;
    playButton->setText("Pause");

    int fps = frameRateSpinBox->value();
    playbackTimer->start(1000 / fps);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::pause()
{
    playing = false;
    playButton->setText("Play");
    playbackTimer->stop();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::stop()
{
    pause();
    setCurrentDirectory(0);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onPlaybackTimer()
{
    if (!playing) return;

    int nextDir = currentDir + 1;
    if (nextDir >= numDirectories) {
        nextDir = 0; // Loop back to beginning
    }
    setCurrentDirectory(nextDir);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onFrameRateChanged(int fps)
{
    if (playing) {
        playbackTimer->setInterval(1000 / fps);
    }

    // Save frame rate setting
    QSettings settings;
    settings.setValue("LAUTiffViewer/frameRate", fps);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onZoomChanged()
{
    QString zoomText = zoomComboBox->currentText();

    if (zoomText == "Fit to Window") {
        fitToWindow();
    } else {
        // Parse percentage
        zoomText.remove('%');
        bool ok;
        double zoom = zoomText.toDouble(&ok);
        if (ok) {
            setZoomFactor(zoom / 100.0);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::setZoomFactor(double factor)
{
    if (factor <= 0) return;

    currentZoom = factor;
    updateImageDisplay();

    // Save zoom setting
    QSettings settings;
    settings.setValue("LAUTiffViewer/lastZoom", factor);

    // Update combo box
    QString zoomText = QString("%1%").arg((int)(factor * 100));
    int index = zoomComboBox->findText(zoomText);
    if (index >= 0) {
        zoomComboBox->blockSignals(true);
        zoomComboBox->setCurrentIndex(index);
        zoomComboBox->blockSignals(false);
        // Save combo box selection too
        settings.setValue("LAUTiffViewer/zoomComboText", zoomText);
    }

    emit zoomChanged(factor);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onFitToWindowClicked()
{
    fitToWindow();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::fitToWindow()
{
    QMutexLocker locker(&cacheMutex);
    if (!imageCache.contains(currentDir)) {
        locker.unlock();
        return;
    }

    LAUMemoryObject fullImage = imageCache[currentDir];
    locker.unlock();

    // Extract camera ROI to get correct dimensions
    LAUMemoryObject cameraImage = extractCameraROI(fullImage, currentCam);
    if (!cameraImage.isValid()) return;

    QSize imageSize(cameraImage.width(), cameraImage.height());
    QSize viewportSize = scrollArea->viewport()->size();

    double scaleX = (double)viewportSize.width() / imageSize.width();
    double scaleY = (double)viewportSize.height() / imageSize.height();
    double scale = qMin(scaleX, scaleY);

    setZoomFactor(scale);

    zoomComboBox->blockSignals(true);
    zoomComboBox->setCurrentText("Fit to Window");
    zoomComboBox->blockSignals(false);

    // Save the fit to window selection
    QSettings settings;
    settings.setValue("LAUTiffViewer/zoomComboText", "Fit to Window");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onActualSizeClicked()
{
    actualSize();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::actualSize()
{
    setZoomFactor(1.0);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onAutoContrastToggled(bool enabled)
{
    autoContrastEnabled = enabled;
    updateImageDisplay();

    // Save auto contrast setting
    QSettings settings;
    settings.setValue("LAUTiffViewer/autoContrast", enabled);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onOpenFileClicked()
{
    QString filename;
    QSettings settings;
    QString directory = settings.value("LAUTiffViewer::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    filename = QFileDialog::getOpenFileName(this, QString("Open image from disk (*.tif)"), directory, QString("*.tif;*.tiff"));
    if (filename.isEmpty()) {
        return;
    }
    settings.setValue("LAUTiffViewer::lastUsedDirectory", QFileInfo(filename).absolutePath());
    loadTiffFile(filename);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onLoadingProgress(int directory)
{
    Q_UNUSED(directory)
    // Could show progress if loading multiple images
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::onLoadingComplete()
{
    loadingProgressBar->setVisible(false);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        if (currentDir > 0) {
            setCurrentDirectory(currentDir - 1);
        }
        break;
    case Qt::Key_Right:
        if (currentDir < numDirectories - 1) {
            setCurrentDirectory(currentDir + 1);
        }
        break;
    case Qt::Key_Home:
        setCurrentDirectory(0);
        break;
    case Qt::Key_End:
        setCurrentDirectory(numDirectories - 1);
        break;
    case Qt::Key_Space:
        onPlayButtonClicked();
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        setZoomFactor(currentZoom * 1.25);
        break;
    case Qt::Key_Minus:
        setZoomFactor(currentZoom / 1.25);
        break;
    case Qt::Key_0:
        actualSize();
        break;
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
    {
        int cameraIndex = event->key() - Qt::Key_1;
        if (cameraIndex < numCameras) {
            setCurrentCamera(cameraIndex);
        }
    }
    break;
    default:
        QWidget::keyPressEvent(event);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && useLookupTables && !lookupTables.isEmpty() && currentCam < lookupTables.size()) {
        // Get current scan from cache
        LAUScan cachedScan = getCurrentScan();

        if (cachedScan.isValid()) {
            // Create a copy with inverse transform to get back to original camera coordinates
            QMatrix4x4 inverseTransform = lookupTables[currentCam].transform().inverted();
            LAUScan originalScan = cachedScan.transformScan(inverseTransform);

            // Inspect the reverted scan
            originalScan.inspectImage();
        }
    }

    QWidget::mouseDoubleClickEvent(event);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom with Ctrl+Wheel
        double factor = event->angleDelta().y() > 0 ? 1.25 : 0.8;
        setZoomFactor(currentZoom * factor);
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // If we're in fit-to-window mode, recalculate zoom
    if (zoomComboBox->currentText() == "Fit to Window") {
        fitToWindow();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::saveSettings()
{
    QSettings settings;

    // Save current file and state
    settings.setValue("LAUTiffViewer/lastOpenedFile", currentFilename);
    settings.setValue("LAUTiffViewer/lastLutxFile", currentLutxFilename);
    settings.setValue("LAUTiffViewer/lastDirectory", currentDir);
    settings.setValue("LAUTiffViewer/lastCamera", currentCam);
    settings.setValue("LAUTiffViewer/lastZoom", currentZoom);
    settings.setValue("LAUTiffViewer/autoContrast", autoContrastEnabled);
    settings.setValue("LAUTiffViewer/frameRate", frameRateSpinBox->value());

    // Save bounding box values
    settings.setValue("LAUTiffViewer/xMin", xMinSpinBox->value());
    settings.setValue("LAUTiffViewer/xMax", xMaxSpinBox->value());
    settings.setValue("LAUTiffViewer/yMin", yMinSpinBox->value());
    settings.setValue("LAUTiffViewer/yMax", yMaxSpinBox->value());
    settings.setValue("LAUTiffViewer/zMin", zMinSpinBox->value());
    settings.setValue("LAUTiffViewer/zMax", zMaxSpinBox->value());

    // Save zoom combo box selection
    settings.setValue("LAUTiffViewer/zoomComboText", zoomComboBox->currentText());

    // Save playback state
    settings.setValue("LAUTiffViewer/wasPlaying", playing);

    qDebug() << "LAUTiffViewer: Settings saved";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::loadSettings()
{
    QSettings settings;

    // Load UI preferences
    double savedZoom = settings.value("LAUTiffViewer/lastZoom", 1.0).toDouble();
    bool savedAutoContrast = settings.value("LAUTiffViewer/autoContrast", false).toBool();
    int savedFrameRate = settings.value("LAUTiffViewer/frameRate", 10).toInt();
    QString savedZoomText = settings.value("LAUTiffViewer/zoomComboText", "100%").toString();

    // Load bounding box values
    int savedXMin = settings.value("LAUTiffViewer/xMin", -1000).toInt();
    int savedXMax = settings.value("LAUTiffViewer/xMax", 1000).toInt();
    int savedYMin = settings.value("LAUTiffViewer/yMin", -1000).toInt();
    int savedYMax = settings.value("LAUTiffViewer/yMax", 1000).toInt();
    int savedZMin = settings.value("LAUTiffViewer/zMin", 500).toInt();
    int savedZMax = settings.value("LAUTiffViewer/zMax", 3000).toInt();

    // Apply settings to UI
    autoContrastEnabled = savedAutoContrast;
    autoContrastCheckBox->setChecked(savedAutoContrast);

    frameRateSpinBox->setValue(savedFrameRate);

    // Set bounding box values
    xMinSpinBox->setValue(savedXMin);
    xMaxSpinBox->setValue(savedXMax);
    yMinSpinBox->setValue(savedYMin);
    yMaxSpinBox->setValue(savedYMax);
    zMinSpinBox->setValue(savedZMin);
    zMaxSpinBox->setValue(savedZMax);

    // Set zoom combo box
    int zoomIndex = zoomComboBox->findText(savedZoomText);
    if (zoomIndex >= 0) {
        zoomComboBox->setCurrentIndex(zoomIndex);
    }
    currentZoom = savedZoom;

    // // Try to load the last LUTX file first
    // QString lastLutxFile = settings.value("LAUTiffViewer/lastLutxFile", QString()).toString();
    // if (!lastLutxFile.isEmpty() && QFile::exists(lastLutxFile)) {
    //     loadLookupTables(lastLutxFile);
    // }

    // // Try to load the last opened file
    // QString lastFile = settings.value("LAUTiffViewer/lastOpenedFile", QString()).toString();
    // if (!lastFile.isEmpty() && QFile::exists(lastFile)) {
    //     if (loadTiffFile(lastFile)) {
    //         // Restore directory and camera
    //         int savedDirectory = settings.value("LAUTiffViewer/lastDirectory", 0).toInt();
    //         int savedCamera = settings.value("LAUTiffViewer/lastCamera", 0).toInt();

    //         // Restore directory (with bounds checking)
    //         if (savedDirectory >= 0 && savedDirectory < numDirectories) {
    //             setCurrentDirectory(savedDirectory);
    //         }

    //         // Restore camera (with bounds checking)
    //         if (savedCamera >= 0 && savedCamera < numCameras) {
    //             setCurrentCamera(savedCamera);
    //         }

    //         // Restore zoom
    //         setZoomFactor(savedZoom);

    //         // Check if we should restore playback state
    //         bool wasPlaying = settings.value("LAUTiffViewer/wasPlaying", false).toBool();
    //         if (wasPlaying && numDirectories > 1) {
    //             // Small delay to ensure everything is loaded before starting playback
    //             QTimer::singleShot(500, this, &LAUTiffViewer::play);
    //         }
    //     }
    // }

    qDebug() << "LAUTiffViewer: Settings loaded";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::saveViewerState()
{
    // This method can be called when significant state changes occur
    saveSettings();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewer::loadViewerState()
{
    // This method can be called to reload state from settings
    loadSettings();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUTiffViewerWorker::LAUTiffViewerWorker(QObject *parent)
    : QObject(parent)
{
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewerWorker::setFilename(const QString &filename)
{
    this->filename = filename;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewerWorker::loadImage(int directory)
{
    if (filename.isEmpty()) {
        emit imageLoaded(LAUMemoryObject(), directory);
        return;
    }

    try {
        LAUMemoryObject image(filename, directory);
        emit imageLoaded(image, directory);
    } catch (...) {
        qDebug() << "Failed to load directory" << directory << "from" << filename;
        emit imageLoaded(LAUMemoryObject(), directory);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewerWorker::preloadRange(int start, int end)
{
    if (filename.isEmpty()) {
        emit loadingComplete();
        return;
    }

    for (int i = start; i <= end; ++i) {
        try {
            LAUMemoryObject image(filename, i);
            emit imageLoaded(image, i);
            emit loadingProgress(i);
        } catch (...) {
            qDebug() << "Failed to preload directory" << i << "from" << filename;
            emit imageLoaded(LAUMemoryObject(), i);
        }
    }

    emit loadingComplete();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewerWorker::preloadImages(int startDir, int endDir)
{
    QMetaObject::invokeMethod(this, "preloadRange", Qt::QueuedConnection,
                              Q_ARG(int, startDir), Q_ARG(int, endDir));
}
