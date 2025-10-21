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

#ifndef LAUGREENSCREENGLFILTER_H
#define LAUGREENSCREENGLFILTER_H

#include "lauabstractfilter.h"
#ifndef EXCLUDE_LAU3DVIDEOWIDGET
#include "lau3dvideorecordingwidget.h"
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUGreenScreenGLFilter : public LAUAbstractGLFilter
{
    Q_OBJECT

public:
    explicit LAUGreenScreenGLFilter(unsigned int depthCols, unsigned int depthRows, unsigned int colorCols, unsigned int colorRows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent = NULL) : LAUAbstractGLFilter(depthCols, depthRows, colorCols, colorRows, color, device, parent), threshold(0.05f), map(LAUMemoryObject(depthCols, depthRows, 1, 1, 1)), enablePixelCountFlag(false), greenScreenTexture(nullptr), grnScrFBO(nullptr), triggerThreshold(-1)
    {
        // INITIALIZE THE CHANNEL TO -1
        channel = -1;

        // ALLOCATE SPACE FOR THE MAGIC WAND INDICES BUFFER
        indices = (unsigned int *)_mm_malloc(map.width() * map.height() * sizeof(unsigned int), 16);
    }

    explicit LAUGreenScreenGLFilter(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent = NULL) : LAUAbstractGLFilter(cols, rows, color, device, parent), threshold(0.05f), map(LAUMemoryObject(cols, rows, 1, 1, 1)), enablePixelCountFlag(false), greenScreenTexture(nullptr), grnScrFBO(nullptr), triggerThreshold(-1)
    {
        // INITIALIZE THE CHANNEL TO -1
        channel = -1;

        // ALLOCATE SPACE FOR THE MAGIC WAND INDICES BUFFER
        indices = (unsigned int *)_mm_malloc(map.width() * map.height() * sizeof(unsigned int), 16);
    }

    ~LAUGreenScreenGLFilter();

    LAUMemoryObject background();

    void setTriggerThreshold(int val)
    {
        triggerThreshold = val;
    }

    void enablePixelCount(bool state)
    {
        enablePixelCountFlag = state;
    }

    void disablePixelCount(bool state)
    {
        enablePixelCountFlag = !state;
    }

    void setSensitivity(float val)
    {
        threshold = val;
    }

    float sensitivity() const
    {
        return (threshold);
    }

public slots:
    void onSetBackgroundTexture(LAUMemoryObject buffer);

protected:
    void initializeGL();
    void updateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

private:
    float threshold;
    int triggerThreshold;
    LAUMemoryObject map;
    LAUMemoryObject localDepth;
    LAUMemoryObject greenScreenObject;
    bool enablePixelCountFlag;
    QOpenGLTexture *greenScreenTexture;     // THIS TEXTURE HOLDS THE MAX VALUE TEXTURE
    QOpenGLShaderProgram grnScrenProgram;   // THIS SHADER PROGRAM APPLIES A GREEN SCREEN TO THE INCOMING TEXTURES
    QOpenGLFramebufferObject *grnScrFBO;    // THIS FRAME BUFFER OBJECT HOLDS THE INTERMEDIATE MAXIMUM PIXEL BUFFER

    unsigned int *indices;
    void magicwand();
};

#ifndef HEADLESS
#ifndef EXCLUDE_LAU3DVIDEOWIDGET
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUGreenScreenWidget : public LAU3DVideoRecordingWidget
{
    Q_OBJECT

public:
    explicit LAUGreenScreenWidget(LAUVideoPlaybackColor color = ColorXYZW, LAUVideoPlaybackDevice device = DevicePrimeSense, QWidget *parent = NULL);
    ~LAUGreenScreenWidget();

public slots:

private:

signals:

};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUGreenScreenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUGreenScreenDialog(LAUVideoPlaybackColor color = ColorXYZRGB, LAUVideoPlaybackDevice device = DevicePrimeSense, QWidget *parent = NULL) : QDialog(parent)
    {
        this->setLayout(new QVBoxLayout());
        this->layout()->setContentsMargins(0, 0, 0, 0);
        widget = new LAUGreenScreenWidget(color, device);
        this->layout()->addWidget(widget);

        // CONNECT THE WIDGET TO ITSELF FOR REPLAYING VIDEO
        connect(widget, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), widget, SLOT(onReceiveVideoFrames(QList<LAUMemoryObject>)));
        connect(widget, SIGNAL(emitVideoFrames(LAUMemoryObject)), widget, SLOT(onReceiveVideoFrames(LAUMemoryObject)));
    }

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
    LAUGreenScreenWidget *widget;
};
#endif // EXCLUDE_LAU3DVIDEOWIDGET
#endif // HEADLESS
#endif // LAUGREENSCREENGLFILTER_H
