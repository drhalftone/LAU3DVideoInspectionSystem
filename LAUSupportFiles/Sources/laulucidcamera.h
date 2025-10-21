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

#ifndef LAULUCIDCONTROLLER_H
#define LAULUCIDCONTROLLER_H

#include <QImage>
#include <QList>
#include <QString>
#include <QObject>
#include <QDebug>
#include <QTimer>

#if !defined(Q_OS_MAC)
#include "ArenaCApi.h"
#endif

#include "lau3dcamera.h"

#define LUCIDRANGEMODESTRING   "Distance4000mmSingleFreq" // "Distance5000mmMultiFreq" //"Distance4000mmSingleFreq" //MultiFreq" Distance8300mmMultiFreq
#define LUCIDDEPTHSENSORWIDTH  640
#define LUCIDDEPTHSENSORHEIGHT 480
#define LUCIDCOLORSENSORWIDTH  640
#define LUCIDCOLORSENSORHEIGHT 480
#define LUCIDDEPTHSENSORHFOV   69.0f/180.0f * 3.14159265359f
#define LUCIDDEPTHSENSORVFOV   51.0f/180.0f * 3.14159265359f

////////////////////////1000000000
#define LUCIDDELTATIME     100000  // 10 milliseconds
#define LUCIDEXPOSURETIME   500.0
#define LUCIDMAXDEVICES     10
#define LUCIDMAXBUF         256

// system timeout
#define SYSTEM_TIMEOUT 100

class LAULucidCamera : public LAU3DCamera
{
    Q_OBJECT

public:
    explicit LAULucidCamera(LAUVideoPlaybackColor color = ColorXYZRGB, QObject *parent = nullptr);
    explicit LAULucidCamera(QString range = QString(LUCIDRANGEMODESTRING), LAUVideoPlaybackColor color = ColorXYZRGB, QObject *parent = nullptr);
    explicit LAULucidCamera(QObject *parent);
    ~LAULucidCamera();

    typedef struct {
#if !defined(Q_OS_MAC)
        acDevice hDevice;
        acNodeMap hNodeMap;
        acNodeMap hTLStreamNodeMap;
#endif
        bool isConnected;
        unsigned int numDepthRows;
        unsigned int numDepthCols;
        unsigned int numColorRows;
        unsigned int numColorCols;
        double scaleFactor;
        QString makeString;
        QString modelString;
        QString serialString;
        QString userDefinedName;
        LookUpTableIntrinsics deviceIntrinsics;
    } CameraPacket;

    bool reset()
    {
        return (false);
    }

    bool hasMapping() const
    {
        return (hasMappingVideo);
    }

    LAUVideoPlaybackDevice device() const
    {
        return (DeviceLucid);
    }

    unsigned short maxIntensityValue() const
    {
        return (zMaxDistance);
    }

    double scaleFactor() const
    {
        return (0.25);
    }

    void setReplicateCount(unsigned int val)
    {
        frameReplicateCount = qMax(val, (unsigned int)1);
    }

    unsigned int depthWidth() const
    {
        return (numDepthCols);
    }

    unsigned int depthHeight() const
    {
        return (numDepthRows);
    }

    unsigned int colorWidth() const
    {
        return (numColorCols);
    }

    unsigned int colorHeight() const
    {
        return (numColorRows);
    }

    void enableReadVideoFromDisk(bool state);

    LAUMemoryObject colorMemoryObject() const;
    LAUMemoryObject depthMemoryObject() const;
    LAUMemoryObject mappiMemoryObject() const;

    QVector<double> jetr(int chn = 0) const;

    QList<LAUVideoPlaybackColor> playbackColors();

    LAULookUpTable lut(int chn = 0, QWidget *widget = nullptr) const;
    unsigned int elapsed()
    {
        return (QTime::currentTime().msecsSinceStartOfDay());
    }

#ifndef Q_OS_MAC
    QString sensorMake(int snr = 0) const
    {
        return(cameras.at(snr).makeString);
    }

    QString sensorModel(int snr = 0) const
    {
        return(cameras.at(snr).modelString);
    }

    QString sensorSerial(int snr = 0) const
    {
        return(cameras.at(snr).serialString);
    }

    unsigned int sensors() const
    {
        return (cameras.count());
    }

    // Static method to set user-defined names for connected Lucid cameras
    static bool setUserDefinedNames(const QStringList &names, QString &errorMessage, QStringList &progressMessages);
#endif

public slots:
    void onThreadStart() override;
    void onUpdateExposure(int microseconds)
    {
        Q_UNUSED(microseconds);
    }
    void onUpdateBuffer(QString filename, int frame);
    void onUpdateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());
    void onUpdateBuffer(LAUMemoryObject buffer, int index, void *userData)
    {
        emit emitBuffer(buffer, index, userData);
    }
    bool onSetCameraUserDefinedName(int channel, QString name);
    QString onGetCameraUserDefinedName(int channel);

signals:
    void emitDebugError(QString);
    void emitBackgroundTexture(LAUMemoryObject buffer);

private:
    typedef struct {
        QString filename;
        int frame;
    } FramePacket;

    unsigned int numDepthRows, numDepthCols, numColorRows, numColorCols;

#if !defined(Q_OS_MAC)
    acSystem hSystem;
    acNodeMap hTLSystemNodeMap;
    static QString errorMessages(AC_ERROR err);
    QString GetNodeValue(acNodeMap hMap, const char *nodeName);
    bool SetNodeValue(acNodeMap hMap, const char *nodeName, const char *pValue);
#endif
    QList<CameraPacket> cameras;

    QString rangeModeString;
    int failCount = 0;
    int imageCounter = 0;
    int frameCounter = 0;
    int framesCount = 0;
    QList<FramePacket> framePackets;
    QList<LAUModalityObject> frameObjects;
    bool readVideoFromDiskFlag;
    LAUMemoryObject depthBuffer;
    LAUMemoryObject colorBuffer;
    QStringList fileStrings;
    unsigned int frameReplicateCount = 1;
    bool hasMappingVideo;
    double localScaleFactor;

    void initialize();
    void sortCameras();
};
#endif // LAULUCIDCONTROLLER_H
