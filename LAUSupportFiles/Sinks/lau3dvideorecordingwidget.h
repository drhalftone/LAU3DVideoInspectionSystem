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

#ifndef LAU3DVideoRecordingWidget_H
#define LAU3DVideoRecordingWidget_H

#include "lau3dvideowidget.h"

#ifndef EXCLUDE_LAUVELMEXWIDGET
#include "laurfidwidget.h"
#endif

#define NUMBEROFFRAMESBEFOREWECANGRABASCAN 5

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DVideoRecordingWidget : public LAU3DVideoWidget
{
    Q_OBJECT

public:
    explicit LAU3DVideoRecordingWidget(LAUVideoPlaybackColor color = ColorXYZG, LAUVideoPlaybackDevice device = DevicePrimeSense, QWidget *parent = NULL);
    ~LAU3DVideoRecordingWidget();

#ifndef EXCLUDE_LAUVELMEXWIDGET
    void enableVelmexScanMode(bool state);
#endif

    void enableSnapShotMode(bool state)
    {
        snapShotModeFlag = state;
    }

    QMatrix4x4 transform() const
    {
        QMatrix4x4 mat;
        mat.setToIdentity();
        return (mat);
    }

    virtual bool validFrame()
    {
        return (true);
    }

protected:
    void updateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);
    void keyPressEvent(QKeyEvent *event);

public slots:
#ifndef EXCLUDE_LAUVELMEXWIDGET
    void onShowVelmexWidget();
#endif
    void onRecordButtonClicked(bool state = true);
    void onTriggerScanner(float pos, int n, int N);
    void onReceiveFrameBuffer(LAUMemoryObject buffer);
    void onReceiveVideoFrames(LAUMemoryObject frame);
    void onReceiveVideoFrames(QList<LAUMemoryObject> frameList);

private:
    LAUVideoPlayerLabel *videoLabel;
#ifndef EXCLUDE_LAUVELMEXWIDGET
    LAUMultiVelmexWidget *velmexWidget;
#endif
    int velmexIteration = -1;
    int velmexNumberOfIterations = -1;
    int scannerModeTriggerCounter = -1;

    QTime pressStartButtonTime;
    QElapsedTimer timeStamp;
    QVector4D scannerPosition;

    QList<LAUMemoryObject> videoFramesBufferList;
    QList<LAUMemoryObject> recordedVideoFramesBufferList;

    bool snapShotModeFlag, videoRecordingFlag, scannerModeFlag, scannerModeTriggerFlag;

    LAUMemoryObject getPacket();
    void releasePacket(LAUMemoryObject packet);

signals:
    void emitGetFrame();
    void emitVideoFrames(LAUMemoryObject frame);
    void emitVideoFrames(QList<LAUMemoryObject> frameList);
    void emitReleaseFrame(LAUMemoryObject frame);
    void emitTriggerScanner(float pos, int n, int N);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DVideoRecordingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAU3DVideoRecordingDialog(LAUVideoPlaybackColor color = ColorXYZG, LAUVideoPlaybackDevice device = DevicePrimeSense, QWidget *parent = nullptr) : QDialog(parent)
    {
        this->setLayout(new QVBoxLayout());
        this->layout()->setContentsMargins(0, 0, 0, 0);
        widget = new LAU3DVideoRecordingWidget(color, device);
        this->layout()->addWidget(widget);

        connect(widget, SIGNAL(emitVideoFrames(LAUMemoryObject)), widget, SLOT(onReceiveVideoFrames(LAUMemoryObject)));
        connect(widget, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), widget, SLOT(onReceiveVideoFrames(QList<LAUMemoryObject>)));
    }

#ifndef EXCLUDE_LAUVELMEXWIDGET
    void enableVelmexScanMode(bool state)
    {
        widget->enableVelmexScanMode(state);
    }
#endif

    void enableSnapShotMode(bool state)
    {
        widget->enableSnapShotMode(state);
    }

    QSize size()
    {
        return (widget->size());
    }

    int step()
    {
        return (widget->step());
    }

    int depth()
    {
        return (widget->depth());
    }

    int colors()
    {
        return (widget->colors());
    }

private:
    LAU3DVideoRecordingWidget *widget;
};

#endif // LAU3DVideoRecordingWidget_H
