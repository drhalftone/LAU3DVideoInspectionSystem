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

#ifndef LAUBACKGROUNDGLFILTER_H
#define LAUBACKGROUNDGLFILTER_H

#ifndef HEADLESS
#include "QPushButton"
#include "lau3dvideowidget.h"
#endif
#include "lauabstractfilter.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUBackgroundGLFilter : public LAUAbstractGLFilter
{
    Q_OBJECT

public:
    explicit LAUBackgroundGLFilter(unsigned int depthCols, unsigned int depthRows, unsigned int colorCols, unsigned int colorRows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent = NULL) : LAUAbstractGLFilter(depthCols, depthRows, colorCols, colorRows, color, device, parent), maxDistance(1e6), frameCounter(0), maxFilterFrameCount(20), maxPixelTextureZ(NULL), maxPixelTextureX(NULL)
    {
        channel = -1;
        for (int n = 0; n < 4; n++) {
            maxPixelFBO[n] = nullptr;
        }
    }

    explicit LAUBackgroundGLFilter(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent = NULL) : LAUAbstractGLFilter(cols, rows, color, device, parent), maxDistance(1e6), frameCounter(0), maxFilterFrameCount(20), maxPixelTextureZ(NULL), maxPixelTextureX(NULL)
    {
        channel = -1;
        for (int n = 0; n < 4; n++) {
            maxPixelFBO[n] = nullptr;
        }
    }

    ~LAUBackgroundGLFilter();

    void setMaxDistance(unsigned short val)
    {
        maxDistance = val;
    }

public slots:
    void onReset()
    {
        frameCounter = 0;
    }
    void onPreserveBackgroundToSettings();
    void onEmitBackground();
    void onSetMaxPixelFilterCount(int val)
    {
        maxFilterFrameCount = val;
    }

protected:
    void initializeGL();
    void updateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

signals:
    void emitBackground(LAUMemoryObject background);

private:
    int frameCounter;
    int maxFilterFrameCount;
    int minFilterFrameCount;
    float maxDistance;
    QOpenGLTexture *maxPixelTextureZ;          // THIS TEXTURE HOLDS THE ALL ZERO TEXTURE
    QOpenGLTexture *maxPixelTextureX;          // THIS TEXTURE HOLDS THE MAX VALUE TEXTURE
    QOpenGLShaderProgram maxPixelProgram;      // THIS SHADER PROGRAM HOLDS THE PROGRAM FOR KEEPING THE MAXIMUM PIXEL VALUE
    QOpenGLShaderProgram minPixelProgram;      // THIS SHADER PROGRAM HOLDS THE PROGRAM FOR KEEPING THE MAXIMUM PIXEL VALUE
    QOpenGLFramebufferObject *maxPixelFBO[4];  // THIS FRAME BUFFER OBJECT HOLDS THE INTERMEDIATE MAXIMUM PIXEL BUFFER

    static void inPaint(LAUMemoryObject object, unsigned short fill);
};

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUBackgroundWidget : public LAU3DVideoWidget
{
    Q_OBJECT

public:
    explicit LAUBackgroundWidget(LAUVideoPlaybackColor color = ColorXYZG, LAUVideoPlaybackDevice device = DevicePrimeSense, QWidget *parent = NULL);
    explicit LAUBackgroundWidget(QList<LAUVideoPlaybackDevice> devices, LAUVideoPlaybackColor color = ColorXYZG, QWidget *parent = NULL);
    ~LAUBackgroundWidget();

    QMatrix4x4 transform() const
    {
        QMatrix4x4 mat;
        mat.setToIdentity();
        return (mat);
    }

public slots:
    void onRecordButtonClicked();
    void onReceiveBackground(LAUMemoryObject background);

signals:
    void emitVideoFrames(LAUMemoryObject frame);

private:
    QPushButton *recordButton;
    QPushButton *resetButton;
    QList<LAUBackgroundGLFilter*> backgroundFilters;
    QList<LAUMemoryObject> collectedBackgrounds;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUBackgroundDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUBackgroundDialog(LAUVideoPlaybackColor color = ColorXYZG, LAUVideoPlaybackDevice device = DevicePrimeSense, QWidget *parent = NULL) : QDialog(parent)
    {
        this->setWindowTitle(QString("Background Filter"));
        this->setLayout(new QVBoxLayout());
#ifdef Q_OS_WIN
        this->layout()->setContentsMargins(6, 6, 6, 6);
#else
        this->layout()->setContentsMargins(6, 6, 6, 6);
#endif
        widget = new LAUBackgroundWidget(color, device);
        this->layout()->addWidget(widget);
    }

    explicit LAUBackgroundDialog(QList<LAUVideoPlaybackDevice> devices, LAUVideoPlaybackColor color = ColorXYZG, QWidget *parent = NULL) : QDialog(parent)
    {
        this->setWindowTitle(QString("Background Filter - Multi-Camera"));
        this->setLayout(new QVBoxLayout());
#ifdef Q_OS_WIN
        this->layout()->setContentsMargins(6, 6, 6, 6);
#else
        this->layout()->setContentsMargins(6, 6, 6, 6);
#endif
        widget = new LAUBackgroundWidget(devices, color);
        this->layout()->addWidget(widget);
    }

    explicit LAUBackgroundDialog(LAUVideoPlaybackColor color, QList<LAU3DCamera*> cameraList, QWidget *parent = NULL);
    ~LAUBackgroundDialog();

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
    LAU3DVideoWidget *widget;
    QList<LAU3DCamera*> cameras;
    int sensorCount;
};
#endif
#endif // LAUBACKGROUNDGLFILTER_H
