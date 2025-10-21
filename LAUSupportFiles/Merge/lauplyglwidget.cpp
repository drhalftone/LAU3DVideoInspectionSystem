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

#include "lauplyglwidget.h"
#include <locale.h>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUPlyGLWidget::setLimits(float xmn, float xmx, float ymn, float ymx, float zmn, float zmx)
{
    // SET THE Z LIMITS AND CALCULATE THE FIELD OF VIEW
    xMin = qMin(xmn, xmx);
    xMax = qMax(xmn, xmx);
    yMin = qMin(ymn, ymx);
    yMax = qMax(ymn, ymx);
    zMin = qMin(zmn, zmx);
    zMax = qMax(zmn, zmx);

    // CALCULATE FOV
    float   phiA = atan(yMin / zMin);
    float   phiB = atan(yMax / zMin);
    float thetaA = atan(xMin / zMin);
    float thetaB = atan(xMax / zMin);

    horizontalFieldOfView = fabs(thetaA) + fabs(thetaB);
    verticalFieldOfView = fabs(phiA) + fabs(phiB);

    // MAKE SURE WE HAVE ALREADY INITIALIZED THIS GLWIDGET BEFORE TELLING IT TO UPDATE ITSELF
    if (wasInitialized()) {
        updateProjectionMatrix();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUPlyGLWidget::wheelEvent(QWheelEvent *event)
{
    // CHANGE THE ZOOM FACTOR BASED ON HOW MUCH WHEEL HAS MOVED
    zoomFactor *= (1.0 + (float)event->angleDelta().y() / 160.0);
    zoomFactor = qMax(0.10f, qMin(zoomFactor, 10.0f));

    // UPDATE THE PROJECTION MATRIX SINCE WE CHANGED THE ZOOM FACTOR
    updateProjectionMatrix();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUPlyGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = lastPos.x() - event->x();
    int dy = event->y() - lastPos.y();

    if (event->buttons() & Qt::LeftButton) {
        xRot = xRot + 4 * dy; //qMin(qMax(xRot + 4 * dy, -1200), 1200);
        yRot = yRot + 4 * dx; //qMin(qMax(yRot + 4 * dx, -1200), 1200);
        zRot = zRot + 0;      //qMin(qMax(zRot + 0, -1200), 1200);
    }
    lastPos = event->pos();

    // UPDATE THE PROJECTION MATRIX SINCE WE CHANGED THE ROTATION ANGLE
    updateProjectionMatrix();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUPlyGLWidget::mouseDoubleClickEvent(QMouseEvent *)
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
void LAUPlyGLWidget::updateProjectionMatrix()
{
    float aspectRatio = (float)width() / (float)height();
    float xCenter = (xMin + xMax) / 2.0f;
    float yCenter = (yMin + yMax) / 2.0f;
    float zCenter = (zMin + zMax) / 2.0f;

    QMatrix4x4 eyeTransform;
    eyeTransform.setToIdentity();
    eyeTransform.translate(xCenter, yCenter, zCenter);
    eyeTransform.rotate(-xRot / 16.0f, 1.0f, 0.0f, 0.0f);
    eyeTransform.rotate(yRot / 16.0f, 0.0f, 1.0f, 0.0f);
    eyeTransform.translate(-xCenter, -yCenter, -zCenter);

    QVector4D eye = eyeTransform * QVector4D(0.0f, 0.0f, 0.0f, 1.0f);

    float fov   = qMin(120.0f, qMax(zoomFactor * verticalFieldOfView, 0.5f));
    float zNear = qMin(qAbs(zMin), qAbs(zMax));
    float zFar  = qMax(qAbs(zMin), qAbs(zMax));

    // INITIALIZE THE PROJECTION MATRIX TO IDENTITY
    projection.setToIdentity();
    projection.perspective(fov, aspectRatio, zNear / 4.0f, 3.0f * zFar);
    projection.lookAt(QVector3D(eye), QVector3D(xCenter, yCenter, zCenter), QVector3D(0.0f, 1.0f, 0.0f));

    // UPDATE THE DISPLAY
    update();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUPlyGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.5f, 0.0f, 0.0f, 1.0f);

    // GET CONTEXT OPENGL-VERSION
    qDebug() << "Really used OpenGl: " << format().majorVersion() << "." << format().minorVersion();
    qDebug() << "OpenGl information: VENDOR:       " << reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    qDebug() << "                    RENDERDER:    " << reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    qDebug() << "                    VERSION:      " << reinterpret_cast<const char *>(glGetString(GL_VERSION));
    qDebug() << "                    GLSL VERSION: " << reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    // CREATE SHADER
    setlocale(LC_NUMERIC, "C");
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/PLY/PLY/plyFileShader.vert")) {
        close();
    }
    if (!program.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/PLY/PLY/plyFileShader.geom")) {
        close();
    }
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/PLY/PLY/plyFileShader.frag")) {
        close();
    }
    if (!program.link()) {
        close();
    }
    setlocale(LC_ALL, "");

    // CREATE THE VERTEX ARRAY OBJECT FOR FEEDING VERTICES TO OUR SHADER PROGRAMS
    vertexArrayObject.create();
    vertexArrayObject.bind();

    if (plyObject.isValid()) {
        // CREATE A BUFFER TO HOLD THE PLY OBJECTS VERTICES
        vertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        vertexBuffer.create();
        vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        if (vertexBuffer.bind()) {
            vertexBuffer.allocate(plyObject.constVertex(), plyObject.verticeLength());
        }

        // CREATE AN INDEX BUFFER FOR THE RESULTING POINT CLOUD DRAWN AS TRIANGLES
        indiceBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        indiceBuffer.create();
        indiceBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        if (indiceBuffer.bind()) {
            indiceBuffer.allocate(plyObject.constIndex(), plyObject.indiceLength());
        }

        if (plyObject.isValid()) {
            setLimits(plyObject.minX(), plyObject.maxX(), plyObject.minY(), plyObject.maxY(), plyObject.minZ(), plyObject.maxZ());
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUPlyGLWidget::resizeGL(int w, int h)
{
    // Get the Desktop Widget so that we can get information about multiple monitors connected to the system.
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
void LAUPlyGLWidget::paintGL()
{
    // SET THE VIEW PORT AND CLEAR THE SCREEN BUFFER
    glViewport(0, 0, localWidth, localHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // MAKE SURE WE HAVE A TEXTURE TO SHOW
    if (plyObject.isValid()) {
        if (program.bind()) {
            if (vertexBuffer.bind()) {
                if (indiceBuffer.bind()) {
                    // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                    program.setAttributeBuffer("qt_vertex", GL_FLOAT, 0, 4, 8 * sizeof(float));
                    program.enableAttributeArray("qt_vertex");

                    // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE TEXTURE DATA
                    program.setAttributeBuffer("qt_texture", GL_FLOAT, 4 * sizeof(float), 4, 8 * sizeof(float));
                    program.enableAttributeArray("qt_texture");

                    // SET THE PROJECTION MATRIX FOR MAPPING COORDINATES TO SCREEN
                    program.setUniformValue("qt_projection", projection);
                    program.setUniformValue("qt_delta", 100.0f);
                    program.setUniformValue("qt_mode", (int)enableTextureFlag);

                    // DRAW THE OBJECT AS TRIANGLES
                    glDrawElements(GL_TRIANGLES, plyObject.indices(), GL_UNSIGNED_INT, 0);

                    indiceBuffer.release();
                }
                vertexBuffer.release();
            }
            program.release();
        }
    }
}
