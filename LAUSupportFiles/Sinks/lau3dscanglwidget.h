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

#ifndef LAU3DSCANGLWIDGET_H
#define LAU3DSCANGLWIDGET_H

#include <QWindow>

#include "laumemoryobject.h"
#include "lauglwidget.h"
#include "lauscan.h"

using namespace LAU3DVideoParameters;

#define LAU3DSCANGLWIDGETDELTA 0.025f

class LAU3DScanGLWidget : public LAUAbstractGLWidget
{
    Q_OBJECT

public:
    enum GrabMouseBufferMode { MouseModeXYZ, MouseModeRGB, MouseModeRowColumn };

    LAU3DScanGLWidget(LAUScan scan, QWidget *parent = nullptr);
    LAU3DScanGLWidget(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent = nullptr);
    ~LAU3DScanGLWidget();

    bool isValid() const
    {
        return ((numCols * numRows) > 0);
    }

    bool flipScan() const
    {
        return (flipScanFlag);
    }

    bool sandboxEnabled() const
    {
        return(sandboxTextureFlag);
    }

    void enableSandboxTexture(bool state)
    {
        sandboxTextureFlag = state;
    }

    void enableFlipScan(bool state)
    {
        flipScanFlag = state;
    }

    QSize size() const
    {
        return (QSize(numCols, numRows));
    }

    bool textureEnabled() const
    {
        return (textureEnableFlag);
    }

    unsigned int numberDrawIndices() const
    {
        return (numInds);
    }

    LAUVideoPlaybackColor color() const
    {
        return (playbackColor);
    }

    float delta() const
    {
        return (qtDelta);
    }

    LAUMemoryObject grabMouseBuffer(GrabMouseBufferMode mode = MouseModeXYZ);

    QMatrix4x4 symmetryTransform()
    {
        return (symProjection);
    }

    void setSymmetryTransform(QMatrix4x4 mat)
    {
        symProjection = mat;
    }

    QMatrix4x4 scanTransform()
    {
        return (scnProjection);
    }

    void setScanTransform(QMatrix4x4 mat)
    {
        scnProjection = mat;
    }

    void enableSymmetry(bool state)
    {
        symEnableFlag = state;
    }

    void setSandboxProjectionMatrix(QMatrix4x4 mat)
    {
        prjProjection = mat;
    }

    void setRangeLimits(float zmn, float zmx, float hFov, float vFov);
    void copyScan(float *buffer);

public slots:
    void onSetTexture(QOpenGLFramebufferObject *fbo);
    void onSetTexture(QOpenGLTexture *txt);

    void onSetDelta(double val)
    {
        qtDelta = static_cast<float>(val);
        update();
    }

    void onSetDelta(float val)
    {
        qtDelta = val;
        update();
    }

    void onUpdateSymmetryTransform(QMatrix4x4 mat)
    {
        symProjection = mat;
        update();
    }

    void onUpdateScanTransform(QMatrix4x4 mat)
    {
        scnProjection = mat;
        update();
    }

    void onEnableTexture(bool state)
    {
        textureEnableFlag = state;
        update();
    }

    void onFlipScan(bool state)
    {
        flipScanFlag = state;
        update();
    }

    void onUpdateBuffer(LAUMemoryObject buffer)
    {
        updateBuffer(buffer);
        emit emitBuffer(buffer);
    }

    void onUpdateBuffer(float *buffer)
    {
        updateBuffer(buffer);
        emit emitBuffer(buffer);
    }

    void onUpdateBuffer(LAUScan scan)
    {
        updateBuffer(scan);
        emit emitBuffer(scan);
    }

protected:
    void showEvent(QShowEvent *event)
    {
        // SEE IF WE SHOULD BE FULL SCREEN FOR SMALL DISPLAYS
        QRect rect = qApp->screens().at(0)->availableGeometry();

#ifdef ENABLETOUCHPANEL
        this->setWindowFlags(Qt::FramelessWindowHint);
        this->setGeometry(rect);
        this->setFixedSize(rect.width(), rect.height());
#else
        if (rect.width() < MINIMUMSCREENWIDTHFORFULLSCREEN || rect.height() < MINIMUMSCREENHEIGTFORFULLSCREEN) {
            this->window()->showMaximized();
        }
#endif
        // CALL THE UNDERLYING WIDGET'S SHOW EVENT HANDLER
        QWidget::showEvent(event);
    }

    void initializeGL();
    void paintGL();

    virtual void clearGL()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    inline unsigned int scale()
    {
        return (static_cast<unsigned int>(LAUMemoryObject::numberOfColors(playbackColor) / 5 + 1));
    }

    bool bindTexture();

private:
    unsigned int numCols, numRows, numInds;
    unsigned int boundTextureWidth, boundTextureHeight;
    LAUVideoPlaybackColor playbackColor;
    bool textureEnableFlag, localTextureFlag, sandboxTextureFlag;
    bool symEnableFlag, flipScanFlag;
    LAUScan localScan;
    float qtDelta;

    QMatrix4x4 scnProjection;
    QMatrix4x4 symProjection;
    QMatrix4x4 prjProjection;
    QOpenGLBuffer pixelVertexBuffer;
    QOpenGLBuffer pixelIndexBuffer;
    QOpenGLShaderProgram program;
    QOpenGLFramebufferObject *frameBufferObject;
    QOpenGLTexture *texture;

    void createTexture();

    void updateBuffer(LAUMemoryObject buffer);
    void updateBuffer(float *buffer);
    void updateBuffer(LAUScan scan);

signals:
    void emitBuffer(LAUMemoryObject buffer);
    void emitBuffer(float *buffer);
    void emitBuffer(LAUScan scan);
};

#endif // LAU3DSCANGLWIDGET_H
