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

#ifndef LAUSCANINSPECTOR_H
#define LAUSCANINSPECTOR_H

#include <QDir>
#include <QFile>
#include <QList>
#include <QTime>
#include <QSize>
#include <QImage>
#include <QDebug>
#include <QLabel>
#include <QtCore>
#include <QDialog>
#include <QObject>
#include <QVector3D>
#include <QCheckBox>
#include <QMatrix4x4>
#include <QByteArray>
#include <QStringList>
#include <QSharedData>
#include <QFileDialog>
#include <QScrollArea>
#include <QPushButton>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QApplication>
#include <QProgressDialog>
#include <QDialogButtonBox>

#include "lau3dfiducialglwidget.h"
#include "lauscan.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUScanInspector : public QDialog
{
    Q_OBJECT

public:
    explicit LAUScanInspector(LAUScan scn, bool enableCancelButton = false, bool enableDoNotShowAgainCheckBox = false, QWidget *parent = NULL);

    bool doNotShowAgainChecked();

public slots:
    void onSetFiducials(QList<QVector3D> fiducials, QList<QVector3D> colors)
    {
        if (scanWidget) {
            scanWidget->onSetFiducials(fiducials, colors);
        }
    }

    void onEnableFiducials(bool state){
        if (scanWidget) {
            scanWidget->onEnableFiducials(state);
        }
    }

protected:
    void showEvent(QShowEvent*);
    void closeEvent(QCloseEvent*) { QDialog::accept(); }

private:
    LAUScan scan;
    LAU3DFiducialGLWidget *scanWidget;
#ifdef SANDBOX
    QDialog *sandboxDialog;
#endif
    QCheckBox *checkBox;
};

#endif // LAUSCANINSPECTOR_H
