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

#include "laucascadeclassifierfromlivevideo.h"

#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QFile>
#include "lau3dvideoglwidget.h"
#include "laucascadeclassifierglfilter.h"

#ifdef ENABLEFILTERS
#include "lausavetodiskfilter.h"
#include "laucolorizedepthglfilter.h"
#include "laugreenscreenglfilter.h"
#endif

#if defined(LUCID)
#include "laulucidcamera.h"
#endif

#if defined(ORBBEC)
#include "lauorbbeccamera.h"
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCascadeClassifierFromLiveVideo::LAUCascadeClassifierFromLiveVideo(QWidget *parent) : QDialog(parent), shutDownFlag(false), filterCount(0), sensorCount(0), cameraCount(0), directoryString(QString()), rfidHashTable(nullptr), rfidObject(nullptr), saveToDiskFilter(nullptr), dataFileCount(-1), callCount(0), monitoringStarted(false), channel(0)
{
    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->setWindowTitle(QString("Raw Video Processor"));

    // CONNECT TO CAMERA
    LAUVideoPlaybackDevice device = DeviceUndefined;
    LAU3DCamera *camera = nullptr;

#if defined(ORBBEC)
#if defined(RECORDRAWVIDEOTODISK)
    camera = new LAUOrbbecCamera(ColorXYZRGB);
#else
    camera = new LAUOrbbecCamera(ColorXYZ);
#endif
    if (camera->isValid()) {
        if (device == DeviceUndefined) {
            device = DeviceOrbbec;
        }
        camera->setStartingFrameIndex(sensorCount);
        sensorCount += camera->sensors();
        cameras << camera;
    } else {
        errorString.append(QString("::")).append(camera->error());
        camera->deleteLater();
    }
#endif

#if defined(LUCID)
    camera = new LAULucidCamera(QString("Distance4000mmSingleFreq"), ColorXYZ);
    if (camera->isValid()) {
        if (device == DeviceUndefined) {
            device = DeviceLucid;
        }
        camera->setStartingFrameIndex(sensorCount);
        sensorCount += camera->sensors();
        cameras << camera;
    } else {
        errorString.append(QString("::")).append(camera->error());
        camera->deleteLater();
    }
#endif

    // Initialize the frame rate monitoring timer
    frameRateTimer = new QTimer(this);
    frameRateTimer->setInterval(measurementIntervalMs);
    frameRateTimer->setSingleShot(false);

    // Connect timer to our checking slot
    connect(frameRateTimer, SIGNAL(timeout()), this, SLOT(checkFrameRate()));

    // Only start the timer if we have cameras connected
    if (sensorCount > 0) {
        // Start the timer and elapsed timer
        frameRateTimer->start();
        elapsedTimer.start();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCascadeClassifierFromLiveVideo::LAUCascadeClassifierFromLiveVideo(QString dirString, int threshold, QWidget *parent) : QDialog(parent), shutDownFlag(false), filterCount(0), sensorCount(0), cameraCount(0), directoryString(dirString), rfidHashTable(nullptr), rfidObject(nullptr), saveToDiskFilter(nullptr), dataFileCount(-1), callCount(0), monitoringStarted(false), channel(0)
{
    Q_UNUSED(threshold);
    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->setWindowTitle(QString("Raw Video Processor"));

    // CONNECT TO CAMERA
    LAUVideoPlaybackDevice device = DeviceUndefined;
    LAU3DCamera *camera = nullptr;
    QList<QVector<double>> jetrVectors;

#if defined(ORBBEC)
#if defined(RECORDRAWVIDEOTODISK)
    camera = new LAUOrbbecCamera(ColorXYZRGB);
#else
    camera = new LAUOrbbecCamera(ColorXYZ);
#endif
    if (camera->isValid()) {
        if (device == DeviceUndefined) {
            device = DeviceOrbbec;
        }
        for (unsigned int n = 0; n < camera->sensors(); n++){
            jetrVectors << camera->jetr(n);
        }
        camera->setStartingFrameIndex(sensorCount);
        sensorCount += camera->sensors();
        cameras << camera;
    } else {
        errorString.append(QString("::")).append(camera->error());
        camera->deleteLater();
    }
#endif

#if defined(LUCID)
    camera = new LAULucidCamera(QString("Distance4000mmSingleFreq"), ColorXYZ);
    if (camera->isValid()) {
        if (device == DeviceUndefined) {
            device = DeviceLucid;
        }
        for (unsigned int n = 0; n < camera->sensors(); n++){
            jetrVectors << camera->jetr(n);
        }
        camera->setStartingFrameIndex(sensorCount);
        sensorCount += camera->sensors();
        cameras << camera;
    } else {
        errorString.append(QString("::")).append(camera->error());
        camera->deleteLater();
    }
#endif

#if defined(LUCID)
    // Program Lucid camera labels from systemConfig.ini before processing
    // Camera labels get cleared when cameras restart, so we restore them from the config file
    QString iniPath = QDir::currentPath() + "/systemConfig.ini";
    if (QFile::exists(iniPath)) {
        qDebug() << "Found systemConfig.ini, loading camera positions...";
        QSettings settings(iniPath, QSettings::IniFormat);

        // Read all camera positions from INI [CameraPosition] section
        settings.beginGroup("CameraPosition");
        QStringList serialNumbers = settings.allKeys();
        settings.endGroup();

        if (serialNumbers.count() >= 2) {
            qDebug() << "Found" << serialNumbers.count() << "camera positions in INI";

            // Build list of positions in order (we expect 2 Lucid cameras)
            QStringList positions;
            for (const QString &serialNumber : serialNumbers) {
                QString position = settings.value(QString("CameraPosition/%1").arg(serialNumber)).toString();
                positions.append(position);
                qDebug() << "  S/N" << serialNumber << "→" << position;
            }

            // Program the cameras with these positions
            QString errorMessage;
            QStringList progressMessages;
            bool success = LAULucidCamera::setUserDefinedNames(positions, errorMessage, progressMessages);

            if (success) {
                qDebug() << "Successfully programmed Lucid camera labels from INI";
            } else {
                qWarning() << "Failed to program Lucid camera labels:" << errorMessage;
                // Don't exit - continue anyway, cameras will use serial numbers
            }
        } else {
            qDebug() << "No camera positions found in INI, cameras will use serial numbers";
        }
    } else {
        qDebug() << "No systemConfig.ini found, cameras will use serial numbers";
    }
#endif

    // Check if sensor count is correct (expecting 3 sensors for Lucid or Orbbec)
    if (sensorCount != 3) {
        if (logFile.isOpen()) {
            logTS << "Sensor count too low (" << sensorCount << "). Automatically triggering power cycle and retry.";
            logTS.flush();
        } else {
            qWarning() << "Sensor count too low (" << sensorCount << "). Automatically triggering power cycle and retry.";
        }

        // Automatically trigger relay cycle and wait (no user interaction needed for remote operation)
        if (logFile.isOpen()) {
            logTS << "Triggering automatic camera power cycle...";
            logTS.flush();
        } else {
            qInfo() << "Triggering automatic camera power cycle...";
        }

        // Trigger relay cycle and wait
        triggerRelayCyclingAndWait();

        // Log that power cycle is complete and application will exit for restart
        if (logFile.isOpen()) {
            logTS << "Camera power cycle complete. Application will now exit for restart.";
            logTS.flush();
        } else {
            qInfo() << "Camera power cycle complete. Application will now exit for restart.";
        }

        QApplication::quit();
        return;
    }
    this->setMinimumHeight(camera->height());
    this->setMinimumWidth(camera->width());

    // MAKE SURE WE HAVE CONNECTED TO AT LEAST ONE CAMERA
    if (sensorCount > 0) {
        // CONNECT ALL THE CAMERAS TOGETHER
        for (int s = 0; s < cameras.count(); s++) {
            LAU3DCamera *camera = cameras.at(s);
            connect(camera, SIGNAL(emitError(QString)), this, SLOT(onCameraError(QString)), Qt::QueuedConnection);
            connect(camera, SIGNAL(destroyed()), this, SLOT(onCameraDeleted()), Qt::QueuedConnection);
            if ((s + 1) < cameras.count()) {
#ifdef SHARED_CAMERA_THREAD
                connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), cameras.at(s + 1), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::DirectConnection);
#else
                connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), cameras.at(s + 1), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
#endif
            }

            // CREATE A THREAD TO HOST THE CAMERA CONTROLLER
            cameraControllers << new LAU3DCameraController(camera);
            cameraCount++;
        }

        // CONNECT TO RFID TAG READER
        rfidObject = new LAURFIDObject(QString("COM1"));
        connect(rfidObject, SIGNAL(emitError(QString)), this, SLOT(onRFIDError(QString)), Qt::QueuedConnection);
        connect(rfidObject, SIGNAL(emitRFID(QString, QTime)), this, SLOT(onRFID(QString, QTime)), Qt::QueuedConnection);

#ifdef ENABLEFILTERS
        // PRESERVE STATUS OF RFID TAG READER IN SAVED FILES
        if (rfidObject->isValid()) {
#ifdef USE_RFID_MAPPING
            // DEFINE RFID HASH TABLE TO CONVERT RFID STRINGS INTO OBJECT ID STRINGS
            rfidHashTable = new LAUObjectHashTable(QString("C:/Users/Public/Documents/objectIDList.csv"));
#endif
            QSettings settings;
            rfidString = settings.value("LAUCascadeClassifierFromLiveVideo::rfidString", QString("empty")).toString();
        } else {
            rfidString = QString("not connected");
        }
#endif
        // LOAD THE BACKGROUND FROM INSTALL FOLDER
        // First try to load from the calibrated background file saved by LAUJetrStandalone
        // Use writable shared directories that don't require admin privileges
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

        QString backgroundFilePath = QString("%1/background.tif").arg(installFolderPath);
        LAUMemoryObject background;

        // Try to load the calibrated background file
        if (QFile::exists(backgroundFilePath)) {
            background = LAUMemoryObject(backgroundFilePath);
            qDebug() << "Loaded calibrated background from:" << backgroundFilePath;
            qDebug() << "Background dimensions:" << background.width() << "x" << background.height();
            qDebug() << "Background frames:" << background.frames();
            qDebug() << "Background JETR elements:" << background.jetr().size();
        } else {
            qWarning() << "Calibrated background file not found at:" << backgroundFilePath;
            qWarning() << "Falling back to QSettings for backward compatibility";

            // FALLBACK: Load from QSettings (old method for backward compatibility)
            QSettings settings;
            QByteArray byteArray(width() * height() * sizeof(unsigned short), (char)0xff);
            background = LAUMemoryObject(width(), height(), 1, sizeof(unsigned short), sensorCount);
            QVector<double> jetr;

            int frm = 0;
            for (int snr = 0; snr < cameras.count(); snr++){
                LAU3DCamera *camera = cameras.at(snr);
                for (unsigned int lcl = 0; lcl < camera->sensors(); lcl++) {
                    QString frameString, jetrvString;
                    if (camera->device() == DeviceLucid) {
                        frameString = QString("LAUBackgroundGLFilter::backgroundTexture::DeviceLucid::%1").arg(lcl);
                        jetrvString = QString("LAUBackgroundGLFilter::jetrVector::DeviceLucid::%1").arg(lcl);
                    } else if (camera->device() == DeviceOrbbec) {
                        frameString = QString("LAUBackgroundGLFilter::backgroundTexture::DeviceOrbbec::%1").arg(lcl);
                        jetrvString = QString("LAUBackgroundGLFilter::jetrVector::DeviceOrbbec::%1").arg(lcl);
                    } else {
                        frameString = QString("LAUBackgroundGLFilter::backgroundTexture::backgroundTexture::%1").arg(lcl);
                        jetrvString = QString("LAUBackgroundGLFilter::jetrVector::backgroundTexture::%1").arg(lcl);
                    }
                    byteArray = settings.value(frameString, byteArray).toByteArray();
                    memcpy(background.constFrame(frm++), byteArray.data(), background.block());

                    QVariantList list = settings.value(jetrvString).toList();
                    for (const QVariant& variant : list) {
                        jetr.append(variant.toDouble());
                    }
                }
            }
            // SET THE JETR VECTOR AS THE CONCATENATION OF ALL JETR VECTORS FROM ALL CAMERAS
            background.setConstJetr(jetr);
        }

        // NOW COMBINE JETR VECTORS FROM THE CAMERAS (for video frames)
        QVector<double> jetr;
        for (int n = 0; n < jetrVectors.count(); n++){
            jetr.append(jetrVectors.at(n));
        }

        // CREATE A LOG FILE TO STORE VIDEO METRICS APPEDNING TO ANY EXISTING FILE
        logFile.setFileName(QString("%1/LAUCascadeClassifierFromLiveVideo.txt").arg(directoryString));
        logFile.open(QIODevice::Append);
        logTS.setDevice(&logFile);

        // ALLOCATE MEMORY OBJECTS TO HOLD INCOMING VIDEO FRAMES
        for (int n = 0; n < NUMFRAMESINBUFFER; n++) {
            LAUModalityObject frame;
            frame.depth = LAUMemoryObject(cameras.first()->width(), cameras.first()->height(), 1, sizeof(unsigned short), sensorCount);
            frame.depth.setJetr(jetr);
#if defined(RECORDRAWVIDEOTODISK) && defined(ORBBEC)
            frame.color = cameras.last()->colorMemoryObject();
#else
            frame.color = LAUMemoryObject();
#endif
            frame.mappi = LAUMemoryObject();
            framesList << frame;
        }

        // CREATE A GLWIDGET TO DISPLAY GRAYSCALE VIDEO
        LAU3DVideoGLWidget *glWidget;
#if defined(RECORDRAWVIDEOTODISK) && defined(ORBBEC)
        glWidget = new LAU3DVideoGLWidget(cameras.first()->width(), cameras.first()->height(), cameras.first()->width(), cameras.first()->height(), ColorRGB, Device2DCamera);
#else
        glWidget = new LAU3DVideoGLWidget(cameras.first()->width(), cameras.first()->height(), cameras.first()->width(), cameras.first()->height(), ColorGray, Device2DCamera);
#endif
        glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        glWidget->setMaximumIntensityValue(16000);
        channel = 0;
        glWidget->onSetCamera(channel);
        glWidget->onFlipScan(true);
        this->layout()->addWidget(glWidget);
        
        // Set initial window title with channel
        this->setWindowTitle(QString("Channel %1").arg(channel % sensorCount));

        // Create timer to switch channels every 5 seconds
        QTimer* channelSwitchTimer = new QTimer();
        channelSwitchTimer->setInterval(5000);
        connect(channelSwitchTimer, &QTimer::timeout, glWidget, &LAU3DVideoGLWidget::incrementChannel);
        connect(channelSwitchTimer, &QTimer::timeout, this, &LAUCascadeClassifierFromLiveVideo::updateWindowTitleWithChannel);
        connect(this, &QObject::destroyed, channelSwitchTimer, &QObject::deleteLater);
        channelSwitchTimer->start();

#ifdef ENABLEFILTERS
        // CREATE A GLCONTEXT FILTER
        saveToDiskFilter = new LAUSaveToDiskFilter(directoryString);
        saveToDiskFilter->setHeader(background);
#ifdef RECORDRAWVIDEOTODISK
        saveToDiskFilter->onRecordButtonClicked(true);
#endif
#endif

#ifdef USE_GREENSCREEN_FILTER
        // CREATE A CLASSIFIER FILTER TO DETECT OBJECT FEATURES
        LAUGreenScreenGLFilter *abstractFilter = new LAUGreenScreenGLFilter(cameras.first()->width(), cameras.first()->height(), cameras.first()->color(), cameras.first()->device());
        abstractFilter->onSetBackgroundTexture(background);
        abstractFilter->enablePixelCount(true);
        abstractFilter->setTriggerThreshold(threshold);
        abstractFilter->setCamera(0);
#else
        // CREATE A TEMPORARY FILE ON DISK TO HOLD THE CLASSIFIER
        QFile xmlFile(QString("%1/LAUCascadeFilterTool.xml").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
        if (xmlFile.open(QIODevice::WriteOnly)) {
            QFile file(":/CLASSIFIERS/cascade.xml");
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray byteArray = file.readAll();
                xmlFile.write(byteArray);
                xmlFile.close();
                file.close();
            } else {
                qDebug() << file.errorString();
            }
            xmlFile.close();
        }

        // CREATE A CLASSIFIER FILTER TO DETECT OBJECT FEATURES
#ifdef ENABLECASCADE
        LAUCascadeClassifierGLFilter *abstractFilter = new LAUCascadeClassifierGLFilter(QFileInfo(xmlFile).absoluteFilePath(), cameras.first()->width(), cameras.first()->height(), ColorXYZ, DeviceOrbbec);
#endif
#endif
        // CONNECT OUR SIGNALS AND SLOTS BETWEEN THIS OBJECT AND THE CAMERA
        connect(this, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), cameras.first(), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
#ifdef ENABLECASCADE
        connect(cameras.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), abstractFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(abstractFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), saveToDiskFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
#else
        connect(cameras.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), saveToDiskFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
#endif
        connect(saveToDiskFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(saveToDiskFilter, SIGNAL(emitNewRecordingOpened(int)), this, SLOT(onNewRecordingOpened(int)), Qt::QueuedConnection);
        connect(glWidget, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), this, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);

        if (saveToDiskFilter) {
            connect(saveToDiskFilter, SIGNAL(destroyed()), this, SLOT(onFilterDestroyed()));
        }
#ifdef ENABLECASCADE
        connect(abstractFilter, SIGNAL(destroyed()), this, SLOT(onFilterDestroyed()));
#endif
        // SPIN THE GREEN SCREEN FILTER INTO ITS OWN CONTROLLER
        if (saveToDiskFilter) {
            filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractFilter *)saveToDiskFilter));
        }
#ifdef USE_GREENSCREEN_FILTER
        filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractGLFilter *)abstractFilter));
#else
#ifdef ENABLECASCADE
        filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractGLFilter *)abstractFilter));
#endif
#endif
        filterCount = filterControllers.count();
    }

    // Initialize the frame rate monitoring timer
    frameRateTimer = new QTimer(this);
    frameRateTimer->setInterval(measurementIntervalMs);
    frameRateTimer->setSingleShot(false);

    // Connect timer to our checking slot
    connect(frameRateTimer, SIGNAL(timeout()), this, SLOT(checkFrameRate()));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCascadeClassifierFromLiveVideo::~LAUCascadeClassifierFromLiveVideo()
{
    // DELETE THE FILTER CONTROLLERS
    while (cameraControllers.isEmpty() == false) {
        delete cameraControllers.takeFirst();
    }

    // DELETE THE FILTER CONTROLLERS
    while (filterControllers.isEmpty() == false) {
        delete filterControllers.takeFirst();
    }

    // SAVE THE RFID STRING FOR THE NEXT TIME THE APPLICATION IS RUN
    QSettings settings;
    settings.setValue("LAUCascadeClassifierFromLiveVideo::rfidString", rfidString);
    settings.setValue("LAUCascadeClassifierFromLiveVideo::lastTimeRun", QTime().currentTime());

    // WAIT UNTIL ALL THE FILTERS HAVE BEEN DESTROYED
    while (filterCount > 0) {
        qApp->processEvents();
    }

    // WAIT UNTIL ALL THE FILTERS HAVE BEEN DESTROYED
    while (cameraCount > 0) {
        qApp->processEvents();
    }

    // DELETE THE RFID HASH TABLE
    if (rfidHashTable) {
        delete rfidHashTable;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCascadeClassifierFromLiveVideo::accept()
{
    // CLOSE THE LOG FILE
    if (logFile.isOpen()) {
        logFile.close();
    }

    // CALL THE BASE CLASS METHOD
    QDialog::accept();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCascadeClassifierFromLiveVideo::onCameraError(QString string)
{
    // EXPORT ERROR MESSAGES TO THE LOG FILE
    if (logFile.isOpen()) {
        logTS << "ERROR ERROR ERROR ::" << string << "\n";
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCascadeClassifierFromLiveVideo::onRFID(QString string, QTime time)
{
    // SEE IF THIS IS AN OLD RFID AND IF SO, IGNORE IT
    if (dataFileCount > -1){
        if (oldRFIDs.contains(string)){
            return;
        } else {
            oldRFIDs << string;
        }
    }

#ifdef ENABLEFILTERS
    if (rfidHashTable) {
        rfidString = rfidHashTable->idString(string, time);
    } else {
        rfidString = string;
    }
#else
    rfidString = string;
#endif
    rfidTime = time;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCascadeClassifierFromLiveVideo::onUpdateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    // Start the timer and elapsed timer
    if (monitoringStarted == false){
        frameRateTimer->start();
        elapsedTimer.start();
        monitoringStarted = true;
    }

    // Increment call counter
    ++callCount;

    // CONSTRUCT A VIDEO MEMORY OBJECT FROM THE INCOMING MEMORY OBJECTS
    LAUModalityObject frame(depth, color, mapping);

    // SEE IF WE SHOULD KEEP THIS PARTICULAR FRAME
    if (depth.isValid()) {
        // EXPORT VIDEO METRICS TO THE LOG FILE
        if (logFile.isOpen()) {
            logTS << depth.elapsed() << ", " << rfidString << ", " << depth.anchor().x() << ", " << depth.anchor().y() << "\n";
        }

        // SET RFID STRING WITH LATEST RECEIVED STRING
        depth.setConstRFID(rfidString);

        // SEE IF WE ARE CURRENTLY TRACKING A TAIL
        if (shutDownFlag) {
            if (depth.anchor().x() < 0) {
                this->accept();
            }
        }

        // ADD INCOMING FRAMES TO THE QUE
        framesList << frame;
    }

    // EMIT THE SIGNAL TO TELL THE FRAME GRABBER TO GRAB THE NEXT SET OF FRAMES
    while (framesList.isEmpty() == false) {
        // GET THE NEXT AVAILABLE FRAME BUFFER FROM OUR BUFFER LIST
        LAUModalityObject frame = framesList.takeFirst();

        // EMIT THE OBJECT BUFFERS TO THE VIDEO GLWIDGET
        emit emitBuffer(frame.depth, frame.color, frame.mappi);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCascadeClassifierFromLiveVideo::updateWindowTitleWithChannel()
{
    // Increment channel first
    channel++;
    // Update window title with current channel number
    setWindowTitle(QString("Channel %1").arg(channel % sensorCount));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCascadeClassifierFromLiveVideo::checkFrameRate()
{
    // Calculate elapsed time in seconds
    qint64 elapsedMs = elapsedTimer.elapsed();
    double elapsedSeconds = elapsedMs / 1000.0;

    // Calculate average calls per second
    double callsPerSecond = callCount / elapsedSeconds;

    qDebug() << "Average calls per second over last" << elapsedSeconds
             << "seconds:" << callsPerSecond;

    // Check if frame rate is too low
    if (callsPerSecond < minCallsPerSecond) {
        if (logFile.isOpen()) {
            logTS << "Frame rate too low (" << callsPerSecond << " calls/sec). Triggering camera power cycle.";
            logTS.flush();
        } else {
            qWarning() << "Frame rate too low (" << callsPerSecond
                       << " calls/sec). Triggering camera power cycle.";
        }
        
        // Attempt camera power cycle before quitting
        triggerRelayCyclingAndWait();
        
        QApplication::quit();
        return;
    }

    // Reset counters for next measurement period
    callCount = 0;
    elapsedTimer.restart();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCascadeClassifierFromLiveVideo::triggerRelayCyclingAndWait()
{
    if (logFile.isOpen()) {
        logTS << "Attempting camera power cycle via LAUOnTrakWidget relay..." << "\n";
        logTS.flush();
    } else {
        qDebug() << "Attempting camera power cycle via LAUOnTrakWidget relay...";
    }

    QLocalSocket socket;
    socket.connectToServer("LAUOnTrakWidget");
    
    if (socket.waitForConnected(5000)) {
        socket.write("CYCLE_RELAYS");
        socket.waitForBytesWritten();
        
        if (socket.waitForReadyRead(5000)) {
            QByteArray response = socket.readAll();
            if (response.contains("OK")) {
                if (logFile.isOpen()) {
                    logTS << "Relay cycling initiated successfully. Waiting 120 seconds for cameras to fully reboot..." << "\n";
                    logTS.flush();
                } else {
                    qDebug() << "Relay cycling initiated successfully. Waiting 120 seconds for cameras to fully reboot...";
                }

                // Wait for full relay cycle + camera boot time (120 seconds = 2 minutes)
                // Increased from 60 seconds to allow more time for cameras to fully initialize
                QEventLoop loop;
                QTimer::singleShot(120000, &loop, &QEventLoop::quit);
                loop.exec();

                if (logFile.isOpen()) {
                    logTS << "Camera power cycle complete. Application will now exit." << "\n";
                    logTS.flush();
                } else {
                    qDebug() << "Camera power cycle complete. Application will now exit.";
                }
            } else {
                if (logFile.isOpen()) {
                    logTS << "Relay cycling failed: " << response << "\n";
                    logTS.flush();
                } else {
                    qDebug() << "Relay cycling failed:" << response;
                }
            }
        } else {
            if (logFile.isOpen()) {
                logTS << "No response from LAUOnTrakWidget" << "\n";
                logTS.flush();
            } else {
                qDebug() << "No response from LAUOnTrakWidget";
            }
        }
        socket.disconnectFromServer();
    } else {
        if (logFile.isOpen()) {
            logTS << "Failed to connect to LAUOnTrakWidget: " << socket.errorString() << "\n";
            logTS.flush();
        } else {
            qDebug() << "Failed to connect to LAUOnTrakWidget:" << socket.errorString();
        }
    }
}
