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

#include "lauabstractfilter.h"
#include <locale.h>

LAUMemoryObject localFrameBufferObject;

float nanOpenColor[4] = { NAN, NAN, NAN, NAN };

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUAbstractFilterController::LAUAbstractFilterController(LAUAbstractGLFilter *contxt, QSurface *srfc, QObject *parent) : QObject(parent), localContext(contxt), localFilter(nullptr), localSurface(false), surface(srfc), thread(nullptr)
{
    // NOW SEE IF WE HAVE A VALID PROCESSING CONTEXT FROM THE USER, AND THEN SPIN IT INTO ITS OWN THREAD
    if (localContext) {
        // SEE IF THE USER GAVE US A TARGET SURFACE, IF NOT, THEN CREATE AN OFFSCREEN SURFACE BY DEFAULT
        if (surface == nullptr) {
            surface = new QOffscreenSurface();
            ((QOffscreenSurface *)surface)->create();
            localSurface = true;
        }

        localContext->setFormat(surface->format());
        localContext->setSurface(surface);
        localContext->create();
        localContext->initialize();
        if (localContext->isValid()) {
            // HAVE THE DESTRUCTION OF THE LOCALSURFACE TRIGGER THE DESTRUCTION OF THE OFFSCREEN SURFACE
            if (localSurface) {
                connect(localContext, SIGNAL(destroyed()), ((QOffscreenSurface *)surface), SLOT(deleteLater()));
            }

            // CREATE A THREAD TO HOST THE LOCAL CONTEXT FILTER
            thread = new QThread();
            connect(thread, SIGNAL(started()), localContext, SLOT(onStart()));
            connect(thread, SIGNAL(finished()), localContext, SLOT(onFinish()));
            connect(thread, SIGNAL(finished()), localContext, SLOT(deleteLater()));
            connect(localContext, SIGNAL(destroyed()), thread, SLOT(deleteLater()));
            localContext->moveToThread(thread);
            thread->start();
        }
    }
}
#endif
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUAbstractFilterController::LAUAbstractFilterController(LAUAbstractFilter *fltr, QObject *parent) : QObject(parent), localContext(nullptr), localFilter(fltr), localSurface(false), surface(nullptr)
#ifndef HEADLESS
    , thread(nullptr)
#endif
{
    if (localFilter) {
        thread = new QThread();
        connect(thread, SIGNAL(started()), localFilter, SLOT(onStart()));
        connect(thread, SIGNAL(finished()), localFilter, SLOT(onFinish()));
        connect(thread, SIGNAL(finished()), localFilter, SLOT(deleteLater()));
        connect(localFilter, SIGNAL(destroyed()), thread, SLOT(deleteLater()));
        localFilter->moveToThread(thread);
        thread->start();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUAbstractFilterController::~LAUAbstractFilterController()
{
    //if (localContext) {
    //    localContext->deleteLater();
    //}

    if (thread) {
        thread->quit();
        //while (thread->isRunning()) {
        //    if (qApp) {
        //        qApp->processEvents();
        //    }
        //}
        //delete thread;
    }

    //if (localFilter) {
    //    delete localFilter;
    //}

    qDebug() << "LAUAbstractFilterController::~LAUAbstractFilterController()";
}

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLFilter::initialize()
{
    // IF WE ARE SHARING OUR CONTEXT, THEN WE CAN ASSUME THAT THE CONTEXT IS ALREADY CURRENT
    // OTHERWISE WE NEED TO MAKE OURSELVES THE CURRENT CONTEXT
    if (surface && makeCurrent(surface)) {
        // SET THE FLAG SO THAT WE KNOW THE SURFACE IS VALID
        surfaceIsValid = true;

        // SET UP CONNECTIONS BETWEEN QT OPENGL CLASSES AND UNDERLYING OPENGL DRIVERS
        initializeOpenGLFunctions();

        // GET CONTEXT OPENGL-VERSION
        qDebug() << "void LAUAbstractGLFilter::initialize()";
        qDebug() << "Really used OpenGl: " << format().majorVersion() << "." << format().minorVersion();
        qDebug() << "OpenGl information: VENDOR:       " << reinterpret_cast<const char *>(glGetString(GL_VENDOR));
        qDebug() << "                    RENDERDER:    " << reinterpret_cast<const char *>(glGetString(GL_RENDERER));
        qDebug() << "                    VERSION:      " << reinterpret_cast<const char *>(glGetString(GL_VERSION));
        qDebug() << "                    GLSL VERSION: " << reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));

        // BIND THE VERTEX ARRAY OBJECT
        vertexArrayObject.create();
        vertexArrayObject.bind();

        // CREATE A BUFFER TO HOLD THE ROW AND COLUMN COORDINATES OF IMAGE PIXELS FOR THE TEXEL FETCHES
        quadVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        quadVertexBuffer.create();
        quadVertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        if (quadVertexBuffer.bind()) {
            quadVertexBuffer.allocate(16 * sizeof(float));
            float *vertices = (float *)quadVertexBuffer.map(QOpenGLBuffer::WriteOnly);
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

                quadVertexBuffer.unmap();
            } else {
                qDebug() << QString("Unable to map quadVertexBuffer from GPU.");
            }
        }

        // CREATE AN INDEX BUFFER FOR THE INCOMING DEPTH VIDEO DRAWN AS POINTS
        quadIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        quadIndexBuffer.create();
        quadIndexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        if (quadIndexBuffer.bind()) {
            quadIndexBuffer.allocate(6 * sizeof(unsigned int));
            unsigned int *indices = (unsigned int *)quadIndexBuffer.map(QOpenGLBuffer::WriteOnly);
            if (indices) {
                indices[0] = 0;
                indices[1] = 1;
                indices[2] = 2;
                indices[3] = 0;
                indices[4] = 2;
                indices[5] = 3;
                quadIndexBuffer.unmap();
            } else {
                qDebug() << QString("quadIndexBuffer buffer mapped from GPU.");
            }
        }

        // CREATE TEXTURE TO HOLD ANY AVAILABLE PHASE CORRECTION TABLE
        texturePhaseCorrection = new QOpenGLTexture(QOpenGLTexture::Target1D);
        texturePhaseCorrection->setSize(LENGTHPHASECORRECTIONTABLE);
        texturePhaseCorrection->setFormat(QOpenGLTexture::R32F);
        texturePhaseCorrection->setWrapMode(QOpenGLTexture::ClampToEdge);
        texturePhaseCorrection->setMinificationFilter(QOpenGLTexture::Linear);
        texturePhaseCorrection->setMagnificationFilter(QOpenGLTexture::Linear);
        texturePhaseCorrection->allocateStorage();

        // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE DEPTH VIDEO
        if (playbackColor == ColorXYZ || playbackColor == ColorXYZG || playbackColor == ColorXYZRGB || playbackColor == ColorXYZWRGBA) {
            if (playbackDevice == DevicePrimeSense || playbackDevice == DeviceKinect || playbackDevice == DeviceOrbbec || playbackDevice == DeviceLucid || playbackDevice == DeviceVZense || playbackDevice == DeviceVidu || playbackDevice == DeviceRealSense) {
                textureDepth = new QOpenGLTexture(QOpenGLTexture::Target2D);
                textureDepth->setSize(numDepthCols / 4, numDepthRows);
                textureDepth->setFormat(QOpenGLTexture::RGBA32F);
            } else if (playbackDevice == DeviceProsilicaLCG) {
                textureDepth = new QOpenGLTexture(QOpenGLTexture::Target2D);
                textureDepth->setSize(3 * numDepthCols, numDepthRows);
                textureDepth->setFormat(QOpenGLTexture::RGBA32F);
            } else if (playbackDevice == DeviceProsilicaTOF) {
                textureDepth = new QOpenGLTexture(QOpenGLTexture::Target2D);
                textureDepth->setSize(3 * numDepthCols, numDepthRows);
                textureDepth->setFormat(QOpenGLTexture::RGBA32F);
            } else if (playbackDevice == DeviceProsilicaDPR) {
                textureDepth = new QOpenGLTexture(QOpenGLTexture::Target2D);
                textureDepth->setSize(3 * numDepthCols, numDepthRows);
                textureDepth->setFormat(QOpenGLTexture::RGBA32F);
            } else if (playbackDevice == DeviceProsilicaAST) {
                textureDepth = new QOpenGLTexture(QOpenGLTexture::Target3D);
                textureDepth->setSize(3 * numDepthCols, numDepthRows, 2);
                textureDepth->setFormat(QOpenGLTexture::RGBA32F);
            } else if (playbackDevice == DeviceProsilicaIOS) {
                textureDepth = new QOpenGLTexture(QOpenGLTexture::Target2D);
                textureDepth->setSize(2 * numDepthCols, numDepthRows);
                textureDepth->setFormat(QOpenGLTexture::RGBA32F);
            }

            if (textureDepth) {
                textureDepth->setWrapMode(QOpenGLTexture::ClampToBorder);
                textureDepth->setMinificationFilter(QOpenGLTexture::Nearest);
                textureDepth->setMagnificationFilter(QOpenGLTexture::Nearest);
                textureDepth->allocateStorage();
            }
        }

        // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE COLOR VIDEO
        if (playbackDevice == DevicePrimeSense || playbackDevice == DeviceKinect || playbackDevice == DeviceLucid || playbackDevice == DeviceOrbbec || playbackDevice == DeviceVZense || playbackDevice == DeviceVidu || playbackDevice == DeviceRealSense || playbackDevice == LAU3DVideoParameters::DeviceSeek) {
            if (playbackColor == ColorGray || playbackColor == ColorXYZG || playbackColor == ColorXYZ || playbackColor == ColorXYZW) {
                textureColor = new QOpenGLTexture(QOpenGLTexture::Target2D);
                textureColor->setSize(numColorCols, numColorRows);
                textureColor->setFormat(QOpenGLTexture::R32F);
                textureColor->setWrapMode(QOpenGLTexture::ClampToBorder);
                textureColor->setMinificationFilter(QOpenGLTexture::Nearest);
                textureColor->setMagnificationFilter(QOpenGLTexture::Nearest);
                textureColor->allocateStorage();
            } else {
                textureColor = new QOpenGLTexture(QOpenGLTexture::Target2D);
                textureColor->setSize(numColorCols, numColorRows);
                textureColor->setFormat(QOpenGLTexture::RGB32F);
                textureColor->setWrapMode(QOpenGLTexture::ClampToBorder);
                textureColor->setMinificationFilter(QOpenGLTexture::Nearest);
                textureColor->setMagnificationFilter(QOpenGLTexture::Nearest);
                textureColor->allocateStorage();
            }
        } else {
            if (playbackColor == ColorGray || playbackColor == ColorXYZG || playbackColor == ColorXYZ || playbackColor == ColorXYZW) {
                textureColor = new QOpenGLTexture(QOpenGLTexture::Target2D);
                textureColor->setMinificationFilter(QOpenGLTexture::Nearest);
                textureColor->setMagnificationFilter(QOpenGLTexture::Nearest);
                textureColor->setWrapMode(QOpenGLTexture::ClampToBorder);
                textureColor->setFormat(QOpenGLTexture::R32F);
                textureColor->setSize(numColorCols, numColorRows);
                textureColor->allocateStorage();
            } else {
                textureColor = new QOpenGLTexture(QOpenGLTexture::Target2D);
                textureColor->setSize(numColorCols, numColorRows);
                textureColor->setFormat(QOpenGLTexture::RGBA32F);
                textureColor->setWrapMode(QOpenGLTexture::ClampToBorder);
                textureColor->setMinificationFilter(QOpenGLTexture::Nearest);
                textureColor->setMagnificationFilter(QOpenGLTexture::Nearest);
                textureColor->allocateStorage();
            }
        }
#ifndef AZUREKINECT
        // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE DEPTH TO COLOR VIDEO MAPPING
        if (playbackDevice == DeviceKinect) {
            if (playbackColor == ColorXYZRGB || playbackColor == ColorXYZWRGBA) {
                textureMapping = new QOpenGLTexture(QOpenGLTexture::Target2D);
                textureMapping->setSize(numDepthCols, numDepthRows);
                textureMapping->setFormat(QOpenGLTexture::RG32F);
                textureMapping->setWrapMode(QOpenGLTexture::ClampToBorder);
                textureMapping->setMinificationFilter(QOpenGLTexture::Nearest);
                textureMapping->setMagnificationFilter(QOpenGLTexture::Nearest);
                textureMapping->allocateStorage();
            }
        }
#endif
        // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE DEPTH TO COLOR VIDEO MAPPING
        if (playbackDevice == DeviceProsilicaTOF) {
            textureMapping = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureMapping->setSize(numDepthCols, numDepthRows);
            textureMapping->setFormat(QOpenGLTexture::R32F);
            textureMapping->setWrapMode(QOpenGLTexture::ClampToBorder);
            textureMapping->setMinificationFilter(QOpenGLTexture::Nearest);
            textureMapping->setMagnificationFilter(QOpenGLTexture::Nearest);
            textureMapping->allocateStorage();
        }

        // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE DEPTH TO COLOR VIDEO MAPPING
        QOpenGLFramebufferObjectFormat frameBufferObjectFormat;
        frameBufferObjectFormat.setInternalTextureFormat(GL_RGBA32F);
        frameBufferObjectFormat.setAttachment(QOpenGLFramebufferObject::Depth);

        if (isARGMode()) {
            frameBufferObject = new QOpenGLFramebufferObject(((scale() + 1) * width()), height(), frameBufferObjectFormat);
        } else {
            frameBufferObject = new QOpenGLFramebufferObject((scale() * width()), height(), frameBufferObjectFormat);
        }
        frameBufferObject->release();

        // CREATE GLSL PROGRAM FOR PROCESSING THE INCOMING VIDEO
        setlocale(LC_NUMERIC, "C");
        if (playbackColor == ColorGray) {
            if (playbackDevice == DeviceProsilicaPST) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/GRAY/processPSTGrayVideo.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/GRAY/processPSTGrayVideo.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/GRAY/processGrayVideo.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/GRAY/processGrayVideo.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            }
        } else if (playbackColor == ColorRGB || playbackColor == ColorRGBA) {
            if (isMachineVision(playbackDevice)) {
#ifdef VIMBA
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/RGB/processRGGBVideo.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/RGB/processRGGBVideo.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
#else
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/RGB/processBGGRVideo.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/RGB/processBGGRVideo.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
#endif
            } else {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/RGB/processRGBVideo.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/RGB/processRGBVideo.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            }
        } else if (playbackColor == ColorXYZG || playbackColor == ColorXYZ || playbackColor == ColorXYZW) {
            if (playbackDevice == DeviceKinect || playbackDevice == DeviceLucid || playbackDevice == DeviceOrbbec || playbackDevice == DeviceVZense || playbackDevice == DeviceVidu) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawKinectVideoToXYZGPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawKinectVideoToXYZGPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DevicePrimeSense) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawPrimeSenseVideoToXYZG.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawPrimeSenseVideoToXYZG.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceRealSense) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawRealSenseVideoToXYZG.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawRealSenseVideoToXYZG.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaLCG) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZG.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZG.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaTOF) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZG.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZG.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaDPR) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZG.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZG.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaAST) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawProsilicaASTVideoToXYZG.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaASTVideoToXYZG.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaIOS) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawProsilicaIOSVideoToXYZG.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaIOSVideoToXYZG.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            }
        } else if (playbackColor == ColorXYZRGB || playbackColor == ColorXYZWRGBA) {
            if (playbackDevice == DeviceKinect) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawKinectVideoToXYZRGBPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawKinectVideoToXYZRGBPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceLucid || playbackDevice == DeviceOrbbec || playbackDevice == DeviceVZense || playbackDevice == DeviceVidu) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawLucidVideoToXYZRGBPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawLucidVideoToXYZRGBPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DevicePrimeSense) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawPrimeSenseVideoToXYZRGB.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawPrimeSenseVideoToXYZRGB.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceRealSense) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawRealSenseVideoToXYZRGB.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawRealSenseVideoToXYZRGB.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaLCG) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawProsilicaLCGVideoToXYZRGB.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawProsilicaLCGVideoToXYZRGB.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaDPR) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawProsilicaDPRVideoToXYZRGB.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawProsilicaDPRVideoToXYZRGB.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaAST) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawProsilicaASTVideoToXYZRGB.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawProsilicaASTVideoToXYZRGB.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaIOS) {
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZRGB/XYZRGB/rawProsilicaIOSVideoToXYZRGB.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawProsilicaIOSVideoToXYZRGB.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            }
        }

        // SEE IF THE USER WANTS TO ENABLE THE BOUNDING BOX FILTER
        if (enableBoundingBoxFlag){
            boundingBoxBufferObject = new QOpenGLFramebufferObject(frameBufferObject->width(), frameBufferObject->height(), frameBufferObjectFormat);
            boundingBoxBufferObject->release();
            if (boundingBoxBufferObject->isValid() == false){
                qDebug() << "Invalid boundingBoxBufferObject!";
                qDebug() << "Invalid boundingBoxBufferObject!";
                qDebug() << "Invalid boundingBoxBufferObject!";
            }

            switch (playbackColor) {
            case ColorXYZ:
            case ColorXYZW:
            case ColorXYZG:
                if (!boundingBoxProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/MEDIAN/MedianFilters/filterBoundingBox.vert")) {
                    qDebug() << "Error adding boundingBoxProgram vertex shader from source.";
                } else if (!boundingBoxProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/MEDIAN/MedianFilters/filterBoundingBoxXYZG.frag")) {
                    qDebug() << "Error adding boundingBoxProgram fragment shader from source.";
                } else if (!boundingBoxProgram.link()) {
                    qDebug() << "Error linking boundingBoxProgram shader.";
                }
                break;
            case ColorXYZRGB:
            case ColorXYZWRGBA:
                if (!boundingBoxProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/MEDIAN/MedianFilters/filterBoundingBox.vert")) {
                    qDebug() << "Error adding boundingBoxProgram vertex shader from source.";
                } else if (!boundingBoxProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/MEDIAN/MedianFilters/filterBoundingBoxXYZRGB.frag")) {
                    qDebug() << "Error adding boundingBoxProgram fragment shader from source.";
                } else if (!boundingBoxProgram.link()) {
                    qDebug() << "Error linking boundingBoxProgram shader.";
                }
                break;
            default:
                break;
            }
        }


        if (playbackDevice == DeviceProsilicaAST) {
            // SET THE DEPTH TEXTURE TO LINEAR INTERPOLATE FOR EPIPOLAR RECTIFICATION
            textureDepth->setWrapMode(QOpenGLTexture::ClampToBorder);
            textureDepth->setMinificationFilter(QOpenGLTexture::Linear);
            textureDepth->setMagnificationFilter(QOpenGLTexture::Linear);

            stereoPhaseBufferObject = new QOpenGLFramebufferObject(width(), height(), frameBufferObjectFormat);
            stereoPhaseBufferObject->release();
            if (stereoPhaseBufferObject->isValid() == false){
                qDebug() << "Invalid stereoPhaseBufferObject!";
                qDebug() << "Invalid stereoPhaseBufferObject!";
                qDebug() << "Invalid stereoPhaseBufferObject!";
            }

            epipolarRectifiedPhaseBufferObject = new QOpenGLFramebufferObject(width(), height(), frameBufferObjectFormat);
            epipolarRectifiedPhaseBufferObject->release();
            if (epipolarRectifiedPhaseBufferObject->isValid() == false){
                qDebug() << "Invalid epipolarRectifiedPhaseBufferObject!";
                qDebug() << "Invalid epipolarRectifiedPhaseBufferObject!";
                qDebug() << "Invalid epipolarRectifiedPhaseBufferObject!";
            }
        }
        setlocale(LC_ALL, "");

        // CREATE A BUFFER TO HOLD THE ROW AND COLUMN COORDINATES OF IMAGE PIXELS FOR THE TEXEL FETCHES
        if (registerDepthToRGBFlag) {
            pixlVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
            pixlVertexBuffer.create();
            pixlVertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
            if (pixlVertexBuffer.bind()) {
                pixlVertexBuffer.allocate(static_cast<int>(numDepthRows * numDepthCols * 2 * sizeof(float)));
                float *vertices = (float *)pixlVertexBuffer.map(QOpenGLBuffer::WriteOnly);
                if (vertices) {
                    for (unsigned int row = 0; row < numDepthRows; row++) {
                        for (unsigned int col = 0; col < numDepthCols; col++) {
                            vertices[2 * (col + row * numDepthCols) + 0] = scale() * col;
                            vertices[2 * (col + row * numDepthCols) + 1] = row;
                        }
                    }
                    pixlVertexBuffer.unmap();
                } else {
                    qDebug() << QString("Unable to map vertexBuffer from GPU.");
                }
            }

            // CREATE AN INDEX BUFFER FOR THE RESULTING POINT CLOUD DRAWN AS TRIANGLES
            int index = 0;
            pixlIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
            pixlIndexBuffer.create();
            pixlIndexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
            if (pixlIndexBuffer.bind()) {
                pixlIndexBuffer.allocate(static_cast<int>(numDepthRows * numDepthCols * 6 * sizeof(unsigned int)));
                unsigned int *indices = (unsigned int *)pixlIndexBuffer.map(QOpenGLBuffer::WriteOnly);
                if (indices) {
                    for (unsigned int row = 0; row < numDepthRows - 1; row++) {
                        for (unsigned int col = 0; col < numDepthCols - 1; col++) {
                            indices[index++] = (row + 0) * numDepthCols + (col + 0);
                            indices[index++] = (row + 0) * numDepthCols + (col + 1);
                            indices[index++] = (row + 1) * numDepthCols + (col + 1);

                            indices[index++] = (row + 0) * numDepthCols + (col + 0);
                            indices[index++] = (row + 1) * numDepthCols + (col + 1);
                            indices[index++] = (row + 1) * numDepthCols + (col + 0);
                        }
                    }
                    pixlIndexBuffer.unmap();
                } else {
                    qDebug() << QString("Unable to map indiceBuffer from GPU.");
                }
            }

            // CREATE FRAME BUFFER OBJECT FOR SHIFTING THE POINT CLOUD TO THE RGB CAMERA FIELD OF VIEW
            registerBufferObject = new QOpenGLFramebufferObject(frameBufferObject->width(), frameBufferObject->height(), frameBufferObjectFormat);
            registerBufferObject->release();
        }

        // CALL THE VIRTUAL METHOD TO BE OVERRIDEN BY SUBCLASSES
        initializeGL();
    }

    // LOAD THE LOOK UP TABLE TO THE GPU, IF IT EXISTS
    if (lookUpTable.isValid()) {
        setLookUpTable(lookUpTable);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLFilter::onUpdateBuffer(LAUScan scan)
{
    updateBuffer(scan);
    emit emitBuffer(scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLFilter::setLookUpTable(LAULookUpTable lut)
{
    // MAKE SURE WE HAVE A VALID LOOK UP TABLE
    if (lut.isValid()) {
        lookUpTable = lut;
    }

    // CHECK TO SEE IF WE CAN COPY THIS LOOK UP TABLE TO THE GPU
    if (lookUpTable.isValid() && wasInitialized() && surface && makeCurrent(surface)) {
        // DELETE ALL LOOK UP TABLE TEXTURES TO MAKE WAY FOR NEW TEXTURES
        if (textureAngles) {
            delete textureAngles;
            textureAngles = nullptr;
        }

        if (textureLookUpTable) {
            delete textureLookUpTable;
            textureLookUpTable = nullptr;
        }

        if (textureMin) {
            delete textureMin;
            textureMin = nullptr;
        }

        if (textureMax) {
            delete textureMax;
            textureMax = nullptr;
        }

        if (texturePhaseUnwrap) {
            delete texturePhaseUnwrap;
            texturePhaseUnwrap = nullptr;
        }

        // SET THE LOCALE PRIOR TO SETTING THE SHADER PROGRAMS
        setlocale(LC_NUMERIC, "C");

        // UPLOAD THE PHASE CORRECTION TABLE IF IT EXISTS AND THEIR IS A TEXTURE TO RECEIVE IT
        if (texturePhaseCorrection->isStorageAllocated() && lookUpTable.constPhaseCorrectionTable()) {
            texturePhaseCorrection->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)lookUpTable.constPhaseCorrectionTable());
        }

        // UPLOAD THE ABCD AND EFGH COEFFICIENTS AS A FLOATING POINT RGBA TEXTURE
        if (lookUpTable.style() == LAULookUpTableStyle::StyleLinear) {
            // CREATE TEXTURE FOR HOLDING SPHERICAL TO CARTESIAN COORDINATE TRANSFORMATION
            textureAngles = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureAngles->setSize(2 * numDepthCols, numDepthRows);
            textureAngles->setFormat(QOpenGLTexture::RGBA32F);
            textureAngles->setWrapMode(QOpenGLTexture::ClampToBorder);
            textureAngles->setMinificationFilter(QOpenGLTexture::Nearest);
            textureAngles->setMagnificationFilter(QOpenGLTexture::Nearest);
            textureAngles->allocateStorage();
            textureAngles->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)lookUpTable.constScanLine(0));

            // LOAD THE RIGHT PROGRAM FOR THIS PARTICULAR LUT
            if (playbackDevice == DeviceProsilicaLCG && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZGLinear.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZGLinear.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaAST && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaASTVideoToXYZGLinear.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaASTVideoToXYZGLinear.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaTOF && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZGLinear.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZGLinear.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaDPR && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZGLinear.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZGLinear.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            }
        } else if (lookUpTable.style() == LAULookUpTableStyle::StyleActiveStereoVisionPoly) {
            // KEEP TRACK OF THE LOOK UP TABLE'S FIRST BYTE ADDRESS
            float *buffer = (float *)lookUpTable.constScanLine(0);

            // CREATE TEXTURE FOR HOLDING A MINIMUM PHASE VALUES
            textureMin = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureMin->setSize(numDepthCols, numDepthRows);
            textureMin->setFormat(QOpenGLTexture::RGBA32F);
            textureMin->allocateStorage();
            if (textureMin->isStorageAllocated()) {
                textureMin->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)(buffer + 0 * numDepthCols * numDepthRows));
            }

            // CREATE TEXTURE FOR HOLDING A MINIMUM PHASE VALUES
            textureMax = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureMax->setSize(numDepthCols, numDepthRows);
            textureMax->setFormat(QOpenGLTexture::RGBA32F);
            textureMax->allocateStorage();
            if (textureMax->isStorageAllocated()) {
                textureMax->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)(buffer + 4 * numDepthCols * numDepthRows));
            }

            // CREATE TEXTURE FOR HOLDING SPHERICAL TO CARTESIAN COORDINATE TRANSFORMATION
            textureAngles = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureAngles->setSize(3 * numDepthCols, numDepthRows);
            textureAngles->setFormat(QOpenGLTexture::RGBA32F);
            textureAngles->setWrapMode(QOpenGLTexture::ClampToBorder);
            textureAngles->setMinificationFilter(QOpenGLTexture::Nearest);
            textureAngles->setMagnificationFilter(QOpenGLTexture::Nearest);
            textureAngles->allocateStorage();
            if (textureAngles->isStorageAllocated()) {
                textureAngles->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)(buffer + 8 * numDepthCols * numDepthRows));
            }

            stereoProgramA.removeAllShaders();
            if (!stereoProgramA.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/FILTERS/STEREO/Stereo/rawProsilicaASTVideoToPhase.vert")) {
                qDebug() << "Error adding vertex shader from source.";
            } else if (!stereoProgramA.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/STEREO/Stereo/rawProsilicaASTVideoToPhase.frag")) {
                qDebug() << "Error adding fragment shader from source.";
            } else if (!stereoProgramA.link()) {
                qDebug() << "Error linking shader.";
            }

            stereoProgramB.removeAllShaders();
            if (!stereoProgramB.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/FILTERS/STEREO/Stereo/rawProsilicaASTEpipolarRectifyPhase.vert")) {
                qDebug() << "Error adding vertex shader from source.";
            } else if (!stereoProgramB.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/STEREO/Stereo/rawProsilicaASTEpipolarRectifyPhase.frag")) {
                qDebug() << "Error adding fragment shader from source.";
            } else if (!stereoProgramB.link()) {
                qDebug() << "Error linking shader.";
            }

            stereoProgramC.removeAllShaders();
            if (!stereoProgramC.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/FILTERS/STEREO/Stereo/rawProsilicaASTFindCorrespondence.vert")) {
                qDebug() << "Error adding vertex shader from source.";
            } else if (!stereoProgramC.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/STEREO/Stereo/rawProsilicaASTFindCorrespondence.frag")) {
                qDebug() << "Error adding fragment shader from source.";
            } else if (!stereoProgramC.link()) {
                qDebug() << "Error linking shader.";
            }

            program.removeAllShaders();
            if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/STEREO/Stereo/rawProsilicaASTCorrespondenceToXYZG.vert")) {
                qDebug() << "Error adding vertex shader from source.";
            } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/STEREO/Stereo/rawProsilicaASTCorrespondenceToXYZG.frag")) {
                qDebug() << "Error adding fragment shader from source.";
            } else if (!program.link()) {
                qDebug() << "Error linking shader.";
            }
        } else if (lookUpTable.style() == LAULookUpTableStyle::StyleFourthOrderPoly) {
            // CREATE TEXTURE FOR HOLDING SPHERICAL TO CARTESIAN COORDINATE TRANSFORMATION
            textureAngles = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureAngles->setSize(3 * numDepthCols, numDepthRows);
            textureAngles->setFormat(QOpenGLTexture::RGBA32F);
            textureAngles->setWrapMode(QOpenGLTexture::ClampToBorder);
            textureAngles->setMinificationFilter(QOpenGLTexture::Nearest);
            textureAngles->setMagnificationFilter(QOpenGLTexture::Nearest);
            textureAngles->allocateStorage();
            if (textureAngles->isStorageAllocated()) {
                textureAngles->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)lookUpTable.constScanLine(0));
            }

            // LOAD THE RIGHT PROGRAM FOR THIS PARTICULAR LUT
            if (playbackDevice == DeviceProsilicaLCG && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZGPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZGPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaAST && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaASTVideoToXYZGPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaASTVideoToXYZGPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaTOF && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZGPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZGPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaDPR && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZGPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZGPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceKinect) {
#ifdef AZUREKINECT
                if (playbackColor == ColorXYZG) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawLucidVideoToXYZGPoly.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawLucidVideoToXYZGPoly.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                } else if (playbackColor == ColorXYZRGB) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZRGB/XYZRGB/rawLucidVideoToXYZRGBPoly.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawLucidVideoToXYZRGBPoly.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                }
#else
                if (playbackColor == ColorXYZG) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawKinectVideoToXYZGPoly.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawKinectVideoToXYZGPoly.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                } else if (playbackColor == ColorXYZRGB) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZRGB/XYZRGB/rawKinectVideoToXYZRGB.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawKinectVideoToXYZRGB.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                }
#endif
            } else if (playbackDevice == DeviceLucid || playbackDevice == DeviceOrbbec || playbackDevice == DeviceVZense || playbackDevice == DeviceVidu) {
                if (playbackColor == ColorXYZG) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawLucidVideoToXYZGPoly.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawLucidVideoToXYZGPoly.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                } else if (playbackColor == ColorXYZRGB) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZRGB/XYZRGB/rawLucidVideoToXYZRGBPoly.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawLucidVideoToXYZRGBPoly.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                }
            }
        } else if (lookUpTable.style() == LAULookUpTableStyle::StyleFourthOrderPolyAugmentedReality) {
            // CREATE TEXTURE FOR HOLDING SPHERICAL TO CARTESIAN COORDINATE TRANSFORMATION
            textureAngles = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureAngles->setSize(4 * numDepthCols, numDepthRows);
            textureAngles->setFormat(QOpenGLTexture::RGBA32F);
            textureAngles->setWrapMode(QOpenGLTexture::ClampToBorder);
            textureAngles->setMinificationFilter(QOpenGLTexture::Nearest);
            textureAngles->setMagnificationFilter(QOpenGLTexture::Nearest);
            textureAngles->allocateStorage();
            textureAngles->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)lookUpTable.constScanLine(0));

            // LOAD THE RIGHT PROGRAM FOR THIS PARTICULAR LUT
            if (playbackDevice == DeviceProsilicaLCG && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZGPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZGPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaAST && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaASTVideoToXYZGPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaASTVideoToXYZGPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaTOF && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZGPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZGPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaDPR && playbackColor == ColorXYZG) {
                if (playbackColor == ColorXYZG) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZGPoly.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZGPoly.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                } else {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawProsilicaDPRVideoToXYZWRGBAPoly.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawProsilicaDPRVideoToXYZWRGBAPoly.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                }
            }
        } else if (lookUpTable.style() == LAULookUpTableStyle::StyleFourthOrderPolyWithPhaseUnwrap) {
            // CREATE TEXTURE FOR HOLDING SPHERICAL TO CARTESIAN COORDINATE TRANSFORMATION
            textureAngles = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureAngles->setSize(3 * numDepthCols, numDepthRows);
            textureAngles->setFormat(QOpenGLTexture::RGBA32F);
            textureAngles->setWrapMode(QOpenGLTexture::ClampToBorder);
            textureAngles->setMinificationFilter(QOpenGLTexture::Nearest);
            textureAngles->setMagnificationFilter(QOpenGLTexture::Nearest);
            textureAngles->allocateStorage();
            textureAngles->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)lookUpTable.constScanLine(0));

            // CREATE TEXTURE FOR HOLDING A MINIMUM PHASE VALUES
            texturePhaseUnwrap = new QOpenGLTexture(QOpenGLTexture::Target2D);
            texturePhaseUnwrap->setSize(numDepthCols, numDepthRows);
            texturePhaseUnwrap->setFormat(QOpenGLTexture::R32F);
            texturePhaseUnwrap->allocateStorage();
            texturePhaseUnwrap->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)((float *)lookUpTable.constScanLine(0) + 12 * numDepthCols * numDepthRows));

            // LOAD THE RIGHT PROGRAM FOR THIS PARTICULAR LUT
            if (playbackDevice == DeviceProsilicaLCG && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZGPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZGPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaAST && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaASTVideoToXYZGPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaASTVideoToXYZGPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaTOF && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZGPoly.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZGPoly.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaDPR && playbackColor == ColorXYZG) {
                if (playbackColor == ColorXYZG) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZGPoly.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZGPoly.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                } else {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawProsilicaDPRVideoToXYZRGBPoly.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawProsilicaDPRVideoToXYZRGBPoly.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                }
            }
        } else if (lookUpTable.style() == LAULookUpTableStyle::StyleXYZPLookUpTable) {
            // KEEP TRACK OF THE LOOK UP TABLE'S FIRST BYTE ADDRESS
            float *buffer = (float *)lookUpTable.constScanLine(0);

            // CREATE TEXTURE FOR HOLDING A MINIMUM PHASE VALUES
            textureMin = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureMin->setSize(numDepthCols, numDepthRows);
            textureMin->setFormat(QOpenGLTexture::RGBA32F);
            textureMin->allocateStorage();
            textureMin->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)(buffer + 0 * numDepthCols * numDepthRows));

            // CREATE TEXTURE FOR HOLDING A MINIMUM PHASE VALUES
            textureMax = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureMax->setSize(numDepthCols, numDepthRows);
            textureMax->setFormat(QOpenGLTexture::RGBA32F);
            textureMax->allocateStorage();
            textureMax->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)(buffer + 4 * numDepthCols * numDepthRows));

            // CREATE TEXTURE FOR HOLDING A 3D LOOK UP TABLE
            textureLookUpTable = new QOpenGLTexture(QOpenGLTexture::Target3D);
            textureLookUpTable->setSize(numDepthCols, numDepthRows, (lookUpTable.colors() - 8) / 4);
            textureLookUpTable->setFormat(QOpenGLTexture::RGBA32F);
            textureLookUpTable->setWrapMode(QOpenGLTexture::ClampToBorder);
            textureLookUpTable->setMinificationFilter(QOpenGLTexture::Linear);
            textureLookUpTable->setMagnificationFilter(QOpenGLTexture::Linear);
            textureLookUpTable->allocateStorage();
            textureLookUpTable->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)(buffer + 8 * numDepthCols * numDepthRows));

            // LOAD THE RIGHT PROGRAM FOR THIS PARTICULAR LUT
            if (playbackDevice == DeviceProsilicaLCG) {
                if (playbackColor == ColorXYZG) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZGLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaLCGVideoToXYZGLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                } else {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawProsilicaLCGVideoToXYZRGBLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawProsilicaLCGVideoToXYZRGBLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                }
            } else if (playbackDevice == DeviceProsilicaAST) {
                if (playbackColor == ColorXYZG) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawProsilicaASTVideoToXYZGLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaASTVideoToXYZGLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                } else {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawProsilicaASTVideoToXYZRGBLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawProsilicaASTVideoToXYZRGBLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                }
            } else if (playbackDevice == DeviceProsilicaTOF && playbackColor == ColorXYZG) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZGLookUpTable.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaTOFVideoToXYZGLookUpTable.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            } else if (playbackDevice == DeviceProsilicaDPR) {
                if (playbackColor == ColorXYZG) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZGLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawProsilicaDPRVideoToXYZGLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                } else {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawProsilicaDPRVideoToXYZRGBLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawProsilicaDPRVideoToXYZRGBLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                }
            } else if (playbackDevice == DeviceRealSense) {
                if (playbackColor == ColorXYZG) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawRealSenseVideoToXYZGLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawRealSenseVideoToXYZGLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                } else {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawRealSenseVideoToXYZRGBLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawRealSenseVideoToXYZRGBLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                }
            } else if (playbackDevice == DeviceKinect || playbackDevice == DeviceLucid || playbackDevice == DeviceOrbbec || playbackDevice == DeviceVZense || playbackDevice == DeviceVidu) {
                if (playbackColor == ColorXYZG) {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawKinectVideoToXYZGLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawKinectVideoToXYZGLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                } else {
                    program.removeAllShaders();
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawKinectVideoToXYZRGBLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawKinectVideoToXYZRGBLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                }
            }
        } else if (lookUpTable.style() == LAULookUpTableStyle::StyleXYZWRCPQLookUpTable) {
            // KEEP TRACK OF THE LOOK UP TABLE'S FIRST BYTE ADDRESS
            float *buffer = (float *)lookUpTable.constScanLine(0);

            // CREATE TEXTURE FOR HOLDING A MINIMUM PHASE VALUES
            textureMin = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureMin->setSize(2 * numDepthCols, numDepthRows);
            textureMin->setFormat(QOpenGLTexture::RGBA32F);
            textureMin->allocateStorage();
            textureMin->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)(buffer + 0 * numDepthCols * numDepthRows));

            // CREATE TEXTURE FOR HOLDING A MINIMUM PHASE VALUES
            textureMax = new QOpenGLTexture(QOpenGLTexture::Target2D);
            textureMax->setSize(2 * numDepthCols, numDepthRows);
            textureMax->setFormat(QOpenGLTexture::RGBA32F);
            textureMax->allocateStorage();
            textureMax->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)(buffer + 8 * numDepthCols * numDepthRows));

            // CREATE TEXTURE FOR HOLDING A 3D LOOK UP TABLE
            textureLookUpTable = new QOpenGLTexture(QOpenGLTexture::Target3D);
            textureLookUpTable->setSize(2 * numDepthCols, numDepthRows, (lookUpTable.colors() - 16) / 8);
            textureLookUpTable->setFormat(QOpenGLTexture::RGBA32F);
            textureLookUpTable->setWrapMode(QOpenGLTexture::ClampToBorder);
            textureLookUpTable->setMinificationFilter(QOpenGLTexture::Linear);
            textureLookUpTable->setMagnificationFilter(QOpenGLTexture::Linear);
            textureLookUpTable->allocateStorage();
            if (textureLookUpTable->isStorageAllocated()) {
                textureLookUpTable->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)(buffer + 16 * numDepthCols * numDepthRows));
            } else {
                qDebug() << "Error allocating lookup table texture.";
            }

            // UPDATE THE GPU SHADERS
            if (playbackDevice == DeviceRealSense) {
                program.removeAllShaders();
                if (playbackColor == ColorXYZG) {
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZG/XYZG/rawRealSenseVideoToXYZGLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/rawRealSenseVideoToXYZGLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                } else {
                    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawRealSenseVideoToXYZRGBLookUpTable.vert")) {
                        qDebug() << "Error adding vertex shader from source.";
                    } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawRealSenseVideoToXYZRGBLookUpTable.frag")) {
                        qDebug() << "Error adding fragment shader from source.";
                    } else if (!program.link()) {
                        qDebug() << "Error linking shader.";
                    }
                }
            } else if (playbackDevice == DeviceKinect || playbackDevice == DeviceLucid || playbackDevice == DeviceOrbbec || playbackDevice == DeviceVZense || playbackDevice == DeviceVidu) {
                program.removeAllShaders();
                if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/XYZRGB/XYZRGB/rawKinectVideoToXYZRGBLookUpTable.vert")) {
                    qDebug() << "Error adding vertex shader from source.";
                } else if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/rawKinectVideoToXYZRGBLookUpTable.frag")) {
                    qDebug() << "Error adding fragment shader from source.";
                } else if (!program.link()) {
                    qDebug() << "Error linking shader.";
                }
            }
        }

        // UNSET THE LOCALE PRIOR TO SETTING THE SHADER PROGRAMS
        setlocale(LC_ALL, "");
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLFilter::updateBuffer(LAUScan scan)
{
    if (surface && makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        glBindTexture(GL_TEXTURE_2D, frameBufferObject->texture());
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        switch (scan.color()) {
            case ColorGray:
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, scan.constPointer());
                break;
            case ColorRGB:
            case ColorXYZ:
            case ColorXYZRGB:
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, scan.constPointer());
                break;
            case ColorRGBA:
            case ColorXYZW:
            case ColorXYZG:
            case ColorXYZWRGBA:
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, scan.constPointer());
                break;
            default:
                break;
        }

        // COPY THE PROJECTION MATRIX OVER FROM THE LOOK UP TABLE
        scan.setProjection(lookUpTable.projection());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLFilter::onUpdateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    // CALL THE FILTER'S UPDATE BUFFER
    updateBuffer(depth, color, mapping);

    // EMIT THE CURRENT CHANNEL INDEX ONLY IF IT CHANGED
    if (channel != lastEmittedChannel) {
        lastEmittedChannel = channel;
        emit emitChannelIndex(channel);
    }

    // SEND THE DATA OBJECTS TO THE NEXT STAGE
    emit emitBuffer(depth, color, mapping);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLFilter::updateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    // SEE IF WE HAVE VALID INCOMING DATA
    if (depth.isElapsedValid() == false && color.isElapsedValid() == false){
        return;
    }

    if (playbackDevice == LAU3DVideoParameters::DeviceProsilicaAST && depth.frames() > 1){
        updateMultiCameraBuffer(depth, color, mapping);
        return;
    }

    if (surface && makeCurrent(surface)) {
        // UPDATE THE COLOR TEXTURE
        if (textureColor) {
            if (color.isValid()) {
                if (color.colors() == 1) {
                    if (color.depth() == sizeof(unsigned char)) {
                        textureColor->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (const void *)color.constFrame(channel % color.frames()));
                    } else if (color.depth() == sizeof(unsigned short)) {
                        textureColor->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, (const void *)color.constFrame(channel % color.frames()));
                    } else if (color.depth() == sizeof(float)) {
                        textureColor->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)color.constFrame(channel % color.frames()));
                    }
                } else if (color.colors() == 3) {
                    if (color.depth() == sizeof(unsigned char)) {
                        textureColor->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, (const void *)color.constFrame(channel % color.frames()));
                    } else if (color.depth() == sizeof(unsigned short)) {
                        textureColor->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt16, (const void *)color.constFrame(channel % color.frames()));
                    } else if (color.depth() == sizeof(float)) {
                        textureColor->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)color.constFrame(channel % color.frames()));
                    }
                } else if (color.colors() == 4) {
                    if (color.depth() == sizeof(unsigned char)) {
                        textureColor->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, (const void *)color.constFrame(channel % color.frames()));
                    } else if (color.depth() == sizeof(unsigned short)) {
                        textureColor->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt16, (const void *)color.constFrame(channel % color.frames()));
                    } else if (color.depth() == sizeof(float)) {
                        textureColor->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)color.constFrame(channel % color.frames()));
                    }
                }
            } else if (depth.isValid()) {
                // WE WANT A COLOR BUT ITS NOT AVAILABLE, SO LET'S HIJACK THE DEPTH BUFFER
                int frameIndex = channel % depth.frames();
                if (textureColor && depth.constFrame(frameIndex)) {
                    if (depth.depth() == sizeof(unsigned char)) {
                        textureColor->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (const void *)depth.constFrame(frameIndex));
                    } else if (depth.depth() == sizeof(unsigned short)) {
                        textureColor->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, (const void *)depth.constFrame(frameIndex));
                    } else if (depth.depth() == sizeof(float)) {
                        textureColor->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)depth.constFrame(frameIndex));
                    }
                }
            }
        }

        // UPDATE THE DEPTH AND MAPPING TEXTURES
        if (textureDepth) {
            if (depth.isValid()) {
                // BIND TEXTURE FOR CONVERTING FROM SPHERICAL TO CARTESIAN
                if (playbackDevice == DevicePrimeSense || playbackDevice == DeviceKinect || playbackDevice == DeviceLucid || playbackDevice == DeviceOrbbec || playbackDevice == DeviceVZense || playbackDevice == DeviceVidu || playbackDevice == DeviceRealSense) {
                    if (depth.depth() == sizeof(unsigned char)) {
                        textureDepth->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, (const void *)depth.constFrame(channel % depth.frames()));
                    } else if (depth.depth() == sizeof(unsigned short)) {
                        textureDepth->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt16, (const void *)depth.constFrame(channel % depth.frames()));
                    } else if (depth.depth() == sizeof(float)) {
                        textureDepth->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)depth.constFrame(channel % depth.frames()));
                    }
                } else if (playbackDevice == DeviceProsilicaLCG || playbackDevice == DeviceProsilicaTOF || playbackDevice == DeviceProsilicaDPR || playbackDevice == DeviceProsilicaIOS) {
                    if (depth.depth() == sizeof(unsigned char)) {
                        textureDepth->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, (const void *)depth.constFrame(qMax(0, channel) % depth.frames()));
                    } else if (depth.depth() == sizeof(unsigned short)) {
                        textureDepth->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Int16, (const void *)depth.constFrame(qMax(0, channel) % depth.frames()));
                    } else if (depth.depth() == sizeof(float)) {
                        textureDepth->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)depth.constFrame(qMax(0, channel) % depth.frames()));
                    }
                }
            }
        }

        // UPDATE THE MAPPING BUFFER
        if (textureMapping) {
            if (playbackDevice == DeviceProsilicaTOF){
                if (depth.isValid()) {
                    if (depth.depth() == sizeof(unsigned char)) {
                        textureMapping->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (const void *)depth.constFrame(depth.frames() - 1));
                    } else if (depth.depth() == sizeof(unsigned short)) {
                        textureMapping->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, (const void *)depth.constFrame(depth.frames() - 1));
                    } else if (depth.depth() == sizeof(float)) {
                        textureMapping->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)depth.constFrame(depth.frames() - 1));
                    }
                }
            } else if (mapping.isValid() && (mapping.colors() == 2)) {
                textureMapping->setData(QOpenGLTexture::RG, QOpenGLTexture::Float32, (const void *)mapping.constFrame(channel % mapping.frames()));
            }
        }

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING THE PHASE DFT COEFFICIENTS
        // ALONG WITH THE GLSL PROGRAMS THAT WILL DO THE PROCESSING
        if (frameBufferObject && frameBufferObject->bind()) {
            if (program.bind()) {
                // CLEAR THE FRAME BUFFER OBJECT
                glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
                glViewport(0, 0, frameBufferObject->width(), frameBufferObject->height());
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                if (quadVertexBuffer.bind()) {
                    if (quadIndexBuffer.bind()) {
                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        if (textureColor) {
                            glActiveTexture(GL_TEXTURE0);
                            textureColor->bind();
                            program.setUniformValue("qt_colorTexture", 0);
                        }

                        // BIND THE DEPTH TEXTURE
                        if (textureDepth) {
                            glActiveTexture(GL_TEXTURE1);
                            textureDepth->bind();
                            program.setUniformValue("qt_depthTexture", 1);
                        }

                        // BIND TEXTURE FOR CONVERTING FROM SPHERICAL TO CARTESIAN
                        if (textureAngles) {
                            if (texturePhaseUnwrap) {
                                glActiveTexture(GL_TEXTURE3);
                                texturePhaseUnwrap->bind();
                                program.setUniformValue("qt_unwrpTexture", 3);
                            }

                            glActiveTexture(GL_TEXTURE4);
                            textureAngles->bind();
                            program.setUniformValue("qt_spherTexture", 4);
                        } else if (textureLookUpTable) {
                            if (textureMin) {
                                glActiveTexture(GL_TEXTURE3);
                                textureMin->bind();
                                program.setUniformValue("qt_minTexture", 3);
                            }

                            if (textureMax) {
                                glActiveTexture(GL_TEXTURE4);
                                textureMax->bind();
                                program.setUniformValue("qt_maxTexture", 4);
                            }

                            glActiveTexture(GL_TEXTURE5);
                            textureLookUpTable->bind();
                            program.setUniformValue("qt_lutTexture", 5);
                            program.setUniformValue("qt_layers", (float)(textureLookUpTable->depth() - 1));
                        }

                        if (playbackDevice == DeviceKinect) {
#ifndef AZUREKINECT
                            // BIND THE MAPPING TEXTURE
                            if ((playbackColor == ColorXYZRGB) || (playbackColor == ColorXYZWRGBA)) {
                                glActiveTexture(GL_TEXTURE2);
                                textureMapping->bind();
                                program.setUniformValue("qt_mappingTexture", 2);
                            }
#endif
                        } else if (playbackDevice == DeviceProsilicaLCG || playbackDevice == DeviceProsilicaDPR || playbackDevice == DeviceProsilicaIOS || playbackDevice == DeviceProsilicaAST) {
                            // SET THE SNR THRESHOLD FOR DELETING BAD POINTS
                            program.setUniformValue("qt_snrThreshold", (float)snrThreshold / 1000.0f);
                            program.setUniformValue("qt_mtnThreshold", (float)qPow((float)mtnThreshold / 1000.0f, 4.0f));
                        } else if (playbackDevice == DeviceProsilicaTOF){
                            // SET THE SNR THRESHOLD FOR DELETING BAD POINTS
                            program.setUniformValue("qt_snrThreshold", (float)snrThreshold / 1000.0f);
                            program.setUniformValue("qt_mtnThreshold", (float)qPow((float)mtnThreshold / 1000.0f, 4.0f));

                            // BIND THE MAPPING TEXTURE
                            glActiveTexture(GL_TEXTURE2);
                            textureMapping->bind();
                            program.setUniformValue("qt_mappingTexture", 2);
                        }

                        // SET THE RANGE LIMITS FOR THE Z AXIS
                        if (lookUpTable.isValid()) {
                            if (lookUpTable.style() == LAULookUpTableStyle::StyleXYZWRCPQLookUpTable) {
                                program.setUniformValue("qt_depthLimits", lookUpTable.pLimits());
                            } else if (lookUpTable.style() == LAULookUpTableStyle::StyleXYZPLookUpTable) {
                                program.setUniformValue("qt_depthLimits", lookUpTable.pLimits());
                            } else {
                                program.setUniformValue("qt_depthLimits", lookUpTable.zLimits());
                            }
                        } else {
                            program.setUniformValue("qt_depthLimits", QPointF(-1e6, 1e6));
                        }

                        // SET THE MAXIMUM INTENSITY VALUE FOR THE INCOMING VIDEO
                        if (playbackDevice == DeviceProsilicaPST) {
                            float gain = 65535.0f / (float)maxIntensityValue;
                            program.setUniformValue("qt_scaleFactor",  QPointF(gain, gain));
                        } else {
                            if (maxIntensityValue > 255) {
                                program.setUniformValue("qt_scaleFactor",  65535.0f / (float)maxIntensityValue);
                            } else {
                                program.setUniformValue("qt_scaleFactor",  255.0f / (float)maxIntensityValue);
                            }
                        }

                        // SET THE PHASE CORRECTION TABLE IF IT EXISTS
                        if (texturePhaseCorrection){
                            glActiveTexture(GL_TEXTURE8);
                            texturePhaseCorrection->bind();
                            program.setUniformValue("qt_phaseTexture", 8);
                        }

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(program.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        program.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        quadIndexBuffer.release();
                    }
                    quadVertexBuffer.release();
                }
                program.release();
            }
            frameBufferObject->release();
        }

        // SEE IF THE USER WANTS TO APPLY THE BOUNDING BOX FILTER
        if (enableBoundingBoxFlag){
            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING
            if (boundingBoxBufferObject && boundingBoxBufferObject->bind()) {
                if (boundingBoxProgram.bind()) {
                    // CLEAR THE FRAME BUFFER OBJECT
                    glClearColor(-1.0f, -1.0f, -1.0f, -1.0f);
                    glViewport(0, 0, boundingBoxBufferObject->width(), boundingBoxBufferObject->height());
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                    if (quadVertexBuffer.bind()) {
                        if (quadIndexBuffer.bind()) {
                            // BIND THE POINT CLOUD TEXTURE
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, frameBufferObject->texture());
                            boundingBoxProgram.setUniformValue("qt_texture", 0);

                            // SET THE CAMERA EXTRINSIC ROTATION PROJECTION MATRIX
                            boundingBoxProgram.setUniformValue("qt_projection", boundingBoxProjectorMatrix);

                            // SET THE RANGE LIMITS OF THE BOUNDING BOX
                            boundingBoxProgram.setUniformValue("qt_xMin", (float)xBoundingBoxMin);
                            boundingBoxProgram.setUniformValue("qt_xMax", (float)xBoundingBoxMax);
                            boundingBoxProgram.setUniformValue("qt_yMin", (float)yBoundingBoxMin);
                            boundingBoxProgram.setUniformValue("qt_yMax", (float)yBoundingBoxMax);
                            boundingBoxProgram.setUniformValue("qt_zMin", (float)zBoundingBoxMin);
                            boundingBoxProgram.setUniformValue("qt_zMax", (float)zBoundingBoxMax);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(boundingBoxProgram.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            boundingBoxProgram.enableAttributeArray("qt_vertex");
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            quadIndexBuffer.release();
                        }
                        quadVertexBuffer.release();
                    }
                    boundingBoxProgram.release();
                }
                boundingBoxBufferObject->release();
            }

            // REPLACE THE MAIN FRAME BUFFER OBJECT FOR THE USER
            QOpenGLFramebufferObject::blitFramebuffer(frameBufferObject, boundingBoxBufferObject);
        }

        // MAKE A COPY OF THE SCAN IMAGE ON THE HARD DISK FOR DEBUGGING
        //if (localFrameBufferObject.isNull()){
        //    // CREATE A LOCAL FRAME BUFFER OBJECT FOR DEBUGGING
        //    localFrameBufferObject = LAUMemoryObject(frameBufferObject->width(), frameBufferObject->height(), 4, sizeof(float));
        //}
        //glBindTexture(GL_TEXTURE_2D, frameBufferObject->texture());
        //glPixelStorei(GL_PACK_ALIGNMENT, 1);
        //glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)localFrameBufferObject.constPointer());
        //localFrameBufferObject.save(QString("/media/drhal/HardDisk/localFrameBufferObject.tif"));

        // SEE IF WE SHOULD ALIGN DEPTH TO RGB CAMERA
        if (registerDepthToRGBFlag == false) {
            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING THE PHASE DFT COEFFICIENTS
            // ALONG WITH THE GLSL PROGRAMS THAT WILL DO THE PROCESSING
            if (registerBufferObject && registerBufferObject->bind()) {
                if (program.bind()) {
                    // CLEAR THE FRAME BUFFER OBJECT
                    glViewport(0, 0, registerBufferObject->width(), registerBufferObject->height());
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                    if (pixlVertexBuffer.bind()) {
                        if (pixlIndexBuffer.bind()) {
                            // BIND THE POINT CLOUD TEXTURE
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, frameBufferObject->texture());
                            program.setUniformValue("qt_pointCloud", 0);

                            // BIND THE COLOR VIDEO FRAME TEXTURE
                            if (textureColor) {
                                glActiveTexture(GL_TEXTURE1);
                                textureColor->bind();
                                program.setUniformValue("qt_colorTexture", 1);
                            }

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(static_cast<unsigned int>(program.attributeLocation("qt_vertex")), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
                            program.enableAttributeArray("qt_vertex");

                            // NOW DRAW ON SCREEN USING OUR POINT CLOUD AS TRIANGLES
                            glDrawElements(GL_TRIANGLES, static_cast<int>((numDepthCols - 1) * (numDepthRows - 1) * 6), GL_UNSIGNED_INT, nullptr);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            pixlIndexBuffer.release();
                        }
                        pixlVertexBuffer.release();
                    }
                    program.release();
                }
                registerBufferObject->release();

                // REPLACE THE MAIN FRAME BUFFER OBJECT FOR THE USER
                QOpenGLFramebufferObject::blitFramebuffer(frameBufferObject, registerBufferObject);
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLFilter::updateMultiCameraBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    Q_UNUSED(color);
    Q_UNUSED(mapping);
    if (surface && makeCurrent(surface)) {
        // UPDATE THE DEPTH AND MAPPING TEXTURES
        if (textureDepth) {
            if (depth.isValid()) {
                // BIND TEXTURE FOR CONVERTING FROM SPHERICAL TO CARTESIAN
                if (playbackDevice == DeviceProsilicaAST) {
                    for (unsigned int cmr = 0; cmr < depth.frames(); cmr++){
                        if (depth.depth() == sizeof(unsigned char)) {
                            textureDepth->setData(0, 0, cmr, textureDepth->width(), textureDepth->height(), 1, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, (const void *)depth.constFrame(cmr));
                        } else if (depth.depth() == sizeof(unsigned short)) {
                            textureDepth->setData(0, 0, cmr, textureDepth->width(), textureDepth->height(), 1, QOpenGLTexture::RGBA, QOpenGLTexture::Int16, (const void *)depth.constFrame(cmr));
                        } else if (depth.depth() == sizeof(float)) {
                            textureDepth->setData(0, 0, cmr, textureDepth->width(), textureDepth->height(), 1, QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)depth.constFrame(cmr));
                        }
                    }
                }
            }
        }

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING THE PHASE DFT COEFFICIENTS
        // ALONG WITH THE GLSL PROGRAMS THAT WILL DO THE PROCESSING
        if (stereoPhaseBufferObject && stereoPhaseBufferObject->bind()) {
            if (stereoProgramA.bind()) {
                // CLEAR THE FRAME BUFFER OBJECT
                glClearColor(-1.0f, -1.0f, -1.0f, -1.0f);
                glViewport(0, 0, stereoPhaseBufferObject->width(), stereoPhaseBufferObject->height());
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                if (quadVertexBuffer.bind()) {
                    if (quadIndexBuffer.bind()) {
                        // BIND THE DEPTH TEXTURE
                        if (textureDepth) {
                            glActiveTexture(GL_TEXTURE1);
                            textureDepth->bind();
                            stereoProgramA.setUniformValue("qt_depthTexture", 1);
                        }

                        // SET THE PHASE CORRECTION TABLE IF IT EXISTS
                        if (texturePhaseCorrection){
                            glActiveTexture(GL_TEXTURE8);
                            texturePhaseCorrection->bind();
                            stereoProgramA.setUniformValue("qt_phaseTexture", 8);
                        }

                        // SET THE SNR THRESHOLD FOR DELETING BAD POINTS
                        stereoProgramA.setUniformValue("qt_numCameras", (int)depth.frames());
                        stereoProgramA.setUniformValue("qt_snrThreshold", (float)snrThreshold / 1000.0f);
                        stereoProgramA.setUniformValue("qt_mtnThreshold", (float)qPow((float)mtnThreshold / 1000.0f, 4.0f));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(stereoProgramA.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        stereoProgramA.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        quadIndexBuffer.release();
                    }
                    quadVertexBuffer.release();
                }
                stereoProgramA.release();
            } else {
                qDebug() << "Failed to bind stereoProgramA!";
                qDebug() << "Failed to bind stereoProgramA!";
                qDebug() << "Failed to bind stereoProgramA!";
            }
            stereoPhaseBufferObject->release();

            // EXPORT A COPY OF THE PHASE SORTING BUFFER FOR DEBUGGING
            // if (localFrameBufferObject.isNull()){
            //     // CREATE A LOCAL FRAME BUFFER OBJECT FOR DEBUGGING
            //     localFrameBufferObject = LAUMemoryObject(stereoPhaseBufferObject->width(), stereoPhaseBufferObject->height(), 4, sizeof(float));
            // }
            // glBindTexture(GL_TEXTURE_2D, stereoPhaseBufferObject->texture());
            // glPixelStorei(GL_PACK_ALIGNMENT, 1);
            // glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)localFrameBufferObject.constPointer());
            // localFrameBufferObject.save(QString("C:/Users/Public/Pictures/stereoPhaseBufferObject.tif"));
        } else {
            qDebug() << "Failed to bind stereoPhaseBufferObject!";
            qDebug() << "Failed to bind stereoPhaseBufferObject!";
            qDebug() << "Failed to bind stereoPhaseBufferObject!";
        }

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING THE PHASE IMAGES AND
        // TO RESAMPLE THE IMAGES INTO AN EPIPOLAR RECTIFIED PAIR
        if (epipolarRectifiedPhaseBufferObject && epipolarRectifiedPhaseBufferObject->bind()) {
            if (stereoProgramB.bind()) {
                // CLEAR THE FRAME BUFFER OBJECT
                glClearColor(-1.0f, -1.0f, -1.0f, -1.0f);
                glViewport(0, 0, epipolarRectifiedPhaseBufferObject->width(), epipolarRectifiedPhaseBufferObject->height());
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                if (quadVertexBuffer.bind()) {
                    if (quadIndexBuffer.bind()) {
                        // BIND THE PHASE TEXTURE
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, stereoPhaseBufferObject->texture());
                        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, nanOpenColor);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        stereoProgramB.setUniformValue("qt_phaseTexture", 0);

                        // BIND THE EPIPOLAR RECTIFICATION LOOK UP TABLE
                        if (textureMin){
                            glActiveTexture(GL_TEXTURE1);
                            textureMin->bind();
                            stereoProgramB.setUniformValue("qt_mappingTexture", 1);
                        }

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(stereoProgramB.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        stereoProgramB.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        quadIndexBuffer.release();
                    }
                    quadVertexBuffer.release();
                }
                stereoProgramB.release();
            } else {
                qDebug() << "Failed to bind stereoProgramB!";
                qDebug() << "Failed to bind stereoProgramB!";
                qDebug() << "Failed to bind stereoProgramB!";
            }
            epipolarRectifiedPhaseBufferObject->release();

            // EXPORT A COPY OF THE PHASE SORTING BUFFER FOR DEBUGGING
            // if (localFrameBufferObject.isNull()){
            //     // CREATE A LOCAL FRAME BUFFER OBJECT FOR DEBUGGING
            //     localFrameBufferObject = LAUMemoryObject(epipolarRectifiedPhaseBufferObject->width(), epipolarRectifiedPhaseBufferObject->height(), 4, sizeof(float));
            // }
            // glBindTexture(GL_TEXTURE_2D, epipolarRectifiedPhaseBufferObject->texture());
            // glPixelStorei(GL_PACK_ALIGNMENT, 1);
            // glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)localFrameBufferObject.constPointer());
            // localFrameBufferObject.save(QString("C:/Users/Public/Pictures/epipolarRectifiedPhaseBufferObject.tif"));
        } else {
            qDebug() << "Failed to bind epipolarRectifiedPhaseBufferObject!";
            qDebug() << "Failed to bind epipolarRectifiedPhaseBufferObject!";
            qDebug() << "Failed to bind epipolarRectifiedPhaseBufferObject!";
        }

        // USE THE MAPPING TEXTURE TO FIND CORRESPONDENCES BETWEEN IMAGES BASED ON PHASE
        if (stereoPhaseBufferObject && stereoPhaseBufferObject->bind()) {
            if (stereoProgramC.bind()) {
                // CLEAR THE FRAME BUFFER OBJECT
                glClearColor(-1.0f, -1.0f, -1.0f, -1.0f);
                glViewport(0, 0, stereoPhaseBufferObject->width(), stereoPhaseBufferObject->height());
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                if (quadVertexBuffer.bind()) {
                    if (quadIndexBuffer.bind()) {
                        // BIND THE PHASE TEXTURE
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, epipolarRectifiedPhaseBufferObject->texture());
                        stereoProgramC.setUniformValue("qt_phaseTexture", 0);

                        // BIND THE EPIPOLAR RECTIFICATION LOOK UP TABLE
                        if (textureMax){
                            glActiveTexture(GL_TEXTURE1);
                            textureMax->bind();
                            stereoProgramC.setUniformValue("qt_mappingTexture", 1);
                        }

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(stereoProgramC.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        stereoProgramC.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        quadIndexBuffer.release();
                    }
                    quadVertexBuffer.release();
                }
                stereoProgramC.release();
            } else {
                qDebug() << "Failed to bind stereoProgramC!";
                qDebug() << "Failed to bind stereoProgramC!";
                qDebug() << "Failed to bind stereoProgramC!";
            }
            stereoPhaseBufferObject->release();

            // EXPORT A COPY OF THE PHASE SORTING BUFFER FOR DEBUGGING
            // if (localFrameBufferObject.isNull()){
            //     // CREATE A LOCAL FRAME BUFFER OBJECT FOR DEBUGGING
            //     localFrameBufferObject = LAUMemoryObject(stereoPhaseBufferObject->width(), stereoPhaseBufferObject->height(), 4, sizeof(float));
            // }
            // glBindTexture(GL_TEXTURE_2D, stereoPhaseBufferObject->texture());
            // glPixelStorei(GL_PACK_ALIGNMENT, 1);
            // glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)localFrameBufferObject.constPointer());
            // localFrameBufferObject.save(QString("C:/Users/Public/Pictures/correspondenceFrameBuffer.tif"));
        } else {
            qDebug() << "Failed to bind stereoPhaseBufferObject 2!";
            qDebug() << "Failed to bind stereoPhaseBufferObject 2!";
            qDebug() << "Failed to bind stereoPhaseBufferObject 2!";
        }

        // CONVERT THE CORRESPONDENCES TO XYZ COORDINATES LIKE YOU WOULD PHASE VALUES
        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING THE PHASE DFT COEFFICIENTS
        // ALONG WITH THE GLSL PROGRAMS THAT WILL DO THE PROCESSING
        if (frameBufferObject && frameBufferObject->bind()) {
            if (program.bind()) {
                // CLEAR THE FRAME BUFFER OBJECT
                glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
                glViewport(0, 0, frameBufferObject->width(), frameBufferObject->height());
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                if (quadVertexBuffer.bind()) {
                    if (quadIndexBuffer.bind()) {
                        // BIND THE PHASE TEXTURE
                        if (stereoPhaseBufferObject){
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, stereoPhaseBufferObject->texture());
                            program.setUniformValue("qt_depthTexture", 0);
                        }

                        // BIND TEXTURE FOR CONVERTING FROM SPHERICAL TO CARTESIAN
                        if (textureAngles) {
                            glActiveTexture(GL_TEXTURE1);
                            textureAngles->bind();
                            program.setUniformValue("qt_spherTexture", 1);
                        }

                        // SET THE RANGE LIMITS FOR THE Z AXIS
                        if (lookUpTable.isValid()) {
                            program.setUniformValue("qt_depthLimits", lookUpTable.zLimits());
                        } else {
                            program.setUniformValue("qt_depthLimits", QPointF(-1e6, 1e6));
                        }

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(program.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        program.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        quadIndexBuffer.release();
                    }
                    quadVertexBuffer.release();
                }
                program.release();
            } else {
                qDebug() << "Failed to bind program!";
                qDebug() << "Failed to bind program!";
                qDebug() << "Failed to bind program!";
            }
            frameBufferObject->release();

            // if (localFrameBufferObject.isNull()){
            //     // CREATE A LOCAL FRAME BUFFER OBJECT FOR DEBUGGING
            //     localFrameBufferObject = LAUMemoryObject(frameBufferObject->width(), frameBufferObject->height(), 4, sizeof(float));
            // }
            // glBindTexture(GL_TEXTURE_2D, frameBufferObject->texture());
            // glPixelStorei(GL_PACK_ALIGNMENT, 1);
            // glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)localFrameBufferObject.constPointer());
            // localFrameBufferObject.save(QString("C:/Users/Public/Pictures/frameBufferObject.tif"));
        } else {
            qDebug() << "Failed to bind frameBufferObject!";
            qDebug() << "Failed to bind frameBufferObject!";
            qDebug() << "Failed to bind frameBufferObject!";
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLFilter::saveTextureToDisk(QOpenGLFramebufferObject *fbo, QString filename)
{
    // CREATE A BUFFER TO HOLD THE TEXTURE
    LAUScan scan(fbo->width(), fbo->height(), ColorXYZW);

    if (surface && makeCurrent(surface)) {
        // NOW COPY THE TEXTURE BACK FROM THE FBO TO THE LAUIMAGE
        glBindTexture(GL_TEXTURE_2D, fbo->texture());
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, scan.constPointer());
    }
    scan.save(filename);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGLFilter::saveTextureToDisk(QOpenGLTexture *texture, QString filename)
{
    // CREATE A BUFFER TO HOLD THE TEXTURE
    LAUScan scan(texture->width(), texture->height(), ColorXYZW);

    if (surface && makeCurrent(surface)) {
        // NOW COPY THE TEXTURE BACK FROM THE FBO TO THE LAUIMAGE
        texture->bind();
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, scan.constPointer());
    }
    scan.save(filename);
}
#endif
