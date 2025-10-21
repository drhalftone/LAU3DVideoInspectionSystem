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

#include "laugreenscreenglfilter.h"
#include <locale.h>

#ifndef HEADLESS
#ifndef EXCLUDE_LAU3DVIDEOWIDGET
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUGreenScreenWidget::LAUGreenScreenWidget(LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent) : LAU3DVideoRecordingWidget(color, device, parent)
{
    if (camera && camera->isValid()) {
        QList<QObject*> filters;
        for (unsigned int chn = 0; chn < camera->sensors(); chn++){
            LAUGreenScreenGLFilter *filter = new LAUGreenScreenGLFilter(camera->depthWidth(), camera->depthHeight(), camera->colorWidth(), camera->colorHeight(), camera->color(), camera->device());
            filter->setFieldsOfView(camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians());
            filter->setCamera(chn);

            // CONNECT THE NEW FILTER TO THE PREVIOUS FILTER
            if (filters.count() > 0){
                connect((LAUGreenScreenGLFilter*)filters.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), filter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
            }

            // ADD THE NEW BACKGROUND FILTER TO THE LIST OF FILTERS
            filters.append(filter);
        }

        // INSERT THE FILTER INTO THE SIGNAL CHAIN
        insertFilters(filters);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUGreenScreenWidget::~LAUGreenScreenWidget()
{
    ;
}
#endif
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUGreenScreenGLFilter::~LAUGreenScreenGLFilter()
{
    makeCurrent(surface);
    if (indices) {
        _mm_free(indices);
    }

    if (greenScreenTexture) {
        delete greenScreenTexture;
    }

    if (grnScrFBO) {
        delete grnScrFBO;
    }
    doneCurrent();

    qDebug() << "LAUGreenScreenGLFilter()::~LAUGreenScreenGLFilter()";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUGreenScreenGLFilter::background()
{
    // CREATE A BYTE ARRAY TO HOLD THE BACKGROUND BUFFER FROM SETTINGS
    QByteArray byteArray(numDepthCols * numDepthRows * sizeof(short), (char)0xff);

    // SAVE THE BACKGROUND BUFFER TO SETTINGS IN CASE WE NEED IT FOR GREEN SCREENING
    QSettings settings;
    QString frameString, jetrvString;
    int localChannel = qMax(0, channel);
    if (playbackDevice == DeviceRealSense) {
        frameString = QString("LAUBackgroundGLFilter::backgroundTexture::DeviceRealSense::%1").arg(localChannel);
        jetrvString = QString("LAUBackgroundGLFilter::jetrVector::DeviceRealSense::%1").arg(localChannel);
    } else if (playbackDevice == DeviceKinect) {
        frameString = QString("LAUBackgroundGLFilter::backgroundTexture::DeviceKinect::%1").arg(localChannel);
        jetrvString = QString("LAUBackgroundGLFilter::jetrVector::DeviceKinect::%1").arg(localChannel);
    } else if (playbackDevice == DeviceLucid) {
        frameString = QString("LAUBackgroundGLFilter::backgroundTexture::DeviceLucid::%1").arg(localChannel);
        jetrvString = QString("LAUBackgroundGLFilter::jetrVector::DeviceLucid::%1").arg(localChannel);
    } else if (playbackDevice == DeviceOrbbec) {
        frameString = QString("LAUBackgroundGLFilter::backgroundTexture::DeviceOrbbec::%1").arg(localChannel);
        jetrvString = QString("LAUBackgroundGLFilter::jetrVector::DeviceOrbbec::%1").arg(localChannel);
    } else if (playbackDevice == DeviceVZense) {
        frameString = QString("LAUBackgroundGLFilter::backgroundTexture::DeviceVZense::%1").arg(localChannel);
        jetrvString = QString("LAUBackgroundGLFilter::jetrVector::DeviceVZense::%1").arg(localChannel);
    } else if (playbackDevice == DevicePrimeSense) {
        frameString = QString("LAUBackgroundGLFilter::backgroundTexture::DevicePrimeSense::%1").arg(localChannel);
        jetrvString = QString("LAUBackgroundGLFilter::jetrVector::DevicePrimeSense::%1").arg(localChannel);
    } else {
        frameString = QString("LAUBackgroundGLFilter::backgroundTexture::backgroundTexture::%1").arg(localChannel);
        jetrvString = QString("LAUBackgroundGLFilter::jetrVector::backgroundTexture::%1").arg(localChannel);
    }
    byteArray = settings.value(frameString, byteArray).toByteArray();

    QVariantList list = settings.value(jetrvString).toList();
    QVector<double> jetr;
    for (const QVariant& variant : list) {
        jetr.append(variant.toDouble());
    }
    this->setJetrVector(localChannel, jetr);

    LAUMemoryObject object(numDepthCols, numDepthRows, 1, sizeof(unsigned short));
    memcpy(object.constPointer(), byteArray.data(), object.length());
    // object.save(QString("C:/background%1.tif").arg(channel));
    return (object);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGreenScreenGLFilter::onSetBackgroundTexture(LAUMemoryObject buffer)
{
    // KEEP LOCAL COPY
    if (buffer.isValid()) {
        greenScreenObject = buffer;
    }

    // UPDATE THE GREEN SCREEN TEXTURE
    if (greenScreenTexture) {
        if (greenScreenObject.isValid()) {
            makeCurrent(surface);
            if (greenScreenObject.depth() == sizeof(unsigned char)) {
                greenScreenTexture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, (const void *)greenScreenObject.constPointer());
            } else if (greenScreenObject.depth() == sizeof(unsigned short)) {
                greenScreenTexture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt16, (const void *)greenScreenObject.constPointer());
            } else if (greenScreenObject.depth() == sizeof(float)) {
                greenScreenTexture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)greenScreenObject.constPointer());
            }
            doneCurrent();
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGreenScreenGLFilter::initializeGL()
{
    // CREATE TEXTURE FOR HOLDING ALL MAX DEPTH FRAME FOR CALCULATING THE BACKGROUND IMAGE
    greenScreenTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    greenScreenTexture->setSize(numDepthCols / 4, numDepthRows);
    greenScreenTexture->setFormat(QOpenGLTexture::RGBA32F);
    greenScreenTexture->setWrapMode(QOpenGLTexture::ClampToBorder);
    greenScreenTexture->setMinificationFilter(QOpenGLTexture::Nearest);
    greenScreenTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
    greenScreenTexture->allocateStorage();

    // CREATE THE FRAME BUFFER OBJECT TO HOLD THE SCAN RESULTS AS A TEXTURE
    QOpenGLFramebufferObjectFormat frameBufferObjectFormat;
    frameBufferObjectFormat.setInternalTextureFormat(GL_RGBA32F);
    grnScrFBO = new QOpenGLFramebufferObject(QSize(numDepthCols / 4, numDepthRows), frameBufferObjectFormat);
    grnScrFBO->release();

    // Override system locale until shaders are compiled
    setlocale(LC_NUMERIC, "C");

    // CREATE SHADER FOR EXTRACTING THE MAXIMUM PIXEL FROM TWO TEXTURES
    grnScrenProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/greenScreenPixel.vert");
    grnScrenProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/greenScreenPixel.frag");
    grnScrenProgram.link();

    // Restore system locale
    setlocale(LC_ALL, "");

    if (greenScreenObject.isNull()) {
        // CREATE A BYTE ARRAY AND DUMP THE LAST BACKGROUND BUFFER INTO IT
        greenScreenTexture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt16, (const void *)background().constPointer());
    } else {
        // SET THE BACKGROUND TEXTURE
        onSetBackgroundTexture(greenScreenObject);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGreenScreenGLFilter::updateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    Q_UNUSED(color);
    Q_UNUSED(mapping);

    if (depth.isValid() && makeCurrent(surface)) {
        // MAKE SURE WE HAVE A LOCAL DEPTH OBJECT
        if (localDepth.isNull()){
            localDepth = LAUMemoryObject(numDepthCols, numDepthRows, 1, depth.depth());
        }

        // FOR EACH AND EVERY FRAME COMING IN FROM THE CAMERA, WE NEED TO APPLY THE
        // MINIMUM PIXEL FILTER IN ORDER TO ACCOUNT FOR GAUSSIAN NOISE ON EACH PIXEL
        textureDepth->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt16, (const void *)depth.constFrame(channel % depth.frames()));
        if (grnScrFBO->bind()) {
            // SET THE VIEW PORT AND CLEAR THE SCREEN BUFFER
            glViewport(0, 0, numDepthCols / 4, numDepthRows);

            // BIND THE GLSL PROGRAM THAT WILL DO THE PROCESSING
            if (grnScrenProgram.bind()) {
                // BIND VERTEX BUFFER FOR DRAWING TRIANGLES ON SCREEN
                if (quadVertexBuffer.bind()) {
                    // BIND INDICES BUFFER FOR DRAWING TRIANGLES ON SCREEN
                    if (quadIndexBuffer.bind()) {
                        // BIND THE INCOMING DEPTH VIDEO FRAME BUFFER
                        glActiveTexture(GL_TEXTURE0);
                        textureDepth->bind();
                        grnScrenProgram.setUniformValue("qt_textureA", 0);

                        // ON EVERY 60TH FRAME, ROTATE IN THE ALL ZERO TEXTURE
                        // OTHERWISE USE THE FBO FROM THE PREVIOUS FRAME AS A WAY
                        // OF ACCUMULATING MAXIMUM PIXELS OVER 60 FRAMES
                        glActiveTexture(GL_TEXTURE1);
                        greenScreenTexture->bind();
                        grnScrenProgram.setUniformValue("qt_textureB", 1);

                        // SET THE USER SENSITIVITY
                        grnScrenProgram.setUniformValue("qt_threshold", threshold);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(grnScrenProgram.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        grnScrenProgram.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE INDICES BUFFERS
                        quadIndexBuffer.release();
                    }
                    // RELEASE THE VERTICES BUFFERS
                    quadVertexBuffer.release();
                }
                // RELEASE THE GLSL PROGRAM
                grnScrenProgram.release();
            }
            // RELEASE THE FRAME BUFFER OBJECT
            grnScrFBO->release();
        }

        if (enablePixelCountFlag) {
            // DOWNLOAD THE MASKED DEPTH BUFFER BACK TO THE CPU
            glBindTexture(GL_TEXTURE_2D, grnScrFBO->texture());
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_SHORT, localDepth.constPointer());

            int blobSize = localDepth.nonZeroPixelsCount();
            *((unsigned int *)depth.constFrame(channel % depth.frames())) = blobSize;

            qDebug() << "blob size" << blobSize;

            // SET THE ANCHOR POINT TO TRIGGER SAVING THIS FRAME TO DISK
            if (triggerThreshold > -1){
                static int blobFrameCounter = 0;
                if (blobSize > triggerThreshold){
                    blobFrameCounter += 2;
                    depth.setConstAnchor(QPoint(blobFrameCounter, numDepthRows/2));
                } else {
                    blobFrameCounter = 0;
                }
            }
        } else {
            // DOWNLOAD THE MASKED DEPTH BUFFER BACK TO THE CPU
            glBindTexture(GL_TEXTURE_2D, grnScrFBO->texture());
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_SHORT, depth.constFrame(channel % depth.frames()));
        }

        // RELEASE THE CURRENT CONTEXT
        doneCurrent();
    }
}

