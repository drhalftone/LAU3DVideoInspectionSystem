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

#include <QWindow>

#include "lauglwidget.h"
#include "lauconstants.h"
#include <locale.h>
#include <math.h>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUAbstractGLWidget::LAUAbstractGLWidget(QWidget *parent) : QOpenGLWidget(parent), noVideoTexture(nullptr)
{
    options.setAlignment(1);
    contextMenu = nullptr;

    lastPos = QPoint(0, 0);
    localHeight = LAU_CAMERA_DEFAULT_HEIGHT;
    localWidth = LAU_CAMERA_DEFAULT_WIDTH;

    zoomFactor = 1.0;

    xRot = 0.0;
    yRot = 0.0;
    zRot = 0.0;

    // Qt 6 compatibility: ensure full buffer clear between frames
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUAbstractGLWidget::~LAUAbstractGLWidget()
{
    if (wasInitialized() && context()) {
        // Make context current before destroying OpenGL resources
        // Only proceed if context is still valid (not already destroyed)
        makeCurrent();
        if (noVideoTexture) {
            delete noVideoTexture;
        }
        vertexArrayObject.release();
        doneCurrent();
    }
    if (contextMenu) {
        delete contextMenu;
    }
    qDebug() << QString("LAUAbstractGLWidget::~LAUAbstractGLWidget()");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLWidget::setLimits(float xmn, float xmx, float ymn, float ymx, float zmn, float zmx, float xme, float yme, float zme)
{
    // SET THE Z LIMITS AND CALCULATE THE FIELD OF VIEW
    xMin = qMin(xmn, xmx);
    xMax = qMax(xmn, xmx);
    xMen = xme;

    yMin = qMin(ymn, ymx);
    yMax = qMax(ymn, ymx);
    yMen = yme;

    zMin = qMin(zmn, zmx);
    zMax = qMax(zmn, zmx);
    zMen = zme;

    // CALCULATE FOV
    float   phiA = atan(yMin / zMin);
    float   phiB = atan(yMax / zMin);
    float thetaA = atan(xMin / zMin);
    float thetaB = atan(xMax / zMin);

    horizontalFieldOfView = fabs(thetaA) + fabs(thetaB);
    verticalFieldOfView   = fabs(phiA) + fabs(phiB);

    // MAKE SURE WE HAVE ALREADY INITIALIZED THIS GLWIDGET BEFORE TELLING IT TO UPDATE ITSELF
    if (wasInitialized()) {
        updateProjectionMatrix();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLWidget::wheelEvent(QWheelEvent *event)
{
    // CHANGE THE ZOOM FACTOR BASED ON HOW MUCH WHEEL HAS MOVED
    zoomFactor *= (1.0f + (float)event->angleDelta().y() / 160.0f);
    zoomFactor = qMax(0.10f, qMin(zoomFactor, 10.0f));

    // UPDATE THE PROJECTION MATRIX SINCE WE CHANGED THE ZOOM FACTOR
    updateProjectionMatrix();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint currentPos = event->position().toPoint();
    int dx = lastPos.x() - currentPos.x();
    int dy = currentPos.y() - lastPos.y();

    if (event->buttons() & Qt::LeftButton) {
        xRot = qMin(qMax(xRot + 4 * dy, -1200), 1200);
        yRot = qMin(qMax(yRot + 4 * dx, -1200), 1200);
        zRot = qMin(qMax(zRot + 0, -1200), 1200);
    } else if (event->buttons() & Qt::RightButton) {
        xRot = qMin(qMax(xRot + 4 * dy, -1200), 1200);
        yRot = qMin(qMax(yRot + 0, -1200), 1200);
        zRot = qMin(qMax(zRot + 4 * dx, -1200), 1200);
    }
    lastPos = currentPos;

    // UPDATE THE PROJECTION MATRIX SINCE WE CHANGED THE ROTATION ANGLE
    updateProjectionMatrix();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    xRot = 0;
    yRot = 0;
    zRot = 0;
    zoomFactor = 1.0;
    updateProjectionMatrix();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLWidget::updateProjectionMatrix()
{
    float aspectRatio = (float)width() / (float)height();
    float xCenter = (isnan(xMen)) ? (xMin + xMax) / 2.0f : xMen;
    float yCenter = (isnan(yMen)) ? (yMin + yMax) / 2.0f : yMen;
    float zCenter = (isnan(zMen)) ? (zMin + zMax) / 2.0f : zMen;

    QMatrix4x4 eyeTransform;
    eyeTransform.setToIdentity();
    eyeTransform.translate(xCenter, yCenter, zCenter);
    eyeTransform.rotate(-xRot / 16.0f, 1.0f, 0.0f, 0.0f);
    eyeTransform.rotate(yRot / 16.0f, 0.0f, 1.0f, 0.0f);
    eyeTransform.translate(-xCenter, -yCenter, -zCenter);

    QVector4D eye = eyeTransform * QVector4D(0.0f, 0.0f, 0.0f, 1.0f);

    float fov   = qMin(120.0f, qMax(zoomFactor * verticalFieldOfView * 180.0f / 3.141592653f, 0.5f));
    float zNear = qMin(qAbs(zMin), qAbs(zMax));
    float zFar  = qMax(qAbs(zMin), qAbs(zMax));

    // INITIALIZE THE PROJECTION MATRIX TO IDENTITY
    projection.setToIdentity();
    projection.perspective(fov, aspectRatio, zNear / 4.0f, 3.0f * zFar);
    projection.lookAt(QVector3D(eye), QVector3D(xCenter, yCenter, zCenter / 2.0), QVector3D(0.0f, 1.0f, 0.0f));

    //qDebug() << projection(0, 0) << projection(0, 1) << projection(0, 2) << projection(0, 3);
    //qDebug() << projection(1, 0) << projection(1, 1) << projection(1, 2) << projection(1, 3);
    //qDebug() << projection(2, 0) << projection(2, 1) << projection(2, 2) << projection(2, 3);
    //qDebug() << projection(3, 0) << projection(3, 1) << projection(3, 2) << projection(3, 3);

    // UPDATE THE DISPLAY
    update();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.5f, 0.0f, 0.0f, 1.0f);

    // GET CONTEXT OPENGL-VERSION
    qDebug() << "void LAUAbstractGLWidget::initializeGL()";
    qDebug() << "Really used OpenGl: " << format().majorVersion() << "." << format().minorVersion();
    qDebug() << "OpenGl information: VENDOR:       " << reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    qDebug() << "                    RENDERDER:    " << reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    qDebug() << "                    VERSION:      " << reinterpret_cast<const char *>(glGetString(GL_VERSION));
    qDebug() << "                    GLSL VERSION: " << reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    // CREATE THE VERTEX ARRAY OBJECT FOR FEEDING VERTICES TO OUR SHADER PROGRAMS
    vertexArrayObject.create();
    vertexArrayObject.bind();

    // CREATE VERTEX BUFFER TO HOLD CORNERS OF QUADRALATERAL
    noVideoVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    noVideoVertexBuffer.create();
    noVideoVertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (noVideoVertexBuffer.bind()) {
        // ALLOCATE THE VERTEX BUFFER FOR HOLDING THE FOUR CORNERS OF A RECTANGLE
        noVideoVertexBuffer.allocate(16 * sizeof(float));
        float *buffer = (float *)noVideoVertexBuffer.map(QOpenGLBuffer::WriteOnly);
        if (buffer) {
            buffer[0]  = -1.0;
            buffer[1]  = -1.0;
            buffer[2]  = 0.0;
            buffer[3]  = 1.0;
            buffer[4]  = +1.0;
            buffer[5]  = -1.0;
            buffer[6]  = 0.0;
            buffer[7]  = 1.0;
            buffer[8]  = +1.0;
            buffer[9]  = +1.0;
            buffer[10] = 0.0;
            buffer[11] = 1.0;
            buffer[12] = -1.0;
            buffer[13] = +1.0;
            buffer[14] = 0.0;
            buffer[15] = 1.0;
            noVideoVertexBuffer.unmap();
        } else {
            qDebug() << QString("noVideoVertexBuffer not allocated.") << glGetError();
        }
        noVideoVertexBuffer.release();
    }

    // CREATE INDEX BUFFER TO ORDERINGS OF VERTICES FORMING POLYGON
    noVideoIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    noVideoIndexBuffer.create();
    noVideoIndexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (noVideoIndexBuffer.bind()) {
        noVideoIndexBuffer.allocate(6 * sizeof(unsigned int));
        unsigned int *indices = (unsigned int *)noVideoIndexBuffer.map(QOpenGLBuffer::WriteOnly);
        if (indices) {
            indices[0] = 0;
            indices[1] = 1;
            indices[2] = 2;
            indices[3] = 0;
            indices[4] = 2;
            indices[5] = 3;
            noVideoIndexBuffer.unmap();
        } else {
            qDebug() << QString("indiceBufferA buffer mapped from GPU.");
        }
        noVideoIndexBuffer.release();
    }

    // CREATE TEXTURE FOR DISPLAYING NO VIDEO SCREEN
    LAUMemoryObject object(QImage(":/Images/NoVideoScreen.jpg"));

    noVideoTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    noVideoTexture->setSize((int)object.width(), (int)object.height());
    noVideoTexture->setFormat(QOpenGLTexture::RGBA32F);
    noVideoTexture->setWrapMode(QOpenGLTexture::ClampToBorder);
    noVideoTexture->setMinificationFilter(QOpenGLTexture::Linear);
    noVideoTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    noVideoTexture->allocateStorage();
    if (object.isValid()){
        noVideoTexture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)object.constPointer(), &options);
    }

    // CREATE SHADER FOR SHOWING THE VIDEO NOT AVAILABLE IMAGE
    setlocale(LC_NUMERIC, "C");
    if (!noVideoProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/RGB/displayRGBVideo.vert")) {
        close();
    } else if (!noVideoProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/RGB/displayRGBVideo.frag")) {
        close();
    } else if (!noVideoProgram.link()) {
        close();
    }
    setlocale(LC_ALL, "");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLWidget::resizeGL(int w, int h)
{
    qreal devicePixelRatio = this->window()->devicePixelRatio();
    localHeight = h * devicePixelRatio;
    localWidth = w * devicePixelRatio;

    // WE'LL SET THE VIEW PORT MANUALLY IN THE PAINTGL METHOD
    // BUT LET'S MAKE SURE WE HAVE THE MOST UP TO DATE PROJECTION MATRIX
    updateProjectionMatrix();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLWidget::paintGL()
{
    // SET THE VIEW PORT AND CLEAR THE SCREEN BUFFER
    glViewport(0, 0, localWidth, localHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // MAKE SURE WE HAVE A TEXTURE TO SHOW
    if (noVideoTexture) {
        if (noVideoProgram.bind()) {
            if (noVideoVertexBuffer.bind()) {
                if (noVideoIndexBuffer.bind()) {
                    // SET THE ACTIVE TEXTURE ON THE GPU
                    glActiveTexture(GL_TEXTURE0);
                    noVideoTexture->bind();
                    noVideoProgram.setUniformValue("qt_texture", 0);

                    // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                    noVideoProgram.setAttributeBuffer("qt_vertex", GL_FLOAT, 0, 4, 4 * sizeof(float));
                    noVideoProgram.enableAttributeArray("qt_vertex");

                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                    noVideoIndexBuffer.release();
                }
                noVideoVertexBuffer.release();
            }
            noVideoProgram.release();
        }
    }
}
