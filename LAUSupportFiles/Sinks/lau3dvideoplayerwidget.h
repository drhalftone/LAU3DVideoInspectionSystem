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

#ifndef LAUKINECTPLAYERWIDGET_H
#define LAUKINECTPLAYERWIDGET_H

#include <QDialog>

#include "lauscan.h"
#include "lau3dscanglwidget.h"
#include "lauvideoplayerlabel.h"

#define MAXRECORDEDFRAMECOUNT 1000

using namespace LAU3DVideoParameters;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DVideoPlayerWidget : public QDialog
{
    Q_OBJECT

public:
    explicit LAU3DVideoPlayerWidget(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent = NULL);
    explicit LAU3DVideoPlayerWidget(QString filenameString, QWidget *parent = NULL);
    ~LAU3DVideoPlayerWidget();

    void setLimits(float xmn, float xmx, float ymn, float ymx, float zmn, float zmx, float xme = NAN, float yme = NAN, float zme = NAN)
    {
        if (replayGLWidget) {
            replayGLWidget->setLimits(xmn, xmx, ymn, ymx, zmn, zmx, xme, yme, zme);
        }
    }

public slots:
    void onInsertPacket(LAUMemoryObject packet);

protected:
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);

private:
    bool valid, saveFlag;
    unsigned int numRows, numCols, numChns;
    LAUVideoPlaybackColor playbackColor;
    LAUVideoPlayerLabel *replayVideoLabel;
    LAU3DScanGLWidget *replayGLWidget;
    QList<LAUMemoryObject> recordedVideoFramesBufferList;

    void initializeInterface();
    bool saveRecordedVideoToDisk();
};

#endif // LAUKINECTPLAYERWIDGET_H
