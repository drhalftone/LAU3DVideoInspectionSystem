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

#ifndef LAUORBBECCONTROLLER_H
#define LAUORBBECCONTROLLER_H

#include <QImage>
#include <QList>
#include <QString>
#include <QObject>
#include <QDebug>
#include <QTimer>

#ifdef ORBBEC_USE_RESOLUTION_DIALOG
#include <QInputDialog>
#include <QMessageBox>
#endif

#include "lau3dcamera.h"

#ifndef Q_OS_MAC
#include "libobsensor/ObSensor.h"
#endif

#define ORBBECDEPTHSENSORNROWHFOV   75.0f/180.0f * 3.14159265359f
#define ORBBECDEPTHSENSORNROWVFOV   65.0f/180.0f * 3.14159265359f
#define ORBBECDEPTHSENSORWIDEHFOV   120.0f/180.0f * 3.14159265359f
#define ORBBECDEPTHSENSORWIDEVFOV   120.0f/180.0f * 3.14159265359f

////////////////////////1000000000
#define ORBBECDELTATIME     1000000  // 10 milliseconds
#define ORBBECEXPOSURETIME   500.0
#define ORBBECMAXDEVICES     10
#define ORBBECMAXBUF         256

// system timeout
#define SYSTEM_TIMEOUT 100

class LAUOrbbecCamera : public LAU3DCamera
{
    Q_OBJECT

public:
    explicit LAUOrbbecCamera(LAUVideoPlaybackColor color = ColorXYZRGB, QObject *parent = nullptr);
    explicit LAUOrbbecCamera(QObject *parent);
    ~LAUOrbbecCamera();

    typedef struct {
#ifndef Q_OS_MAC
        ob_config *config;
        ob_pipeline *pipeline;
        ob_device *device;
#endif
        bool isConnected;
        bool isPseudoDepthProfile;  // Track if using pseudo 640x480 depth profile
        bool isPseudoColorProfile;  // Track if using pseudo 640x480 NIR profile
        unsigned int numDepthRows;
        unsigned int numDepthCols;
        unsigned int numColorRows;
        unsigned int numColorCols;
        QString modelString;
        QString serialString;
        QString makeString;
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
        return (LAU3DVideoParameters::DeviceOrbbec);
    }

    unsigned short maxIntensityValue() const
    {
        if (playbackColor == ColorXYZRGB) {
            return (255);
        }
        return (zMaxDistance);
    }

    double scaleFactor() const
    {
        return (localScaleFactor);
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

    LAUMemoryObject colorMemoryObject() const;
    LAUMemoryObject depthMemoryObject() const;
    LAUMemoryObject mappiMemoryObject() const;

    QList<LAUVideoPlaybackColor> playbackColors();

    QVector<double> jetr(int chn) const;
    LAULookUpTable lut(int chn = 0, QWidget *widget = nullptr) const;
    unsigned int elapsed()
    {
        return (QTime::currentTime().msecsSinceStartOfDay());
    }

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

public slots:
    void onUpdateExposure(int microseconds)
    {
        Q_UNUSED(microseconds);
    }
    void onUpdateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());
    void onUpdateBuffer(LAUMemoryObject buffer, int index, void *userData)
    {
        emit emitBuffer(buffer, index, userData);
    }

signals:
    void emitDebugError(QString);
    void emitBackgroundTexture(LAUMemoryObject buffer);

private:
    typedef struct {
        QString filename;
        int frame;
    } FramePacket;

#ifdef ORBBEC_USE_RESOLUTION_DIALOG
    typedef struct {
        QString name;
        uint32_t width;
        uint32_t height;
        uint32_t fps;
#ifndef Q_OS_MAC
        ob_format format;
        ob_stream_profile *profile;
#endif
    } StreamProfileInfo;

#ifndef Q_OS_MAC
    QList<StreamProfileInfo> getAvailableDepthProfiles(ob_sensor *sensor);
    QList<StreamProfileInfo> getAvailableColorProfiles(ob_sensor *sensor);
    StreamProfileInfo selectBestColorProfile(const QList<StreamProfileInfo> &colorProfiles, uint32_t targetFps);
#endif
#endif

    unsigned int numDepthRows, numDepthCols, numColorRows, numColorCols;

#ifndef Q_OS_MAC
    // CREATE A CONTEXT HANDLE FOR THE LIBRARY
    ob_context *context;
    void processError(ob_error **err);
#else
    void *context;
#endif

    // Get OrbbecSDK version.
    int major_version;
    int minor_version;
    int patch_version;

    QList<CameraPacket> cameras;

    QString rangeModeString;
    int failCount = 0;
    int imageCounter = 0;
    int frameCounter = 0;
    int framesCount = 0;
    QList<FramePacket> framePackets;
    QList<LAUModalityObject> frameObjects;

    LAUMemoryObject depthBuffer;
    LAUMemoryObject colorBuffer;
    QStringList fileStrings;
    unsigned int frameReplicateCount = 1;
    bool hasMappingVideo;
    double localScaleFactor;

    void initialize();
};
#endif // LAUORBBECCONTROLLER_H
