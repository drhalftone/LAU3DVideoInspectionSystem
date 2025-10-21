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

#ifndef LAUGLWIDGET_H
#define LAUGLWIDGET_H

#include <QMenu>
#include <QScreen>
#include <QWidget>
#include <QObject>
#include <QMatrix4x4>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPixelTransferOptions>

#include "laumemoryobject.h"

#define MINIMUMSCREENWIDTHFORFULLSCREEN 800
#define MINIMUMSCREENHEIGTFORFULLSCREEN 600

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUAbstractGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit LAUAbstractGLWidget(QWidget *parent = nullptr);
    ~LAUAbstractGLWidget();

    virtual bool isValid() const
    {
        return (wasInitialized());
    }

    bool wasInitialized() const
    {
        return (vertexArrayObject.isCreated());
    }

    QMenu *menu() const
    {
        return (contextMenu);
    }

    void setLimits(float xmn, float xmx, float ymn, float ymx, float zmn, float zmx, float xme = NAN, float yme = NAN, float zme = NAN);

    QVector2D xLimits() const
    {
        return (QVector2D(xMin, xMax));
    }

    QVector2D yLimits() const
    {
        return (QVector2D(yMin, yMax));
    }

    QVector2D zLimits() const
    {
        return (QVector2D(zMin, zMax));
    }

    void setColorTransform(QMatrix4x4 mat)
    {
        clrTransform = mat;
        update();
    }

    QMatrix4x4 colorTransform() const
    {
        return (clrTransform);
    }

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void mousePressEvent(QMouseEvent *event)
    {
        lastPos = event->pos();
        if (event->button() == Qt::RightButton) {
            if (contextMenu && (contextMenu->actions().count() > 0)) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                contextMenu->popup(event->globalPosition().toPoint());
#else
                contextMenu->popup(event->globalPos());
#endif
            }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            emit emitClicked(event->globalPosition().toPoint());
#else
            emit emitClicked(event->globalPos());
#endif
        }
    }
    void mouseReleaseEvent(QMouseEvent *) { ; }
    void mouseDoubleClickEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

    void updateProjectionMatrix();

    int localWidth, localHeight;
    float scaleFactor, offset;
    qreal devicePixelRatio;

    float xMin, xMax, xMen;
    float yMin, yMax, yMen;
    float zMin, zMax, zMen;

    float horizontalFieldOfView, verticalFieldOfView;
    float zoomFactor;

    QMenu *contextMenu;

    int xRot, yRot, zRot;
    QMatrix4x4 projection;
    QMatrix4x4 clrTransform;
    QOpenGLPixelTransferOptions options;
    QPoint lastPos;

private:
    QOpenGLVertexArrayObject vertexArrayObject;
    QOpenGLBuffer noVideoVertexBuffer, noVideoIndexBuffer;
    QOpenGLShaderProgram noVideoProgram;
    QOpenGLTexture *noVideoTexture;
    bool initializedFlag;

signals:
    void emitActivated();
    void emitClicked(QPoint pos);
};

#endif // LAUGLWIDGET_H
