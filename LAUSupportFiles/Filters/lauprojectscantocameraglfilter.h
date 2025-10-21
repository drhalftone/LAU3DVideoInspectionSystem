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

#ifndef LAUPROJECTSCANTOCAMERAGLFILTER_H
#define LAUPROJECTSCANTOCAMERAGLFILTER_H

#include <QWidget>
#include <QSpinBox>
#include <QSettings>
#include <QFormLayout>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>

#include "lauscan.h"
#include "laulookuptable.h"
#include "lau3dfiducialglwidget.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUProjectScanToCameraGLFilter : public QOpenGLContext, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit LAUProjectScanToCameraGLFilter(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent = NULL);
    explicit LAUProjectScanToCameraGLFilter(LAUScan scan, unsigned int rds = 2, QWidget *parent = nullptr);
    ~LAUProjectScanToCameraGLFilter();

    bool isValid() const
    {
        return (wasInitialized());
    }

    bool wasInitialized() const
    {
        return (vertexArrayObject.isCreated());
    }

    int width() const
    {
        return (numCols);
    }

    int height() const
    {
        return (numRows);
    }

    void setLookUpTable(LAULookUpTable tbl)
    {
        table = tbl;
    }

    LAUVideoPlaybackColor color() const
    {
        return (playbackColor);
    }

    void initialize();
    void grabScan(float *buffer);
    void setSurface(QSurface *srfc)
    {
        surface = srfc;
    }

public slots:
    void onUpdateScan(LAUScan scan);

private:
    LAULookUpTable table;
    unsigned int numCols, numRows;

    LAUVideoPlaybackColor playbackColor;

    QSurface *surface;
    QOpenGLShaderProgram program, pixelMappingProgram;
    QOpenGLBuffer pixlVertexBuffer, pixlIndexBuffer;
    QOpenGLBuffer vertexBuffer, indexBuffer;
    QOpenGLVertexArrayObject vertexArrayObject;
    QOpenGLFramebufferObject *pixelMappingBufferObject;
    QOpenGLFramebufferObject *frameBufferObject;
    QOpenGLTexture *textureScan;

signals:
    void emitScan(LAUScan scan);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUProjectScanToCameraDialog : public QDialog
{
    Q_OBJECT

public:
    LAUProjectScanToCameraDialog(LAUScan scn, QWidget *parent = nullptr);
    ~LAUProjectScanToCameraDialog();

    bool isValid() const
    {
        return (validFlag);
    }

    LAUScan smooth() const
    {
        return (result);
    }

    void setLookUpTable(LAULookUpTable table);

public slots:
    void onPreview();

protected:
    void accept()
    {
        onPreview();
        QDialog::accept();
    }

private:
    bool validFlag;
    LAUScan scan, result;
    LAUProjectScanToCameraGLFilter *glFilter;
    LAU3DFiducialGLWidget *scanWidget;
};

#endif // LAUPROJECTSCANTOCAMERAGLFILTER_H
