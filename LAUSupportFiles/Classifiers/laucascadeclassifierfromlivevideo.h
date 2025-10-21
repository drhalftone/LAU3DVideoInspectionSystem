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

#ifndef LAUCASCADECLASSIFIERFROMLIVEVIDEO_H
#define LAUCASCADECLASSIFIERFROMLIVEVIDEO_H

#include <QWidget>
#include <QEventLoop>
#include <QtNetwork/QLocalSocket>

#include "lau3dcamera.h"
#include "laurfidwidget.h"
#include "lauobjecthashtable.h"
#include "laumemoryobject.h"
#include "lausavetodiskfilter.h"

#ifdef USE_GREENSCREEN_FILTER
#include "laugreenscreenglfilter.h"
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCascadeClassifierFromLiveVideo : public QDialog
{
    Q_OBJECT

public:
    explicit LAUCascadeClassifierFromLiveVideo(QWidget *parent = nullptr);
    explicit LAUCascadeClassifierFromLiveVideo(QString dirString, int threshold = -1, QWidget *parent = nullptr);
    ~LAUCascadeClassifierFromLiveVideo();

    bool isValid() const
    {
        return (cameraCount > 0);
    }

    bool isNull() const
    {
        return (cameraCount < 1);
    }

    int sensors() const
    {
        return (sensorCount);
    }

    LAULookUpTable lut(int sns)
    {
        int sensors = 0;
        for (int cam = 0; cam < cameras.count(); cam++) {
            sensors += cameras.at(cam)->sensors();
            if (sensors > sns) {
                return (cameras.at(cam)->lut(sns));
            } else {
                sns -= cameras.at(cam)->sensors();
            }
        }
    }

    void setTimer(QString duration)
    {
        timeString = duration;
    }

    QString error() const
    {
        return (errorString);
    }

    void testRelayCycling()
    {
        triggerRelayCyclingAndWait();
    }

public slots:
    void onCameraError(QString string);
    void onRFID(QString string, QTime time);
    void onFilterDestroyed()
    {
        filterCount--;
    }

    void onCameraDeleted()
    {
        cameraCount--;
    }

    void onShutDown()
    {
        shutDownFlag = true;
    }

    void onUpdateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

    void onRFIDError(QString string)
    {
        qDebug() << string;
    }

    void onNewRecordingOpened(int index){
        dataFileCount = index;
    }

protected:
    void accept();
    void showEvent(QShowEvent *event)
    {
        // CALL THE UNDERLYING WIDGET'S SHOW EVENT HANDLER
        QWidget::showEvent(event);

        // MAKE SURE THE DURATION STRING IS PROPERLY FORMATED
        QStringList strings = timeString.split(":");
        if (strings.count() == 3) {
            // GET THE TOTAL TIME WE ARE TO RECORD VIDEO TO DISK
            int hours = strings.at(0).toInt();
            int mints = strings.at(1).toInt();
            int secds = strings.at(2).toInt();

            // SET THE TIMER FOR COUNTING DOWN THE RECORDING DURATION IN MILLISECONDS
            endTime.singleShot((60 * (60 * hours + mints) + secds) * 1000 + 1000, this, SLOT(accept()));
            endTime.start();
        }

        // MAKE THE FIRST CALL TO ONUPDATEBUFFER TO START FRAME PROCESSING
        QTimer::singleShot(1000, this, SLOT(onUpdateBuffer()));
    }

private:
    int filterCount;
    int cameraCount;
    int sensorCount;

    bool shutDownFlag;
    QList<LAU3DCamera *> cameras;
    QList<LAU3DCameraController *> cameraControllers;

    QFile logFile;
    QTextStream logTS;

    QTimer endTime;
    QString timeString;
    QString directoryString;
    QList<LAUModalityObject> framesList;
    QList<LAUAbstractFilterController *> filterControllers;

    // CREATE VARIABLES TO DETERMINE WHEN OBJECTS ARE FLOWING THROUGH
    // AND WHEN WE SHOULD IGNORE RFIDS OF OBJECTS MOVING IN BACKWARD
    int dataFileCount;
    QStringList oldRFIDs;

    QString errorString;

    QTime rfidTime;
    QString rfidString;
#ifdef ENABLEFILTERS
    LAURFIDObject *rfidObject;
    LAUObjectHashTable *rfidHashTable;
    LAUSaveToDiskFilter *saveToDiskFilter;
#else
    QObject *rfidObject;
    QObject *rfidHashTable;
    QObject *saveToDiskFilter;
#endif
    // Frame rate monitoring variables
    bool monitoringStarted;
    QTimer* frameRateTimer;
    qint64 callCount;
    QElapsedTimer elapsedTimer;
    static constexpr double minCallsPerSecond = 5.0;
    static constexpr int measurementIntervalMs = 30000; // 30 seconds
    
    // Channel tracking
    int channel;

private slots:
    void checkFrameRate();
    void updateWindowTitleWithChannel();

private:
    void triggerRelayCyclingAndWait();

signals:
    void emitBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);

};
#endif // LAUCASCADECLASSIFIERFROMLIVEVIDEO_H
