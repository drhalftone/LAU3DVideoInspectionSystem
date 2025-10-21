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

#include "lauscaninspector.h"
#ifdef SANDBOX
#include "laulookuptable.h"
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QPointF> extractYoloFromXMLString(QByteArray inXml)
{
    // CREATE LIST OF POINTS TO SEND BACK TO USER
    QList<QPointF> points;

    // GRAB THE CURRENT XML FIELD IN HASH TABLE FORM
    QHash<QString, QString> hashTable = LAUMemoryObject::xmlToHash(inXml);

    // CREATE LABEL STRING FROM USER SUPPLIED POINTS
    QString string = hashTable["YoloPoseLabel"];

    // SEE IF THERE WAS A VALID YOLO REGION OF INTEREST IN THE XML FIELD
    if (string.isEmpty() == false){
        QStringList strings = string.split(",");
        if (strings.count() > 4){
            // REMOVE THE BOUNDBOX COORDINATES
            strings.takeFirst();
            strings.takeFirst();
            strings.takeFirst();
            strings.takeFirst();

            // ADD ALL REMAINING INTEGER PAIRS AS POINTS
            while (strings.count() >= 2){
                double x = strings.takeFirst().toDouble();
                double y = strings.takeFirst().toDouble();
                points << QPointF(x, y);
            }
        }
    }
    return(points);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScanInspector::LAUScanInspector(LAUScan scn, bool enableCancelButton, bool enableDoNotShowAgainCheckBox, QWidget *parent) : QDialog(parent), scan(scn)
{
    this->setWindowTitle(QString("Scan Inspector"));
    this->setLayout(new QVBoxLayout());
#ifdef Q_OS_WIN
    this->layout()->setContentsMargins(6, 6, 6, 6);
#else
    this->layout()->setContentsMargins(6, 6, 6, 6);
#endif

    // CREATE A GLWIDGET TO DISPLAY THE SCAN
    scanWidget = new LAU3DFiducialGLWidget(scan);
    scanWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scanWidget->setFocusPolicy(Qt::StrongFocus);
    scanWidget->setMinimumSize(320, 240);
    scanWidget->onEnableFiducials(false);

    // SEE IF THERE ARE ANY YOLO POSE COORDINATES
    QList<QPointF> pointFs = extractYoloFromXMLString(scan.xml());
    if (pointFs.isEmpty() == false){
        QList<QPoint> points;
        for (int m = 0; m < pointFs.count(); m++){
            points << QPoint(qRound(pointFs.at(m).x()), qRound(pointFs.at(m).y()));
        }
        scanWidget->onSetFiducials(points);
    }

#ifdef SANDBOX
    sandboxDialog = nullptr;
    if (scan.projection().isIdentity() == false){
        // CREATE A SCANGLWIDGET TO DISPLAY THE SCAN IN THE PROJECTOR
        LAU3DFiducialGLWidget *sandboxWidget = new LAU3DFiducialGLWidget(scan);
        sandboxWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sandboxWidget->setFocusPolicy(Qt::StrongFocus);
        sandboxWidget->setMinimumSize(320, 240);

        // PUT GLWIDGET INTO SANDBOX MODE
        sandboxWidget->enableSandboxTexture(true);
        sandboxWidget->setSandboxProjectionMatrix(scan.projection());
        sandboxWidget->onEnableFiducials(true);

        // SEND FIDUCIALS FROM THE DESKTOP WIDGET TO THE PROJECTOR WIDGET
        connect(scanWidget, SIGNAL(emitFiducialsChanged(QList<QVector3D>)), sandboxWidget, SLOT(onSetFiducials(QList<QVector3D>)));
        connect(scanWidget, SIGNAL(emitFiducialsChanged(QVector3D,int)), sandboxWidget, SLOT(onSetFiducials(QVector3D,int)));

        sandboxDialog = new QDialog(this);
        sandboxDialog->setWindowFlag(Qt::Tool);
        sandboxDialog->setLayout(new QVBoxLayout());
        sandboxDialog->layout()->setContentsMargins(0, 0, 0, 0);
        sandboxDialog->layout()->addWidget(sandboxWidget);
        connect(scanWidget, SIGNAL(destroyed()), sandboxDialog, SLOT(deleteLater()));
    }
    // LAULookUpTable table((QString()));
    // if (table.isValid()){
    //     // CREATE A SCANGLWIDGET TO DISPLAY THE SCAN IN THE PROJECTOR
    //     LAU3DFiducialGLWidget *sandboxWidget = new LAU3DFiducialGLWidget(scan);
    //     sandboxWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //     sandboxWidget->setFocusPolicy(Qt::StrongFocus);
    //     sandboxWidget->setMinimumSize(320, 240);
    //
    //     // PUT GLWIDGET INTO SANDBOX MODE
    //     sandboxWidget->enableSandboxTexture(true);
    //     sandboxWidget->setSandboxProjectionMatrix(table.projection());
    //     sandboxWidget->onEnableFiducials(true);
    //
    //     // SEND FIDUCIALS FROM THE DESKTOP WIDGET TO THE PROJECTOR WIDGET
    //     connect(scanWidget, SIGNAL(emitFiducialsChanged(QList<QVector3D>)), sandboxWidget, SLOT(onSetFiducials(QList<QVector3D>)));
    //     connect(scanWidget, SIGNAL(emitFiducialsChanged(QVector3D,int)), sandboxWidget, SLOT(onSetFiducials(QVector3D,int)));
    //
    //     sandboxDialog = new QDialog(this);
    //     sandboxDialog->setWindowFlag(Qt::Tool);
    //     sandboxDialog->setLayout(new QVBoxLayout());
    //     sandboxDialog->layout()->setContentsMargins(0, 0, 0, 0);
    //     sandboxDialog->layout()->addWidget(sandboxWidget);
    //     connect(scanWidget, SIGNAL(destroyed()), sandboxDialog, SLOT(deleteLater()));
    // }
#endif

    if (scanWidget){
        this->layout()->addWidget(scanWidget);
    }

    // ADD OKAY AND CANCEL BUTTONS
    QWidget *buttonBox = new QWidget();
    buttonBox->setLayout(new QHBoxLayout());
    buttonBox->layout()->setContentsMargins(0, 0, 0, 0);

    // CREATE DO NOT SHOW AGAIN CHECK BOX IF REQUESTED BY USER
    if (enableDoNotShowAgainCheckBox) {
        checkBox = new QCheckBox(QString("Do not show again"));
        checkBox->setChecked(false);
        buttonBox->layout()->addWidget(checkBox);
    }

    // INSERT SPACER
    ((QHBoxLayout *)(buttonBox->layout()))->addStretch();

    // INSERT CANCEL BUTTON IF REQUESTED BY USER
    if (enableCancelButton) {
        QPushButton *button = new QPushButton(QString("Cancel"));
        button->setFixedWidth(80);
        connect(button, SIGNAL(clicked()), this, SLOT(reject()));
        buttonBox->layout()->addWidget(button);
    }

    // INSERT OK BUTTON TO ACCEPT THE DIALOG
    QPushButton *button = new QPushButton(QString("Ok"));
    button->setFixedWidth(80);
    connect(button, SIGNAL(clicked()), this, SLOT(accept()));
    buttonBox->layout()->addWidget(button);
    this->layout()->addWidget(buttonBox);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUScanInspector::showEvent(QShowEvent *event)
{
#ifdef SANDBOX
    // IF WE HAVE A SCREEN WIDGET (I.E. AR SANDBOX MODE) AND A SECOND SCREEN, THEN MAKE IT FULL SCREEN
    if (sandboxDialog) {
        if (QGuiApplication::screens().count() > 1) {
            sandboxDialog->setGeometry(QGuiApplication::screens().last()->availableGeometry());
            sandboxDialog->showFullScreen();
        } else {
            sandboxDialog->show();
        }
    }
#endif
    // CALL THE UNDERLYING WIDGET'S SHOW EVENT HANDLER
    QWidget::showEvent(event);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUScanInspector::doNotShowAgainChecked()
{
    if (checkBox) {
        return (checkBox->isChecked());
    }
    return (false);
}

