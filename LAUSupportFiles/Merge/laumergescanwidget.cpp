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

#include "laumergescanwidget.h"
#include "lautransformeditorwidget.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMergeScanWidget::LAUMergeScanWidget(LAUScan mstr, LAUScan slv, QWidget *parent) : QWidget(parent), master(mstr), slave(slv)
{
    this->setFocusPolicy(Qt::StrongFocus);

    numCols = master.width();
    numRows = master.height();
    playbackColor = master.color();

    // TRANSFORM INCOMING SCANS IF EITHER HAS A TRANSFORM MATRIX
    //if (slave.transform() != QMatrix4x4()){
    //    slave = slave.transformScan(slave.transform());
    //    slave.setTransform(QMatrix4x4());
    //} else {
    //    slave.updateLimits();
    //}

    //if (master.transform() != QMatrix4x4()){
    //    master = master.transformScan(master.transform());
    //    master.setTransform(QMatrix4x4());
    //} else {
    //    master.updateLimits();
    //}

    if (slave.transform() != QMatrix4x4()) {
        slave.setConstTransform(QMatrix4x4());
    }

    if (master.transform() != QMatrix4x4()) {
        master.setConstTransform(QMatrix4x4());
    }

    if (slave.width() != numCols || slave.height() != numRows) {
        slave = slave.resize(numCols, numRows);
    }

    if (slave.color() != playbackColor) {
        slave = slave.convertToColor(playbackColor);
    }

    initialize();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMergeScanWidget::LAUMergeScanWidget(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent) : QWidget(parent), numCols(cols), numRows(rows), playbackColor(color)
{
    // CREATE EMPTY SCANS
    master = LAUScan(numCols, numRows, playbackColor);
    slave = LAUScan(numCols, numRows, playbackColor);

    initialize();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanWidget::initialize()
{
    // MAKE SURE WE CAN SEND VECTOR3D LISTS AS SIGNALS
    qRegisterMetaType< QList<QVector3D> >("QList<QVector3D>");

    // SET THE FOCUS FOR KEYPRESS EVENTS
    focusState = FocusNone;

    this->setLayout(new QVBoxLayout());
#ifdef Q_OS_WIN
    this->layout()->setContentsMargins(6, 6, 6, 6);
#else
    this->layout()->setContentsMargins(6, 6, 6, 6);
#endif

    splitterA = new QSplitter();
    splitterB = new QSplitter();
    splitterB->setOrientation(Qt::Vertical);
    splitterA->addWidget(splitterB);

    masterWidget = new LAU3DFiducialGLWidget(master);
    masterWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(masterWidget, SIGNAL(emitFiducialsChanged(QVector3D, int)), this, SLOT(onUpdateMasterFiducials(QVector3D, int)));
    connect(masterWidget, SIGNAL(emitFiducialsChanged(QList<QVector3D>)), this, SLOT(onUpdateMasterFiducials(QList<QVector3D>)));
    connect(masterWidget, SIGNAL(emitActivated()), this, SLOT(onScanWidgetActivated()));
    
    // Disable texture display - show point cloud only
    masterWidget->onEnableTexture(false);

    slaveWidget = new LAU3DFiducialGLWidget(slave);
    slaveWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(slaveWidget, SIGNAL(emitFiducialsChanged(QVector3D, int)), this, SLOT(onUpdateSlaveFiducials(QVector3D, int)));
    connect(slaveWidget, SIGNAL(emitFiducialsChanged(QList<QVector3D>)), this, SLOT(onUpdateSlaveFiducials(QList<QVector3D>)));
    connect(slaveWidget, SIGNAL(emitActivated()), this, SLOT(onScanWidgetActivated()));
    
    // Disable texture display - show point cloud only
    slaveWidget->onEnableTexture(false);

    mergeWidget = new LAU3DMultiScanGLWidget(numCols, numRows, playbackColor);
    mergeWidget->setMutuallyExclusive(false);
    mergeWidget->onInsertScan(master);
    mergeWidget->onInsertScan(slave);
    
    // Disable texture display on merge widget as well
    mergeWidget->onEnableTexture(false);

    mergeWidget->setMinimumSize(320, 240);
    masterWidget->setMinimumSize(40, 40);
    slaveWidget->setMinimumSize(40, 40);

    splitterA->addWidget(mergeWidget);
    splitterB->addWidget(masterWidget);
    splitterB->addWidget(slaveWidget);

    icpObject = new LAUIterativeClosestPointObject();
    connect(this, SIGNAL(emitAlignPointLists(QList<QVector3D>, QList<QVector3D>)), icpObject, SLOT(onAlignPointLists(QList<QVector3D>, QList<QVector3D>)), Qt::QueuedConnection);
    connect(this, SIGNAL(emitAlignPointClouds(QList<QVector3D>, QList<QVector3D>)), icpObject, SLOT(onAlignPointClouds(QList<QVector3D>, QList<QVector3D>)), Qt::QueuedConnection);
    connect(icpObject, SIGNAL(emitTransform(QMatrix4x4)), this, SLOT(onUpdateTransforms(QMatrix4x4)), Qt::QueuedConnection);
    icpController = new LAUController(icpObject);

    if (master.isValid()) {
        icpObject->setToScan(master);
    }

    if (slave.isValid()) {
        icpObject->setFmScan(slave);
    }

    this->layout()->addWidget(splitterA);
    this->setMinimumSize(320, 240);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanWidget::showEvent(QShowEvent *)
{
    QRect rect = this->geometry();
    rect.setWidth(numCols + 20);
    rect.setHeight(numRows + 20);
    this->setGeometry(rect);

    QList<int> sizes;
    sizes << numRows / 2;
    sizes << numRows / 2;
    splitterB->setSizes(sizes);

    sizes.clear();
    sizes << numCols / 2;
    sizes << numCols / 2;
    splitterA->setSizes(sizes);
    
    // Disable texture and enable fiducials on OpenGL widgets
    if (masterWidget) {
        masterWidget->onEnableTexture(false);
        masterWidget->onEnableFiducials(true);
    }
    
    if (slaveWidget) {
        slaveWidget->onEnableTexture(false);
        slaveWidget->onEnableFiducials(true);
    }
    
    if (mergeWidget) {
        mergeWidget->onEnableTexture(false);
    }

    // EMIT THE POINT CLOUDS TO THE ICP OBJECT
    emit emitAlignPointClouds(slaveFiducials, masterFiducials);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanWidget::keyPressEvent(QKeyEvent *event)
{
    if (focusState == FocusSlave) {
        slaveWidget->onKeyPressEvent(event);
    } else if (focusState == FocusMaster) {
        masterWidget->onKeyPressEvent(event);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanWidget::setMaster(LAUScan scan)
{
    if (scan.width() != numCols || scan.height() != numRows) {
        scan = scan.resize(numCols, numRows);
    }
    if (scan.color() != playbackColor) {
        scan = scan.convertToColor(playbackColor);
    }
    master = scan;
    masterWidget->onUpdateBuffer(master);
    masterWidget->setRangeLimits(master.minZ(), master.maxZ(), master.fieldOfView().x(), master.fieldOfView().y());
    masterWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mergeWidget->onInsertScan(master);

    if (master.isValid()) {
        icpObject->setToScan(master);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanWidget::setSlave(LAUScan scan)
{
    if (scan.width() != numCols || scan.height() != numRows) {
        scan = scan.resize(numCols, numRows);
    }
    if (scan.color() != playbackColor) {
        scan = scan.convertToColor(playbackColor);
    }
    slave = scan;
    slaveWidget->onUpdateBuffer(slave);
    slaveWidget->setRangeLimits(master.minZ(), master.maxZ(), master.fieldOfView().x(), master.fieldOfView().y());
    slaveWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mergeWidget->onInsertScan(slave);

    if (slave.isValid()) {
        icpObject->setFmScan(slave);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanWidget::onScanWidgetActivated()
{
    if (QObject::sender() == masterWidget) {
        focusState = FocusMaster;
    } else if (QObject::sender() == slaveWidget) {
        focusState = FocusSlave;
    } else {
        focusState = FocusNone;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanWidget::onUpdateMasterFiducials(QVector3D fiducial, int index)
{
    masterFiducials.replace(index, fiducial);
    mergeWidget->onSetFiducials(master.parentName(), masterFiducials);

    // CHECK TO SEE IF THE ICP OBJECT IS STILL CRUNCHING THE LAST SET OF NUMBERS
    if (LAUIterativeClosestPointObject::icpBusyCounterA == 0) {
        // EMIT THE POINT CLOUDS TO THE ICP OBJECT
        emit emitAlignPointLists(slaveFiducials, masterFiducials);
    }
    // INCREMENT THE ICP BUSY COUNTER SO WE KNOW HOW FAR BEHIND WE ARE
    LAUIterativeClosestPointObject::icpBusyCounterA++;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanWidget::onUpdateMasterFiducials(QList<QVector3D> fiducials)
{
    masterFiducials = fiducials;
    mergeWidget->onSetFiducials(master.parentName(), masterFiducials);

    if (slaveFiducials.count() > 2 && masterFiducials.count() > 2) {
        // CHECK TO SEE IF THE ICP OBJECT IS STILL CRUNCHING THE LAST SET OF NUMBERS
        if (LAUIterativeClosestPointObject::icpBusyCounterB == 0) {
            // EMIT THE POINT CLOUDS TO THE ICP OBJECT
            emit emitAlignPointClouds(slaveFiducials, masterFiducials);
        }
        // INCREMENT THE ICP BUSY COUNTER SO WE KNOW HOW FAR BEHIND WE ARE
        LAUIterativeClosestPointObject::icpBusyCounterB++;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanWidget::onUpdateSlaveFiducials(QVector3D fiducial, int index)
{
    slaveFiducials.replace(index, fiducial);
    mergeWidget->onSetFiducials(slave.parentName(), slaveFiducials);

    // CHECK TO SEE IF THE ICP OBJECT IS STILL CRUNCHING THE LAST SET OF NUMBERS
    if (LAUIterativeClosestPointObject::icpBusyCounterA == 0) {
        // EMIT THE POINT CLOUDS TO THE ICP OBJECT
        emit emitAlignPointLists(slaveFiducials, masterFiducials);
    }
    // INCREMENT THE ICP BUSY COUNTER SO WE KNOW HOW FAR BEHIND WE ARE
    LAUIterativeClosestPointObject::icpBusyCounterA++;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanWidget::onUpdateSlaveFiducials(QList<QVector3D> fiducials)
{
    slaveFiducials = fiducials;
    mergeWidget->onSetFiducials(slave.parentName(), slaveFiducials);

    if (slaveFiducials.count() > 2 && masterFiducials.count() > 2) {
        // CHECK TO SEE IF THE ICP OBJECT IS STILL CRUNCHING THE LAST SET OF NUMBERS
        if (LAUIterativeClosestPointObject::icpBusyCounterB == 0) {
            // EMIT THE POINT CLOUDS TO THE ICP OBJECT
            emit emitAlignPointClouds(slaveFiducials, masterFiducials);
        }
        // INCREMENT THE ICP BUSY COUNTER SO WE KNOW HOW FAR BEHIND WE ARE
        LAUIterativeClosestPointObject::icpBusyCounterB++;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanWidget::onUpdateTransforms(QMatrix4x4 transform)
{
    optTransform = transform;
    mergeWidget->onSetTransform(slave.parentName(), transform);

    // IF THE ICP BUSY COUNTER IS GREATER THAN ONE, THEN USER MAY HAVE STOPPED MOVING
    // BUT THE LAST RECEIVED CLOUDS MAY NOT ACTUALLY BE THOSE LAST CLOUDS. SO WE NEED
    // TO CALL THE ALIGN POINT CLOUDS ONE MORE TIME BEFORE RESETTING THE COUNTER TO ZERO
    if (LAUIterativeClosestPointObject::icpBusyCounterB > 1) {
        // EMIT THE POINT CLOUDS TO THE ICP OBJECT
        emit emitAlignPointClouds(slaveFiducials, masterFiducials);
    } else if (LAUIterativeClosestPointObject::icpBusyCounterA > 1) {
        // EMIT THE POINT CLOUDS TO THE ICP OBJECT
        emit emitAlignPointLists(slaveFiducials, masterFiducials);
    }
    LAUIterativeClosestPointObject::icpBusyCounterA = 0;
    LAUIterativeClosestPointObject::icpBusyCounterB = 0;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeScanDialog::accept()
{
    LAUTransformEditorDialog dialog(widget->transform(), this);
    if (dialog.exec() == QDialog::Accepted){
        QDialog::accept();
    }
}
