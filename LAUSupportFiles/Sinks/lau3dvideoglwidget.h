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

#ifndef LAUVIDEOGLWIDGET_H
#define LAUVIDEOGLWIDGET_H

#include "lauabstractfilter.h"
#include "lau3dfiducialglwidget.h"

using namespace LAU3DVideoParameters;

class LAU3DVideoGLWidget : public LAU3DFiducialGLWidget
{
    Q_OBJECT

public:
    LAU3DVideoGLWidget(unsigned int depthCols, unsigned int depthRows, unsigned int colorCols, unsigned int colorRows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device = DevicePrimeSense, QWidget *parent = NULL);
    LAU3DVideoGLWidget(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device = DevicePrimeSense, QWidget *parent = NULL);
    ~LAU3DVideoGLWidget();

    LAUVideoPlaybackDevice device() const
    {
        return (playbackDevice);
    }

    void setLookUpTable(LAULookUpTable lut = LAULookUpTable(QString()));                 // SET THE SCANNER LOOK UP TABLE THAT INCLUDES XYZ LIMITS
    void setMaximumIntensityValue(unsigned short val)
    {
        if (glFilter) {
            glFilter->setMaximumIntensityValue(val);
            update();
        }
    }

    void setJetrVector(int chn, QVector<double> vector)
    {
        if (glFilter) {
            glFilter->setJetrVector(chn, vector);
        }
    }

    QVector<double> jetr(int chn = 0)
    {
        if (glFilter) {
            return(glFilter->jetr(chn));
        }
        return QVector<double>();
    }

    LAULookUpTable* lutHandle()
    {
        return(&lookUpTable);
    }

    LAUAbstractGLFilter* filter()
    {
        return(glFilter);
    }

    void enableBoundingBox(bool state)
    {
        if (glFilter){
            glFilter->enableBoundingBox(state);
        }
    }

public slots:
    void onExportLookUpTable()
    {
        if (lookUpTable.isValid()){
            lookUpTable.save(QString());
        }
    }

    void onSetMTNThreshold(int val)
    {
        if (glFilter) {
            glFilter->onSetMTNThreshold(val);
            update();
        }
    }

    void onSetSNRThreshold(int val)
    {
        if (glFilter) {
            glFilter->onSetSNRThreshold(val);
            update();
        }
    }

    void onSetCamera(unsigned int val)
    {
        if (glFilter) {
            glFilter->onSetCamera(val);
            update();
        }
    }

    inline int camera() const
    {
        if (glFilter){
            return(glFilter->camera());
        }
        return(-1);
    }

    void incrementChannel()
    {
        if (glFilter) {
            int currentChannel = glFilter->camera();
            glFilter->onSetCamera(currentChannel + 1);
            update();
        }
    }

    void onSetBoundingBoxTransform(QMatrix4x4 mat)
    {
        if (glFilter) {
            glFilter->onSetBoundingBoxTransform(mat);
            update();
        }
    }

    void onSetBoundingBoxXMin(double val)
    {
        if (glFilter) {
            glFilter->onSetBoundingBoxXMin(val);
            update();
        }
    }

    void onSetBoundingBoxXMax(double val)
    {
        if (glFilter) {
            glFilter->onSetBoundingBoxXMax(val);
            update();
        }
    }

    void onSetBoundingBoxYMin(double val)
    {
        if (glFilter) {
            glFilter->onSetBoundingBoxYMin(val);
            update();
        }
    }

    void onSetBoundingBoxYMax(double val)
    {
        if (glFilter) {
            glFilter->onSetBoundingBoxYMax(val);
            update();
        }
    }

    void onSetBoundingBoxZMin(double val)
    {
        if (glFilter) {
            glFilter->onSetBoundingBoxZMin(val);
            update();
        }
    }

    void onSetBoundingBoxZMax(double val)
    {
        if (glFilter) {
            glFilter->onSetBoundingBoxZMax(val);
            update();
        }
    }

    void onUpdateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

protected:
    void keyPressEvent(QKeyEvent *event)
    {
        if (event->key() == Qt::Key_B) {
            qDebug() << "LAU3DVideoGLWidget::Key_B";
        } else if (event->key() == Qt::Key_PageDown) {
            qDebug() << "LAU3DVideoGLWidget::Key_PageDown";
        } else if (event->key() == Qt::Key_PageUp) {
            qDebug() << "LAU3DVideoGLWidget::Key_PageUp";
        } else if (event->key() == Qt::Key_Shift) {
            qDebug() << "LAU3DVideoGLWidget::Key_Shift";
        }
    }
    void initializeGL();

private:
    LAUAbstractGLFilter *glFilter;
    unsigned int numDepthRows, numDepthCols;
    unsigned int numColorRows, numColorCols;
    LAUVideoPlaybackDevice playbackDevice;
    LAULookUpTable lookUpTable;
    QAction *lutAction;

signals:
    void emitBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);
};

#endif // LAUVIDEOGLWIDGET_H
