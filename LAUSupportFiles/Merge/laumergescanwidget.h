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

#ifndef LAUMERGESCANWIDGET_H
#define LAUMERGESCANWIDGET_H

#include <QWidget>
#include <QDialog>
#include <QSplitter>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QSettings>

#include "laucontroller.h"
#include "lau3dmultiscanglwidget.h"
#include "lauiterativeclosestpointobject.h"

using namespace LAU3DVideoParameters;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUMergeScanWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LAUMergeScanWidget(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color = ColorXYZG, QWidget *parent = NULL);
    explicit LAUMergeScanWidget(LAUScan mstr, LAUScan slv, QWidget *parent = NULL);
    ~LAUMergeScanWidget(){ delete icpController; }

    QMatrix4x4 transform() { return(optTransform); }
    void setMaster(LAUScan scan);
    void setSlave(LAUScan scan);

protected:
    void showEvent(QShowEvent *);
    void keyPressEvent(QKeyEvent *event);

public slots:
    void onScanWidgetActivated();
    void onUpdateTransforms(QMatrix4x4 transform);
    void onUpdateMasterFiducials(QVector3D fiducial, int index);
    void onUpdateMasterFiducials(QList<QVector3D> fiducials);
    void onUpdateSlaveFiducials(QVector3D fiducial, int index);
    void onUpdateSlaveFiducials(QList<QVector3D> fiducials);

private:
    enum FocusState { FocusMaster, FocusSlave, FocusNone };
    FocusState focusState;

    unsigned int numCols, numRows;
    LAUVideoPlaybackColor playbackColor;
    LAUScan master, slave;
    QMatrix4x4 optTransform;

    QSplitter *splitterA, *splitterB;
    LAU3DMultiScanGLWidget *mergeWidget;
    LAU3DFiducialGLWidget *masterWidget;
    LAU3DFiducialGLWidget *slaveWidget;

    LAUController *icpController;
    LAUIterativeClosestPointObject *icpObject;

    QList<QVector3D> masterFiducials;
    QList<QVector3D> slaveFiducials;

    void initialize();

signals:
    void emitAlignPointLists(QList<QVector3D>, QList<QVector3D>);
    void emitAlignPointClouds(QList<QVector3D>, QList<QVector3D>);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUMergeScanDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUMergeScanDialog(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color = ColorXYZG, QWidget *parent = NULL) : QDialog(parent)
    {
        widget = new LAUMergeScanWidget(cols, rows, color);
        initialize();
        
        // Restore dialog geometry from QSettings
        QSettings settings;
        settings.beginGroup("DialogGeometry");
        QByteArray geometry = settings.value("LAUMergeScanDialog/geometry").toByteArray();
        if (!geometry.isEmpty()) {
            restoreGeometry(geometry);
        }
        settings.endGroup();
    }

    explicit LAUMergeScanDialog(LAUScan mstr, LAUScan slv, QWidget *parent = NULL) : QDialog(parent)
    {
        widget = new LAUMergeScanWidget(mstr, slv, 0);
        initialize();
        
        // Restore dialog geometry from QSettings
        QSettings settings;
        settings.beginGroup("DialogGeometry");
        QByteArray geometry = settings.value("LAUMergeScanDialog/geometry").toByteArray();
        if (!geometry.isEmpty()) {
            restoreGeometry(geometry);
        }
        settings.endGroup();
    }
    
    ~LAUMergeScanDialog()
    {
        // Save dialog geometry to QSettings
        QSettings settings;
        settings.beginGroup("DialogGeometry");
        settings.setValue("LAUMergeScanDialog/geometry", saveGeometry());
        settings.endGroup();
    }

    QMatrix4x4 transform() { return(widget->transform()); }

signals:

public slots:

protected:
    void accept();

private:
    LAUMergeScanWidget *widget;

    void initialize()
    {
        this->setLayout(new QVBoxLayout());
        this->layout()->setContentsMargins(6,6,6,6);
        this->layout()->addWidget(widget);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
        connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
        this->layout()->addWidget(buttonBox);
    }
};

#endif // LAUMERGESCANWIDGET_H
