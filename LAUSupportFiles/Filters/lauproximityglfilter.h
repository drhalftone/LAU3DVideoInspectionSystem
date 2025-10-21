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

#ifndef LAUPROXIMITYGLFILTER_H
#define LAUPROXIMITYGLFILTER_H

#include <QWidget>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>

#include "lauscan.h"

#define MAXNUMBERITERATIONS 5

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUProximityGLFilter : public QOpenGLContext, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit LAUProximityGLFilter(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent = NULL);
    explicit LAUProximityGLFilter(LAUScan scan, unsigned int itrs = MAXNUMBERITERATIONS, QWidget *parent = NULL);
    ~LAUProximityGLFilter();

    bool isValid() const { return(wasInitialized()); }
    bool wasInitialized() const { return(vertexArrayObject.isCreated()); }

    int width() const { return(numCols); }
    int height() const { return(numRows); }
    unsigned int iterations() const { return(numItrs); }
    LAUVideoPlaybackColor color() const { return(ColorXYZW); }

    void initialize();
    void grabScan(float *buffer);
    void setIterations(unsigned int itrs) { numItrs = qMin(itrs, (unsigned int)MAXNUMBERITERATIONS); }

public slots:
    void onUpdateToScan(LAUScan scan);
    void onUpdateFmScan(LAUScan scan);
    void onUpdateScans(LAUScan fmScan, LAUScan toScan) { onUpdateToScan(toScan); onUpdateFmScan(fmScan); emit emitScans(fmScan, toScan); }

private:
    unsigned int numCols, numRows, numItrs, numInds;
    unsigned int fboHeight, fboWidth;

    LAUVideoPlaybackColor playbackColor;

    QSurface *surface;
    QMatrix4x4 transform;
    QOpenGLShaderProgram programA, programB, programC, programD;
    QOpenGLBuffer vertexBufferA, indexBufferA;
    QOpenGLBuffer vertexBufferB, indexBufferB;
    QOpenGLVertexArrayObject vertexArrayObject;
    QOpenGLFramebufferObject* frameBufferObjectsA[MAXNUMBERITERATIONS];
    QOpenGLFramebufferObject* frameBufferObjectsB[MAXNUMBERITERATIONS];
    QOpenGLFramebufferObject* frameBufferObjectsC;
    QOpenGLTexture *textureScan;

    void initializeShaders();
    void initializeTextures();
    void initializeVertices();

signals:
    void emitFmScan(LAUScan);
    void emitToScan(LAUScan);
    void emitScans(LAUScan, LAUScan);
};

#endif // LAUPROXIMITYGLFILTER_H
