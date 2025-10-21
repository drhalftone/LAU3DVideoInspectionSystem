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

#include "lau3dmultisensorvideowidget.h"
#include "lauconstants.h"
#include "laucameraconnectiondialog.h"

#ifdef ORBBEC
#include "lauorbbeccamera.h"
#endif

#ifdef LUCID
#include "laulucidcamera.h"
#endif

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QTimer>
#include <QProcess>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DMultiSensorVideoWidget::LAU3DMultiSensorVideoWidget(QList<LAUVideoPlaybackDevice> devices, LAUVideoPlaybackColor color, QWidget *parent) : QWidget(parent), sensorCount(0), playbackColor(color), glWidget(nullptr), cameraIndex(0), fpsCounter(0), connectionDialog(nullptr), currentFps(0.0f), fpsWarningShown(false), savingBackground(false)
{
    this->setLayout(new QVBoxLayout());
    this->setWindowTitle(QString("Multi-Sensor Video Widget"));
    this->layout()->setContentsMargins(0, 0, 0, 0);

    // SHOW PROGRESS DIALOG DURING CAMERA CONNECTION
    // This is especially important for Lucid cameras which can take 30-60 seconds for PTP sync
    // The dialog will stay visible until we receive the first valid frame buffers
    connectionDialog = new LAUCameraConnectionDialog(parent);
    connectionDialog->installMessageHandler();
    connectionDialog->show();
    QApplication::processEvents();  // Ensure dialog is visible

    // BUILD LIST OF CAMERA INSTANCES, MIMICKING LAU3DVIDEORECORDER PATTERN
    // CREATE ONE CAMERA DRIVER INSTANCE PER DEVICE TYPE AND ACCUMULATE SENSOR COUNT
    for (int n = 0; n < devices.count(); n++) {
        LAU3DCamera *camera = nullptr;
        LAUVideoPlaybackDevice device = devices.at(n);

        // CREATE CAMERA INSTANCE BASED ON DEVICE TYPE
        // RESTRICTED TO ORBBEC AND LUCID CAMERAS AT 640X480, COLORXYZ ONLY
        if (device == DeviceOrbbec) {
#ifdef ORBBEC
            camera = new LAUOrbbecCamera(color);
            if (camera && camera->isValid()) {
                // VERIFY RESOLUTION IS 640X480
                // For ColorGray mode (NIR only), check color dimensions
                // For other modes with depth, check depth dimensions
                unsigned int width = (color == ColorGray) ? camera->colorWidth() : camera->depthWidth();
                unsigned int height = (color == ColorGray) ? camera->colorHeight() : camera->depthHeight();

                if (width != LAU_CAMERA_DEFAULT_WIDTH || height != LAU_CAMERA_DEFAULT_HEIGHT) {
                    errorString.append(QString("Orbbec camera resolution must be 640x480, got %1x%2. ").arg(width).arg(height));
                    delete camera;
                    camera = nullptr;
                }
            }
#else
            errorString.append(QString("Orbbec support not compiled in. "));
#endif
        } else if (device == DeviceLucid) {
#ifdef LUCID
            camera = new LAULucidCamera(QString("Distance4000mmSingleFreq"), color);
            if (camera && camera->isValid()) {
                // VERIFY RESOLUTION IS 640X480
                if (camera->depthWidth() != LAU_CAMERA_DEFAULT_WIDTH || camera->depthHeight() != LAU_CAMERA_DEFAULT_HEIGHT) {
                    errorString.append(QString("Lucid camera resolution must be 640x480, got %1x%2. ").arg(camera->depthWidth()).arg(camera->depthHeight()));
                    delete camera;
                    camera = nullptr;
                }
            }
#else
            errorString.append(QString("Lucid support not compiled in. "));
#endif
        } else {
            errorString.append(QString("Unsupported device type. Only Orbbec and Lucid are supported. "));
        }

        // IF CAMERA WAS CREATED AND IS VALID, ADD TO LIST
        if (camera && camera->isValid()) {
            camera->setStartingFrameIndex(sensorCount);  // SET STARTING SENSOR INDEX
            sensorCount += camera->sensors();            // ACCUMULATE SENSOR COUNT
            cameras.append(camera);
        } else if (camera) {
            errorString.append(QString("Camera failed to initialize: %1 ").arg(camera->error()));
            delete camera;
        }
    }

    // DON'T CLOSE PROGRESS DIALOG YET - it will stay visible until first valid frames arrive
    // in onUpdateBuffer()

    // VERIFY WE HAVE AT LEAST ONE VALID CAMERA
    if (cameras.isEmpty() || sensorCount == 0) {
        errorString.append(QString("No valid cameras found for multi-sensor video widget."));
        qDebug() << errorString;

        // CLEAN UP CONNECTION DIALOG BEFORE EARLY RETURN
        if (connectionDialog) {
            connectionDialog->uninstallMessageHandler();
            connectionDialog->reject();
            connectionDialog->deleteLater();
            connectionDialog = nullptr;
        }
        return;
    }

    // ALLOCATE FRAME BUFFERS FOR RAW VIDEO (UNSIGNED SHORT PER CAMERA)
    // FOLLOWING LAURECORDMULTIVIDEOTODISKOBJECT PATTERN - ALLOCATE 10 FRAMES
    // GET MEMORY OBJECT TEMPLATES FROM FIRST CAMERA
    LAUMemoryObject depthTemplate = cameras.first()->depthMemoryObject();
    LAUMemoryObject colorTemplate = cameras.first()->colorMemoryObject();

    unsigned int depthWidth = depthTemplate.width();
    unsigned int depthHeight = depthTemplate.height();
    unsigned int colorWidth = colorTemplate.width();
    unsigned int colorHeight = colorTemplate.height();

    // ALLOCATE MODALITY OBJECT BUFFERS (depth + color + mapping bundles)
    // Use actual depth/color size from camera templates (handles ColorGray 16-bit vs ColorRGB 8-bit)
    while (frameBuffers.count() < 10) {
        LAUMemoryObject depth = LAUMemoryObject(depthWidth, depthHeight, depthTemplate.colors(), depthTemplate.depth(), sensorCount);
        LAUMemoryObject color = LAUMemoryObject(colorWidth, colorHeight, colorTemplate.colors(), colorTemplate.depth(), sensorCount);
        frameBuffers.append(LAUModalityObject(depth, color, LAUMemoryObject()));
    }
    qDebug() << "Allocated" << frameBuffers.count() << "modality object buffers: depth" << depthWidth << "x" << depthHeight << depthTemplate.depth() << "bytes x" << depthTemplate.colors() << "channels, color" << colorWidth << "x" << colorHeight << colorTemplate.depth() << "bytes x" << colorTemplate.colors() << "channels for" << sensorCount << "sensors";

    // CREATE A GLWIDGET TO DISPLAY THE VIDEO
    // USE FIRST CAMERA'S PARAMETERS
    LAU3DCamera *firstCamera = cameras.first();

    glWidget = new LAU3DVideoGLWidget(depthWidth, depthHeight, colorWidth, colorHeight, playbackColor, firstCamera->device());
    glWidget->onSetCamera(0);  // DEFAULT TO DISPLAYING FIRST SENSOR
    glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    glWidget->setMaximumIntensityValue(firstCamera->maxIntensityValue());

#ifndef RAW_NIR_VIDEO
    // CREATE TEMPORARY DIRECTORY FOR LUT CACHE
    // LUTs not needed in RAW_NIR_VIDEO mode (passive grayscale only)
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString lutCachePath = tempPath + "/LAULookUpTableCache";
    QDir lutCacheDir(lutCachePath);
    if (!lutCacheDir.exists()) {
        lutCacheDir.mkpath(".");
        qDebug() << "Created LUT cache directory:" << lutCachePath;
    }

    // CLEAN UP OLD LUT FILES (OLDER THAN 24 HOURS)
    QFileInfoList lutFiles = lutCacheDir.entryInfoList(QStringList() << "*.tif", QDir::Files);
    QDateTime now = QDateTime::currentDateTime();
    for (const QFileInfo &fileInfo : lutFiles) {
        qint64 ageInSeconds = fileInfo.lastModified().secsTo(now);
        if (ageInSeconds > 86400) {  // 24 hours = 86400 seconds
            QFile::remove(fileInfo.absoluteFilePath());
            qDebug() << "Removed old LUT cache file:" << fileInfo.fileName() << "(" << (ageInSeconds / 3600) << "hours old)";
        }
    }

    // BUILD LIST OF LOOKUP TABLES FROM ALL CAMERA SENSORS
    for (int n = 0; n < sensorCount; n++) {
        // FIND WHICH CAMERA THIS SENSOR BELONGS TO
        int sensorIndex = n;
        LAU3DCamera *camera = nullptr;
        for (int c = 0; c < cameras.count(); c++) {
            camera = cameras.at(c);
            if (sensorIndex < (int)camera->sensors()) {
                // THIS SENSOR BELONGS TO THIS CAMERA
                break;
            }
            sensorIndex -= camera->sensors();
        }

        if (camera) {
            // BUILD CACHE FILENAME USING SERIAL NUMBER
            QString serialNumber = camera->sensorSerial(sensorIndex);
            qDebug() << "Sensor" << n << "serial number:" << serialNumber;

            QString lutCacheFile = lutCachePath + "/" + serialNumber + ".tif";
            qDebug() << "  Looking for cache file:" << lutCacheFile;
            qDebug() << "  Cache file exists:" << QFile::exists(lutCacheFile);

            LAULookUpTable lut;

            // TRY TO LOAD FROM CACHE
            if (QFile::exists(lutCacheFile)) {
                qDebug() << "  Loading cached LUT for sensor" << n << "serial" << serialNumber << "from" << lutCacheFile;
                lut = LAULookUpTable(lutCacheFile);
                qDebug() << "  Loaded LUT, isValid():" << lut.isValid();
                if (lut.isValid()) {
                    qDebug() << "  Successfully loaded cached LUT for sensor" << n;
                } else {
                    qDebug() << "  Cached LUT invalid, regenerating...";
                    lut = LAULookUpTable();
                }
            } else {
                qDebug() << "  No cache file found for sensor" << n;
            }

            // IF NOT IN CACHE OR INVALID, GENERATE NEW LUT FROM CAMERA
            if (!lut.isValid()) {
                qDebug() << "  Generating new LUT for sensor" << n << "serial" << serialNumber;
                lut = camera->lut(sensorIndex, this);
                qDebug() << "  Generated LUT, isValid():" << lut.isValid();

                // SAVE TO CACHE IF VALID
                if (lut.isValid()) {
                    qDebug() << "  Attempting to save LUT to cache:" << lutCacheFile;
                    if (lut.save(lutCacheFile)) {
                        qDebug() << "  Successfully saved LUT to cache:" << lutCacheFile;
                        // VERIFY FILE WAS ACTUALLY CREATED
                        if (QFile::exists(lutCacheFile)) {
                            QFileInfo info(lutCacheFile);
                            qDebug() << "  Verified cache file exists, size:" << info.size() << "bytes";
                        } else {
                            qDebug() << "  ERROR: Cache file does not exist after save!";
                        }
                    } else {
                        qDebug() << "  ERROR: Failed to save LUT to cache:" << lutCacheFile;
                    }
                }
            }

            lookUpTables.append(lut);
        }
    }

    // GET LOOKUP TABLE FROM FIRST SENSOR AND SET IT IN THE GL WIDGET
    if (lookUpTables.count() > 0 && lookUpTables.first().isValid()) {
        glWidget->setLookUpTable(lookUpTables.first());
    }

    qDebug() << "Built" << lookUpTables.count() << "lookup tables for" << sensorCount << "sensors";
#endif

    // CREATE BACKGROUND FILTERS FOR EACH SENSOR
    // This follows the pattern from LAUBackgroundWidget multi-sensor constructor
    QList<QObject*> filters;
    for (int chn = 0; chn < sensorCount; chn++) {
        // Find which camera owns this sensor
        LAU3DCamera *owningCamera = nullptr;
        int localSensorIndex = chn;
        for (LAU3DCamera *cam : cameras) {
            if (localSensorIndex < (int)cam->sensors()) {
                owningCamera = cam;
                break;
            }
            localSensorIndex -= cam->sensors();
        }

        if (!owningCamera) {
            qWarning() << "Failed to find owning camera for sensor" << chn;
            continue;
        }

        // Create filter with camera dimensions
        LAUBackgroundGLFilter *filter = new LAUBackgroundGLFilter(
            owningCamera->depthWidth(), owningCamera->depthHeight(),
            owningCamera->colorWidth(), owningCamera->colorHeight(),
            playbackColor, owningCamera->device());

        // Set filter parameters
        filter->setMaxDistance(owningCamera->maxDistance());
        filter->setFieldsOfView(owningCamera->horizontalFieldOfViewInRadians(),
                                owningCamera->verticalFieldOfViewInRadians());
        filter->setCamera(chn);
        filter->setJetrVector(chn, owningCamera->jetr(localSensorIndex));

        // Store filter in list
        backgroundFilters.append(filter);

        // Chain filters together (filter 0 → filter 1 → filter 2)
        // insertFilters() does NOT do this - it only connects endpoints
        if (filters.count() > 0) {
            connect((LAUBackgroundGLFilter*)filters.last(),
                    SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
                    filter,
                    SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
                    Qt::QueuedConnection);
        }

        // Add to filters list for insertion
        filters.append(filter);
    }

    qDebug() << "Created" << backgroundFilters.count() << "background filters";

    // ADD GL WIDGET TO LAYOUT
    this->layout()->addWidget(glWidget);

    // CREATE FPS LABEL IN BOTTOM LEFT CORNER
    fpsLabel = new QLabel(QString("0.0 fps"), glWidget);
    fpsLabel->setStyleSheet("QLabel { background-color: rgba(0, 0, 0, 200); color: yellow; padding: 8px; font-size: 18px; font-weight: bold; }");
    fpsLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    fpsLabel->setFixedSize(120, 40);
    fpsLabel->move(10, 10);  // Top-left corner for visibility
    fpsLabel->raise();  // Bring to front
    fpsLabel->show();

#ifdef RAW_NIR_VIDEO
    // INITIALIZE CAMERA POSITION STORAGE FOR EACH SENSOR
    // Load positions from systemConfig.ini based on camera serial numbers
    QString iniPath = QDir::currentPath() + "/systemConfig.ini";
    QSettings settings(iniPath, QSettings::IniFormat);

    for (int i = 0; i < sensorCount; i++) {
        // Get the serial number for this sensor
        QString serialNumber;

        // Calculate which camera this sensor belongs to
        int cameraIndex = 0;
        int localSensorIndex = i;
        for (int j = 0; j < cameras.count(); j++) {
            int numSensors = cameras.at(j)->sensors();
            if (localSensorIndex < numSensors) {
                cameraIndex = j;
                break;
            }
            localSensorIndex -= numSensors;
        }

        // Get serial number based on device type
        LAU3DCamera *camera = cameras.at(cameraIndex);
        if (camera->device() == DeviceLucid) {
#ifdef LUCID
            LAULucidCamera *lucidCamera = qobject_cast<LAULucidCamera*>(camera);
            if (lucidCamera) {
                serialNumber = lucidCamera->sensorSerial(localSensorIndex);
            }
#endif
        } else if (camera->device() == DeviceOrbbec) {
#ifdef ORBBEC
            LAUOrbbecCamera *orbbecCamera = qobject_cast<LAUOrbbecCamera*>(camera);
            if (orbbecCamera) {
                serialNumber = orbbecCamera->sensorSerial(localSensorIndex);
            }
#endif
        }

        // Load position from QSettings using serial number as key
        QString position = "H Unknown";  // Default to "H Unknown" (matches combo box data)
        if (!serialNumber.isEmpty()) {
            position = settings.value(QString("CameraPosition/%1").arg(serialNumber), "H Unknown").toString();
            qDebug() << "Loaded position for sensor" << i << "(S/N:" << serialNumber << "):" << position;
        } else {
            qWarning() << "Could not get serial number for sensor" << i;
        }

        cameraPositions.append(position);
    }

    // CREATE CAMERA POSITION COMBO BOX FOR NIR VIDEO MODE
    // Display user-friendly names, but store with letter prefixes for alphabetical sorting
    // Letter prefixes (A-H) establish clear priority ordering when cameras are sorted
    QHBoxLayout *controlLayout = new QHBoxLayout();
    QLabel *positionLabel = new QLabel("Camera Position:");
    cameraPositionCombo = new QComboBox();
    cameraPositionCombo->setFocusPolicy(Qt::NoFocus);
    // Display text is user-friendly (no prefix), data value has prefix for sorting/camera programming
    cameraPositionCombo->addItem("Top", "A Top");
    cameraPositionCombo->addItem("Side", "B Side");
    cameraPositionCombo->addItem("Bottom", "C Bottom");
    cameraPositionCombo->addItem("Front", "D Front");
    cameraPositionCombo->addItem("Back", "E Back");
    cameraPositionCombo->addItem("Quarter", "F Quarter");
    cameraPositionCombo->addItem("Rump", "G Rump");
    cameraPositionCombo->addItem("Unknown", "H Unknown");

    // Set initial value to match sensor 0's position
    int initialIndex = cameraPositionCombo->findData(cameraPositions.at(0));
    if (initialIndex >= 0) {
        cameraPositionCombo->setCurrentIndex(initialIndex);
    }

    controlLayout->addWidget(positionLabel);
    controlLayout->addWidget(cameraPositionCombo);
    controlLayout->addStretch();

    // ADD PROGRAM LABELS BUTTON
    recordButton = new QPushButton(QString("Program Camera Labels"));
    recordButton->setFocusPolicy(Qt::NoFocus);
    controlLayout->addWidget(recordButton);

    ((QVBoxLayout *)(this->layout()))->addSpacing(12);
    ((QVBoxLayout *)(this->layout()))->addLayout(controlLayout);

    // CONNECT COMBO BOX AND BUTTON TO SLOTS
    connect(cameraPositionCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onCameraPositionChanged(int)));
    connect(recordButton, SIGNAL(clicked()), this, SLOT(onProgramCameraLabels()));
#else
    // CREATE RECORD AND RESET BUTTONS FOR NORMAL MODE
    recordButton = new QPushButton(QString("Record"));
    recordButton->setFocusPolicy(Qt::NoFocus);
    resetButton = new QPushButton(QString("Reset"));
    resetButton->setFocusPolicy(Qt::NoFocus);

    QDialogButtonBox *box = new QDialogButtonBox();
    box->addButton(recordButton, QDialogButtonBox::AcceptRole);
    box->addButton(resetButton, QDialogButtonBox::RejectRole);

    ((QVBoxLayout *)(this->layout()))->addSpacing(12);
    this->layout()->addWidget(box);

    // CONNECT BUTTONS TO SLOTS
    connect(recordButton, SIGNAL(clicked()), this, SLOT(onRecordButtonClicked()));
    connect(resetButton, SIGNAL(clicked()), this, SLOT(onResetButtonClicked()));

    // CONNECT RECORD AND RESET BUTTONS TO ALL BACKGROUND FILTERS
    for (LAUBackgroundGLFilter *filter : backgroundFilters) {
        // Use onEmitBackground() instead of onPreserveBackgroundToSettings()
        // so backgrounds are emitted without saving to QSettings
        connect(recordButton, SIGNAL(clicked()), filter, SLOT(onEmitBackground()));
        connect(resetButton, SIGNAL(clicked()), filter, SLOT(onReset()));

        // CONNECT FILTER'S BACKGROUND SIGNAL TO THIS WIDGET'S SLOT
        connect(filter, SIGNAL(emitBackground(LAUMemoryObject)), this, SLOT(onReceiveBackground(LAUMemoryObject)));
    }
#endif

    // CONNECT FILTER DELETION SIGNALS
    // When filters are deleted, the filterControllers will automatically clean up
    for (LAUBackgroundGLFilter *filter : backgroundFilters) {
        connect(filter, SIGNAL(destroyed()), this, SLOT(onCameraDeleted()), Qt::QueuedConnection);
    }

    // CONNECT CHANNEL INDEX SIGNAL FROM GLWIDGET'S INTERNAL FILTER
    // The glWidget filter is what actually renders the display, so it knows which channel is shown
    if (glWidget && glWidget->filter()) {
        qDebug() << "Connecting emitChannelIndex signal from glWidget filter";
        connect(glWidget->filter(), SIGNAL(emitChannelIndex(int)), this, SLOT(onChannelIndexChanged(int)), Qt::QueuedConnection);
    } else {
        qWarning() << "Failed to connect emitChannelIndex - glWidget or filter is null";
    }

    // CONNECT ALL THE CAMERAS TOGETHER
    for (int s = 0; s < cameras.count(); s++) {
        LAU3DCamera *camera = cameras.at(s);
        connect(camera, SIGNAL(emitError(QString)), this, SLOT(onCameraError(QString)), Qt::QueuedConnection);
        connect(camera, SIGNAL(destroyed()), this, SLOT(onCameraDeleted()), Qt::QueuedConnection);
        if ((s + 1) < cameras.count()) {
            // CHAIN CAMERAS TOGETHER - EACH CAMERA FEEDS INTO THE NEXT
            connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
                    cameras.at(s + 1), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
                    Qt::QueuedConnection);
        } else {
            // LAST CAMERA - insertFilters() will handle connection to filters or GL widget
            // Don't connect here to avoid duplicate connections
        }

        // CREATE A THREAD TO HOST THE CAMERA CONTROLLER
        cameraControllers << new LAU3DCameraController(camera);
    }

    // INSERT FILTERS INTO SIGNAL CHAIN (creates filter controllers and moves to threads)
    // This will:
    // 1. Connect last camera → first filter
    // 2. Chain filters together (filter 0 → filter 1 → filter 2)
    // 3. Connect last filter → GL widget
    if (filters.count() > 0) {
        insertFilters(filters);
    } else {
        // NO FILTERS - CONNECT LAST CAMERA DIRECTLY TO GL WIDGET
        connect(cameras.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
                glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
                Qt::QueuedConnection);
    }

    // CONNECT GL WIDGET OUTPUT BACK TO THIS WIDGET
    connect(glWidget, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
            this, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
            Qt::DirectConnection);

    // COMPLETE THE LOOP - CONNECT THIS WIDGET BACK TO THE FIRST CAMERA TO REQUEST NEXT FRAME
    connect(this, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
            cameras.first(), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
            Qt::QueuedConnection);

    // START THE FRAME RATE TIMER
    time.start();

    qDebug() << "Multi-sensor video widget created:" << cameras.count() << "devices," << sensorCount << "total sensors";

    // SET FOCUS POLICY SO ARROW KEYS REACH THIS WIDGET'S KEY HANDLER
    // THIS MUST BE DONE AT THE END AFTER ALL WIDGETS ARE CREATED
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DMultiSensorVideoWidget::~LAU3DMultiSensorVideoWidget()
{
    // DELETE CONTROLLERS WHICH WILL DELETE THEIR CAMERA OBJECTS
    while (cameraControllers.count() > 0) {
        delete cameraControllers.takeFirst();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiSensorVideoWidget::insertFilters(QList<QObject *> filters)
{
    if (filters.isEmpty()) {
        return;
    }

    if (filterControllers.isEmpty()) {
        // DISCONNECT THE SIGNAL FROM THE LAST CAMERA THAT WOULD GO TO THE GLWIDGET
        disconnect(cameras.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

        // SPLICE THE FILTERS INTO THE SIGNAL PATH
        connect(cameras.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), filters.first(), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(filters.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
    } else {
        // GRAB THE LAST FILTER IN THE LIST
        LAUAbstractFilterController *controller = filterControllers.last();
        if (controller->glFilter()) {
            // DISCONNECT THAT LAST GLFILTER IN THE LIST
            LAUAbstractGLFilter *oldFilter = controller->glFilter();
            disconnect(oldFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

            // SPLICE THE FILTERS INTO THE SIGNAL PATH
            connect(oldFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), filters.first(), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(filters.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        } else if (controller->filter()) {
            LAUAbstractFilter *oldFilter = controller->filter();
            disconnect(oldFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

            // SPLICE THE FILTERS INTO THE SIGNAL PATH
            connect(oldFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), filters.first(), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(filters.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        }
    }

    // CREATE CONTROLLERS TO WRAP AROUND THE FILTER OBJECTS
    while (filters.count() > 0) {
        QObject *filter = filters.takeFirst();
        if (filter) {
            if (dynamic_cast<LAUAbstractGLFilter *>(filter) != nullptr) {
                filterControllers.append(new LAUAbstractFilterController((LAUAbstractGLFilter *)filter));
            } else if (dynamic_cast<LAUAbstractFilter *>(filter) != nullptr) {
                filterControllers.append(new LAUAbstractFilterController((LAUAbstractFilter *)filter));
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiSensorVideoWidget::onCameraError(QString string)
{
    qDebug() << "Camera Error:" << string;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiSensorVideoWidget::onCameraDeleted()
{
    qDebug() << "Camera deleted";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiSensorVideoWidget::onUpdateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    // CLOSE CONNECTION DIALOG ONLY WHEN WE RECEIVE VALID MEMORY OBJECTS
    // This ensures the dialog stays visible during the entire camera initialization,
    // including PTP synchronization which can take 30-60 seconds
    if (connectionDialog && (depth.isValid() || color.isValid())) {
        qInfo() << "First valid frame received - closing connection dialog";
        connectionDialog->uninstallMessageHandler();
        connectionDialog->accept();
        connectionDialog->deleteLater();  // Use deleteLater() for thread safety
        connectionDialog = nullptr;

        // Start FPS monitoring timer after first valid frame
        fpsMonitorTimer.start();
        qDebug() << "FPS monitoring started - will check performance after 5 seconds";
    }

    // COUNT INCOMING SIGNALS (this is called once per complete camera cycle)
    fpsCounter++;
    if (fpsCounter >= LAU_FPS_COUNTER_FRAMES) {
        currentFps = 1000.0 * (float)fpsCounter / (float)time.elapsed();
        fpsLabel->setText(QString("%1 fps").arg(currentFps, 0, 'f', 1));
        time.restart();
        fpsCounter = 0;

        // CHECK FPS PERFORMANCE AFTER 5 SECONDS
        // Expected: ~12.5 fps (normal mode), ~8 fps (NIR mode with all 3 cameras)
        // Warn if below threshold after 5 seconds
        // Skip FPS warning if we're currently saving background (file dialog blocks processing)
#ifdef RAW_NIR_VIDEO
        const float fpsThreshold = 7.0f;  // NIR mode runs slower (~8 fps is normal)
        const QString expectedFpsText = "~8 fps";
#else
        const float fpsThreshold = 10.0f;  // Normal mode (~12.5 fps)
        const QString expectedFpsText = "~12.5 fps";
#endif
        if (!fpsWarningShown && !savingBackground && fpsMonitorTimer.isValid() && fpsMonitorTimer.elapsed() >= LAU_FPS_WARNING_THRESHOLD_MS) {
            if (currentFps < fpsThreshold) {
                fpsWarningShown = true;  // Only show once per session
                qWarning() << "Low FPS detected:" << currentFps << "fps (expected" << expectedFpsText << ")";

                // Build detailed warning message
                QString warningMessage = QString(
                    "<b>Low Frame Rate Detected</b><br><br>"
                    "Current FPS: <font color='red'><b>%1 fps</b></font><br>"
                    "Expected FPS: <b>%2</b> (with all 3 cameras)<br><br>"
                    "<b>Common causes of low FPS:</b><br>"
                    "  • OnTrak relay intermittent or losing power<br>"
                    "  • One or more cameras not receiving power via PoE<br>"
                    "  • GigE network bandwidth issues (multiple cameras on same switch)<br>"
                    "  • Ethernet cable problem or loose connection<br>"
                    "  • Network switch not handling multicast properly<br>"
                    "  • CPU overload or thermal throttling<br>"
                    "  • Camera firmware issue requiring power cycle<br><br>"
                    "<b>Recommended actions:</b><br>"
                    "  1. Check OnTrak widget shows <font color='green'>GREEN</font> buttons (PoE power ON)<br>"
                    "  2. Verify all GigE Ethernet cable connections are secure<br>"
                    "  3. Check network switch activity lights for all cameras<br>"
                    "  4. Power cycle cameras via OnTrak relay<br>"
                    "  5. Restart application and retry<br><br>"
                    "<b>Do you want to continue anyway?</b><br>"
                    "Recording with low FPS may result in missed data or poor quality."
                ).arg(currentFps, 0, 'f', 1).arg(expectedFpsText);

                // Create window-modal dialog (blocks this window but not the entire app)
                // This allows video processing thread to continue while dialog is shown
                QMessageBox msgBox(this);
                msgBox.setWindowModality(Qt::WindowModal);
                msgBox.setWindowTitle("Low Frame Rate Warning");
                msgBox.setTextFormat(Qt::RichText);
                msgBox.setText(warningMessage);
                msgBox.setIcon(QMessageBox::Warning);

                Q_UNUSED(msgBox.addButton("Continue Anyway", QMessageBox::AcceptRole));
                QPushButton *powerCycleButton = msgBox.addButton("Power Cycle Cameras", QMessageBox::RejectRole);
                QPushButton *restartButton = msgBox.addButton("Restart Application", QMessageBox::DestructiveRole);
                msgBox.setDefaultButton(powerCycleButton);

                msgBox.exec();

                if (msgBox.clickedButton() == restartButton) {
                    // User wants to restart - relaunch application
                    qDebug() << "User clicked Restart - relaunching application";
                    QTimer::singleShot(0, this, []() {
                        QString appPath = qApp->applicationFilePath();
                        QStringList args = qApp->arguments();
                        qDebug() << "Attempting to restart:" << appPath << "with args:" << args;
                        bool success = QProcess::startDetached(appPath, args);
                        qDebug() << "startDetached returned:" << success;
                        qApp->quit();
                    });
                } else if (msgBox.clickedButton() == powerCycleButton) {
                    // Show instructions for power cycling
                    QMessageBox::information(this, "Power Cycle Instructions",
                        "To power cycle the cameras:\n\n"
                        "1. Open the LAUOnTrakWidget application\n"
                        "2. Click the relay buttons to turn cameras OFF (red buttons)\n"
                        "3. Wait " + QString::number(LAU_RECOMMENDED_WAIT_SECONDS) + " seconds\n"
                        "4. Click the relay buttons to turn cameras ON (green buttons)\n"
                        "5. Close and restart LAUBackgroundFilter\n\n"
                        "This application will now close so you can power cycle the cameras.");
                    qApp->quit();
                }
                // If "Continue Anyway" was clicked, just continue without action
            } else {
                qInfo() << "FPS check passed:" << currentFps << "fps (>=" << fpsThreshold << "fps threshold)";
            }
        }
    }

    // STORE INCOMING FRAME AS A MODALITY OBJECT (depth + color + mapping bundle)
    LAUModalityObject frame(depth, color, mapping);
    if (frame.isAnyValid()) {
        frameBuffers << frame;
    }

    // EMIT ONE FRAME AT A TIME WHEN VISIBLE
    // THIS PRIMES THE PUMP WITH 10 FRAMES ON STARTUP, THEN MAINTAINS 1:1 FLOW
    // THE SIGNAL CHAIN RUNS AT THE SPEED OF THE SLOWEST COMPONENT (CAMERA)
    if (isVisible() && frameBuffers.count() > 0) {
        LAUModalityObject packet = frameBuffers.takeFirst();
        emit emitBuffer(packet.depth, packet.color, packet.mappi);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiSensorVideoWidget::onChannelIndexChanged(int channelIndex)
{
    qDebug() << "<<< SIGNAL RECEIVED: onChannelIndexChanged called with channelIndex:" << channelIndex;
#ifdef RAW_NIR_VIDEO
    // Update the combo box to reflect the actual channel being displayed
    // Handle negative indices properly by ensuring positive modulo result
    int sensorIndex = ((channelIndex % sensorCount) + sensorCount) % sensorCount;
    qDebug() << "    Calculated sensorIndex:" << sensorIndex << "(sensorCount:" << sensorCount << ")";
    if (sensorIndex >= 0 && sensorIndex < cameraPositions.count()) {
        QString position = cameraPositions.at(sensorIndex);
        qDebug() << "    Position for sensor" << sensorIndex << ":" << position;
        int comboIndex = cameraPositionCombo->findData(position);
        if (comboIndex >= 0) {
            qDebug() << "    Updating combo box to index:" << comboIndex;
            cameraPositionCombo->blockSignals(true);
            cameraPositionCombo->setCurrentIndex(comboIndex);
            cameraPositionCombo->blockSignals(false);
        } else {
            qDebug() << "    WARNING: Could not find combo index for position:" << position;
        }
    } else {
        qDebug() << "    WARNING: sensorIndex" << sensorIndex << "out of range (0 -" << cameraPositions.count()-1 << ")";
    }
#else
    Q_UNUSED(channelIndex);
    // Channel index tracking available but not used in normal mode
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
#ifdef RAW_NIR_VIDEO
void LAU3DMultiSensorVideoWidget::onCameraPositionChanged(int index)
{
    Q_UNUSED(index);
    QString position = cameraPositionCombo->currentData().toString();

    // Get the currently displayed sensor index
    int currentSensorIndex = cameraIndex % sensorCount;

    // Save the position for this sensor
    if (currentSensorIndex < cameraPositions.count()) {
        cameraPositions[currentSensorIndex] = position;
        qDebug() << "Sensor" << currentSensorIndex << "position set to:" << position;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiSensorVideoWidget::onProgramCameraLabels()
{
    qDebug() << "LAU3DMultiSensorVideoWidget::onProgramCameraLabels()";
    qDebug() << "Programming camera labels with positions:" << cameraPositions;

#ifdef LUCID
    // VALIDATE CAMERA LABELS BEFORE PROGRAMMING
    // Requirements:
    // - First camera (index 0) must be "top" (Orbbec camera, not programmed)
    // - Second camera (index 1) must be "side" or "quarter"
    // - Third camera (index 2) must be "side" or "quarter"

    QStringList validationErrors;

    // Helper to strip prefix from position (e.g., "A Top" -> "Top")
    auto stripPrefix = [](const QString &position) -> QString {
        if (position.length() >= 3 && position.at(1) == ' ') {
            return position.mid(2);  // Skip "A " prefix
        }
        return position;
    };

    // Check if we have at least 3 cameras
    if (cameraPositions.count() < LAU_MIN_CAMERA_COUNT) {
        validationErrors.append(QString("Expected at least 3 cameras, but only found %1").arg(cameraPositions.count()));
    } else {
        // Check first camera is "Top" (strip prefix before comparing)
        QString cam0 = stripPrefix(cameraPositions.at(0)).toLower();
        if (cam0 != "top") {
            validationErrors.append(QString("Camera 0 must be 'Top' (Orbbec), but found '%1'").arg(stripPrefix(cameraPositions.at(0))));
        }

        // Check second camera is "Side" or "Quarter" (strip prefix before comparing)
        QString cam1 = stripPrefix(cameraPositions.at(1)).toLower();
        if (cam1 != "side" && cam1 != "quarter") {
            validationErrors.append(QString("Camera 1 must be 'Side' or 'Quarter', but found '%1'").arg(stripPrefix(cameraPositions.at(1))));
        }

        // Check third camera is "Side" or "Quarter" (strip prefix before comparing)
        QString cam2 = stripPrefix(cameraPositions.at(2)).toLower();
        if (cam2 != "side" && cam2 != "quarter") {
            validationErrors.append(QString("Camera 2 must be 'Side' or 'Quarter', but found '%1'").arg(stripPrefix(cameraPositions.at(2))));
        }
    }

    // If validation failed, show error dialog and return
    if (!validationErrors.isEmpty()) {
        QString errorMessage = "<b>Invalid Camera Label Configuration</b><br><br>";
        errorMessage += "<b>Requirements:</b><br>";
        errorMessage += "  • Camera 0 (Orbbec) must be labeled 'top'<br>";
        errorMessage += "  • Camera 1 (Lucid) must be labeled 'side' or 'quarter'<br>";
        errorMessage += "  • Camera 2 (Lucid) must be labeled 'side' or 'quarter'<br><br>";
        errorMessage += "<b>Current configuration:</b><br>";
        for (int i = 0; i < cameraPositions.count(); i++) {
            errorMessage += QString("  • Camera %1: <font color='red'><b>%2</b></font><br>").arg(i).arg(cameraPositions.at(i));
        }
        errorMessage += "<br><b>Errors found:</b><br>";
        for (const QString &error : validationErrors) {
            errorMessage += QString("  • %1<br>").arg(error);
        }
        errorMessage += "<br><b>Please set the labels correctly and try again.</b>";

        qWarning() << "Camera label validation failed:";
        for (const QString &error : validationErrors) {
            qWarning() << "  " << error;
        }

        // Use window-modal dialog to avoid blocking video processing
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setWindowTitle("Invalid Camera Labels");
        msgBox->setText(errorMessage);
        msgBox->setIcon(QMessageBox::Critical);
        msgBox->setWindowModality(Qt::WindowModal);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->show();
        return;
    }

    // Validation passed - proceed with programming
    qDebug() << "Camera label validation passed!";

    // Find the Lucid camera object(s) and program each one with its label
    QStringList successMessages;
    QStringList errorMessages;
    int successCount = 0;
    int failureCount = 0;

    for (int i = 0; i < cameras.count(); i++) {
        LAU3DCamera *camera = cameras.at(i);

        // Skip camera 0 (Orbbec "top" camera) - we only program Lucid cameras
        if (i == 0) {
            qDebug() << "Skipping camera 0 (Orbbec top camera) - not a Lucid camera";
            continue;
        }

        // Check if this is a Lucid camera
        if (camera->device() == DeviceLucid) {
            LAULucidCamera *lucidCamera = qobject_cast<LAULucidCamera*>(camera);
            if (lucidCamera) {
                int numSensors = lucidCamera->sensors();
                qDebug() << "Camera" << i << "has" << numSensors << "sensors";

                // Program each sensor in this camera
                for (int sensorIndex = 0; sensorIndex < numSensors; sensorIndex++) {
                    // Calculate the global sensor index across all cameras
                    int globalSensorIndex = 0;
                    for (int j = 0; j < i; j++) {
                        globalSensorIndex += cameras.at(j)->sensors();
                    }
                    globalSensorIndex += sensorIndex;

                    if (globalSensorIndex < cameraPositions.count()) {
                        QString position = cameraPositions.at(globalSensorIndex);
                        QString serialNumber = lucidCamera->sensorSerial(sensorIndex);

                        qDebug() << "  Setting sensor" << sensorIndex << "(global index" << globalSensorIndex
                                 << ", S/N:" << serialNumber << ") to position:" << position;

                        // Write the label WITH prefix (e.g., "A Top", "B Side") for alphabetical sorting
                        bool writeSuccess = lucidCamera->onSetCameraUserDefinedName(sensorIndex, position);
                        if (!writeSuccess) {
                            failureCount++;
                            errorMessages.append(QString("  ✗ Failed to write sensor %1 (S/N: %2)")
                                .arg(globalSensorIndex).arg(serialNumber));
                            continue;
                        }

                        // Read back the label to verify it was written correctly
                        QString readBackName = lucidCamera->onGetCameraUserDefinedName(sensorIndex);
                        if (readBackName.isEmpty()) {
                            failureCount++;
                            errorMessages.append(QString("  ✗ Failed to read back sensor %1 (S/N: %2)")
                                .arg(globalSensorIndex).arg(serialNumber));
                            continue;
                        }

                        // Verify the read-back value matches what we wrote
                        // Position includes prefix (e.g., "A Top", "B Side") for alphabetical sorting
                        if (readBackName.toLower() == position.toLower()) {
                            successCount++;
                            successMessages.append(QString("  ✓ Sensor %1 (S/N: %2) → '%3' (verified)")
                                .arg(globalSensorIndex).arg(serialNumber).arg(position));
                        } else {
                            failureCount++;
                            errorMessages.append(QString("  ✗ Verification failed for sensor %1 (S/N: %2): wrote '%3', read back '%4'")
                                .arg(globalSensorIndex).arg(serialNumber).arg(position).arg(readBackName));
                        }
                    }
                }
            }
        }
    }

    // Display results
    QString resultMessage;
    if (successCount > 0) {
        resultMessage += QString("Successfully programmed %1 camera(s):\n\n").arg(successCount);
        resultMessage += successMessages.join("\n");
    }
    if (failureCount > 0) {
        if (successCount > 0) {
            resultMessage += "\n\n";
        }
        resultMessage += QString("Failed to program %1 camera(s):\n\n").arg(failureCount);
        resultMessage += errorMessages.join("\n");
    }

    if (failureCount == 0) {
        qDebug() << "Successfully programmed all camera labels!";

        // SAVE SERIAL NUMBER TO POSITION MAPPING TO systemConfig.ini
        QString iniPath = QDir::currentPath() + "/systemConfig.ini";
        QSettings settings(iniPath, QSettings::IniFormat);
        settings.beginGroup("CameraPosition");

        for (int i = 0; i < sensorCount; i++) {
            // Get the serial number for this sensor
            QString serialNumber;

            // Calculate which camera this sensor belongs to
            int cameraIndex = 0;
            int localSensorIndex = i;
            for (int j = 0; j < cameras.count(); j++) {
                int numSensors = cameras.at(j)->sensors();
                if (localSensorIndex < numSensors) {
                    cameraIndex = j;
                    break;
                }
                localSensorIndex -= numSensors;
            }

            // Get serial number based on device type
            LAU3DCamera *camera = cameras.at(cameraIndex);
            if (camera->device() == DeviceLucid) {
#ifdef LUCID
                LAULucidCamera *lucidCamera = qobject_cast<LAULucidCamera*>(camera);
                if (lucidCamera) {
                    serialNumber = lucidCamera->sensorSerial(localSensorIndex);
                }
#endif
            } else if (camera->device() == DeviceOrbbec) {
#ifdef ORBBEC
                LAUOrbbecCamera *orbbecCamera = qobject_cast<LAUOrbbecCamera*>(camera);
                if (orbbecCamera) {
                    serialNumber = orbbecCamera->sensorSerial(localSensorIndex);
                }
#endif
            }

            // Save position to QSettings using serial number as key
            if (!serialNumber.isEmpty() && i < cameraPositions.count()) {
                QString position = cameraPositions.at(i);
                settings.setValue(serialNumber, position);
                qDebug() << "Saved to INI: S/N" << serialNumber << "-> position" << position;
            }
        }

        settings.endGroup();
        settings.sync();  // Force write to disk

        // Check for QSettings errors
        QSettings::Status settingsStatus = settings.status();
        qDebug() << "QSettings status after sync:" << settingsStatus;
        qDebug() << "Camera position mappings saved to INI file:" << settings.fileName();

        // Check if the file was actually written
        QFileInfo iniFileInfo(settings.fileName());
        bool fileExists = iniFileInfo.exists();
        bool fileWritable = iniFileInfo.isWritable();

        qDebug() << "INI file exists:" << fileExists;
        qDebug() << "INI file writable:" << fileWritable;
        qDebug() << "INI file absolute path:" << iniFileInfo.absoluteFilePath();

        // Add message about quitting
        resultMessage += "\n\nAll camera labels have been verified successfully.";
        resultMessage += "\n\nCamera positions configuration:";
        resultMessage += QString("\nFile: %1").arg(QDir::toNativeSeparators(iniFileInfo.absoluteFilePath()));

        if (settingsStatus != QSettings::NoError) {
            resultMessage += QString("\n\nWARNING: Settings error code: %1").arg(settingsStatus);
            resultMessage += "\nThe file may not have been saved!";
            resultMessage += "\nTry running this application as Administrator.";
        } else if (!fileExists) {
            resultMessage += "\n\nWARNING: Configuration file was not created!";
            resultMessage += "\nThis may be a permissions issue.";
            resultMessage += "\nTry running this application as Administrator.";
        } else {
            resultMessage += "\nStatus: Successfully saved";
        }

        resultMessage += "\n\nThe application will now close.";

        // Use non-modal dialog for success message (doesn't block video processing)
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setWindowTitle("Success");
        msgBox->setText(resultMessage);
        msgBox->setIcon(QMessageBox::Information);
        msgBox->setWindowModality(Qt::NonModal);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);

        // Connect the finished signal to quit the application
        connect(msgBox, &QMessageBox::finished, []() {
            qDebug() << "Quitting application after successful camera label programming";
            QApplication::quit();
        });

        msgBox->show();
    } else if (successCount > 0) {
        qWarning() << "Partially succeeded - some cameras failed";

        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setWindowTitle("Partial Success");
        msgBox->setText(resultMessage);
        msgBox->setIcon(QMessageBox::Warning);
        msgBox->setWindowModality(Qt::NonModal);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->show();
    } else {
        qWarning() << "Failed to program any camera labels";

        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setWindowTitle("Error");
        msgBox->setText(resultMessage);
        msgBox->setIcon(QMessageBox::Critical);
        msgBox->setWindowModality(Qt::NonModal);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->show();
    }
#else
    qWarning() << "LUCID not defined - camera label programming not available";

    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setWindowTitle("Not Available");
    msgBox->setText("Camera label programming is only available when compiled with Lucid camera support.");
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->setWindowModality(Qt::WindowModal);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->show();
#endif
}
#else
void LAU3DMultiSensorVideoWidget::onRecordButtonClicked()
{
    qDebug() << "LAU3DMultiSensorVideoWidget::onRecordButtonClicked()";
    // TODO: Implement background recording/snapshot functionality
    // This should capture the current scan from glWidget and save it
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiSensorVideoWidget::onResetButtonClicked()
{
    qDebug() << "LAU3DMultiSensorVideoWidget::onResetButtonClicked()";
    // TODO: Implement reset functionality
    // This should reset the background filter counters
}
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiSensorVideoWidget::onReceiveBackground(LAUMemoryObject background)
{
    // DETERMINE WHICH FILTER SENT THIS BACKGROUND BY FINDING THE SENDER
    LAUBackgroundGLFilter *senderFilter = qobject_cast<LAUBackgroundGLFilter*>(sender());
    if (!senderFilter) {
        qWarning() << "onReceiveBackground: Could not identify sender filter";
        return;
    }

    // FIND THE CHANNEL INDEX OF THIS FILTER
    int channel = backgroundFilters.indexOf(senderFilter);
    if (channel < 0) {
        qWarning() << "onReceiveBackground: Sender filter not found in backgroundFilters list";
        return;
    }

    // STORE THE BACKGROUND WITH ITS CHANNEL NUMBER TO MAINTAIN ORDER
    collectedBackgrounds[channel] = background;

    // DEBUG: Check if background has valid data
    const unsigned short *bgData = (const unsigned short *)background.constPointer();
    int sampleCount = 100;
    int nonZeroCount = 0;
    for (int i = 0; i < sampleCount && i < (int)(background.width() * background.height() * 4); i++) {
        if (bgData[i] > 0) nonZeroCount++;
    }

    qDebug() << "Received background from channel" << channel << "(" << collectedBackgrounds.count() << "of" << backgroundFilters.count() << ")"
             << "Size:" << background.width() << "x" << background.height()
             << "Non-zero samples:" << nonZeroCount << "/" << sampleCount;

    // CHECK IF WE'VE RECEIVED ALL BACKGROUNDS (one per filter)
    if (collectedBackgrounds.count() == backgroundFilters.count()) {
        qDebug() << "All backgrounds received - concatenating into tall image in correct channel order";

        // Get dimensions from the first available background in the map
        LAUMemoryObject firstBg = collectedBackgrounds.first();
        unsigned int width = firstBg.width();
        unsigned int height = firstBg.height();
        unsigned int colors = firstBg.colors();
        unsigned int depth = firstBg.depth();

        // Calculate total stacked height
        unsigned int totalHeight = height * collectedBackgrounds.count();

        // CREATE A NEW MEMORY OBJECT TO HOLD THE STACKED IMAGES
        // Format: LAU_CAMERA_DEFAULT_WIDTH x (LAU_CAMERA_DEFAULT_HEIGHT * 3) for 3 sensors = 640 x 1440
        LAUMemoryObject stackedBackground(width, totalHeight, colors, depth, 1);

        // COPY EACH BACKGROUND INTO THE STACKED IMAGE IN CHANNEL ORDER
        for (int i = 0; i < backgroundFilters.count(); i++) {
            if (!collectedBackgrounds.contains(i)) {
                qWarning() << "Missing background for channel" << i;
                continue;
            }

            unsigned int yOffset = i * height;
            unsigned char *srcPtr = (unsigned char*)collectedBackgrounds[i].constPointer();
            unsigned char *dstPtr = (unsigned char*)stackedBackground.constPointer();

            // Calculate bytes per row
            unsigned int bytesPerRow = width * colors * depth;

            // Copy this background to its position in the stacked image
            for (unsigned int row = 0; row < height; row++) {
                unsigned char *srcRowPtr = srcPtr + (row * bytesPerRow);
                unsigned char *dstRowPtr = dstPtr + ((yOffset + row) * bytesPerRow);
                memcpy(dstRowPtr, srcRowPtr, bytesPerRow);
            }
        }

        // CONCATENATE JETR VECTORS IN CHANNEL ORDER
        QVector<double> concatenatedJetr;
        for (int i = 0; i < backgroundFilters.count(); i++) {
            if (!collectedBackgrounds.contains(i)) {
                continue;
            }
            QVector<double> jetrVector = collectedBackgrounds[i].jetr();
            if (!jetrVector.isEmpty()) {
                concatenatedJetr.append(jetrVector);
            }
        }

        // SET THE CONCATENATED JETR VECTOR IN THE STACKED BACKGROUND
        if (!concatenatedJetr.isEmpty()) {
            stackedBackground.setConstJetr(concatenatedJetr);
            qDebug() << "Concatenated" << concatenatedJetr.size() << "JETR elements from" << collectedBackgrounds.count() << "sensors";
        }

        // SAVE THE STACKED BACKGROUND TO DISK
        // Passing an empty QString will open a file dialog
        // Use the optional savedFilePath parameter to get the absolute path of the saved file
        // Set flag to pause FPS monitoring while file dialog is open
        savingBackground = true;
        bool saveSuccess = stackedBackground.save(QString(), &lastSavedFilename);
        savingBackground = false;

        if (saveSuccess) {
            qDebug() << "Saved stacked background:" << width << "x" << totalHeight << "to" << lastSavedFilename;

            // AUTOMATICALLY COPY TO SHARED FOLDER FOR LAUProcessVideos
            // Determine install/data folder path based on platform
            QString installFolderPath;
#ifdef Q_OS_WIN
            // Windows: Use ProgramData (writable by all users without admin)
            installFolderPath = "C:/ProgramData/3DVideoInspectionTools";
#elif defined(Q_OS_MAC)
            // macOS: Use /Users/Shared (writable by all users)
            installFolderPath = "/Users/Shared/3DVideoInspectionTools";
#else
            // Linux: Use /var/lib (standard for application data)
            installFolderPath = "/var/lib/3DVideoInspectionTools";
#endif

            // Ensure the directory exists
            QDir installDir(installFolderPath);
            if (!installDir.exists()) {
                if (!installDir.mkpath(".")) {
                    qWarning() << "Failed to create install folder:" << installFolderPath;
                } else {
                    qDebug() << "Created install folder:" << installFolderPath;
                }
            }

            // Construct the background file path
            QString sharedBackgroundPath = installDir.absoluteFilePath("background.tif");

            // Copy the saved file to the shared folder
            if (installDir.exists()) {
                // Remove existing background.tif if it exists
                if (QFile::exists(sharedBackgroundPath)) {
                    if (!QFile::remove(sharedBackgroundPath)) {
                        qWarning() << "Failed to remove existing background file:" << sharedBackgroundPath;
                    }
                }

                // Copy the file
                if (QFile::copy(lastSavedFilename, sharedBackgroundPath)) {
                    qDebug() << "Successfully copied background to shared folder:" << sharedBackgroundPath;
                    qDebug() << "LAUProcessVideos will use this calibration file";

                    // Show confirmation to user
                    QMessageBox::information(nullptr, "Background Saved",
                        QString("Background calibration saved successfully!\n\n"
                                "User file: %1\n\n"
                                "Shared folder: %2\n\n"
                                "LAUProcessVideos will now use this calibration.\n"
                                "You can refine the calibration later in LAUJetrStandalone.")
                        .arg(QFileInfo(lastSavedFilename).fileName())
                        .arg(sharedBackgroundPath));
                } else {
                    qWarning() << "Failed to copy background to shared folder:" << sharedBackgroundPath;
                    QMessageBox::warning(nullptr, "Copy Failed",
                        QString("Background saved to your chosen location, but failed to copy to shared folder:\n%1\n\n"
                                "LAUProcessVideos may not work correctly.")
                        .arg(sharedBackgroundPath));
                }
            }
        } else {
            qDebug() << "User cancelled save dialog or save failed";
            lastSavedFilename.clear();
        }

        // CLEAR THE COLLECTED BACKGROUNDS FOR NEXT TIME
        collectedBackgrounds.clear();
    }
}
