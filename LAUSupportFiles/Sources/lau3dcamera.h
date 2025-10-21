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

#ifndef LAU3DCAMERA_H
#define LAU3DCAMERA_H

#include <QDebug>
#include <QObject>
#include <QThread>

#include "laulookuptable.h"

#define NUMFRAMESINBUFFER 2

using namespace LAU3DVideoParameters;

#ifndef HEADLESS
#include <QLabel>
#include <QSlider>
#include <QWidget>
#include <QLayout>
#include <QSpinBox>
#include <QGridLayout>
#include <QMessageBox>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCameraWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LAUCameraWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        this->setWindowFlags(Qt::Tool);
        this->setWindowTitle(QString("Camera Settings"));
        this->setLayout(new QGridLayout());
#ifdef Q_OS_WIN
        this->layout()->setContentsMargins(6, 6, 6, 6);
#else
        this->layout()->setContentsMargins(6, 6, 6, 6);
#endif
        this->layout()->setSpacing(3);
        this->setFixedSize(435, 45);

        QSettings settings;
        exposure = settings.value(QString("LAUCameraWidget::exposure"), 1000).toInt();

        expSlider = new QSlider(Qt::Horizontal);
        expSlider->setMinimum(1);
        expSlider->setMaximum(50000);
        expSlider->setValue(exposure);

        expSpinBox = new QSpinBox();
        expSpinBox->setFixedWidth(80);
        expSpinBox->setAlignment(Qt::AlignRight);
        expSpinBox->setMinimum(1);
        expSpinBox->setMaximum(50000);
        expSpinBox->setValue(exposure);

        QLabel *label = new QLabel(QString("Exposure"));
        label->setToolTip(QString("exposure time of the camera in microseconds"));
        ((QGridLayout *)(this->layout()))->addWidget(label, 0, 0, 1, 1, Qt::AlignRight);
        ((QGridLayout *)(this->layout()))->addWidget(expSlider, 0, 1, 1, 1);
        ((QGridLayout *)(this->layout()))->addWidget(expSpinBox, 0, 2, 1, 1);

        connect(expSlider, SIGNAL(valueChanged(int)), expSpinBox, SLOT(setValue(int)));
        connect(expSpinBox, SIGNAL(valueChanged(int)), expSlider, SLOT(setValue(int)));
        connect(expSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onUpdateExposurePrivate(int)));
    }

    ~LAUCameraWidget()
    {
        QSettings settings;
        settings.setValue(QString("LAUCameraWidget::exposure"), exposure);
        qDebug() << "LAUCameraWidget::~LAUCameraWidget()";
    }

    void setExp(int val)
    {
        expSpinBox->setValue(val);
    }

    int exp()
    {
        return (exposure);
    }

public slots:
    void onUpdateExposure(int val)
    {
        if (val != exposure) {
            exposure = val;
            expSpinBox->setValue(exposure);
        }
    }

private slots:
    void onUpdateExposurePrivate(int val)
    {
        if (val != exposure) {
            exposure = val;
            emit emitUpdateExposure(exposure);
        }
    }

private:
    int exposure;
    QSlider *expSlider;
    QSpinBox *expSpinBox;

signals:
    void emitUpdateExposure(int val);
};
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DCamera : public QObject
{
    Q_OBJECT

public:
    LAU3DCamera(LAUVideoPlaybackColor color = ColorXYZRGB, QObject *parent = nullptr) : QObject(parent), playbackColor(color), frameReplicateCount(1), startingIndex(0), stopFlag(false), bitsPerPixel(8), zMinDistance(0), zMaxDistance(65535), localScaleFactor(1.0), horizontalFieldOfView(0.0f), verticalFieldOfView(0.0f), hasDepthVideo(false), hasColorVideo(false), isConnected(false) { ; }
    LAU3DCamera(QObject *parent) : QObject(parent), playbackColor(ColorXYZRGB), stopFlag(false), startingIndex(0), frameReplicateCount(1), bitsPerPixel(8), zMinDistance(0), zMaxDistance(65535), localScaleFactor(1.0), horizontalFieldOfView(0.0f), verticalFieldOfView(0.0f), hasDepthVideo(false), hasColorVideo(false), isConnected(false) { ; }
    ~LAU3DCamera()
    {
        qDebug() << QString("LAU3DCamera::~LAU3DCamera()");
    }

    static void fillHoles(LAUMemoryObject object);

    virtual LAUVideoPlaybackDevice device() const = 0;
    virtual bool reset() = 0;

    virtual bool hasDepth() const
    {
        return (hasDepthVideo);
    }

    virtual bool hasColor() const
    {
        return (hasColorVideo);
    }

    virtual bool hasMapping() const
    {
        return (false);
    }

    virtual bool isValid() const
    {
        return (isConnected);
    }

    virtual bool isNull() const
    {
        return (!isValid());
    }

    virtual bool isStereo() const
    {
        return (false);
    }

    virtual QString error() const
    {
        return (errorString);
    }

    virtual unsigned short maxIntensityValue() const
    {
        return (static_cast<unsigned short>((1 << bitsPerPixel) - 1));
    }

    virtual float horizontalFieldOfViewInRadians() const
    {
        return (horizontalFieldOfView);
    }

    virtual float verticalFieldOfViewInRadians() const
    {
        return (verticalFieldOfView);
    }

    virtual float horizontalFieldOfViewInDegrees() const
    {
        return (horizontalFieldOfView * 180.0f / 3.14159265359f);
    }

    virtual float verticalFieldOfViewInDegrees() const
    {
        return (verticalFieldOfView * 180.0f / 3.14159265359f);
    }

    virtual double scaleFactor() const
    {
        return (localScaleFactor);
    }

    virtual float minDistance() const
    {
        return (zMinDistance);
    }

    virtual float maxDistance() const
    {
        return (zMaxDistance);
    }

    virtual unsigned int sensors() const
    {
        if (isValid()) {
            return (1);
        }
        return (0);
    }

    virtual QVector<double> jetr(int chn = 0) const
    {
        Q_UNUSED(chn);
        QVector<double> vector(37, NAN);
        return(vector);
    }

    virtual unsigned int depthWidth() const = 0;
    virtual unsigned int depthHeight() const = 0;
    virtual unsigned int colorWidth() const = 0;
    virtual unsigned int colorHeight() const = 0;
    virtual QList<LAUVideoPlaybackColor> playbackColors() = 0;

    LAUVideoPlaybackColor color() const
    {
        return (playbackColor);
    }

    virtual unsigned int colors() const
    {
        switch (playbackColor) {
            case ColorUndefined:
                return (0);
            case ColorGray:
                return (1);
            case ColorRGB:
                return (3);
            case ColorRGBA:
                return (4);
            case ColorXYZ:
                return (3);
            case ColorXYZW:
                return (4);
            case ColorXYZG:
                return (4);
            case ColorXYZRGB:
                return (6);
            case ColorXYZWRGBA:
                return (8);
        }
        return (0);
    }

    virtual LAUMemoryObject colorMemoryObject() const = 0;
    virtual LAUMemoryObject depthMemoryObject() const = 0;
    virtual LAUMemoryObject mappiMemoryObject() const = 0;

    virtual QString make() const
    {
        return (makeString);
    }

    virtual QString model() const
    {
        return (modelString);
    }

    virtual QString serial() const
    {
        return (serialString);
    }

    virtual QString sensorMake(int snr = 0) const
    {
        Q_UNUSED(snr);
        return(make());
    }

    virtual QString sensorModel(int snr = 0) const
    {
        Q_UNUSED(snr);
        return(model());
    }

    virtual QString sensorSerial(int snr = 0) const
    {
        Q_UNUSED(snr);
        return(serial());
    }

    virtual unsigned int frames() const
    {
        return (1);
    }

    virtual LAULookUpTable lut(int chn = 0, QWidget *widget = nullptr) const
    {
        Q_UNUSED(chn);
        Q_UNUSED(widget);
        return (LAULookUpTable());
    }

    unsigned int height()
    {
        if (playbackColor == ColorGray || playbackColor == ColorRGB) {
            return (colorHeight());
        } else {
            return (depthHeight());
        }
    }

    unsigned int width()
    {
        if (playbackColor == ColorGray || playbackColor == ColorRGB) {
            return (colorWidth());
        } else {
            return (depthWidth());
        }
    }

    void setStartingFrameIndex(unsigned int val)
    {
        startingIndex = val;
    }

    unsigned int startingFrameIndex() const
    {
        return (startingIndex);
    }

    QSize size()
    {
        return (QSize(static_cast<int>(width()), static_cast<int>(height())));
    }

    virtual void restart()
    {
        timer.restart();
    }

    virtual unsigned int elapsed()
    {
        return (timer.elapsed());
    }

    unsigned int replicateCount()
    {
        return (frameReplicateCount);
    }

    void setReplicateCount(unsigned int val)
    {
        frameReplicateCount = qMax(val, (unsigned int)1);
    }

    void stopCamera()
    {
        stopFlag = true;
    }

public slots:
    virtual void onThreadStop() { ; }
    virtual void onThreadStart() { ; }
    virtual void onUpdateExposure(int microseconds) = 0;
    virtual void onUpdateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject()) = 0;
    virtual void onUpdateBuffer(LAUMemoryObject buffer, int index, void *userData) = 0;

protected:
    LAUVideoPlaybackColor playbackColor;
    QString makeString;
    QString modelString;
    QString serialString;

    QString errorString;

    unsigned int frameReplicateCount;
    unsigned int startingIndex;
    unsigned short bitsPerPixel;
    unsigned short zMinDistance;
    unsigned short zMaxDistance;
    double localScaleFactor;
    float horizontalFieldOfView;
    float verticalFieldOfView;
    bool hasDepthVideo, hasColorVideo;
    bool isConnected;
    bool stopFlag;

    QElapsedTimer timer;

signals:
    void emitError(QString);
    void emitBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);
    void emitBuffer(LAUMemoryObject buffer, int index, void *userData);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DCameraController : public QObject
{
    Q_OBJECT

public:
    explicit LAU3DCameraController(LAU3DCamera *cam, QObject *parent = nullptr);
    ~LAU3DCameraController();

public slots:
    void onError(QString string)
    {
        qDebug() << string;
    }

signals:
    void emitStopCameraTimer();

private:
    LAU3DCamera *camera;
    QThread *cameraThread;
#ifdef SHARED_CAMERA_THREAD
    static QThread *sharedCameraThread;
    static int sharedThreadRefCount;
#endif
};

#endif // LAU3DCAMERA_H
