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

#include "lauproximityglfilter.h"

#include <locale.h>

#include <QtMath>
#include <QOpenGLContext>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUProximityGLFilter::LAUProximityGLFilter(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent) : QOpenGLContext(parent)
{
    // INITIALIZE PRIVATE VARIABLES
    numCols = cols;
    numRows = rows;
    numInds = 0;
    playbackColor = color;
    numItrs = MAXNUMBERITERATIONS;

    // INITIALIZE THE TEXTURE POINTERS TO NULL
    textureScan = NULL;
    for (int n = 0; n < MAXNUMBERITERATIONS; n++) {
        frameBufferObjectsA[n] = NULL;
        frameBufferObjectsB[n] = NULL;
    }
    frameBufferObjectsC = NULL;

    // CALCULATE THE SIZE OF THE VOXEL MAP
    unsigned int fboPxels = (unsigned int)pow(3.0, (double)numItrs);
    fboWidth  = fboPxels;
    fboHeight = fboPxels * fboPxels;

    // SEE IF THE USER GAVE US A TARGET SURFACE, IF NOT, THEN CREATE AN OFFSCREEN SURFACE BY DEFAULT
    surface = new QOffscreenSurface();
    ((QOffscreenSurface *)surface)->create();

    // NOW SEE IF WE HAVE A VALID PROCESSING CONTEXT FROM THE USER, AND THEN SPIN IT INTO ITS OWN THREAD
    this->setFormat(surface->format());
    this->create();
    this->initialize();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUProximityGLFilter::LAUProximityGLFilter(LAUScan scan, unsigned int itrs, QWidget *parent) : QOpenGLContext(parent)
{
    // INITIALIZE PRIVATE VARIABLES
    numCols = scan.width();
    numRows = scan.height();
    numInds = 0;
    playbackColor = scan.color();
    numItrs = qMin(itrs, (unsigned int)MAXNUMBERITERATIONS);

    // INITIALIZE THE TEXTURE POINTERS TO NULL
    textureScan = NULL;
    for (int n = 0; n < MAXNUMBERITERATIONS; n++) {
        frameBufferObjectsA[n] = NULL;
        frameBufferObjectsB[n] = NULL;
    }
    frameBufferObjectsC = NULL;

    // CALCULATE THE SIZE OF THE VOXEL MAP
    unsigned int fboPxels = (unsigned int)pow(3.0, (double)numItrs);
    fboWidth  = fboPxels;
    fboHeight = fboPxels * fboPxels;

    // SEE IF THE USER GAVE US A TARGET SURFACE, IF NOT, THEN CREATE AN OFFSCREEN SURFACE BY DEFAULT
    surface = new QOffscreenSurface();
    ((QOffscreenSurface *)surface)->create();

    // NOW SEE IF WE HAVE A VALID PROCESSING CONTEXT FROM THE USER, AND THEN SPIN IT INTO ITS OWN THREAD
    this->setFormat(surface->format());
    this->create();
    this->initialize();

    this->onUpdateToScan(scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUProximityGLFilter::~LAUProximityGLFilter()
{
    if (surface && makeCurrent(surface)) {
        for (int n = 0; n < MAXNUMBERITERATIONS; n++) {
            if (frameBufferObjectsA[n]) {
                delete frameBufferObjectsA[n];
            }
            if (frameBufferObjectsB[n]) {
                delete frameBufferObjectsB[n];
            }
        }
        if (frameBufferObjectsC) {
            delete frameBufferObjectsC;
        }
        if (textureScan) {
            delete textureScan;
        }
        if (wasInitialized()) {
            vertexArrayObject.release();
        }
        doneCurrent();

        delete surface;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUProximityGLFilter::initialize()
{
    if (makeCurrent(surface)) {
        initializeOpenGLFunctions();
        glClearColor(-1.0f, -1.0f, -1.0f, -1.0f);

        // get context opengl-version
        qDebug() << "Really used OpenGl: " << format().majorVersion() << "." << format().minorVersion();
        qDebug() << "OpenGl information: VENDOR:       " << (const char *)glGetString(GL_VENDOR);
        qDebug() << "                    RENDERDER:    " << (const char *)glGetString(GL_RENDERER);
        qDebug() << "                    VERSION:      " << (const char *)glGetString(GL_VERSION);
        qDebug() << "                    GLSL VERSION: " << (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);

        initializeVertices();
        initializeTextures();
        initializeShaders();

        // RELEASE THIS CONTEXT AS THE CURRENT GL CONTEXT
        doneCurrent();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUProximityGLFilter::initializeVertices()
{
    // CREATE THE VERTEX ARRAY OBJECT FOR FEEDING VERTICES TO OUR SHADER PROGRAMS
    vertexArrayObject.create();
    vertexArrayObject.bind();

    // CREATE A BUFFER TO HOLD THE ROW AND COLUMN COORDINATES OF IMAGE PIXELS FOR THE TEXEL FETCHES
    vertexBufferA = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vertexBufferA.create();
    vertexBufferA.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (vertexBufferA.bind()) {
        vertexBufferA.allocate(numRows * numCols * 2 * sizeof(float));
        float *vertices = (float *)vertexBufferA.map(QOpenGLBuffer::WriteOnly);
        if (vertices) {
            if (playbackColor == ColorXYZRGB || playbackColor == ColorXYZWRGBA) {
                for (unsigned int row = 0; row < numRows; row++) {
                    for (unsigned int col = 0; col < numCols; col++) {
                        vertices[2 * (col + row * numCols) + 0] = 2 * col;
                        vertices[2 * (col + row * numCols) + 1] =   row;
                    }
                }
            } else {
                for (unsigned int row = 0; row < numRows; row++) {
                    for (unsigned int col = 0; col < numCols; col++) {
                        vertices[2 * (col + row * numCols) + 0] = col;
                        vertices[2 * (col + row * numCols) + 1] = row;
                    }
                }
            }
            vertexBufferA.unmap();
        } else {
            qDebug() << QString("Unable to map vertexBufferA from GPU.");
        }
    }

    // CREATE AN INDEX BUFFER FOR THE RESULTING POINT CLOUD DRAWN AS TRIANGLES
    indexBufferA = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    indexBufferA.create();
    indexBufferA.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (indexBufferA.bind()) {
        indexBufferA.allocate(numRows * numCols * sizeof(unsigned int));
        unsigned int *indices = (unsigned int *)indexBufferA.map(QOpenGLBuffer::WriteOnly);
        if (indices) {
            for (unsigned int row = 0; row < numRows; row++) {
                for (unsigned int col = 0; col < numCols; col++) {
                    indices[numInds] = numInds;
                    numInds++;
                }
            }
            indexBufferA.unmap();
        } else {
            qDebug() << QString("Unable to map indexBufferA from GPU.");
        }
    }

    // CREATE A BUFFER TO HOLD THE ROW AND COLUMN COORDINATES OF IMAGE PIXELS FOR THE TEXEL FETCHES
    vertexBufferB = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vertexBufferB.create();
    vertexBufferB.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (vertexBufferB.bind()) {
        vertexBufferB.allocate(16 * sizeof(float));
        float *vertices = (float *)vertexBufferB.map(QOpenGLBuffer::WriteOnly);
        if (vertices) {
            vertices[0]  = -1.0;
            vertices[1]  = -1.0;
            vertices[2]  = 0.0;
            vertices[3]  = 1.0;
            vertices[4]  = +1.0;
            vertices[5]  = -1.0;
            vertices[6]  = 0.0;
            vertices[7]  = 1.0;
            vertices[8]  = +1.0;
            vertices[9]  = +1.0;
            vertices[10] = 0.0;
            vertices[11] = 1.0;
            vertices[12] = -1.0;
            vertices[13] = +1.0;
            vertices[14] = 0.0;
            vertices[15] = 1.0;

            vertexBufferB.unmap();
        } else {
            qDebug() << QString("Unable to map vertexBufferB from GPU.");
        }
        vertexBufferB.release();
    }

    // CREATE AN INDEX BUFFER FOR THE INCOMING DEPTH VIDEO DRAWN AS POINTS
    // CREATE AN INDEX BUFFER FOR THE INCOMING DEPTH VIDEO DRAWN AS POINTS
    indexBufferB = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    indexBufferB.create();
    indexBufferB.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (indexBufferB.bind()) {
        indexBufferB.allocate(6 * sizeof(unsigned int));
        unsigned int *indices = (unsigned int *)indexBufferB.map(QOpenGLBuffer::WriteOnly);
        if (indices) {
            indices[0] = 0;
            indices[1] = 1;
            indices[2] = 2;
            indices[3] = 0;
            indices[4] = 2;
            indices[5] = 3;
            indexBufferB.unmap();
        } else {
            qDebug() << QString("indexBufferB buffer mapped from GPU.");
        }
        indexBufferB.release();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUProximityGLFilter::initializeShaders()
{
    // CREATE GLSL PROGRAM FOR PROCESSING THE INCOMING VIDEO
    setlocale(LC_NUMERIC, "C");

    programA.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/VOXELMAP/VoxelMapFilters/filterVoxelMapA.vert");
    programA.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/VOXELMAP/VoxelMapFilters/filterVoxelMapA.frag");
    programA.link();

    programB.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/VOXELMAP/VoxelMapFilters/filterVoxelMapB.vert");
    programB.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/VOXELMAP/VoxelMapFilters/filterVoxelMapB.frag");
    programB.link();

    programC.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/VOXELMAP/VoxelMapFilters/filterVoxelMapC.vert");
    programC.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/VOXELMAP/VoxelMapFilters/filterVoxelMapC.frag");
    programC.link();

    setlocale(LC_ALL, "");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUProximityGLFilter::initializeTextures()
{
    // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE INCOMING VIDEO
    textureScan = new QOpenGLTexture(QOpenGLTexture::Target2D);
    switch (playbackColor) {
        case ColorGray:
            textureScan->setSize(numCols, numRows);
            textureScan->setFormat(QOpenGLTexture::R32F);
            break;
        case ColorRGB:
        case ColorXYZ:
            textureScan->setSize(numCols, numRows);
            textureScan->setFormat(QOpenGLTexture::RGB32F);
            break;
        case ColorXYZRGB:
            textureScan->setSize(2 * numCols, numRows);
            textureScan->setFormat(QOpenGLTexture::RGB32F);
            break;
        case ColorRGBA:
        case ColorXYZW:
        case ColorXYZG:
            textureScan->setSize(numCols, numRows);
            textureScan->setFormat(QOpenGLTexture::RGBA32F);
            break;
        case ColorXYZWRGBA:
            textureScan->setSize(2 * numCols, numRows);
            textureScan->setFormat(QOpenGLTexture::RGBA32F);
            break;
        default:
            break;
    }
    textureScan->setWrapMode(QOpenGLTexture::ClampToBorder);
    textureScan->setMinificationFilter(QOpenGLTexture::Nearest);
    textureScan->setMagnificationFilter(QOpenGLTexture::Nearest);
    textureScan->allocateStorage();

    // CREATE A FORMAT OBJECT FOR CREATING THE FRAME BUFFER
    QOpenGLFramebufferObjectFormat frameBufferObjectFormat;
    frameBufferObjectFormat.setInternalTextureFormat(GL_RGBA32F);

    for (unsigned int n = 0; n < numItrs; n++) {
        int dlta = (int)qPow(3.0, (double)(n));
        fboWidth = dlta;
        fboHeight = dlta * dlta;

        frameBufferObjectsA[n] = new QOpenGLFramebufferObject(QSize(fboWidth, fboHeight), frameBufferObjectFormat);
        frameBufferObjectsA[n]->setAttachment(QOpenGLFramebufferObject::Depth);
        frameBufferObjectsA[n]->release();

        frameBufferObjectsB[n] = new QOpenGLFramebufferObject(QSize(fboWidth, fboHeight), frameBufferObjectFormat);
        frameBufferObjectsB[n]->release();
    }

    //  CREATE THE BUFFER TO HOLD THE PROXIMITY MAP
    frameBufferObjectsC = new QOpenGLFramebufferObject(QSize(numCols, numRows), frameBufferObjectFormat);
    frameBufferObjectsC->release();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUProximityGLFilter::onUpdateToScan(LAUScan scan)
{
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        switch (playbackColor) {
            case ColorGray:
                textureScan->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            case ColorRGB:
            case ColorXYZ:
            case ColorXYZRGB:
                textureScan->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            case ColorRGBA:
            case ColorXYZW:
            case ColorXYZG:
            case ColorXYZWRGBA:
                textureScan->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            default:
                break;
        }

        // ENABLE THE DEPTH FILTER
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING
        for (unsigned int n = 0; n < numItrs; n++) {
            if (frameBufferObjectsA[n] && frameBufferObjectsA[n]->bind()) {
                if (programA.bind()) {
                    // CLEAR THE FRAME BUFFER OBJECT
                    glViewport(0, 0, frameBufferObjectsA[n]->width(), frameBufferObjectsA[n]->height());
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glPointSize(1.0f);

                    // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                    if (vertexBufferA.bind()) {
                        if (indexBufferA.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, frameBufferObjectsB[n]->texture());
                            programB.setUniformValue("qt_coordTexture", 0);

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE1);
                            textureScan->bind();
                            programA.setUniformValue("qt_scanTexture",   1);

                            // DERIVE THE TRANSFORM FROM SCAN SPACE TO VOXEL MAP SPACE
                            float x = qMin(scan.maxX(), scan.minX());
                            float y = qMin(scan.maxY(), scan.minY());
                            float z = qMin(scan.maxZ(), scan.minZ());

                            float dx = qMax(scan.maxX(), scan.minX()) - x;
                            float dy = qMax(scan.maxY(), scan.minY()) - y;
                            float dz = qMax(scan.maxZ(), scan.minZ()) - z;
                            float da = qMax(dx, qMax(dy, dz));

                            x  -= 0.02f * da;
                            y  -= 0.02f * da;
                            z  -= 0.02f * da;
                            da  = (float)frameBufferObjectsA[n]->width() / (1.04f * da);

                            transform = QMatrix4x4(da, 0.0f, 0.0f, -x * da,
                                                   0.0f,   da, 0.0f, -y * da,
                                                   0.0f, 0.0f,   da, -z * da,
                                                   0.0f, 0.0f, 0.0f,  1.0f);

                            // PASS THE TRANSFORM OVER TO THE SHADER AS A UNIFORM VARIABLE
                            programA.setUniformValue("qt_transform", transform);
                            programA.setUniformValue("qt_width", frameBufferObjectsA[n]->width());

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(programA.attributeLocation("qt_vertex"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
                            programA.enableAttributeArray("qt_vertex");
                            glDrawElements(GL_POINTS, numInds, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBufferA.release();
                        }
                        vertexBufferA.release();
                    }
                    programA.release();
                }
                frameBufferObjectsA[n]->release();
            }
        }

        // NOW POPULATE THE FRAME BUFFERS ON THE WAY DOWN
        for (unsigned int n = 0; n < numItrs; n++) {
            if (frameBufferObjectsB[n] && frameBufferObjectsB[n]->bind()) {
                if (programB.bind()) {
                    // CLEAR THE FRAME BUFFER OBJECT
                    glViewport(0, 0, frameBufferObjectsB[n]->width(), frameBufferObjectsB[n]->height());
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                    if (vertexBufferB.bind()) {
                        if (indexBufferB.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE1);
                            glBindTexture(GL_TEXTURE_2D, frameBufferObjectsA[n]->texture());
                            programB.setUniformValue("qt_textureA", 1);

                            if (n == 0) {
                                // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                                programB.setUniformValue("qt_textureB", 1);
                            } else {
                                // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                                glActiveTexture(GL_TEXTURE2);
                                glBindTexture(GL_TEXTURE_2D, frameBufferObjectsB[n - 1]->texture());
                                programB.setUniformValue("qt_textureB", 2);
                            }

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(programB.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            programB.enableAttributeArray("qt_vertex");
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBufferB.release();
                        }
                        vertexBufferB.release();
                    }
                    programB.release();
                }
                frameBufferObjectsB[n]->release();
            }
        }

        // RELEASE GLCONTEXT
        doneCurrent();
    }
    emit emitToScan(scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUProximityGLFilter::onUpdateFmScan(LAUScan scan)
{
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        switch (playbackColor) {
            case ColorGray:
                textureScan->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            case ColorRGB:
            case ColorXYZ:
            case ColorXYZRGB:
                textureScan->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            case ColorRGBA:
            case ColorXYZW:
            case ColorXYZG:
            case ColorXYZWRGBA:
                textureScan->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            default:
                break;
        }

        // NOW POPULATE THE FRAME BUFFERS ON THE WAY DOWN
        if (frameBufferObjectsC && frameBufferObjectsC->bind()) {
            if (programC.bind()) {
                // CLEAR THE FRAME BUFFER OBJECT
                glViewport(0, 0, numCols, numRows);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                if (vertexBufferB.bind()) {
                    if (indexBufferB.bind()) {
                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        textureScan->bind();
                        programC.setUniformValue("qt_scanTexture", 0);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE4);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectsB[numItrs - 1]->texture());
                        programC.setUniformValue("qt_mapTexture", 4);

                        // SET THE COLUMN STEP SIDE FOR TEXEL FETCHES SO WE CAN HANDLE DIFFERENT COLOR SCANS
                        if (playbackColor == ColorXYZ || playbackColor == ColorXYZG || playbackColor == ColorXYZW) {
                            programC.setUniformValue("qt_step", (int)1);
                        } else {
                            programC.setUniformValue("qt_step", (int)2);
                        }

                        qDebug() << transform(0, 0) << transform(0, 1) << transform(0, 2) << transform(0, 3);
                        qDebug() << transform(1, 0) << transform(1, 1) << transform(1, 2) << transform(1, 3);
                        qDebug() << transform(2, 0) << transform(2, 1) << transform(2, 2) << transform(2, 3);
                        qDebug() << transform(3, 0) << transform(3, 1) << transform(3, 2) << transform(3, 3);

                        // PASS THE TRANSFORM OVER TO THE SHADER AS A UNIFORM VARIABLE
                        programC.setUniformValue("qt_width", frameBufferObjectsB[numItrs - 1]->width());
                        programC.setUniformValue("qt_transform", transform);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programC.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programC.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBufferB.release();
                    }
                    vertexBufferB.release();
                }
                programC.release();
            }
            frameBufferObjectsC->release();
        }

        // RELEASE GLCONTEXT
        doneCurrent();
    }
    emit emitFmScan(scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUProximityGLFilter::grabScan(float *buffer)
{
    if (buffer) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        makeCurrent(surface);
        glBindTexture(GL_TEXTURE_2D, frameBufferObjectsC->texture());
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)buffer);
        doneCurrent();
    }
}
