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

#ifndef LAUABSTRACTFILTERGLWIDGET_H
#define LAUABSTRACTFILTERGLWIDGET_H

#include <QObject>
#include <QThread>

#ifndef HEADLESS
#include <QWidget>
#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#endif

#include <math.h>
#include "lauscan.h"
#include "laulookuptable.h"

class LAUAbstractFilter;

using namespace LAU3DVideoParameters;

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUAbstractGLFilter : public QOpenGLContext, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit LAUAbstractGLFilter(unsigned int depthCols, unsigned int depthRows, unsigned int colorCols, unsigned int colorRows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent = nullptr) : QOpenGLContext(parent), numDepthCols((depthCols == 0) ? colorCols : depthCols), numDepthRows((depthRows == 0) ? colorRows : depthRows), numColorCols((colorCols == 0) ? depthCols : colorCols), numColorRows((colorRows == 0) ? depthRows : colorRows), maxIntensityValue(65535), snrThreshold(0), mtnThreshold(1000), channel(0), numChannels(1), lastEmittedChannel(-1), registerDepthToRGBFlag(false), playbackColor(color), playbackDevice(device), surface(nullptr), surfaceIsValid(false), frameBufferObject(nullptr), boundingBoxBufferObject(nullptr), registerBufferObject(nullptr), stereoPhaseBufferObject(nullptr), epipolarRectifiedPhaseBufferObject(nullptr), textureDepth(nullptr), textureColor(nullptr), textureMapping(nullptr), textureAngles(nullptr), textureMin(nullptr), textureMax(nullptr), textureLookUpTable(nullptr), texturePhaseUnwrap(nullptr), enableBoundingBoxFlag(false), xBoundingBoxMin(-10000.0), xBoundingBoxMax(10000.0), yBoundingBoxMin(-10000.0), yBoundingBoxMax(10000.0), zBoundingBoxMin(-10000.0), zBoundingBoxMax(10000.0){ ; }
    explicit LAUAbstractGLFilter(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent) : QOpenGLContext(parent), numDepthCols(cols), numDepthRows(rows), numColorCols(cols), numColorRows(rows), snrThreshold(0), mtnThreshold(1000), channel(0), numChannels(1), lastEmittedChannel(-1), registerDepthToRGBFlag(false), maxIntensityValue(65535), playbackColor(color), playbackDevice(device), surface(nullptr), surfaceIsValid(false), frameBufferObject(nullptr), boundingBoxBufferObject(nullptr), registerBufferObject(nullptr), stereoPhaseBufferObject(nullptr), epipolarRectifiedPhaseBufferObject(nullptr), textureDepth(nullptr), textureColor(nullptr), textureMapping(nullptr), textureAngles(nullptr), textureMin(nullptr), textureMax(nullptr), textureLookUpTable(nullptr), texturePhaseUnwrap(nullptr), enableBoundingBoxFlag(false), xBoundingBoxMin(-10000.0), xBoundingBoxMax(10000.0), yBoundingBoxMin(-10000.0), yBoundingBoxMax(10000.0), zBoundingBoxMin(-10000.0), zBoundingBoxMax(10000.0){ ; }

    ~LAUAbstractGLFilter()
    {
        if (wasInitialized() && makeCurrent(surface)) {
            if (textureMin) {
                delete textureMin;
            }
            if (textureMax) {
                delete textureMax;
            }
            if (textureDepth) {
                delete textureDepth;
            }
            if (textureColor) {
                delete textureColor;
            }
            if (textureMapping) {
                delete textureMapping;
            }
            if (textureAngles) {
                delete textureAngles;
            }
            if (textureLookUpTable) {
                delete textureLookUpTable;
            }
            if (texturePhaseUnwrap) {
                delete texturePhaseUnwrap;
            }
            if (boundingBoxBufferObject){
                delete boundingBoxBufferObject;
            }
            if (frameBufferObject) {
                delete frameBufferObject;
            }
            if (registerBufferObject) {
                delete registerBufferObject;
            }
            if (stereoPhaseBufferObject) {
                delete stereoPhaseBufferObject;
            }
            if (epipolarRectifiedPhaseBufferObject) {
                delete epipolarRectifiedPhaseBufferObject;
            }
            surfaceIsValid = false;
        }
    }

    int scale()
    {
        return (LAUMemoryObject::numberOfColors(playbackColor) / 5 + 1);
    }

    bool isValid() const
    {
        return (wasInitialized());
    }

    bool isARGMode() const
    {
        return (false);
    }

    bool wasInitialized() const
    {
        return (surfaceIsValid && vertexArrayObject.isCreated());
    }

    int width() const
    {
        return (numDepthCols);
    }

    int height() const
    {
        return (numDepthRows);
    }

    inline int camera() const
    {
        return(channel);
    }

    void flush()
    {
        glFlush();
    }

    void setRegisterDepthToRGB(bool flag)
    {
        registerDepthToRGBFlag = flag;
    }

    void setFieldsOfView(float hFov, float vFov)
    {
        horizontalFieldOfView = hFov;
        verticalFieldOfView = vFov;
    }

    void setSurface(QSurface *srfc)
    {
        surface = srfc;
    }

    void setMaximumIntensityValue(unsigned short val)
    {
        maxIntensityValue = val;
    }

    void setCamera(unsigned int val)
    {
        channel = val;
    }

    void setCameraCount(unsigned int val)
    {
        numChannels = val;
    }

    void setJetrVector(int chn, QVector<double> vector)
    {
        if (chn < 0){
            ;
        } else if (chn < jetrVectors.count()){
            jetrVectors.replace(chn, vector);
        } else {
            while (jetrVectors.count() < chn){
                jetrVectors << QVector<double>(36, NAN);
            }
            jetrVectors << vector;
        }
    }

    QVector<double> jetr(int chn = 0)
    {
        if (chn < 0){
            return(QVector<double>(36, NAN));
        } else if (chn < jetrVectors.count()){
            return(jetrVectors[chn]);
        } else {
            return(QVector<double>(36, NAN));
        }
    }

    void initialize();
    void saveTextureToDisk(QOpenGLFramebufferObject *fbo, QString filename);
    void saveTextureToDisk(QOpenGLTexture *texture, QString filename);

    void enableBoundingBox(bool state)
    {
        enableBoundingBoxFlag = state;
    }

    virtual void setLookUpTable(LAULookUpTable lut = LAULookUpTable(QString()));
    virtual void updateBuffer(LAUScan scan);
    virtual void updateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

    QOpenGLFramebufferObject *fbo() const
    {
        return (frameBufferObject);
    }

public slots:
    void onUpdateBuffer(LAUScan scan);
    void onUpdateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

    void onSetCamera(int val)
    {
        if (val != channel) {
            channel = val;
        }
    }

    void onSetMTNThreshold(int val)
    {
        if (val != mtnThreshold) {
            mtnThreshold = val;
        }
    }

    void onSetSNRThreshold(int val)
    {
        if (val != snrThreshold) {
            snrThreshold = val;
        }
    }

    void onSetBoundingBoxTransform(QMatrix4x4 mat)
    {
        boundingBoxProjectorMatrix = mat;
    }

    void onSetBoundingBoxXMin(double val)
    {
        xBoundingBoxMin = val;
    }

    void onSetBoundingBoxXMax(double val)
    {
        xBoundingBoxMax = val;
    }

    void onSetBoundingBoxYMin(double val)
    {
        yBoundingBoxMin = val;
    }

    void onSetBoundingBoxYMax(double val)
    {
        yBoundingBoxMax = val;
    }

    void onSetBoundingBoxZMin(double val)
    {
        zBoundingBoxMin = val;
    }

    void onSetBoundingBoxZMax(double val)
    {
        zBoundingBoxMax = val;
    }

    virtual void onStart() { ; }
    virtual void onFinish() { ; }

protected:
    unsigned int numDepthCols, numDepthRows;
    unsigned int numColorCols, numColorRows;
    float horizontalFieldOfView, verticalFieldOfView;
    unsigned short maxIntensityValue;
    int snrThreshold, mtnThreshold;
    int channel, numChannels;
    int lastEmittedChannel;  // Track last emitted channel to avoid duplicate emissions
    bool surfaceIsValid;
    bool registerDepthToRGBFlag;

    LAULookUpTable lookUpTable;
    LAUVideoPlaybackColor playbackColor;
    LAUVideoPlaybackDevice playbackDevice;

    QList<QVector<double>> jetrVectors;

    QSurface *surface;
    QOpenGLShaderProgram program;
    QOpenGLShaderProgram stereoProgramA, stereoProgramB, stereoProgramC, stereoProgramD;
    QOpenGLShaderProgram boundingBoxProgram;
    QOpenGLBuffer pixlVertexBuffer, pixlIndexBuffer;
    QOpenGLBuffer quadVertexBuffer, quadIndexBuffer;
    QOpenGLVertexArrayObject vertexArrayObject;
    QOpenGLFramebufferObject *frameBufferObject, *registerBufferObject, *stereoPhaseBufferObject, *epipolarRectifiedPhaseBufferObject, *boundingBoxBufferObject;
    QOpenGLTexture *textureDepth, *textureColor, *textureMapping, *textureAngles, *texturePhaseCorrection;
    QOpenGLTexture *textureMin, *textureMax, *textureLookUpTable, *texturePhaseUnwrap;

    bool enableBoundingBoxFlag;
    QMatrix4x4 boundingBoxProjectorMatrix;
    double xBoundingBoxMin, xBoundingBoxMax, yBoundingBoxMin, yBoundingBoxMax, zBoundingBoxMin, zBoundingBoxMax;

    virtual void initializeGL() { ; }

private:
    void updateMultiCameraBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

signals:
    void emitBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);
    void emitBuffer(LAUScan scan);
    void emitChannelIndex(int index);
};
#endif
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUAbstractFilter : public QObject
{
    Q_OBJECT

public:
    explicit LAUAbstractFilter(int cols, int rows, QObject *parent = nullptr) : QObject(parent), numCols(cols), numRows(rows) { ; }
    ~LAUAbstractFilter()
    {
        qDebug() << QString("LAUAbstractFilter::~LAUAbstractFilter()");
    }

    int width() const
    {
        return (numCols);
    }

    int height() const
    {
        return (numRows);
    }

    void setCamera(unsigned int val)
    {
        channel = val;
    }

public slots:
    void onUpdateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject())
    {
        updateBuffer(depth, color, mapping);
        emit emitBuffer(depth, color, mapping);
    }

    void onUpdateBuffer(LAUScan scan)
    {
        updateBuffer(scan);
        emit emitBuffer(scan);
    }

    virtual void onStart() { ; }
    virtual void onFinish() { ; }

protected:
    int numCols, numRows;
    int channel, numChannels;

    virtual void updateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject())
    {
        Q_UNUSED(depth);
        Q_UNUSED(color);
        Q_UNUSED(mapping);
    }

    virtual void updateBuffer(LAUScan scan = LAUScan())
    {
        Q_UNUSED(scan);
    }

signals:
    void emitBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);
    void emitBuffer(LAUScan scan);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUAbstractFilterController : public QObject
{
    Q_OBJECT

public:
#ifndef HEADLESS
    explicit LAUAbstractFilterController(LAUAbstractGLFilter *contxt, QSurface *srfc = nullptr, QObject *parent = nullptr);
    explicit LAUAbstractFilterController(LAUAbstractGLFilter *contxt, QOpenGLWidget *wdgt, QObject *parent = nullptr);
#endif
    explicit LAUAbstractFilterController(LAUAbstractFilter *fltr, QObject *parent = nullptr);
    ~LAUAbstractFilterController();

#ifndef HEADLESS
    LAUAbstractGLFilter *glFilter() const
    {
        return (localContext);
    }
#endif
    LAUAbstractFilter *filter() const
    {
        return (localFilter);
    }

protected:
#ifndef HEADLESS
    LAUAbstractGLFilter *localContext;
#endif
    LAUAbstractFilter *localFilter;
    bool localSurface;
    QSurface *surface;
    QThread *thread;
};

#endif // LAUABSTRACTFILTERGLWIDGET_H
