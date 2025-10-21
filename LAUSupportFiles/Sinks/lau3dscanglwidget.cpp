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

#include "lau3dscanglwidget.h"
#include "laulookuptable.h"
#include "lauscan.h"
#include <locale.h>
#include <math.h>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DScanGLWidget::LAU3DScanGLWidget(LAUScan scan,  QWidget *parent) : LAUAbstractGLWidget(parent), numCols(scan.width()), numRows(scan.height()), numInds(0), playbackColor(scan.color()), textureEnableFlag(true), localTextureFlag(false), sandboxTextureFlag(false), symEnableFlag(false), flipScanFlag(false), localScan(scan), frameBufferObject(nullptr), texture(nullptr), qtDelta(LAU3DSCANGLWIDGETDELTA)
{
    // SET MINIMUM WIDGET SIZE ON SCREEN
    setMinimumWidth(320);
    setMinimumHeight(240);

    // INITIALIZE THE PROJECTION MATRIX
    prjProjection.setToIdentity();
    prjProjection(2, 2) = 0.0f;

    // CREATE A CONTEXT MENU FOR TOGGLING TEXTURE
    contextMenu = new QMenu(this);
    if (playbackColor != ColorGray && playbackColor != ColorRGB) {
        QAction *action = contextMenu->addAction(QString("Show Texture"));
        action->setCheckable(true);
        action->setChecked(textureEnableFlag);
        connect(action, SIGNAL(triggered(bool)), this, SLOT(onEnableTexture(bool)));

        action = contextMenu->addAction(QString("Flip Scan"));
        action->setCheckable(true);
        action->setChecked(flipScanFlag);
        connect(action, SIGNAL(triggered(bool)), this, SLOT(onFlipScan(bool)));
    }

    // USE THE INCOMING SCAN TO SET THE RANGE LIMITS
    setLimits(scan.minX(), scan.maxX(), scan.minY(), scan.maxY(), scan.minZ(), scan.maxZ(), scan.centroid().x(), scan.centroid().y(), scan.centroid().z());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DScanGLWidget::LAU3DScanGLWidget(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent) : LAUAbstractGLWidget(parent), numCols(cols), numRows(rows), numInds(0), playbackColor(color), textureEnableFlag(true), localTextureFlag(false), sandboxTextureFlag(false), symEnableFlag(false), flipScanFlag(false), localScan(LAUScan()), frameBufferObject(nullptr), texture(nullptr), qtDelta(LAU3DSCANGLWIDGETDELTA)
{
    // INITIALIZE THE PROJECTION MATRIX
    prjProjection.setToIdentity();
    prjProjection(2, 2) = 0.0f;

    // SET MINIMUM WIDGET SIZE ON SCREEN
    setMinimumWidth(320);
    setMinimumHeight(240);

    // CREATE A CONTEXT MENU FOR TOGGLING TEXTURE
    contextMenu = new QMenu(this);
    if (playbackColor != ColorGray && playbackColor != ColorRGB) {
        QAction *action = contextMenu->addAction(QString("Show Texture"));
        action->setCheckable(true);
        action->setChecked(textureEnableFlag);
        connect(action, SIGNAL(triggered(bool)), this, SLOT(onEnableTexture(bool)));

        action = contextMenu->addAction(QString("Flip Scan"));
        action->setCheckable(true);
        action->setChecked(flipScanFlag);
        connect(action, SIGNAL(triggered(bool)), this, SLOT(onFlipScan(bool)));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DScanGLWidget::~LAU3DScanGLWidget()
{
    if (wasInitialized()) {
        makeCurrent();
        if (localTextureFlag && texture) {
            delete texture;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DScanGLWidget::setRangeLimits(float zmn, float zmx, float hFov, float vFov)
{
    // SAVE THE FIELDS OF VIEW
    horizontalFieldOfView = hFov;
    verticalFieldOfView = vFov;

    // SET THE Z LIMITS AND CALCULATE THE FIELD OF VIEW
    zMax = -qMin(fabs(zmn), fabs(zmx));
    zMin = -qMax(fabs(zmn), fabs(zmx));
    xMax = tan(horizontalFieldOfView / 2.0f) * zMin;
    xMin = -xMax;
    yMax = tan(verticalFieldOfView / 2.0f) * zMin;
    yMin = -yMax;

    // CALL THE UNDERLYING CLASS'S SET LIMITS TO PROPERLY GENERATE THE PROJECTION MATRIX
    LAUAbstractGLWidget::setLimits(xMin, xMax, yMin, yMax, zMin, zMax);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DScanGLWidget::updateBuffer(LAUMemoryObject buffer)
{
    // UPDATE THE PLAYBACK BUFFER
    if (buffer.isValid() && wasInitialized()) {
        // MAKE THIS THE CURRENT OPENGL CONTEXT
        makeCurrent();

        // MAKE SURE WE HAVE A TEXTURE THAT CAN HANDLE THE BUFFER
        createTexture();

        // BIND THE FRAME BUFFER OBJECTS TEXTURE
        texture->bind();

        // UPLOAD THE INCOMING BUFFER TO THE GPU
        switch (playbackColor) {
            case ColorGray:
                texture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)buffer.constPointer());
                break;
            case ColorRGB:
            case ColorXYZRGB:
                texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)buffer.constPointer());
                break;
            case ColorRGBA:
            case ColorXYZG:
            case ColorXYZWRGBA:
                texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)buffer.constPointer());
                break;
            default:
                break;
        }

        // TRIGGER AN UPDATE OF THE WIDGET FOR THE USER
        update();
    } else {
        // KEEP A LOCAL CPU COPY WHILE WE WAIT FOR INITIALIZEGL TO RUN
        memcpy(localScan.constPointer(), buffer.constPointer(), qMin(localScan.length(), buffer.length()));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DScanGLWidget::updateBuffer(float *buffer)
{
    // UPDATE THE PLAYBACK BUFFER
    if (buffer && wasInitialized()) {
        // MAKE THIS THE CURRENT OPENGL CONTEXT
        makeCurrent();

        // MAKE SURE WE HAVE A TEXTURE THAT CAN HANDLE THE BUFFER
        createTexture();

        // BIND THE FRAME BUFFER OBJECTS TEXTURE
        texture->bind();

        // UPLOAD THE INCOMING BUFFER TO THE GPU
        switch (playbackColor) {
            case ColorGray:
                texture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)buffer);
                break;
            case ColorRGB:
            case ColorXYZRGB:
                texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)buffer);
                break;
            case ColorRGBA:
            case ColorXYZG:
            case ColorXYZWRGBA:
                texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)buffer);
                break;
            default:
                break;
        }

        // TRIGGER AN UPDATE OF THE WIDGET FOR THE USER
        update();
    } else {
        // KEEP A LOCAL CPU COPY WHILE WE WAIT FOR INITIALIZEGL TO RUN
        memcpy(localScan.constPointer(), (unsigned char *)buffer, localScan.length());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DScanGLWidget::updateBuffer(LAUScan scan)
{
    // UPDATE THE PLAYBACK BUFFER
    if (scan.isValid() && wasInitialized()) {
        // MAKE THIS THE CURRENT OPENGL CONTEXT
        makeCurrent();

        // MAKE SURE WE HAVE A TEXTURE THAT CAN HANDLE THE BUFFER
        createTexture();

        // BIND THE FRAME BUFFER OBJECTS TEXTURE
        texture->bind();

        // SET THE SCAN TRANSFORM
        setScanTransform(scan.transform());

        // UPLOAD THE INCOMING BUFFER TO THE GPU
        switch (scan.color()) {
            case ColorGray:
                texture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            case ColorRGB:
            case ColorXYZRGB:
                texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            case ColorRGBA:
            case ColorXYZG:
            case ColorXYZWRGBA:
                texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            default:
                break;
        }

        // TRIGGER AN UPDATE OF THE WIDGET FOR THE USER
        update();
    } else {
        // KEEP A LOCAL CPU COPY WHILE WE WAIT FOR INITIALIZEGL TO RUN
        localScan = scan;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAU3DScanGLWidget::grabMouseBuffer(GrabMouseBufferMode mode)
{
    if (texture || frameBufferObject) {
        // MAKE THE CONTEXT CURRENT
        makeCurrent();

        // CREATE A NEW FRAME BUFFER OBJECT TO HOLD THE BUFFER
        // MAKING SURE TO INCLUDE A DEPTH BUFFER FOR OCCLUDING SURFACES
        QOpenGLFramebufferObjectFormat frameBufferObjectFormat;
        frameBufferObjectFormat.setInternalTextureFormat(GL_RGBA32F);
        frameBufferObjectFormat.setAttachment(QOpenGLFramebufferObject::Depth);
        QOpenGLFramebufferObject *fbo = new QOpenGLFramebufferObject(localWidth, localHeight, frameBufferObjectFormat);

        // ENABLE THE DEPTH FILTER
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        // BIND THE GLSL PROGRAMS RESPONSIBLE FOR CONVERTING OUR FRAME BUFFER
        // OBJECT TO AN XYZ+TEXTURE POINT CLOUD FOR DISPLAY ON SCREEN
        if (fbo->bind()) {
            // SET BACKGROUND COLOR TO SENTINEL VALUE (Qt 6 on macOS may not handle NaN correctly)
            glClearColor(-1.0f, -1.0f, -1.0f, -1.0f);
            glViewport(0, 0, localWidth, localHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (program.bind()) {
                // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                if (pixelVertexBuffer.bind()) {
                    if (pixelIndexBuffer.bind()) {
                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        if (bindTexture()) {
                            program.setUniformValue("qt_texture", 0);

                            if ((playbackColor == ColorGray) || (playbackColor == ColorRGB) || (playbackColor == ColorRGBA)) {
                                glVertexAttribPointer(static_cast<unsigned int>(program.attributeLocation("qt_vertex")), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
                                program.enableAttributeArray("qt_vertex");
                                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
                            } else if ((playbackColor == ColorXYZ) || (playbackColor == ColorXYZG) || (playbackColor == ColorXYZRGB) || (playbackColor == ColorXYZWRGBA)) {
                                // SET THE PROJECTION MATRIX IN THE SHADER PROGRAM
                                program.setUniformValue("qt_projection", projection);

                                // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                                glVertexAttribPointer(static_cast<unsigned int>(program.attributeLocation("qt_vertex")), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
                                program.enableAttributeArray("qt_vertex");

                                // SET THE DELTA VALUE FOR TRIANGLE CULLING IN THE GEOMETRY SHADER
                                program.setUniformValue("qt_delta", qtDelta);

                                switch (mode) {
                                    case MouseModeRGB:
                                        program.setUniformValue("qt_color", QMatrix4x4());
                                        program.setUniformValue("qt_mode", (int)0);
                                        break;
                                    case MouseModeXYZ:
                                        program.setUniformValue("qt_color", QMatrix4x4());
                                        program.setUniformValue("qt_mode", (int)2);
                                        break;
                                    case MouseModeRowColumn:
                                        program.setUniformValue("qt_color", QMatrix4x4());
                                        program.setUniformValue("qt_mode", (int)12);
                                        break;
                                }

                                // NOW DRAW ON SCREEN USING OUR POINT CLOUD AS TRIANGLES
                                glDrawElements(GL_TRIANGLES, static_cast<int>(numInds), GL_UNSIGNED_INT, nullptr);
                            }
                        }

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        pixelIndexBuffer.release();
                    }
                    pixelVertexBuffer.release();
                }
                program.release();
            }
            fbo->release();
        }
        // FORCE ALL DRAWING COMMANDS TO EXECUTE
        glFlush();

        // NOW DOWNLOAD THE BUFFER
        LAUMemoryObject object(static_cast<unsigned int>(localWidth), static_cast<unsigned int>(localHeight), 4, sizeof(float));
        glBindTexture(GL_TEXTURE_2D, fbo->texture());
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, object.constPointer());

        // SET BACKGROUND COLOR TO DEFAULT COLOR FOR ON SCREEN UPDATES
        glClearColor(0.5f, 0.0f, 0.0f, 1.0f);

        // DELETE THE FRAME BUFFER OBJECT SINCE WE ARE DONE WITH IT
        delete fbo;

        // RETURN THE BUFFER OBJECT TO THE USER
        return (object);
    }
    return (LAUMemoryObject());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DScanGLWidget::onSetTexture(QOpenGLTexture *txt)
{
    if (txt) {
        frameBufferObject = nullptr;
        if (localTextureFlag && texture) {
            localTextureFlag = false;
            delete texture;
            texture = nullptr;
        }
        texture = txt;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DScanGLWidget::onSetTexture(QOpenGLFramebufferObject *fbo)
{
    if (fbo) {
        frameBufferObject = fbo;
        if (localTextureFlag && texture) {
            localTextureFlag = false;
            delete texture;
        }
        texture = nullptr;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DScanGLWidget::createTexture()
{
    // WE NO LONGER WANT TO KEEP TRACK OF SOMEONE ELSE'S TEXTURE
    if (localTextureFlag == false) {
        texture = nullptr;
    }

    // MAKE SURE WE HAVE A TEXTURE THAT CAN HANDLE THE BUFFER
    if (texture == nullptr || texture->width() != (int)(scale() * numCols) || texture->height() != (int)numRows) {
        // DELETE THE OLD TEXTURE IF IT WON'T WORK
        if (texture) {
            delete texture;
        }

        // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE DEPTH TO COLOR VIDEO MAPPING
        texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        if (texture) {
            texture->setSize(static_cast<int>(scale() * numCols), static_cast<int>(numRows));
            texture->setFormat(QOpenGLTexture::RGBA32F);
            texture->setWrapMode(QOpenGLTexture::ClampToBorder);
            texture->setMinificationFilter(QOpenGLTexture::Nearest);
            texture->setMagnificationFilter(QOpenGLTexture::Nearest);
            texture->allocateStorage();
        }

        // SET LOCAL TEXTURE FLAG TRUE SO WE KNOW TO DELETE IT
        localTextureFlag = true;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DScanGLWidget::initializeGL()
{
    // CALL THE UNDERLYING CLASS TO INITIALIZE THE WIDGET
    LAUAbstractGLWidget::initializeGL();

    if (isValid()) {
        // CREATE VERTEX AN INDEX BUFFERS
        if (playbackColor == ColorGray || playbackColor == ColorRGB || playbackColor == ColorRGBA) {
            // CREATE A BUFFER TO HOLD THE ROW AND COLUMN COORDINATES OF IMAGE PIXELS FOR THE TEXEL FETCHES
            pixelVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
            pixelVertexBuffer.create();
            pixelVertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
            if (pixelVertexBuffer.bind()) {
                pixelVertexBuffer.allocate(16 * sizeof(float));
                float *vertices = (float *)pixelVertexBuffer.map(QOpenGLBuffer::WriteOnly);
                if (vertices) {
                    vertices[0]  = -1.0;
                    vertices[1]  = -1.0;
                    vertices[2]  = 0.0;
                    vertices[3]  = 1.0;
                    vertices[4]  =  1.0;
                    vertices[5]  = -1.0;
                    vertices[6]  = 0.0;
                    vertices[7]  = 1.0;
                    vertices[8]  =  1.0;
                    vertices[9]  =  1.0;
                    vertices[10] = 0.0;
                    vertices[11] = 1.0;
                    vertices[12] = -1.0;
                    vertices[13] =  1.0;
                    vertices[14] = 0.0;
                    vertices[15] = 1.0;

                    pixelVertexBuffer.unmap();
                } else {
                    qDebug() << QString("Unable to map vertexBuffer from GPU.");
                }
            }

            // CREATE AN INDEX BUFFER FOR THE INCOMING DEPTH VIDEO DRAWN AS POINTS
            pixelIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
            pixelIndexBuffer.create();
            pixelIndexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
            if (pixelIndexBuffer.bind()) {
                pixelIndexBuffer.allocate(6 * sizeof(unsigned int));
                unsigned int *indices = (unsigned int *)pixelIndexBuffer.map(QOpenGLBuffer::WriteOnly);
                if (indices) {
                    indices[0] = 0;
                    indices[1] = 1;
                    indices[2] = 2;
                    indices[3] = 0;
                    indices[4] = 2;
                    indices[5] = 3;
                    pixelIndexBuffer.unmap();
                } else {
                    qDebug() << QString("indiceBuffer buffer mapped from GPU.");
                }
            }

            // SET NUMBER OF INDICES FOR RAW VIDEO
            numInds = 6;
        } else {
            // CREATE A BUFFER TO HOLD THE ROW AND COLUMN COORDINATES OF IMAGE PIXELS FOR THE TEXEL FETCHES
            pixelVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
            pixelVertexBuffer.create();
            pixelVertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
            if (pixelVertexBuffer.bind()) {
                pixelVertexBuffer.allocate(static_cast<int>(numRows * numCols * 2 * sizeof(float)));
                float *vertices = (float *)pixelVertexBuffer.map(QOpenGLBuffer::WriteOnly);
                if (vertices) {
                    for (unsigned int row = 0; row < numRows; row++) {
                        for (unsigned int col = 0; col < numCols; col++) {
                            vertices[2 * (col + row * numCols) + 0] = scale() * col;
                            vertices[2 * (col + row * numCols) + 1] = row;
                        }
                    }
                    pixelVertexBuffer.unmap();
                } else {
                    qDebug() << QString("Unable to map vertexBuffer from GPU.");
                }
            }

            // CREATE AN INDEX BUFFER FOR THE RESULTING POINT CLOUD DRAWN AS TRIANGLES
            numInds = 0;
            pixelIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
            pixelIndexBuffer.create();
            pixelIndexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
            if (pixelIndexBuffer.bind()) {
                pixelIndexBuffer.allocate(static_cast<int>(numRows * numCols * 6 * sizeof(unsigned int)));
                unsigned int *indices = (unsigned int *)pixelIndexBuffer.map(QOpenGLBuffer::WriteOnly);
                if (indices) {
                    for (unsigned int row = 0; row < numRows - 1; row++) {
                        for (unsigned int col = 0; col < numCols - 1; col++) {
                            indices[numInds++] = (row + 0) * numCols + (col + 0);
                            indices[numInds++] = (row + 0) * numCols + (col + 1);
                            indices[numInds++] = (row + 1) * numCols + (col + 1);

                            indices[numInds++] = (row + 0) * numCols + (col + 0);
                            indices[numInds++] = (row + 1) * numCols + (col + 1);
                            indices[numInds++] = (row + 1) * numCols + (col + 0);
                        }
                    }
                    pixelIndexBuffer.unmap();
                } else {
                    qDebug() << QString("Unable to map indiceBuffer from GPU.");
                }
            }
        }

        // CREATE GLSL PROGRAM FOR PROCESSING THE INCOMING VIDEO
        setlocale(LC_NUMERIC, "C");
        if (playbackColor == ColorGray) {
            if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/GRAY/displayGrayVideo.vert")) {
                qDebug() << program.log();
                close();
            } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/GRAY/displayGrayVideo.frag")) {
                qDebug() << program.log();
                close();
            } else if (!program.link()) {
                qDebug() << program.log();
                close();
            }
        } else if (playbackColor == ColorRGB || playbackColor == ColorRGBA) {
            if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/RGB/displayRGBVideo.vert")) {
                qDebug() << program.log();
                close();
            } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/RGB/displayRGBVideo.frag")) {
                qDebug() << program.log();
                close();
            } else if (!program.link()) {
                qDebug() << program.log();
                close();
            }
        } else if (playbackColor == ColorXYZG) {
            if (symEnableFlag) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/displayXYZGTextureAsPointCloudWithSymmetry.vert")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/XYZG/XYZG/displayXYZGTextureAsPointCloudWithSymmetry.geom")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/displayXYZGTextureAsPointCloudWithSymmetry.frag")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.link()) {
                    qDebug() << program.log();
                    close();
                }
            } else {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/displayXYZGTextureAsPointCloud.vert")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/XYZG/XYZG/displayXYZGTextureAsPointCloud.geom")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/displayXYZGTextureAsPointCloud.frag")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.link()) {
                    qDebug() << program.log();
                    close();
                }
            }
        } else if (playbackColor == ColorXYZRGB || playbackColor == ColorXYZWRGBA) {
            if (symEnableFlag) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZRGB/XYZRGB/displayXYZRGBTextureAsPointCloudWithSymmetry.vert")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/XYZRGB/XYZRGB/displayXYZRGBTextureAsPointCloudWithSymmetry.geom")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/displayXYZRGBTextureAsPointCloudWithSymmetry.frag")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.link()) {
                    qDebug() << program.log();
                    close();
                }
            } else {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/displayXYZRGBTextureAsPointCloud.vert")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/XYZRGB/XYZRGB/displayXYZRGBTextureAsPointCloud.geom")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/displayXYZRGBTextureAsPointCloud.frag")) {
                    qDebug() << program.log();
                    close();
                } else if (!program.link()) {
                    qDebug() << program.log();
                    close();
                }
            }
        }
        setlocale(LC_ALL, "");
    }

    // CHECK TO SEE IF WE HAVE A BUFFER TO DISPLAY
    updateBuffer(localScan);

    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DScanGLWidget::copyScan(float *buffer)
{
    // MAKE SURE WE HAVE ATLEAST ONE BINDABLE TEXTURE
    if (buffer && (texture || frameBufferObject)) {
        // SET THE GRAPHICS CARD CONTEXT TO THIS ONE
        makeCurrent();

        // BIND THE TEXTURE HOLDING THE SCAN
        if (bindTexture()) {
            glPixelStorei(GL_PACK_ALIGNMENT, 1);

            qDebug() << "LAU3DScanGLWidget::copyScan(float *buffer)";

            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            switch (color()) {
                case ColorGray:
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, (unsigned char *)buffer);
                    break;
                case ColorRGB:
                case ColorXYZ:
                case ColorXYZRGB:
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, (unsigned char *)buffer);
                    break;
                case ColorRGBA:
                case ColorXYZW:
                case ColorXYZG:
                case ColorXYZWRGBA:
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)buffer);
                    break;
                default:
                    break;
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAU3DScanGLWidget::bindTexture()
{
    if (texture) {
        boundTextureWidth = texture->width();
        boundTextureHeight = texture->height();
        texture->bind();
        return (glGetError() == GL_NO_ERROR);
    } else if (frameBufferObject) {
        boundTextureWidth = frameBufferObject->width();
        boundTextureHeight = frameBufferObject->height();
        glBindTexture(GL_TEXTURE_2D, frameBufferObject->texture());
        return (glGetError() == GL_NO_ERROR);
    }
    return (false);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DScanGLWidget::paintGL()
{
    // SET THE PROJECTION MATRIX FOR SANDBOX MODE
    if (sandboxTextureFlag){
        projection = prjProjection;
    }

    // ENABLE THE DEPTH FILTER
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    if (frameBufferObject || texture) {
        // ENABLE THE DEPTH FILTER
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        // BIND THE GLSL PROGRAMS RESPONSIBLE FOR CONVERTING OUR FRAME BUFFER
        // OBJECT TO AN XYZ+TEXTURE POINT CLOUD FOR DISPLAY ON SCREEN
        if (program.bind()) {
            glViewport(0, 0, localWidth, localHeight);
            clearGL();

            // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (pixelVertexBuffer.bind()) {
                if (pixelIndexBuffer.bind()) {
                    // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                    glActiveTexture(GL_TEXTURE0);
                    if (bindTexture()) {
                        program.setUniformValue("qt_texture", 0);

                        if ((playbackColor == ColorGray) || (playbackColor == ColorRGB) || (playbackColor == ColorRGBA)) {
                            program.setUniformValue("qt_scaleFactor", 1.0f);
                            glVertexAttribPointer(static_cast<unsigned int>(program.attributeLocation("qt_vertex")), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
                            program.enableAttributeArray("qt_vertex");
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
                        } else if ((playbackColor == ColorXYZ) || (playbackColor == ColorXYZW) || (playbackColor == ColorXYZG) || (playbackColor == ColorXYZRGB) || (playbackColor == ColorXYZWRGBA)) {
                            // SET THE PROJECTION MATRIX IN THE SHADER PROGRAM
                            if (flipScanFlag) {
                                QMatrix4x4 matrix;
                                matrix.scale(-1.0f, -1.0f, 1.0f);
                                program.setUniformValue("qt_projection", matrix * projection * scnProjection);
                            } else {
                                program.setUniformValue("qt_projection", projection * scnProjection);
                            }

                            // SET THE SYMMETRY PROJECTION MATRIX IF IN SYMMETRY MODE
                            if (symEnableFlag) {
                                program.setUniformValue("qt_symmetry", symProjection); // * scnProjection);
                            }

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(static_cast<unsigned int>(program.attributeLocation("qt_vertex")), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
                            program.enableAttributeArray("qt_vertex");

                            // SET THE DELTA VALUE FOR TRIANGLE CULLING IN THE GEOMETRY SHADER
                            program.setUniformValue("qt_color", clrTransform);
                            program.setUniformValue("qt_delta", qtDelta);
                            program.setUniformValue("qt_arg", 0);
                            if (sandboxTextureFlag){
                                if (textureEnableFlag){
                                    program.setUniformValue("qt_mode", (int)3);
                                } else {
                                    program.setUniformValue("qt_mode", (int)1);
                                }
                                program.setUniformValue("qt_arg", 2);
                                program.setUniformValue("qt_scale", (float)30.0);
                            } else {
                                program.setUniformValue("qt_mode", (int)(!textureEnableFlag));
                            }

                            // NOW DRAW ON SCREEN USING OUR POINT CLOUD AS TRIANGLES
                            glDrawElements(GL_TRIANGLES, static_cast<int>(numInds), GL_UNSIGNED_INT, nullptr);
                        }
                    }
                    // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                    pixelIndexBuffer.release();
                }
                pixelVertexBuffer.release();
            }
            program.release();
        }
    } else {
        LAUAbstractGLWidget::paintGL();
    }
    return;
}
