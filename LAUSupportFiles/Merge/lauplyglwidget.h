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

#ifndef LAUPLYGLWIDGET_H
#define LAUPLYGLWIDGET_H

#include <QFile>
#include <QMenu>
#include <QtCore>
#include <QScreen>
#include <QWidget>
#include <QObject>
#include <QDialog>
#include <QAction>
#include <QMatrix4x4>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QOpenGLFunctions>
#include <QDialogButtonBox>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>

#include "lauplyobject.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPlyGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit LAUPlyGLWidget(LAUPlyObject obj, QWidget *parent = NULL) : QOpenGLWidget(parent), plyObject(obj), enableTextureFlag(true), zoomFactor(1.0f), xRot(0), yRot(0), zRot(0)
    {
        // Qt 6 compatibility: ensure full buffer clear between frames
        setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

        // CREATE A CONTEXT MENU FOR TOGGLING TEXTURE
        contextMenu = new QMenu(this);
        QAction *action = contextMenu->addAction(QString("Show Texture"));
        action->setCheckable(enableTextureFlag);
        action->setChecked(enableTextureFlag);
        connect(action, SIGNAL(triggered(bool)), this, SLOT(onEnableTexture(bool)));
    }
    ~LAUPlyGLWidget()
    {
        if (wasInitialized()) {
            makeCurrent();
            vertexArrayObject.release();
        }
    }

    virtual bool isValid() const
    {
        return (wasInitialized());
    }
    bool wasInitialized() const
    {
        return (vertexArrayObject.isCreated());
    }

    void setLimits(float xmn, float xmx, float ymn, float ymx, float zmn, float zmx);

public slots:
    void onEnableTexture(bool state)
    {
        enableTextureFlag = state;
        update();
    }

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void mousePressEvent(QMouseEvent *event)
    {
        lastPos = event->pos();
        if (event->button() == Qt::RightButton) {
            if (contextMenu) {
                contextMenu->popup(event->globalPos());
            }
        }
    }
    void mouseReleaseEvent(QMouseEvent *) { ; }
    void mouseDoubleClickEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    LAUPlyObject plyObject;
    bool enableTextureFlag;
    QMenu *contextMenu;
    QOpenGLVertexArrayObject vertexArrayObject;
    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer indiceBuffer;
    QOpenGLShaderProgram program;

    void updateProjectionMatrix();

    int localWidth, localHeight;
    float scaleFactor, offset;
    qreal devicePixelRatio;

    float xMin, xMax;
    float yMin, yMax;
    float zMin, zMax;
    float horizontalFieldOfView, verticalFieldOfView;
    float zoomFactor;

    int xRot, yRot, zRot;
    QMatrix4x4 projection;
    QPoint lastPos;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPlyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUPlyDialog(QString filename, QWidget *parent = NULL) : QDialog(parent), plyObject(filename)
    {
        initialize();
    }
    explicit LAUPlyDialog(QList<LAUScan> scans, QWidget *parent = NULL) : QDialog(parent), plyObject(scans)
    {
        initialize();
    }

    void initialize()
    {
        glWidget = new LAUPlyGLWidget(plyObject);
        glWidget->setMinimumSize(640, 480);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
        connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));

        this->setLayout(new QVBoxLayout());
        this->setWindowTitle(QString("Fuse Scans Dialog"));
#ifdef Q_OS_WIN
        this->layout()->setContentsMargins(6, 6, 6, 6);
#else
        this->layout()->setContentsMargins(6, 6, 6, 6);
#endif
        this->layout()->addWidget(glWidget);
        this->layout()->addWidget(buttonBox);
    }
    ~LAUPlyDialog() { ; }

    bool save(QString string)
    {
        return (plyObject.save(string));
    }

protected:
    void closeEvent(QCloseEvent *)
    {
        this->reject();
    }
    void accept()
    {
        if (save(QString())) {
            QDialog::accept();
        }
    }

private:
    LAUPlyObject plyObject;
    LAUPlyGLWidget *glWidget;
};

#endif // LAUPLYGLWIDGET_H
