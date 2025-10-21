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

#include "lau3dvideowidget.h"
#include "lau3dmachinevisionscannerwidget.h"
#include "lauconstants.h"

#ifdef ENABLECASCADE
#include "laucolorizedepthglfilter.h"
#endif
#ifdef SEEK
#include "lauseekcamera.h"
#endif
#ifdef HYPERSPECTRAL
#include "ruit265Filter.h"
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DVideoWidget::LAU3DVideoWidget(LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent) : QWidget(parent), camera(nullptr), glWidget(nullptr), cameraController(nullptr), frameBufferManager(nullptr), frameBufferManagerController(nullptr), playbackColor(color), playbackDevice(device), labelWidget(nullptr), projectorWidget(nullptr), scannerWidget(nullptr), screenWidget(nullptr), counter(0), cameraObjectStillExists(false)
{
    this->setLayout(new QVBoxLayout());
    this->setWindowTitle(QString("Video Widget"));
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->layout()->setSpacing(0);

    // GET A POINTER TO A CAMERA INSTANCE
    camera = LAU3DCameras::getCamera(color, device);

    // SEE IF WE HAVE A VALID CAMERA
    if (camera && camera->isValid()) {
        unsigned int channelCount = 0;

        // ALLOCATE MEMORY OBJECTS TO HOLD INCOMING VIDEO FRAMES
        for (int n = 0; n < NUMFRAMESINBUFFER; n++) {
            LAUModalityObject frame;
            frame.depth = camera->depthMemoryObject();
            frame.color = camera->colorMemoryObject();
            frame.mappi = camera->mappiMemoryObject();
            framesList << frame;

            // COUNT THE NUMBER OF FRAMES IN EACH MEMORY OBJECT
            channelCount = qMax(channelCount, frame.depth.frames());
            channelCount = qMax(channelCount, frame.color.frames());
            channelCount = qMax(channelCount, frame.mappi.frames());
        }

        // CREATE A GLWIDGET TO PROCESS THE DFT COEFFICIENTS AND DISPLAY THE POINT CLOUD
        glWidget = new LAU3DVideoGLWidget(camera->depthWidth(), camera->depthHeight(), camera->colorWidth(), camera->colorHeight(), playbackColor, playbackDevice);
        glWidget->onSetCamera(0);
        glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        glWidget->setMaximumIntensityValue(camera->maxIntensityValue());
        this->layout()->addWidget(glWidget);

        // SET THE JETR VECTORS IN THE ABSTRACT FILTER WIDGET
        for (unsigned int n = 0; n < camera->sensors(); n++){
            glWidget->setJetrVector(n, camera->jetr(n));
        }

        connect(this, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), camera, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(glWidget, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), this, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::DirectConnection);
        //connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), this, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);

        // CREATE A THREAD TO HOST THE CAMERA CONTROLLER
        cameraController = new LAU3DCameraController(camera);

        // CONNECT THE EMITTED CAMERA ERROR TO THIS WIDGET IF THE CAMERA CONTROLLER ISNT GOING TO RECEIVE IT
        if (!cameraController) {
            connect(camera, SIGNAL(emitError(QString)), this, SLOT(onError(QString)));
        }

        // CONNECT THE CAMERA OBJECTS DESTROYED SIGNAL BACK TO THIS
        cameraObjectStillExists = true;
        connect(camera, SIGNAL(destroyed()), this, SLOT(onCameraObjectDestroyed()));

#if defined(PROSILICA) || defined(VIMBA) || defined(BASLERUSB)
        // DO CAMERA SPECIFIC ACTIONS
        if (isMachineVision(camera->device())) {
            if (camera->device() == DeviceProsilicaLCG) {
                insertFilter(new LAUDFTFilter(camera->width(), camera->height(), LAUDFTFilter::PatternEightEightEight));
//            } else if (camera->device() == DeviceProsilicaTOF) {
//                LAUDepth2PhaseGLFilter *filter = new LAUDepth2PhaseGLFilter(camera->width(), camera->height(), camera->color(), camera->device());
//                filter->setLookUpTable(LAULookUpTable(QString()));
//                insertFilter(new LAUDFTFilter(camera->width(), camera->height(), LAUDFTFilter::PatternEightEightEight));
//                insertFilter(filter);
            } else if (camera->device() == DeviceProsilicaIOS) {
#ifdef VIMBA
#ifdef ENABLE_IMU
                QList<QObject *> filters;
                filters << new LAUDFTFilter(camera->width(), camera->height(), LAUDFTFilter::PatternDualFrequency);
                filters << new LAUSmoothDFTGLFilter(camera->width(), camera->height());
                filters << new LAUBiosIMUObject(camera->width(), camera->height());
                connect((LAUAbstractFilter *)filters.first(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), (LAUAbstractFilter *)filters.last(), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
                insertFilters(filters);
#else
                insertFilter(new LAUDFTFilter(camera->width(), camera->height(), LAUDFTFilter::PatternDualFrequency));
#endif
#else
                insertFilter(new LAUDFTFilter(camera->width(), camera->height(), LAUDFTFilter::PatternDualFrequency));
#endif
            } else if (camera->device() == DeviceProsilicaAST) {
                insertFilter(new LAUDFTFilter(camera->width(), camera->height(), LAUDFTFilter::PatternEightEightEight));
#if defined(BASLERUSB)
            } else if (camera->device() == DeviceProsilicaPST) {
                //LAUStereoVisionGLFilter *filter = new LAUStereoVisionGLFilter(camera->width(), camera->height(), camera->color(), camera->device());
                //filter->setMaximumIntensityValue(camera->maxIntensityValue());
                //insertFilter(filter);
#endif
            } else if (camera->device() == DeviceProsilicaGRY) {
#ifdef HYPERSPECTRAL
                RUIT265Filter *filter = new RUIT265Filter(camera->width(), camera->height());
                if (filter && filter->isValid()) {
                    insertFilter(filter);
                } else {
                    QMessageBox::warning(this, QString("Video Recorder"), filter->error());
                    delete filter;
                }
#endif
            }

            // SET THE LOOKUP TABLE FOR CONVERTING DEPTH/PHASE TO XYZ
            if (playbackDevice != DeviceProsilicaPST && playbackDevice != DeviceProsilicaGRY && playbackDevice != DeviceProsilicaRGB) {
                glWidget->setLookUpTable(LAULookUpTable(QString()));
            }

            QAction *action = new QAction(QString("Adjust camera settings..."), nullptr);
            action->setCheckable(false);
            connect(action, SIGNAL(triggered()), this, SLOT(onContextMenuTriggered()));
            insertAction(action);

            // CREATE A SCANNER WIDGET FOR ADJUSTING SCAN PARAMETERS
            scannerWidget = new LAU3DMachineVisionScannerWidget(this);
            connect((LAU3DMachineVisionScannerWidget *)scannerWidget, SIGNAL(emitUpdateExposure(int)), camera, SLOT(onUpdateExposure(int)), Qt::QueuedConnection);
            connect((LAU3DMachineVisionScannerWidget *)scannerWidget, SIGNAL(emitUpdateSnrThreshold(int)), glWidget, SLOT(onSetSNRThreshold(int)), Qt::QueuedConnection);
            connect((LAU3DMachineVisionScannerWidget *)scannerWidget, SIGNAL(emitUpdateMtnThreshold(int)), glWidget, SLOT(onSetMTNThreshold(int)), Qt::QueuedConnection);

            if (camera->hasDepth() == false) {
                ((LAU3DMachineVisionScannerWidget *)scannerWidget)->enableSNRWidget(false);
                ((LAU3DMachineVisionScannerWidget *)scannerWidget)->enableMTNWidget(false);
            }

            // SYNCHRONIZE THE WIDGET PARAMETERS WITH THE CAMERA AND GLWIDGET
            camera->onUpdateExposure(((LAU3DMachineVisionScannerWidget *)scannerWidget)->exp());
            glWidget->onSetSNRThreshold(((LAU3DMachineVisionScannerWidget *)scannerWidget)->snr());
            glWidget->onSetMTNThreshold(((LAU3DMachineVisionScannerWidget *)scannerWidget)->mtn());
            //glWidget->setPhaseCorrection(QString());
        } else if (camera->device() == DeviceProsilicaGRY || camera->device() == DeviceProsilicaRGB) {
            QAction *action = new QAction(QString("Adjust camera settings..."), nullptr);
            action->setCheckable(false);
            connect(action, SIGNAL(triggered()), this, SLOT(onContextMenuTriggered()));
            insertAction(action);

            // CREATE A SCANNER WIDGET FOR ADJUSTING SCAN PARAMETERS AND
            // SYNCHRONIZE THE WIDGET PARAMETERS WITH THE CAMERA AND GLWIDGET
            scannerWidget = new LAU3DMachineVisionScannerWidget(this);
            connect((LAU3DMachineVisionScannerWidget *)scannerWidget, SIGNAL(emitUpdateExposure(int)), camera, SLOT(onUpdateExposure(int)), Qt::QueuedConnection);
            camera->onUpdateExposure(((LAU3DMachineVisionScannerWidget *)scannerWidget)->exp());

            if (camera->hasDepth() == false) {
                ((LAU3DMachineVisionScannerWidget *)scannerWidget)->enableSNRWidget(false);
                ((LAU3DMachineVisionScannerWidget *)scannerWidget)->enableMTNWidget(false);
            }
        }
#endif
#ifdef SEEK
        // DO CAMERA SPECIFIC ACTIONS FOR SEEK THERMAL CAMERA
        if (camera->device() == DeviceSeek) {
            QAction *action = new QAction(QString("Adjust camera settings..."), nullptr);
            action->setCheckable(false);
            connect(action, SIGNAL(triggered()), this, SLOT(onContextMenuTriggered()));
            insertAction(action);

            // CREATE A SCANNER WIDGET FOR ADJUSTING SCAN PARAMETERS
            scannerWidget = new LAU3DMachineVisionScannerWidget(this, false, DeviceSeek);
            connect((LAU3DMachineVisionScannerWidget *)scannerWidget, SIGNAL(emitUpdateExposure(int)), camera, SLOT(onUpdateExposure(int)), Qt::QueuedConnection);
            connect((LAU3DMachineVisionScannerWidget *)scannerWidget, SIGNAL(emitUpdateSharpenFilter(bool)), (LAUSeekCamera *)camera, SLOT(enableSharpenFilter(bool)), Qt::QueuedConnection);
            
            // SYNCHRONIZE THE WIDGET PARAMETERS WITH THE CAMERA
            camera->onUpdateExposure(((LAU3DMachineVisionScannerWidget *)scannerWidget)->exp());
        }
#endif
        // SET THE BOUNDING BOX AND LOOK UP TABLE FOR THE GLWIDGET
        if (camera->device() == DevicePrimeSense || camera->device() == DeviceDemo) {
            glWidget->setLookUpTable(LAULookUpTable(camera->width(), camera->height(), camera->device(), camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians(), camera->minDistance(), camera->maxDistance()));
            glWidget->setRangeLimits(camera->minDistance(), camera->maxDistance(), camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians());
        } else if (camera->device() == DeviceRealSense || camera->device() == DeviceKinect || camera->device() == DeviceOrbbec || camera->device() == DeviceLucid || camera->device() == DeviceVZense || camera->device() == DeviceVidu) {
#ifdef HYPERSPECTRAL
            insertFilter(new RUIT265Filter(camera->width(), camera->height()));
#endif
            if (camera->hasDepth()) {
                LAULookUpTable lookUpTable;
                bool lutLoaded = false;

                // Try to load LUT from disk, validating dimensions and camera info
                while (!lutLoaded) {
                    lookUpTable = LAULookUpTable(QString());

                    if (!lookUpTable.isValid()) {
                        // User cancelled, break out of loop
                        break;
                    }

                    // Validate LUT dimensions match camera
                    if (lookUpTable.width() != camera->width() || lookUpTable.height() != camera->height()) {
                        int ret = QMessageBox::warning(this, QString("Look Up Table Dimension Mismatch"),
                                                       QString("The selected look up table has dimensions %1x%2, but the camera is %3x%4.\n\nWould you like to select a different look up table?")
                                                       .arg(lookUpTable.width()).arg(lookUpTable.height())
                                                       .arg(camera->width()).arg(camera->height()),
                                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                        if (ret == QMessageBox::No) {
                            // User doesn't want to try again
                            lookUpTable = LAULookUpTable();  // Invalidate
                            break;
                        }
                        // Loop will continue and show file dialog again
                    } else {
                        // Dimensions match, check make and model
                        bool makeModelMatch = (lookUpTable.makeString() == camera->make() && lookUpTable.modelString() == camera->model());

                        if (!makeModelMatch && !lookUpTable.makeString().isEmpty() && !lookUpTable.modelString().isEmpty()) {
                            // Make/model don't match - warn user but allow them to use it anyway
                            int ret = QMessageBox::warning(this, QString("Look Up Table Camera Mismatch"),
                                                           QString("The selected look up table is for:\n  Make: %1\n  Model: %2\n\nBut the current camera is:\n  Make: %3\n  Model: %4\n\nWould you like to select a different look up table?\n(Click No to use this LUT anyway)")
                                                           .arg(lookUpTable.makeString()).arg(lookUpTable.modelString())
                                                           .arg(camera->make()).arg(camera->model()),
                                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                            if (ret == QMessageBox::Yes) {
                                // User wants to select a different LUT
                                continue;  // Loop will show file dialog again
                            }
                            // User clicked No - use this LUT anyway
                        }
                        // Dimensions match and either make/model match or user accepted mismatch
                        lutLoaded = true;
                    }
                }

                if (lookUpTable.isValid()) {
                    glWidget->setLookUpTable(lookUpTable);
                } else {
                    // No valid LUT from disk, try camera
                    lookUpTable = camera->lut(0, this);
                    if (lookUpTable.isValid()) {
                        glWidget->setLookUpTable(lookUpTable);
                    } else {
                        glWidget->setLookUpTable(LAULookUpTable(camera->width(), camera->height(), camera->device(), camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians(), camera->minDistance(), camera->maxDistance()));
                        glWidget->setRangeLimits(camera->minDistance(), camera->maxDistance(), camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians());
                    }
                }

                // SEE IF THIS IS A LUCID CAMERA AND IF SO ADD A COLOR FILTER
                // if (camera->device() == DeviceLucid) {
                //     if (camera->make().toLower().indexOf(QString("lucid")) > -1) {
                //         LAUColorizeDepthGLFilter *filter = new LAUColorizeDepthGLFilter(camera->depthWidth(), camera->depthHeight(), camera->colorWidth(), camera->colorHeight(), camera->color(), camera->device());
                //         filter->onSetRadius(20);
                //         insertFilter(filter);
                //     }
                // }
            } else {
                // CREATE A SCANNER WIDGET FOR ADJUSTING SCAN PARAMETERS
                if (device == DeviceRealSense && color == ColorGray) {
                    // CREATE AN ACTION THAT WILL TRIGGER AN EVENT WHEN THE VIDEO GLWIDGET IS RIGHT CLICKED
                    QAction *action = new QAction(QString("Adjust camera settings..."), nullptr);
                    action->setCheckable(false);
                    connect(action, SIGNAL(triggered()), this, SLOT(onContextMenuTriggered()));
                    insertAction(action);
#ifdef REALSENSE
                    // CREATE A SCANNER WIDGET FOR ADJUSTING SCAN PARAMETERS AND
                    // SYNCHRONIZE THE WIDGET PARAMETERS WITH THE CAMERA AND GLWIDGET
                    scannerWidget = new LAU3DMachineVisionScannerWidget(this, false);
                    connect((LAU3DMachineVisionScannerWidget *)scannerWidget, SIGNAL(emitUpdateExposure(int)), camera, SLOT(onUpdateExposure(int)), Qt::QueuedConnection);
                    ((LAURealSenseCamera *)camera)->onEnableAutoExposure(false);
                    camera->onUpdateExposure(((LAU3DMachineVisionScannerWidget *)scannerWidget)->exp());
#endif
                }
                glWidget->setMaximumIntensityValue(camera->maxIntensityValue());
            }
        }
#ifdef XIMEA
        // DO CAMERA SPECIFIC ACTIONS
        if (camera->device() == DeviceXimea) {
            QAction *action = new QAction(QString("Adjust camera settings..."), nullptr);
            connect(action, SIGNAL(triggered()), this, SLOT(onContextMenuTriggered()), Qt::QueuedConnection);
            action->setCheckable(false);
            insertAction(action);

            // CREATE A SCANNER WIDGET FOR ADJUSTING SCAN PARAMETERS
            scannerWidget = new LAUCameraWidget(this);
            connect((LAUCameraWidget *)scannerWidget, SIGNAL(emitUpdateExposure(int)), (LAUXimeaCamera *)camera, SLOT(onUpdateExposure(int)), Qt::QueuedConnection);

            // SYNCHRONIZE THE WIDGET PARAMETERS WITH THE CAMERA AND GLWIDGET
            ((LAUXimeaCamera *)camera)->onUpdateExposure(((LAUCameraWidget *)scannerWidget)->exp());

            // TELL THE GLWIDGET WHAT THE MAXIMUM INTENSITY VALUE IS FOR THE XIMEA CAMERA
            glWidget->setMaximumIntensityValue(camera->maxIntensityValue());
        }
#endif
#if defined(LUCID)
#ifdef DONTCOMPILE
    } else if (camera && camera->make() == QString("Lucid")) {
        // ENABLE VIDEO FROM DISK CAMERA
        ((LAULucidCamera *)camera)->enableReadVideoFromDisk(true);

        // ALLOCATE MEMORY OBJECTS TO HOLD INCOMING VIDEO FRAMES
        unsigned int channelCount = 0;
        for (int n = 0; n < NUMFRAMESINBUFFER; n++) {
            LAUModalityObject frame;
            frame.depth = camera->depthMemoryObject();
            frame.color = camera->colorMemoryObject();
            frame.mappi = camera->mappiMemoryObject();
            framesList << frame;

            // COUNT THE NUMBER OF FRAMES IN EACH MEMORY OBJECT
            channelCount = qMax(channelCount, frame.depth.frames());
            channelCount = qMax(channelCount, frame.color.frames());
            channelCount = qMax(channelCount, frame.mappi.frames());
        }

        // CREATE A GLWIDGET TO PROCESS THE DFT COEFFICIENTS AND DISPLAY THE POINT CLOUD
        glWidget = new LAU3DVideoGLWidget(camera->depthWidth(), camera->depthHeight(), camera->colorWidth(), camera->colorHeight(), playbackColor, playbackDevice);
        glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        glWidget->setMaximumIntensityValue(camera->maxIntensityValue());
        this->layout()->addWidget(glWidget);

        connect(this, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), camera, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(glWidget, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), this, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::DirectConnection);

        if (camera->hasDepth()) {
            LAULookUpTable lookUpTable;
            bool lutLoaded = false;

            // Try to load LUT from disk, validating dimensions and camera info
            while (!lutLoaded) {
                lookUpTable = LAULookUpTable(QString());

                if (!lookUpTable.isValid()) {
                    // User cancelled, break out of loop
                    break;
                }

                // Validate LUT dimensions match camera
                if (lookUpTable.width() != camera->width() || lookUpTable.height() != camera->height()) {
                    int ret = QMessageBox::warning(this, QString("Look Up Table Dimension Mismatch"),
                                                   QString("The selected look up table has dimensions %1x%2, but the camera is %3x%4.\n\nWould you like to select a different look up table?")
                                                   .arg(lookUpTable.width()).arg(lookUpTable.height())
                                                   .arg(camera->width()).arg(camera->height()),
                                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                    if (ret == QMessageBox::No) {
                        // User doesn't want to try again
                        lookUpTable = LAULookUpTable();  // Invalidate
                        break;
                    }
                    // Loop will continue and show file dialog again
                } else {
                    // Dimensions match, check make and model
                    bool makeModelMatch = (lookUpTable.makeString() == camera->make() && lookUpTable.modelString() == camera->model());

                    if (!makeModelMatch && !lookUpTable.makeString().isEmpty() && !lookUpTable.modelString().isEmpty()) {
                        // Make/model don't match - warn user but allow them to use it anyway
                        int ret = QMessageBox::warning(this, QString("Look Up Table Camera Mismatch"),
                                                       QString("The selected look up table is for:\n  Make: %1\n  Model: %2\n\nBut the current camera is:\n  Make: %3\n  Model: %4\n\nWould you like to select a different look up table?\n(Click No to use this LUT anyway)")
                                                       .arg(lookUpTable.makeString()).arg(lookUpTable.modelString())
                                                       .arg(camera->make()).arg(camera->model()),
                                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                        if (ret == QMessageBox::Yes) {
                            // User wants to select a different LUT
                            continue;  // Loop will show file dialog again
                        }
                        // User clicked No - use this LUT anyway
                    }
                    // Dimensions match and either make/model match or user accepted mismatch
                    lutLoaded = true;
                }
            }

            if (lookUpTable.isValid()) {
                glWidget->setLookUpTable(lookUpTable);
            } else {
                // No valid LUT from disk, try camera
                lookUpTable = camera->lut(0, this);
                if (lookUpTable.isValid()) {
                    glWidget->setLookUpTable(lookUpTable);
                } else {
                    glWidget->setLookUpTable(LAULookUpTable(camera->width(), camera->height(), camera->device(), camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians(), camera->minDistance(), camera->maxDistance()));
                    glWidget->setRangeLimits(camera->minDistance(), camera->maxDistance(), camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians());
                }
            }
#ifdef ENABLECASCADE
            if (camera->device() == DeviceLucid || camera->device() == DeviceVZense || camera->device() == DeviceVidu) {
                if (camera->color() == ColorXYZRGB || camera->color() == ColorXYZWRGBA) {
                    prependFilter(new LAUColorizeDepthGLFilter(camera->depthWidth(), camera->depthHeight(), camera->colorWidth(), camera->colorHeight(), camera->color(), camera->device()));
                }
            }
#endif
        }

        // CREATE A THREAD TO HOST THE CAMERA CONTROLLER
        cameraController = new LAU3DCameraController(camera);

        // CONNECT THE CAMERA OBJECTS DESTROYED SIGNAL BACK TO THIS
        cameraObjectStillExists = true;
        connect(camera, SIGNAL(destroyed()), this, SLOT(onCameraObjectDestroyed()));

        // CREATE AN ACTION THAT WILL TRIGGER AN EVENT WHEN THE VIDEO GLWIDGET IS RIGHT CLICKED
        QAction *action = new QAction(QString("Display labeling widget..."), nullptr);
        action->setCheckable(false);
        connect(action, SIGNAL(triggered()), this, SLOT(onShowMachineVisionLabelingWidget()));
        insertAction(action);
#endif
#endif
    } else {
        // CREATE A GLWIDGET TO PROCESS THE DFT COEFFICIENTS AND DISPLAY THE POINT CLOUD
        LAUAbstractGLWidget *abstractGLWidget = new LAUAbstractGLWidget();
        abstractGLWidget->setMinimumSize(320, 240);
        abstractGLWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        this->layout()->addWidget(abstractGLWidget);

        if (camera == nullptr) {
            QMessageBox::warning(this, QString("Video Recorder"), QString("Invalid device."));
        } else {
            QMessageBox::warning(this, QString("Video Recorder"), camera->error());
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DVideoWidget::~LAU3DVideoWidget()
{
    // DELETE CONTROLLERS WHICH WILL DELETE THEIR OBJECTS
    if (cameraController) {
        delete cameraController;
    } else if (camera) {
        delete camera;
    }

    // WAIT HERE UNTIL THE CAMERA OBJECT HAS BEEN DELETED
    while (cameraObjectStillExists) {
        qApp->processEvents();
    }

    if (labelWidget) {
        delete labelWidget;
    }

    // DELETE THE FILTER CONTROLLERS
    while (filterControllers.isEmpty() == false) {
        delete filterControllers.takeFirst();
    }

    // IF GLWIDGET IS ON ITS OWN SCREEN, WE NEED TO DELETE IT
    if (screenWidget) {
        delete screenWidget;
    }

    // DELETE CONTROLLERS WHICH WILL DELETE THEIR OBJECTS
    if (frameBufferManagerController) {
        delete frameBufferManagerController;
    } else if (frameBufferManager) {
        delete frameBufferManager;
    }

    // IF GLWIDGET IS ON ITS OWN SCREEN, WE NEED TO DELETE IT
    if (projectorWidget) {
        delete projectorWidget;
    }

    qDebug() << QString("LAU3DVideoWidget::~LAU3DVideoWidget()");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoWidget::insertAction(QAction *action)
{
    QMenu *menu = glWidget->menu();
    if (menu) {
        menu->addAction(action);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoWidget::insertActions(QList<QAction *> actions)
{
    QMenu *menu = glWidget->menu();
    if (menu) {
        menu->addActions(actions);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoWidget::insertFilter(QObject *newFilter, QSurface *srfc)
{
    if (newFilter == nullptr) {
        return;
    }

    if (filterControllers.isEmpty()) {
        // DISCONNECT THE SIGNAL FROM THE CAMERA THAT WOULD GO TO THE GLWIDGET
        disconnect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

        // SPLICE THE FILTER INTO THE SIGNAL PATH
        connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), newFilter,   SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(newFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
    } else {
        // GRAB THE LAST FILTER IN THE LIST
        LAUAbstractFilterController *controller = filterControllers.last();
        if (controller->glFilter()) {
            // DISCONNECT THAT LAST GLFILTER IN THE LIST
            LAUAbstractGLFilter *oldFilter = controller->glFilter();
            disconnect(oldFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

            // SPLICE THE NEW FILTER INTO THE SIGNAL PATH
            connect(oldFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), newFilter,   SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(newFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        } else if (controller->filter()) {
            LAUAbstractFilter *oldFilter = controller->filter();
            disconnect(oldFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

            // SPLICE THE NEW FILTER INTO THE SIGNAL PATH
            connect(oldFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), newFilter,   SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(newFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        }
    }

    // CREATE CONTROLLERS TO WRAP AROUND THE FILTER OBJECTS
    if (dynamic_cast<LAUAbstractGLFilter *>(newFilter) != nullptr) {
        filterControllers.append(new LAUAbstractFilterController((LAUAbstractGLFilter *)newFilter, srfc));
    } else if (dynamic_cast<LAUAbstractFilter *>(newFilter) != nullptr) {
        filterControllers.append(new LAUAbstractFilterController((LAUAbstractFilter *)newFilter));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoWidget::insertFilters(QList<QObject *> filters)
{
    if (filters.isEmpty()) {
        return;
    }

    if (filterControllers.isEmpty()) {
        // DISCONNECT THE SIGNAL FROM THE CAMERA THAT WOULD GO TO THE GLWIDGET
        disconnect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

        // SPLICE THE FILTER INTO THE SIGNAL PATH
        connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), filters.first(), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(filters.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
    } else {
        // GRAB THE LAST FILTER IN THE LIST
        LAUAbstractFilterController *controller = filterControllers.last();
        if (controller->glFilter()) {
            // DISCONNECT THAT LAST GLFILTER IN THE LIST
            LAUAbstractGLFilter *oldFilter = controller->glFilter();
            disconnect(oldFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

            // SPLICE THE FILTER INTO THE SIGNAL PATH
            connect(oldFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), filters.first(), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(filters.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        } else if (controller->filter()) {
            LAUAbstractFilter *oldFilter = controller->filter();
            disconnect(oldFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

            // SPLICE THE FILTER INTO THE SIGNAL PATH
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
void LAU3DVideoWidget::prependFilter(QObject *newFilter)
{
    if (newFilter == nullptr) {
        return;
    }

    if (filterControllers.isEmpty()) {
        // DISCONNECT THE SIGNAL FROM THE CAMERA THAT WOULD GO TO THE GLWIDGET
        disconnect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

        // SPLICE THE FILTER INTO THE SIGNAL PATH
        connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), newFilter,   SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(newFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
    } else {
        // GRAB THE LAST FILTER IN THE LIST
        LAUAbstractFilterController *controller = filterControllers.first();
        if (controller->glFilter()) {
            // DISCONNECT THAT LAST GLFILTER IN THE LIST
            LAUAbstractGLFilter *oldFilter = controller->glFilter();
            disconnect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), oldFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

            // SPLICE THE NEW FILTER INTO THE SIGNAL PATH
            connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), newFilter,   SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(newFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), oldFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        } else if (controller->filter()) {
            LAUAbstractFilter *oldFilter = controller->filter();
            disconnect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), oldFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

            // SPLICE THE NEW FILTER INTO THE SIGNAL PATH
            connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), newFilter,   SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(newFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), oldFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        }
    }

    // CREATE CONTROLLERS TO WRAP AROUND THE FILTER OBJECTS
    if (dynamic_cast<LAUAbstractGLFilter *>(newFilter) != nullptr) {
        filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractGLFilter *)newFilter));
    } else if (dynamic_cast<LAUAbstractFilter *>(newFilter) != nullptr) {
        filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractFilter *)newFilter));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoWidget::prependFilters(QList<QObject *> filters)
{
    if (filters.isEmpty()) {
        return;
    }

    if (filterControllers.isEmpty()) {
        // DISCONNECT THE SIGNAL FROM THE CAMERA THAT WOULD GO TO THE GLWIDGET
        disconnect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

        // SPLICE THE FILTER INTO THE SIGNAL PATH
        connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), filters.first(), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        connect(filters.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
    } else {
        // GRAB THE LAST FILTER IN THE LIST
        LAUAbstractFilterController *controller = filterControllers.first();
        if (controller->glFilter()) {
            // DISCONNECT THAT LAST GLFILTER IN THE LIST
            LAUAbstractGLFilter *oldFilter = controller->glFilter();
            disconnect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), oldFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

            // SPLICE THE NEW FILTER INTO THE SIGNAL PATH
            connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), filters.first(),   SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(filters.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), oldFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        } else if (controller->filter()) {
            LAUAbstractFilter *oldFilter = controller->filter();
            disconnect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), oldFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));

            // SPLICE THE NEW FILTER INTO THE SIGNAL PATH
            connect(camera, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), filters.first(),   SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(filters.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), oldFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        }
    }

    // CREATE CONTROLLERS TO WRAP AROUND THE FILTER OBJECTS
    while (filters.count() > 0) {
        QObject *filter = filters.takeLast();
        if (filter) {
            if (dynamic_cast<LAUAbstractGLFilter *>(filter) != nullptr) {
                filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractGLFilter *)filter));
            } else if (dynamic_cast<LAUAbstractFilter *>(filter) != nullptr) {
                filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractFilter *)filter));
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoWidget::onContextMenuTriggered()
{
    if (scannerWidget) {
        scannerWidget->hide();
        scannerWidget->show();
    } else {
        qDebug() << "LAU3DVideoWidget::onContextMenuTriggered()";
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoWidget::onShowMachineVisionLabelingWidget()
{
#ifdef LUCID
    if (labelWidget) {
        // DISPLAY THE WIDGET
        labelWidget->hide();
        labelWidget->show();
    } else {
        // CREATE A LABEL WIDGET FOR LOADING VIDEO FROM DISK
        if (camera->color() == ColorRGB){
            labelWidget = new LAUMachineLearningVideoFrameLabelerWidget(3, this);
        } else {
            labelWidget = new LAUMachineLearningVideoFrameLabelerWidget(1, this);
        }
        connect(labelWidget, SIGNAL(emitBuffer(QString, int)), camera, SLOT(onUpdateBuffer(QString, int)), Qt::QueuedConnection);
        labelWidget->show();
    }
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoWidget::onUpdateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    // CALL THE VIRTUAL FUNCTION SO THAT SUBCLASSES CAN OVERRIDE IT AND USE THEIR OWN PROCESSING
    updateBuffer(depth, color, mapping);

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
        if (counter >= LAU_FRAME_CAPTURE_RETRY_LIMIT) {
            qDebug() << QString("%1 fps").arg(1000.0 * (float)counter / (float)time.elapsed());
            time.restart();
            counter = 0;
            static bool state = false;
            emit emitEnableEmitter(state);
            state = !state;
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
