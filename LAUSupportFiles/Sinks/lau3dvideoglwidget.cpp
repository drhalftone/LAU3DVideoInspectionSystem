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

#include "lau3dvideoglwidget.h"
#include <locale.h>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DVideoGLWidget::LAU3DVideoGLWidget(unsigned int depthCols, unsigned int depthRows, unsigned int colorCols, unsigned int colorRows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent) : LAU3DFiducialGLWidget(((depthCols == 0) ? colorCols : depthCols), ((depthRows == 0) ? colorRows : depthRows), color, parent), playbackDevice(device), glFilter(NULL)
{
    numDepthCols = (depthCols == 0) ? colorCols : depthCols;
    numDepthRows = (depthRows == 0) ? colorRows : depthRows;
    numColorCols = (colorCols == 0) ? depthCols : colorCols;
    numColorRows = (colorRows == 0) ? depthRows : colorRows;

    glFilter = new LAUAbstractGLFilter(numDepthCols, numDepthRows, numColorCols, numColorRows, color, device);
    lutAction = nullptr;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DVideoGLWidget::LAU3DVideoGLWidget(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent) : LAU3DFiducialGLWidget(cols, rows, color, parent), playbackDevice(device), glFilter(NULL)
{
    numDepthCols = cols;
    numDepthRows = rows;
    numColorCols = cols;
    numColorRows = rows;

    glFilter = new LAUAbstractGLFilter(numDepthCols, numDepthRows, numColorCols, numColorRows, color, device);
    lutAction = nullptr;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DVideoGLWidget::~LAU3DVideoGLWidget()
{
    if (glFilter) {
        delete glFilter;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoGLWidget::setLookUpTable(LAULookUpTable lut)
{
    // MAKE SURE WE HAVE A VALID LOOK UP TABLE
    if (lut.isValid()) {
        lookUpTable = lut;

        // GIVE THE USER A CHANCE TO EXPORT THIS LUT TO DISK
        if (lutAction == nullptr){
            lutAction = contextMenu->addAction(QString("Export Look-Up Table"));
            lutAction->setCheckable(false);
            connect(lutAction, SIGNAL(triggered()), this, SLOT(onExportLookUpTable()));
        }
    } else {
        lookUpTable = LAULookUpTable(numDepthCols, numDepthRows, playbackDevice);
    }

    // SET THE LOOKUP TABLE ON THE CONTEXT OBJECT
    if (glFilter) {
        glFilter->setLookUpTable(lut);
    }

    // MAKE SURE WE HAVE A VALID LOOK UP TABLE
    if (lookUpTable.isValid()) {
        setLimits(lookUpTable.xLimits().x(), lookUpTable.xLimits().y(), lookUpTable.yLimits().x(), lookUpTable.yLimits().y(), lookUpTable.zLimits().x(), lookUpTable.zLimits().y());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoGLWidget::initializeGL()
{
    // CALL THE UNDERLYING CLASS TO INITIALIZE THE WIDGET
    LAU3DFiducialGLWidget::initializeGL();

    if (isValid() && glFilter) {
        glFilter->setFormat(this->format());
        glFilter->setSurface(this->context()->surface());
        glFilter->setShareContext(this->context());
        glFilter->create();
        glFilter->initialize();

        // SET THE GLFILTER'S FBO AS THE TARGET FOR THE UNDERLYING SCANGLWIDGET
        onSetTexture(glFilter->fbo());
    }

    // SET THE LOOK UP TABLE IF IT EXISTS
    if (lookUpTable.isValid()) {
        setLookUpTable(lookUpTable);
    }

    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoGLWidget::onUpdateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    // MAKE SURE WE HAVE A GLFILTER TO PROCESS THE INCOMING BUFFERS
    if (glFilter) {
        // Call onUpdateBuffer instead of updateBuffer to trigger signals (e.g., emitChannelIndex)
        glFilter->onUpdateBuffer(depth, color, mapping);
    }

    // REDRAW THE WIDGET ON SCREEN
    update();

    // EMIT THE BUFFERS TO THE NEXT OBJECT
    emit emitBuffer(depth, color, mapping);
}
