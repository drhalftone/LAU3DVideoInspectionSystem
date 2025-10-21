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

#ifndef LAU3DTRACKINGWIDGET_H
#define LAU3DTRACKINGWIDGET_H

#include "lau3dvideowidget.h"
#include "lau3dtrackingfilter.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DTrackingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LAU3DTrackingWidget(LAUVideoPlaybackColor color = ColorXYZWRGBA, LAUVideoPlaybackDevice device = DevicePrimeSense,  QWidget *parent = NULL);
    ~LAU3DTrackingWidget();

    int step() const
    {
        if (camera) {
            return (colors() * depth() * camera->depthWidth());
        } else {
            return (0);
        }
    }

    int depth() const
    {
        return (sizeof(float));
    }

    int width() const
    {
        return (size().width());
    }

    int height() const
    {
        return (size().height());
    }

    QSize size() const
    {
        if (camera) {
            return (QSize(camera->depthWidth(), camera->depthHeight()));
        } else {
            return (QSize(0, 0));
        }
    }

    LAUVideoPlaybackColor color()
    {
        return (playbackColor);
    }

    void enableSnapShotMode(bool state)
    {
        snapShotModeFlag = state;
    }

    int colors() const
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
            default:
                return (0);
        }
    }

    QString make() const
    {
        if (camera) {
            return (camera->make());
        } else {
            return (QString());
        }
    }

    QString model() const
    {
        if (camera) {
            return (camera->model());
        } else {
            return (QString());
        }
    }

    QString serial() const
    {
        if (camera) {
            return (camera->serial());
        } else {
            return (QString());
        }
    }

    bool isValid() const
    {
        if (camera) {
            return (camera->isValid());
        } else {
            return (false);
        }
    }

    bool isNull() const
    {
        return (!isValid());
    }

protected:
    void showEvent(QShowEvent *)
    {
        onUpdateBuffer(LAUScan());
    }

public slots:
    void onUpdateBuffer(LAUScan scan);
    void onUpdateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

    void onContextMenuTriggered();
    void onRecordButtonClicked(bool state);
    void onReceiveFrameBuffer(LAUMemoryObject buffer);
    void onReceiveVideoFrames(LAUScan scan);
    void onReceiveVideoFrames(QList<LAUScan> scanList);

private:
    LAUVideoPlaybackColor playbackColor;
    LAUVideoPlaybackDevice playbackDevice;

    LAU3DCamera *camera;
    LAU3DCameraController *cameraController;
    LAUAbstractFilter *dftFilter;
    LAUAbstractGLFilter *glFilter;
    LAUAbstractFilterController *glFilterController, *dftController;
    LAU3DTrackingController *trackingController;
    LAU3DVideoGLWidget *glWidget;

    LAUMemoryObjectManager *frameBufferManager;
    LAUController *frameBufferManagerController;
    LAUVideoPlayerLabel *videoLabel;
#if defined(PROSILICA) || defined(VIMBA) || defined(BASLERUSB)
    LAU3DMachineVisionScannerWidget *prosilicaScannerWidget;
#endif
    bool snapShotModeFlag, videoRecordingFlag;

    int counter;
    QTime pressStartButtonTime;
    QElapsedTimer time, timeStamp;

    QList<LAUModalityObject> framesList;
    QList<LAUScan> recordList;
    QList<LAUScan> scanList;

signals:
    void emitGetFrame();
    void emitVideoFrames(LAUScan scan);
    void emitVideoFrames(QList<LAUScan> scans);
    void emitReleaseFrame(LAUMemoryObject frame);

    void emitBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);
    void emitBuffer(LAUScan scan);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DTrackingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAU3DTrackingDialog(LAUVideoPlaybackColor color = ColorXYZRGB, LAUVideoPlaybackDevice device = DevicePrimeSense, QWidget *parent = NULL) : QDialog(parent)
    {
        // CREATE A WIDGET TO WRAP AROUND
        widget = new LAU3DTrackingWidget(color, device);

        // SET THE LAYOUT AND DISPLAY OUR WIDGET INSIDE OF IT
        this->setWindowTitle(QString("Video Recorder"));
        this->setLayout(new QVBoxLayout());
        this->layout()->setContentsMargins(0, 0, 0, 0);
        this->layout()->addWidget(widget);

        QObject::connect(widget, SIGNAL(emitVideoFrames(QList<LAUScan>)), widget, SLOT(onReceiveVideoFrames(QList<LAUScan>)), Qt::QueuedConnection);
        QObject::connect(widget, SIGNAL(emitVideoFrames(LAUScan)), widget, SLOT(onReceiveVideoFrames(LAUScan)), Qt::QueuedConnection);
        widget->enableSnapShotMode(false);
    }

private:
    LAU3DTrackingWidget *widget;
};

#endif // LAU3DTRACKINGWIDGET_H
