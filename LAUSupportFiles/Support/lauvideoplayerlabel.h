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

#ifndef LAUVIDEOPLAYERLABEL_H
#define LAUVIDEOPLAYERLABEL_H

#include <QTime>
#include <QList>
#include <QLabel>
#include <QDebug>
#include <QPixmap>
#include <QWidget>
#include <QPainter>
#include <QMatrix4x4>
#include <QPaintDevice>
#include <QResizeEvent>
#include <QBackingStore>
#include <QCoreApplication>

#include "laumemoryobject.h"

class LAUVideoPlayerLabel : public QLabel
{
    Q_OBJECT

public:
    enum VideoRecorderState { StateVideoPlayer, StateVideoRecorder };
    enum ButtoDownState { StateNoButtons, StateLeftLeftButton, StateLeftButton, StatePlayButton, StateRightButton, StateRightRightButton, StateSliderButton };

    explicit LAUVideoPlayerLabel(VideoRecorderState state, QWidget *parent = NULL);
    ~LAUVideoPlayerLabel();

    inline VideoRecorderState recorderState()
    {
        return (videoRecorderState);
    }

    inline void reset()
    {
        packetList.clear();
    }

    void togglePlayback();

public slots:
    void onPlayButtonClicked(bool state);
    void onLeftLeftButtonClicked();
    void onLeftButtonClicked();
    void onRightButtonClicked();
    void onRightRightButtonClicked();
    void onRemovePacket(LAUMemoryObject packet);
    void onInsertPacket(LAUMemoryObject packet);

    void onUpdateSliderPosition(double lambda);
    void onUpdateTimeStamp(unsigned int mSec);

protected:
    VideoRecorderState videoRecorderState;
    ButtoDownState buttonDownState;
    bool playState;

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void timerEvent(QTimerEvent *);
    void showEvent(QShowEvent *event);

private:
    QList<LAUMemoryObject> packetList;
    int timeStamp, minTimeStamp, maxTimeStamp, currentTimeStamp;
    int newPacketIndex, previousPacketIndex, timerID;
    double sliderPosition;
    QList<QPixmap> pixmapList;

signals:
    void updateSliderPosition(double);
    void playButtonClicked(bool);
    void leftLeftButtonClicked();
    void leftButtonClicked();
    void rightButtonClicked();
    void rightRightButtonClicked();
    void emitPacket(LAUMemoryObject packet);
};

#endif // LAUVIDEOPLAYERLABEL_H
