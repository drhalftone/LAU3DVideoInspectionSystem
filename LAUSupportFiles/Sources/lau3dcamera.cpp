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

#include "lau3dcamera.h"
#ifdef KINECT
#include "laukinectcamera.h"
#endif

#ifdef SHARED_CAMERA_THREAD
QThread* LAU3DCameraController::sharedCameraThread = nullptr;
int LAU3DCameraController::sharedThreadRefCount = 0;
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DCamera::fillHoles(LAUMemoryObject object)
{
    if (object.depth() == sizeof(unsigned short)) {
        if (object.colors() == 1) {
            for (unsigned int row = 0; row < object.height(); row++) {
                unsigned short *buffer = (unsigned short *)object.constScanLine(row);
                for (unsigned int col = 1; col < object.width(); col++) {
                    if (buffer[col] == 0xffff) {
                        buffer[col] = buffer[col - 1];
                    }
                }
                for (unsigned int col = object.width() / 4; col > 0; col--) {
                    if (buffer[col - 1] == 0xffff) {
                        buffer[col - 1] = buffer[col];
                    }
                }
            }
        } else if (object.colors() == 3) {
            for (unsigned int row = 0; row < object.height(); row++) {
                unsigned short *buffer = (unsigned short *)object.constScanLine(row);
                for (unsigned int col = 1; col < object.width(); col++) {
                    if (buffer[3 * col + 0] == 0xffff) {
                        buffer[3 * col + 0] = buffer[3 * col + 0 - 3];
                    }
                    if (buffer[3 * col + 1] == 0xffff) {
                        buffer[3 * col + 1] = buffer[3 * col + 1 - 3];
                    }
                    if (buffer[3 * col + 2] == 0xffff) {
                        buffer[3 * col + 2] = buffer[3 * col + 2 - 3];
                    }
                }
                for (unsigned int col = object.width() / 4; col > 0; col--) {
                    if (buffer[3 * col + 0 - 3] == 0xffff) {
                        buffer[3 * col + 0 - 3] = buffer[3 * col + 0];
                    }
                    if (buffer[3 * col + 1 - 3] == 0xffff) {
                        buffer[3 * col + 1 - 3] = buffer[3 * col + 1];
                    }
                    if (buffer[3 * col + 2 - 3] == 0xffff) {
                        buffer[3 * col + 2 - 3] = buffer[3 * col + 2];
                    }
                }
            }
        } else if (object.colors() == 4) {
            for (unsigned int row = 0; row < object.height(); row++) {
                unsigned short *buffer = (unsigned short *)object.constScanLine(row);
                for (unsigned int col = 1; col < object.width(); col++) {
                    if (buffer[4 * col + 0] == 0xffff) {
                        buffer[4 * col + 0] = buffer[4 * col + 0 - 4];
                    }
                    if (buffer[4 * col + 1] == 0xffff) {
                        buffer[4 * col + 1] = buffer[4 * col + 1 - 4];
                    }
                    if (buffer[4 * col + 2] == 0xffff) {
                        buffer[4 * col + 2] = buffer[4 * col + 2 - 4];
                    }
                    if (buffer[4 * col + 3] == 0xffff) {
                        buffer[4 * col + 3] = buffer[4 * col + 3 - 4];
                    }
                }
                for (unsigned int col = object.width() / 4; col > 0; col--) {
                    if (buffer[4 * col + 0 - 4] == 0xffff) {
                        buffer[4 * col + 0 - 4] = buffer[4 * col + 0];
                    }
                    if (buffer[4 * col + 1 - 4] == 0xffff) {
                        buffer[4 * col + 1 - 4] = buffer[4 * col + 1];
                    }
                    if (buffer[4 * col + 2 - 4] == 0xffff) {
                        buffer[4 * col + 2 - 4] = buffer[4 * col + 2];
                    }
                    if (buffer[4 * col + 3 - 4] == 0xffff) {
                        buffer[4 * col + 3 - 4] = buffer[4 * col + 3];
                    }
                }
            }
        }
    } else if (object.depth() == sizeof(unsigned short)) {
        if (object.colors() == 1) {
            for (unsigned int row = 0; row < object.height(); row++) {
                unsigned char *buffer = (unsigned char *)object.constScanLine(row);
                for (unsigned int col = 1; col < object.width(); col++) {
                    if (buffer[col] == 0xff) {
                        buffer[col] = buffer[col - 1];
                    }
                }
                for (unsigned int col = object.width() / 4; col > 0; col--) {
                    if (buffer[col - 1] == 0xff) {
                        buffer[col - 1] = buffer[col];
                    }
                }
            }
        } else if (object.colors() == 3) {
            for (unsigned int row = 0; row < object.height(); row++) {
                unsigned char *buffer = (unsigned char *)object.constScanLine(row);
                for (unsigned int col = 1; col < object.width(); col++) {
                    if (buffer[3 * col + 0] == 0xff) {
                        buffer[3 * col + 0] = buffer[3 * col + 0 - 3];
                    }
                    if (buffer[3 * col + 1] == 0xff) {
                        buffer[3 * col + 1] = buffer[3 * col + 1 - 3];
                    }
                    if (buffer[3 * col + 2] == 0xff) {
                        buffer[3 * col + 2] = buffer[3 * col + 2 - 3];
                    }
                }
                for (unsigned int col = object.width() / 4; col > 0; col--) {
                    if (buffer[3 * col + 0 - 3] == 0xff) {
                        buffer[3 * col + 0 - 3] = buffer[3 * col + 0];
                    }
                    if (buffer[3 * col + 1 - 3] == 0xff) {
                        buffer[3 * col + 1 - 3] = buffer[3 * col + 1];
                    }
                    if (buffer[3 * col + 2 - 3] == 0xff) {
                        buffer[3 * col + 2 - 3] = buffer[3 * col + 2];
                    }
                }
            }
        } else if (object.colors() == 4) {
            for (unsigned int row = 0; row < object.height(); row++) {
                unsigned char *buffer = (unsigned char *)object.constScanLine(row);
                for (unsigned int col = 1; col < object.width(); col++) {
                    if (buffer[4 * col + 0] == 0xff) {
                        buffer[4 * col + 0] = buffer[4 * col + 0 - 4];
                    }
                    if (buffer[4 * col + 1] == 0xff) {
                        buffer[4 * col + 1] = buffer[4 * col + 1 - 4];
                    }
                    if (buffer[4 * col + 2] == 0xff) {
                        buffer[4 * col + 2] = buffer[4 * col + 2 - 4];
                    }
                    if (buffer[4 * col + 3] == 0xff) {
                        buffer[4 * col + 3] = buffer[4 * col + 3 - 4];
                    }
                }
                for (unsigned int col = object.width() / 4; col > 0; col--) {
                    if (buffer[4 * col + 0 - 4] == 0xff) {
                        buffer[4 * col + 0 - 4] = buffer[4 * col + 0];
                    }
                    if (buffer[4 * col + 1 - 4] == 0xff) {
                        buffer[4 * col + 1 - 4] = buffer[4 * col + 1];
                    }
                    if (buffer[4 * col + 2 - 4] == 0xff) {
                        buffer[4 * col + 2 - 4] = buffer[4 * col + 2];
                    }
                    if (buffer[4 * col + 3 - 4] == 0xff) {
                        buffer[4 * col + 3 - 4] = buffer[4 * col + 3];
                    }
                }
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DCameraController::LAU3DCameraController(LAU3DCamera *cam, QObject *parent) : QObject(parent), camera(cam), cameraThread(nullptr)
{
#ifdef SHARED_CAMERA_THREAD
    // USE SHARED THREAD FOR ALL CAMERAS
    if (sharedCameraThread == nullptr) {
        sharedCameraThread = new QThread();
        sharedCameraThread->start();
    }
    cameraThread = sharedCameraThread;
    sharedThreadRefCount++;
#else
    // CREATE A THREAD AND MOVE THE CAMERA TO IT
    cameraThread = new QThread();
#endif

#ifdef KINECT
    // SEE IF WE NEED TO DO ANY CAMERA SPECIFIC OPERATIONS
    if (camera->device() == DeviceKinect) {
        // PLEASE KEEP THE CONNECTION ABOVE, ONLY REPORT MULTI-STEAMS FAILURE WHEN REAL CRASHING, REALLY HELPFUL AND IMPORTANT FOR DEBUGGING CONNECTION,.
        if (((LAUKinectCamera *)camera)->timer) {
            connect(cameraThread, SIGNAL(started()), ((LAUKinectCamera *)camera)->timer, SLOT(start()), Qt::QueuedConnection);
            connect(this, SIGNAL(emitStopCameraTimer()), (LAUKinectCamera *)camera, SLOT(onCommandStopTimer()), Qt::QueuedConnection);
        }
    }
#endif
    // CONNECT THE ERROR STRING SIGNAL
    connect(camera, SIGNAL(emitError(QString)), this, SLOT(onError(QString)), Qt::QueuedConnection);

    // MAKE CONNECTIONS TO HANDLE DECONSTRUCTION
    connect(cameraThread, SIGNAL(started()), camera, SLOT(onThreadStart()));
    connect(cameraThread, SIGNAL(finished()), camera, SLOT(onThreadStop()));
    connect(cameraThread, SIGNAL(finished()), camera, SLOT(deleteLater()));
#ifndef SHARED_CAMERA_THREAD
    connect(camera, SIGNAL(destroyed()), cameraThread, SLOT(deleteLater()));
#endif

    // START THE CAMERA THREAD
    camera->moveToThread(cameraThread);
#ifndef SHARED_CAMERA_THREAD
    cameraThread->start();
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DCameraController::~LAU3DCameraController()
{
#ifdef KINECT
    // SEE IF WE NEED TO DO ANY CAMERA SPECIFIC OPERATIONS
    if (camera->device() == DeviceKinect) {
        emit emitStopCameraTimer();
        if (((LAUKinectCamera *)camera)->timer) {
            while (((LAUKinectCamera *)camera)->timer->isActive()) {
                qApp->processEvents();
            }
        }
    }
#endif

#ifdef SHARED_CAMERA_THREAD
    // DELETE THE CAMERA
    if (camera) {
        camera->deleteLater();
    }
    
    // DECREMENT REFERENCE COUNT AND DELETE THREAD IF LAST CONTROLLER
    sharedThreadRefCount--;
    if (sharedThreadRefCount == 0 && sharedCameraThread) {
        sharedCameraThread->quit();
        while (sharedCameraThread->isRunning()) {
            QThread::msleep(50);
        }
        sharedCameraThread->deleteLater();
        sharedCameraThread = nullptr;
    }
#else
    // STOP AND DELETE THREAD
    if (cameraThread) {
        cameraThread->quit();
        while (cameraThread->isRunning()) {
            QThread::msleep(50);
        }
        //delete cameraThread;
    } else if (camera) {
        // DELETE THE CAMERA IF IT EXISTS
        camera->deleteLater();
    }
#endif

    // REPORT TO THE APPLICATION OUTPUT THAT WE ARE HERE
    qDebug() << QString("LAU3DCameraController::~LAU3DCameraController()");
}
