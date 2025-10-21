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

#include "lau3dtrackingwidget.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DTrackingWidget::LAU3DTrackingWidget(LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device,  QWidget *parent) : QWidget(parent),
    playbackColor(color), playbackDevice(device), camera(NULL), cameraController(NULL), dftFilter(NULL), glFilter(NULL), glFilterController(NULL),
    dftController(NULL), trackingController(NULL), glWidget(NULL), frameBufferManager(NULL), frameBufferManagerController(NULL), videoLabel(NULL),
    snapShotModeFlag(false), videoRecordingFlag(false), counter(0)
{
    qRegisterMetaType<LAUMemoryObject>("LAUMemoryObject");
    qRegisterMetaType<QList<LAUScan> >("QList<LAUScan>");
    qRegisterMetaType<LAUScan>("LAUScan");

#if defined(LAUMachineVisionCamera)
    prosilicaScannerWidget = NULL;
#endif
    this->setWindowTitle(QString("Video Recorder"));
    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->layout()->setSpacing(0);
    this->setMinimumSize(320, 240);

    camera = LAU3DCameras::getCamera(color, device);

    if (camera && camera->isValid()) {
        // ALLOCATE FRAME BUFFER MANAGER TO MANAGE MEMORY FOR US
        frameBufferManager = new LAUMemoryObjectManager(camera->depthWidth(), camera->depthHeight(), colors(), sizeof(float), 1, NULL);

        // CONNECT SIGNALS AND SLOTS BETWEEN THIS AND THE FRAME BUFFER MANAGER
        connect(this, SIGNAL(emitGetFrame()), frameBufferManager, SLOT(onGetFrame()));
        connect(this, SIGNAL(emitReleaseFrame(LAUMemoryObject)), frameBufferManager, SLOT(onReleaseFrame(LAUMemoryObject)));
        connect(frameBufferManager, SIGNAL(emitFrame(LAUMemoryObject)), this, SLOT(onReceiveFrameBuffer(LAUMemoryObject)));

        // SPIN THE FRAME BUFFER MANAGER INTO ITS OWN THREAD
        frameBufferManagerController = new LAUController(frameBufferManager);

        // NOW ASK THE FRAME BUFFER MANAGER TO GIVE US A SMALL STASH OF 10 OR SO FRAMES
        for (int n = scanList.count(); n < 10; n++) {
            emit emitGetFrame();
        }

        // ALLOCATE MEMORY OBJECTS TO HOLD INCOMING VIDEO FRAMES
        for (int n = 0; n < NUMFRAMESINBUFFER; n++) {
            LAUModalityObject frame;
            frame.depth = camera->depthMemoryObject();
            frame.color = camera->colorMemoryObject();
            frame.mappi = camera->mappiMemoryObject();
            framesList << frame;
        }

        // CREATE THE TRACKING FILTER
        trackingController = new LAU3DTrackingController(camera->depthWidth(), camera->depthHeight());

        // CREATE AN ABSTRACT FILTER TO CONVERT RAW VIDEO TO LAUSCANS
        glFilter = new LAUAbstractGLFilter(camera->depthWidth(), camera->depthHeight(), camera->colorWidth(), camera->colorHeight(), playbackColor, playbackDevice);
        glFilter->setFieldsOfView(camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians());
        glFilter->setLookUpTable(LAULookUpTable(camera->width(), camera->height(), camera->device(), camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians()));

        // CREATE A GLWIDGET TO DISPLAY THE POINT CLOUD
        glWidget = new LAU3DVideoGLWidget(camera->depthWidth(), camera->depthHeight(), playbackColor);
        glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        glWidget->setMinimumSize(320, 240);
        this->layout()->addWidget(glWidget);

#if defined(LAUMachineVisionCamera)
        if (isMachineVision(camera->device())) {
            LAULookUpTable lookUpTable = LAULookUpTable(QString());
            if (lookUpTable.isValid()) {
                glFilter->setLookUpTable(lookUpTable);
                glWidget->setLimits(lookUpTable.xLimits().x(), lookUpTable.xLimits().y(), lookUpTable.yLimits().x(), lookUpTable.yLimits().y(), lookUpTable.zLimits().x(), lookUpTable.zLimits().y());
            } else {
                glWidget->setLimits(-300.0f, 300.0f, -300.0f, 300.0f, -1.0f, -600.0f);
            }

            // CREATE DFT FILTER TO HANDLE RAW VIDEO FROM PROSILICA DEVICE
            if (camera->device() == DeviceProsilicaLCG) {
                dftFilter = new LAUDFTFilter(camera->width(), camera->height(), LAUDFTFilter::PatternEightEightEight);
            } else if (camera->device() == DeviceProsilicaIOS) {
                dftFilter = new LAUDFTFilter(camera->width(), camera->height(), LAUDFTFilter::PatternDualFrequency);
            }

            // CONNECT THE SIGNALS AND SLOTS FOR PROCESSING AND MERGING VIDEO
            connect(this, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), camera, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
            connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), dftFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
            connect(dftFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
            connect(glFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), this, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
            connect(this, SIGNAL(emitBuffer(LAUScan)), glFilter, SLOT(onUpdateBuffer(LAUScan)));
            connect(glFilter, SIGNAL(emitBuffer(LAUScan)), trackingController, SLOT(onUpdateBuffer(LAUScan)));
            connect(trackingController, SIGNAL(emitBuffer(LAUScan)), glWidget, SLOT(onUpdateBuffer(LAUScan)));
            connect(glWidget, SIGNAL(emitBuffer(LAUScan)), this, SLOT(onUpdateBuffer(LAUScan)));

            // CREATE A FILTER CONTROLLER AND GIVE IT THE GLFILTER
            dftController = new LAUAbstractFilterController(dftFilter);
        } else {
#endif
            // CONNECT THE SIGNALS AND SLOTS FOR PROCESSING AND MERGING VIDEO
            connect(this, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), camera, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
            connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
            connect(glFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), this, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
            connect(this, SIGNAL(emitBuffer(LAUScan)), glFilter, SLOT(onUpdateBuffer(LAUScan)));
            connect(glFilter, SIGNAL(emitBuffer(LAUScan)), trackingController, SLOT(onUpdateBuffer(LAUScan)));
            connect(trackingController, SIGNAL(emitBuffer(LAUScan)), glWidget, SLOT(onUpdateBuffer(LAUScan)));
            connect(glWidget, SIGNAL(emitBuffer(LAUScan)), this, SLOT(onUpdateBuffer(LAUScan)));
#if defined(LAUMachineVisionCamera)
        }
#endif
        // CREATE A FILTER CONTROLLER AND GIVE IT THE GLFILTER
        glFilterController = new LAUAbstractFilterController(glFilter);

        // CREATE A THREAD TO HOST THE CAMERA CONTROLLER
        cameraController = new LAU3DCameraController(camera);

        // DO CAMERA SPECIFIC ACTIONS
#if defined(LAUMachineVisionCamera)
        if (camera->device() == DeviceProsilicaLCG || camera->device() == DeviceProsilicaIOS) {
            QAction *action = new QAction(QString("Adjust camera settings..."), NULL);
            action->setCheckable(false);
            connect(action, SIGNAL(triggered()), this, SLOT(onContextMenuTriggered()));
            glWidget->menu()->addAction(action);

            // CREATE A SCANNER WIDGET FOR ADJUSTING SCAN PARAMETERS
            prosilicaScannerWidget = new LAU3DMachineVisionScannerWidget(this);
            connect(prosilicaScannerWidget, SIGNAL(emitUpdateExposure(int)), camera, SLOT(onUpdateExposure(int)));
            connect(prosilicaScannerWidget, SIGNAL(emitUpdateSnrThreshold(int)), glFilter, SLOT(onSetSNRThreshold(int)));
            connect(prosilicaScannerWidget, SIGNAL(emitUpdateMtnThreshold(int)), glFilter, SLOT(onSetMTNThreshold(int)));

            // SYNCHRONIZE THE WIDGET PARAMETERS WITH THE CAMERA AND GLWIDGET
            camera->onUpdateExposure(prosilicaScannerWidget->exp());
            glFilter->onSetSNRThreshold(prosilicaScannerWidget->snr());
            glFilter->onSetMTNThreshold(prosilicaScannerWidget->mtn());
        } else if (camera->device() == DevicePrimeSense || camera->device() == DeviceKinect || camera->device() == DeviceOrbbec) {
            // SET THE BOUNDING BOX FOR THE GLWIDGET
            glWidget->setRangeLimits(camera->minDistance(), camera->maxDistance(), camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians());
            glWidget->setLookUpTable(LAULookUpTable(camera->width(), camera->height(), camera->device(), camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians()));
        }
#else
        if (camera->device() == DevicePrimeSense || camera->device() == DeviceKinect || camera->device() == DeviceOrbbec || camera->device() == DeviceLucid || camera->device() == DeviceVZense || camera->device() == DeviceRealSense || camera->device() == DeviceDemo) {
            // SET THE BOUNDING BOX AND LOOK UP TABLE FOR THE GLWIDGET
            glWidget->setRangeLimits(camera->minDistance(), camera->maxDistance(), camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians());
        }
#endif
    } else {
        // CREATE A GLWIDGET TO PROCESS THE DFT COEFFICIENTS AND DISPLAY THE POINT CLOUD
        LAUAbstractGLWidget *abstractGLWidget = new LAUAbstractGLWidget();
        abstractGLWidget->setMinimumSize(320, 240);
        abstractGLWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        this->layout()->addWidget(abstractGLWidget);

        // TELL THE USER THAT WE COULDN'T FIND A SENSOR TO CONNECT
        if (camera == NULL) {
            QMessageBox::warning(this, QString("Video Recorder"), QString("Invalid device."));
        } else {
            QMessageBox::warning(this, QString("Video Recorder"), camera->error());
        }
    }
    timeStamp.restart();
    videoLabel = new LAUVideoPlayerLabel(LAUVideoPlayerLabel::StateVideoRecorder);
    connect(videoLabel, SIGNAL(playButtonClicked(bool)), this, SLOT(onRecordButtonClicked(bool)));
    this->layout()->addWidget(videoLabel);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DTrackingWidget::~LAU3DTrackingWidget()
{
    if (dftController) {
        delete dftController;
    }
    if (cameraController) {
        delete cameraController;
    }
    if (glFilterController) {
        delete glFilterController;
    }
    if (trackingController) {
        delete trackingController;
    }
    if (frameBufferManagerController) {
        delete frameBufferManagerController;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingWidget::onRecordButtonClicked(bool state)
{
    if (camera && camera->isValid()) {
        if (snapShotModeFlag) {
            videoRecordingFlag = state;
            if (state) {
                // RESET RECORDING FRAME COUNTER AND TIMER AND WE CAN JUST DUMP
                // THE INCOMING VIDEO TO OUR VIDEO FRAME BUFFER LIST
                pressStartButtonTime = QTime::currentTime();
                timeStamp.restart();
            } else {
                // STOP THE FRAME RECORDING
                videoLabel->onPlayButtonClicked(false);
            }
        } else {
            videoRecordingFlag = state;
            if (state) {
                // RESET RECORDING FRAME COUNTER AND TIMER AND WE CAN JUST DUMP
                // THE INCOMING VIDEO TO OUR VIDEO FRAME BUFFER LIST
                pressStartButtonTime = QTime::currentTime();
                timeStamp.restart();
            } else {
                if (recordList.count() > 0) {
                    // EMIT THE ACCUMULATED VIDEO FRAMES THAT WE HAVE JUST STOPPED RECORDING
                    emit emitVideoFrames(recordList);

                    // NOW COPY OVER SOME OF THE VIDEO FRAMES FOR FUTURE RECORDINGS
                    while (scanList.count() < 10 && recordList.count() > 0) {
                        scanList << recordList.takeFirst();
                    }

                    // AND DELETE ANY REMAINING VIDEO FRAMES
                    while (scanList.count() > 10) {
                        emitReleaseFrame(scanList.takeFirst());
                    }
                    while (recordList.count() > 0) {
                        emitReleaseFrame(recordList.takeFirst());
                    }

                    // RESET THE PROGRESS BAR TO SHOW NO VIDEO IS CURRENTLY RECORDED
                    videoLabel->onPlayButtonClicked(false);
                    videoLabel->onUpdateSliderPosition(0.0f);
                    videoLabel->onUpdateTimeStamp(0);
                }
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingWidget::onReceiveFrameBuffer(LAUMemoryObject buffer)
{
    scanList << LAUScan(buffer, playbackColor);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingWidget::onReceiveVideoFrames(LAUScan scan)
{
    // CREATE A LAUSCAN TO HOLD THE INCOMING SNAP SHOT
    if (scan.isValid()) {
        scan.updateLimits();
        scan.setSoftware(QString("Lau 3D Video Recorder"));
        scan.setMake(camera->make());
        scan.setModel(camera->model());

        // ASK THE USER TO APPROVE THE SCAN BEFORE SAVING TO DISK
        while (scan.approveImage()) {
            // NOW LET THE USER SAVE THE SCAN TO DISK
            if (scan.save() == true) {
                break;
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingWidget::onReceiveVideoFrames(QList<LAUScan> scanList)
{
    // NOW WE NEED TO PASS OUR VIDEO FRAMES TO A DEPTH VIDEO OBJECT
    // WHICH CAN THEN BRING UP A VIDEO PLAYER WIDGET TO REPLAYING
    // THE VIDEO ON SCREEN, AND GIVE THE USER THE CHANCE TO SAVE TO DISK
    if (scanList.count() > 0) {
        // CREATE THE VIDEO PLAYER WIDGET AND SEND IT THE VIDEO FRAMES
        LAU3DVideoPlayerWidget *replayWidget = new LAU3DVideoPlayerWidget(camera->width(), camera->height(), playbackColor, this);

        // DISPLAY THE REPLAY WIDGET SO THE USER CAN SEE THE FINE MERGE'S PROGRESS
        // SEND THE RECORDED FRAME PACKETS TO OUR REPLAY VIDEO WIDGET FOR PLAYBACK
        while (!scanList.isEmpty()) {
            replayWidget->onInsertPacket(scanList.takeFirst());
        }

        // GET THE VIEW LIMITS OF THE GPU WIDGET AND COPY THEM TO THE REPLAY WIDGET
        QVector2D xLimits = glWidget->xLimits();
        QVector2D yLimits = glWidget->yLimits();
        QVector2D zLimits = glWidget->zLimits();

        replayWidget->setLimits(xLimits.x(), xLimits.y(), yLimits.x(), yLimits.y(), zLimits.x(), zLimits.y());
        replayWidget->setAttribute(Qt::WA_DeleteOnClose);
        replayWidget->show();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingWidget::onContextMenuTriggered()
{
#if defined(LAUMachineVisionCamera)
    if (prosilicaScannerWidget) {
        prosilicaScannerWidget->hide();
        prosilicaScannerWidget->show();
    }
#endif
    qDebug() << "LAU3DVideoWidget::onContextMenuTriggered()";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingWidget::onUpdateBuffer(LAUScan scan)
{
    // SEE IF WE SHOULD KEEP THIS PARTICULAR SCAN
    if (scan.isValid()) {
        if (videoRecordingFlag == true) {
            if (snapShotModeFlag) {
                // EMIT THE INCOMING VIDEO FRAME TO OUR SNAP SHOT HANDLER
                emit emitVideoFrames(scan);

                // AND STOP GRABBING ANY MORE
                onRecordButtonClicked(false);

                // REQUEST A NEW BLANK SCAN OBJECT TO OUR SCAN LIST FOR FUTURE FRAMES
                emit emitGetFrame();
            } else {
                // HAVE THE VIDEO LABEL UPDATE ITS PROGRESS BAR SO THE USER KNOWS HOW MUCH SPACE IS LEFT
                videoLabel->onUpdateSliderPosition((float)recordList.count() / (float)MAXRECORDEDFRAMECOUNT);
                videoLabel->onUpdateTimeStamp(scan.elapsed());

                // IF WE ARE RECORDING, THEN DIRECT THE INCOMING FRAME TO OUR RECORD LIST
                recordList << scan;

                // MAKE SURE WE HAVEN'T FILLED OUR RECORDING BUFFER
                if (recordList.count() >= MAXRECORDEDFRAMECOUNT) {
                    // AND STOP GRABBING ANY MORE IF SO
                    onRecordButtonClicked(false);
                }

                // REQUEST A NEW BLANK SCAN OBJECT TO OUR SCAN LIST FOR FUTURE FRAMES
                emit emitGetFrame();
            }
        } else {
            // IF WE ARE NOT RECORDING, THEN DIRECT THE INCOMING FRAME TO OUR SCAN LIST FOR FUTURE FRAMES
            scanList << scan;
        }
    }

    // EMIT THE SIGNAL TO TELL THE FRAME GRABBER TO GRAB THE NEXT SET OF FRAMES
    if (isVisible()) {
        while (framesList.count() > 0) {
            LAUModalityObject frame = framesList.takeFirst();
            emit emitBuffer(frame.depth, frame.color, frame.mappi);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingWidget::onUpdateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    // CONSTRUCT A VIDEO MEMORY OBJECT FROM THE INCOMING MEMORY OBJECTS
    LAUModalityObject frame(depth, color, mapping);

    // SEE IF WE SHOULD KEEP THIS PARTICULAR FRAME
    if (frame.isAnyValid()) {
        framesList << frame;
    }

    // UPDATE THE TEXTURE BUFFERS IF WE HAVE AT LEAST ONE VALID POINTER
    if (depth.isValid() || color.isValid()) {
        // REPORT FRAME RATE TO THE CONSOLE
        counter++;
        if (counter >= 30) {
            qDebug() << QString("%1 fps").arg(1000.0 * (float)counter / (float)time.elapsed());
            time.restart();
            counter = 0;
        }
    }

    // EMIT THE SIGNAL TO TELL THE FRAME GRABBER TO GRAB THE NEXT SET OF FRAMES
    if (isVisible()) {
        if (scanList.count() > 0) {
            LAUScan scan = scanList.takeFirst();
            if (depth.isValid()) {
                scan.setConstAnchor(depth.anchor());
            }
            scan.setConstElapsed(timeStamp.elapsed());
            emit emitBuffer(scan);
        }
    }
}
