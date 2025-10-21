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

#include "lau3dvideorecordingwidget.h"
#include "laurfidwidget.h"

#ifdef RECORDRAWVIDEOTODISK
#include "lausavetodiskfilter.h"
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DVideoRecordingWidget::LAU3DVideoRecordingWidget(LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent) : LAU3DVideoWidget(color, device, parent), videoLabel(nullptr), snapShotModeFlag(false), videoRecordingFlag(false), scannerModeFlag(false), scannerModeTriggerFlag(false)
#ifndef EXCLUDE_LAUVELMEXWIDGET
    , velmexWidget(nullptr)
#endif
{
    // SET THE FOCUS IN ORDER TO RECEIVE KEYPRESS EVENTS
    this->setFocusPolicy(Qt::StrongFocus);

#ifdef RECORDRAWVIDEOTODISK
    LAUSaveToDiskFilter *filter = nullptr;
#endif

    // SEE IF WE HAVE A VALID CAMERA IN ORDER TO DETERMINE IF WE NEED A FRAME BUFFER MANAGER
    if (camera && camera->isValid()) {
#ifdef RECORDRAWVIDEOTODISK
        // CREATE A FILTER TO RECORD RAW VIDEO TO DISK
        filter = new LAUSaveToDiskFilter(QString());
#endif
        // ALLOCATE FRAME BUFFER MANAGER TO MANAGE MEMORY FOR US
        frameBufferManager = new LAUMemoryObjectManager(camera->width(), camera->height(), colors(), sizeof(float), camera->sensors(), nullptr);

        // CONNECT SIGNALS AND SLOTS BETWEEN THIS AND THE FRAME BUFFER MANAGER
        connect(this, SIGNAL(emitGetFrame()), frameBufferManager, SLOT(onGetFrame()), Qt::QueuedConnection);
        connect(this, SIGNAL(emitReleaseFrame(LAUMemoryObject)), frameBufferManager, SLOT(onReleaseFrame(LAUMemoryObject)), Qt::QueuedConnection);
        connect(frameBufferManager, SIGNAL(emitFrame(LAUMemoryObject)), this, SLOT(onReceiveFrameBuffer(LAUMemoryObject)), Qt::QueuedConnection);

        // SPIN THE FRAME BUFFER MANAGER INTO ITS OWN THREAD
        frameBufferManagerController = new LAUController(frameBufferManager);

        // NOW ASK THE FRAME BUFFER MANAGER TO GIVE US A SMALL STASH OF 10 OR SO FRAMES
        for (int n = videoFramesBufferList.count(); n < 4; n++) {
            emit emitGetFrame();
        }
    }

    // ADD A VIDEO LABEL TO THE BOTTOM OF THE WINDOW
    videoLabel = new LAUVideoPlayerLabel(LAUVideoPlayerLabel::StateVideoRecorder);
#ifdef RECORDRAWVIDEOTODISK
    if (filter) {
        // CONNECT THE RECORD BUTTON TO THE SAVE TO DISK FILTER
        connect(videoLabel, SIGNAL(playButtonClicked(bool)), filter, SLOT(onRecordButtonClicked(bool)), Qt::QueuedConnection);

        // APPEND SAVE TO DISK FILTER INTO FILTER CHAIN
        this->appendFilter(filter);
    } else {
        // CONNECT THE RECORD BUTTON TO THIS OBJECT TO SAVE VIDEO TO THE USER INTERFACE
        connect(videoLabel, SIGNAL(playButtonClicked(bool)), this, SLOT(onRecordButtonClicked(bool)), Qt::QueuedConnection);
    }
#else
    // CONNECT THE RECORD BUTTON TO THIS OBJECT TO SAVE VIDEO TO THE USER INTERFACE
    connect(videoLabel, SIGNAL(playButtonClicked(bool)), this, SLOT(onRecordButtonClicked(bool)), Qt::QueuedConnection);
#endif
    // ADD THE VIDEO RECORDING CONTROLS TO THE USER INTERFACE
    this->layout()->addWidget(videoLabel);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DVideoRecordingWidget::~LAU3DVideoRecordingWidget()
{
    // DELETE ALL ACCUMULATED VIDEO FRAME BUFFERS
    while (videoFramesBufferList.isEmpty() == false) {
        emit emitReleaseFrame(videoFramesBufferList.takeFirst());
    }

#ifndef EXCLUDE_LAUVELMEXWIDGET
    // DELETE THE VELMEX WIDGET IF IT EXISTS
    if (velmexWidget) {
        delete velmexWidget;
    }
#endif

    // CLEAR THE RECORDED VIDEO FRAMES LIST
    recordedVideoFramesBufferList.clear();

    qDebug() << QString("LAU3DVideoRecordingWidget::~LAU3DVideoRecordingWidget()");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoRecordingWidget::onRecordButtonClicked(bool state)
{
    if (camera && camera->isValid()) {
#ifndef IDS
        if (snapShotModeFlag) {
            if (state) {
                // GRAB A COPY OF THE PACKET WE INTEND TO COPY INTO
                LAUMemoryObject packet = getPacket();

                // COPY SCAN FROM GLWIDGET AND SEND TO VIDEO SINK OBJECT RUNNING IN A SEPARATE THREAD
                if (glWidget) {
                    // FIRST GRAB THE CURRENT POINT CLOUD FROM THE GPU
                    glWidget->copyScan((float *)packet.constPointer());

                    // NOW COPY THE JETR VECTOR FROM THE GLWIDGET
                    packet.setConstJetr(glWidget->jetr(glWidget->camera()));
                }

                // PRESERVE THE TIME ELAPSED
                packet.setConstElapsed(timeStamp.elapsed());
                packet.setTransform(glWidget->lutHandle()->transform());
                packet.setProjection(glWidget->lutHandle()->projection());

                // ENCODE THE SCANNER POSITION INTO THE TRANSFORM OF THE SCAN
#ifndef EXCLUDE_LAUVELMEXWIDGET
                if (velmexWidget){
                    float *buffer = (float*)packet.constPointer();
                    buffer[0] = scannerPosition.x();
                    buffer[1] = scannerPosition.y();
                    buffer[2] = scannerPosition.z();
                    buffer[3] = scannerPosition.w();
                }
#endif
                // EMIT THE VIDEO FRAME
                emit emitVideoFrames(packet);

                // TELL THE VELMEX RAIL THAT WE ARE READY TO MOVE TO THE NEXT POSITION
                emit emitTriggerScanner(0.0f, velmexIteration, velmexNumberOfIterations);

                // STOP THE FRAME RECORDING
                videoLabel->onPlayButtonClicked(false);
            }
        } else {
#endif
            videoRecordingFlag = state;
            if (state) {
                // RESET RECORDING FRAME COUNTER AND TIMER AND WE CAN JUST DUMP
                // THE INCOMING VIDEO TO OUR VIDEO FRAME BUFFER LIST
                pressStartButtonTime = QTime::currentTime();
                timeStamp.restart();

                // TELL THE VELMEX RAIL THAT WE ARE READY TO MOVE TO THE NEXT POSITION
                emit emitTriggerScanner(0.0f, velmexIteration, velmexNumberOfIterations);
#ifndef IDS
            } else {
                // EMIT THE LIST OF RECORDED FRAMES OUT TO A RECEIVER OBJECT
                emit emitVideoFrames(recordedVideoFramesBufferList);

                // NOW THAT THE RECIEVER OBJECT HAS THE LIST, WE CAN DELETE IT
                recordedVideoFramesBufferList.clear();

                // RESET THE PROGRESS BAR TO SHOW NO VIDEO IS CURRENTLY RECORDED
                videoLabel->onUpdateSliderPosition(0.0f);
                videoLabel->onUpdateTimeStamp(0);
            }
#endif
        }
    } else {
        if (state) {
            videoLabel->onPlayButtonClicked(false);
            QMessageBox::warning(this, QString("Video Recorded Widget"), QString("No device available."));
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoRecordingWidget::onReceiveVideoFrames(LAUMemoryObject frame)
{
    // CREATE A LAUSCAN TO HOLD THE INCOMING SNAP SHOT
    LAUScan scan(frame, playbackColor);
    if (scan.isValid()) {
        scan.updateLimits();
        scan.setSoftware(QString("Lau 3D Video Recorder"));
        scan.setMake(camera->make());
        scan.setModel(camera->model());

        // ASK THE USER TO APPROVE THE SCAN BEFORE SAVING TO DISK
#ifndef EXCLUDE_LAUSCANINSPECTOR
        while (scan.approveImage()) {
            // NOW LET THE USER SAVE THE SCAN TO DISK
            if (scan.save() == true) {
                break;
            }
        }
#else
        scan.save();
#endif
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoRecordingWidget::onReceiveVideoFrames(QList<LAUMemoryObject> frameList)
{
    // NOW WE NEED TO PASS OUR VIDEO FRAMES TO A DEPTH VIDEO OBJECT
    // WHICH CAN THEN BRING UP A VIDEO PLAYER WIDGET TO REPLAYING
    // THE VIDEO ON SCREEN, AND GIVE THE USER THE CHANCE TO SAVE TO DISK
    if (frameList.count() > 0) {
#ifndef EXCLUDE_LAUVIDEOPLAYERWIDGET
        // CREATE THE VIDEO PLAYER WIDGET AND SEND IT THE VIDEO FRAMES
        LAU3DVideoPlayerWidget *replayWidget = new LAU3DVideoPlayerWidget(camera->width(), camera->height(), playbackColor, this);

        // DISPLAY THE REPLAY WIDGET SO THE USER CAN SEE THE FINE MERGE'S PROGRESS
        // SEND THE RECORDED FRAME PACKETS TO OUR REPLAY VIDEO WIDGET FOR PLAYBACK
        while (!frameList.isEmpty()) {
            replayWidget->onInsertPacket(frameList.takeFirst());
        }

        // GET THE VIEW LIMITS OF THE GPU WIDGET AND COPY THEM TO THE REPLAY WIDGET
        QVector2D xLimits = glWidget->xLimits();
        QVector2D yLimits = glWidget->yLimits();
        QVector2D zLimits = glWidget->zLimits();

        replayWidget->setLimits(xLimits.x(), xLimits.y(), yLimits.x(), yLimits.y(), zLimits.x(), zLimits.y());
        replayWidget->setAttribute(Qt::WA_DeleteOnClose);
        replayWidget->show();
#endif
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoRecordingWidget::onReceiveFrameBuffer(LAUMemoryObject buffer)
{
    videoFramesBufferList << buffer;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAU3DVideoRecordingWidget::getPacket()
{
    LAUMemoryObject packet;
    if (!videoFramesBufferList.isEmpty()) {
        packet = videoFramesBufferList.takeFirst();
    }
    for (int n = videoFramesBufferList.count(); n < 4; n++) {
        emit emitGetFrame();
    }
    return (packet);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoRecordingWidget::keyPressEvent(QKeyEvent *event)
{
    qDebug() << "LAU3DVideoRecordingWidget::keyPressEvent" << event->key();

    if (event->key() == Qt::Key_B) {
        videoLabel->togglePlayback();
    } else {
        LAU3DVideoWidget::keyPressEvent(event);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoRecordingWidget::releasePacket(LAUMemoryObject packet)
{
    videoFramesBufferList << packet;
    while (videoFramesBufferList.count() > 4) {
        emit emitReleaseFrame(videoFramesBufferList.takeFirst());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoRecordingWidget::updateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    // UPDATE THE TEXTURE BUFFERS IF WE HAVE AT LEAST ONE VALID POINTER
    if (depth.isValid() || color.isValid() || mapping.isValid()) {
        // SEE IF WE SHOULD RESPOND TO A SCANNER TRIGGER
        if (scannerModeFlag && scannerModeTriggerFlag){
            qDebug() << "WAITING FOR VIDEO" << scannerModeTriggerCounter;

            // SEE IF ENOUGH FRAMES HAVE PASSED THAT WE CAN GRAB A FRAME WITH A RECORD CLICK
            if (scannerModeTriggerCounter++ > NUMBEROFFRAMESBEFOREWECANGRABASCAN){
                qDebug() << "TRIGGERING VIDEO FRAME";

                // CAPTURE A FRAME OF VIDEO
                this->onRecordButtonClicked(true);

                // RESET FRAME COUNTER FOR NEXT SCANNER TRIGGER
                scannerModeTriggerFlag = false;
                scannerModeTriggerCounter = -1;
            }
        }

        if (videoRecordingFlag && validFrame()) {
#ifdef IDS
            // FOR IDS, WE ONLY PERFORM SNAP SHOT RECORDING
            videoLabel->onPlayButtonClicked(false);

            // FOR IDS RECORDING, SEND THE MEMORY OBJECT TO A SPECIAL SAVE METHOD
            // HOSTED AS A STATIC METHOD BY THE LAUMSCOLORHISTOGRAMGLFILTER CLASS
            LAUMSColorHistogramGLFilter::save(color);
#else
            // NOW THE BUFFER IS FULL, TELL VIDEO LABEL TO STOP RECORDING
            if (recordedVideoFramesBufferList.count() < MAXRECORDEDFRAMECOUNT) {
                // GRAB A COPY OF THE PACKET WE INTEND TO COPY INTO
                LAUMemoryObject packet = getPacket();

                // COPY SCAN FROM GLWIDGET AND SEND TO VIDEO SINK OBJECT RUNNING IN A SEPARATE THREAD
                if (glWidget) {
                    // FIRST GRAB THE CURRENT POINT CLOUD FROM THE GPU
                    glWidget->copyScan((float *)packet.constPointer());

                    // NOW COPY THE JETR VECTOR FROM THE GLWIDGET
                    packet.setConstJetr(glWidget->jetr(glWidget->camera()));
                }

                // PRESERVE THE TIME ELAPSED
                packet.setConstElapsed(timeStamp.elapsed());

                // HAVE THE VIDEO LABEL UPDATE ITS PROGRESS BAR SO THE USER KNOWS HOW MUCH SPACE IS LEFT
                videoLabel->onUpdateSliderPosition((float)recordedVideoFramesBufferList.count() / (float)MAXRECORDEDFRAMECOUNT);
                videoLabel->onUpdateTimeStamp(packet.elapsed());

                // ADD THE INCOMING PACKET TO OUR RECORDED FRAME BUFFER LIST
                recordedVideoFramesBufferList << packet;
            } else {
                // NOW THE BUFFER IS FULL, TELL VIDEO LABEL TO STOP RECORDING
                videoLabel->onPlayButtonClicked(false);
            }
#endif
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoRecordingWidget::onTriggerScanner(float pos, int n, int N)
{
    Q_UNUSED(pos);

    static QProgressDialog *dialog = nullptr;

    // SET THESE FLAGS FOR FALSE CONDITION AND WE WILL RESET LATER IF THE CONDITIONS ARE RIGHT
    scannerModeTriggerCounter = -1;
    scannerModeTriggerFlag = false;

    if (snapShotModeFlag == true) {
        if (n < 0) {
            // CREATE A PROGRESS DIALOG SO USER CAN CANCEL SCANNING
            dialog = new QProgressDialog(QString("Scanning..."), QString("Abort"), 0, N, this, Qt::Sheet);
            dialog->setModal(Qt::WindowModal);
            dialog->show();

            // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
            velmexIteration = n;
            velmexNumberOfIterations = N;
       } else if (n >= N) {
            // CLOSE THE PROGRESS DIALOG
            if (dialog){
                dialog->setValue(N);
                dialog->deleteLater();
                dialog = nullptr;
            }

            // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
            velmexIteration = -1;
            velmexNumberOfIterations = -1;
        } else {
            // SAVE THE CURRENT POSITION OF THE VELMEX WIDGET
#ifndef EXCLUDE_LAUVELMEXWIDGET
            scannerPosition = LAUVelmexWidget::scannerPosition;
#endif
            if (dialog){
                if (dialog->wasCanceled()){
                    dialog->setValue(N);
                    dialog->deleteLater();
                    dialog = nullptr;

                    velmexIteration = -1;
                    velmexNumberOfIterations = N;

                    // TELL THE VELMEX RAIL THAT WE WANT TO CANCEL THE SCAN
                    emit emitTriggerScanner(pos, -1, N);
                } else {
                    dialog->setValue(n);

                    // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
                    velmexIteration = n;
                    velmexNumberOfIterations = N;

                    // TRIGGER A SCAN
                    scannerModeTriggerCounter = 0;
                    scannerModeTriggerFlag = true;
                    //this->onRecordButtonClicked(true);
                }
            } else {
                // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
                velmexIteration = n;
                velmexNumberOfIterations = N;

                // TRIGGER A SCAN
                scannerModeTriggerCounter = 0;
                scannerModeTriggerFlag = true;
                //this->onRecordButtonClicked(true);
            }
        }
    } else {
        if (n < 0) {
            // CREATE A PROGRESS DIALOG SO USER CAN CANCEL SCANNING
            dialog = new QProgressDialog(QString("Scanning..."), QString("Abort"), 0, N, this, Qt::Sheet);
            dialog->setModal(Qt::WindowModal);
            dialog->show();

            // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
            velmexIteration = n;
            velmexNumberOfIterations = N;
        } else if (n >= N) {
            // CLOSE THE PROGRESS DIALOG
            if (dialog){
                dialog->setValue(N);
                dialog->deleteLater();
                dialog = nullptr;
            }

            // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
            velmexIteration = -1;
            velmexNumberOfIterations = -1;

            // TRIGGER A SCAN
            this->onRecordButtonClicked(false);
        } else {
            // SAVE THE CURRENT POSITION OF THE VELMEX WIDGET
#ifndef EXCLUDE_LAUVELMEXWIDGET
            scannerPosition = LAUVelmexWidget::scannerPosition;
#endif
            if (dialog){
                if (dialog->wasCanceled()){
                    dialog->setValue(N);
                    dialog->deleteLater();
                    dialog = nullptr;

                    velmexIteration = -1;
                    velmexNumberOfIterations = N;

                    // TRIGGER A SCAN
                    this->onRecordButtonClicked(false);

                    // TELL THE VELMEX RAIL THAT WE WANT TO CANCEL THE SCAN
                    emit emitTriggerScanner(pos, -1, N);
                } else {
                    dialog->setValue(n);

                    // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
                    velmexIteration = n;
                    velmexNumberOfIterations = N;

                    // TRIGGER A SCAN
                    scannerModeTriggerCounter = 0;
                    scannerModeTriggerFlag = true;
                    //this->onRecordButtonClicked(true);
                }
            } else {
                // KEEP TRACK OF THE ITERATION NUMBER AND NUMBER OF ITERATIONS
                velmexIteration = n;
                velmexNumberOfIterations = N;

                // TRIGGER A SCAN
                scannerModeTriggerCounter = 0;
                scannerModeTriggerFlag = true;
                //this->onRecordButtonClicked(true);
            }
        }
    }
}

#ifndef EXCLUDE_LAUVELMEXWIDGET
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoRecordingWidget::enableVelmexScanMode(bool state)
{
    // KEEP TRACK IF THIS WIDGET IS CONNECTED TO A SCANNER
    scannerModeFlag = state;

    // SEE IF WE NEED TO CREATE A VELMEX WIDGET
    if (state && velmexWidget == nullptr) {
        // GET THE NUMBER OF DIMENSIONS FROM THE VELMEX RAIL CONTROLLER
        int dims = LAUMultiVelmexWidget::getDimensions();

        // IF WE HAVE MORE THAN ONE DIMENSION THEN CREATE A VELMEX WIDGET TO CONTROL IT
        if (dims > 0){
            // CREATE VELMEX RAIL WIDGET TO CONTROL RAIL
            velmexWidget = new LAUMultiVelmexWidget(dims, this);
            if (velmexWidget->isValid()) {
                // SEE IF WE SHOULD BE SCANNING WHILE MOVING
                velmexWidget->setWindowFlag(Qt::Tool);
                connect(velmexWidget, SIGNAL(emitTriggerScanner(float, int, int)), this, SLOT(onTriggerScanner(float, int, int)), Qt::QueuedConnection);
                connect(this, SIGNAL(emitTriggerScanner(float, int, int)), velmexWidget, SLOT(onTriggerScanner(float, int, int)), Qt::QueuedConnection);

                // ENABLE THE WIDGET SO THAT THE USER CAN INTERACT WITH IT
                velmexWidget->setEnabled(true);

                // CREATE A CONTEXT MENU ACTION FOR DISPLAYING THE VELMEX RAIL
                QAction *action = new QAction(QString("Show Velmex rail controller..."), nullptr);
                action->setCheckable(false);
                connect(action, SIGNAL(triggered()), this, SLOT(onShowVelmexWidget()));
                this->insertAction(action);
            } else {
                // DISABLE THE WIDGET SO THAT THE USER CANNOT INTERACT WITH IT
                velmexWidget->hide();
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoRecordingWidget::onShowVelmexWidget()
{
    // HIDE AND THEN SHOW THE VELMEX WIDGET
    // SO IT ENDS UP AS THE TOP WINDOW
    if (velmexWidget) {
        velmexWidget->hide();
        velmexWidget->show();
    }
}
#endif
