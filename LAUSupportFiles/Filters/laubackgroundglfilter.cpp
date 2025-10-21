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

#include "laubackgroundglfilter.h"

#include <QInputDialog>
#include <QLabel>
#include <locale.h>

using namespace libtiff;

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUBackgroundWidget::LAUBackgroundWidget(LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent) : LAU3DVideoWidget(color, device, parent)
{
    // POPULATE THE INTERFACE WITH A RECORD AND RESET BUTTON
    recordButton = new QPushButton(QString("Record"));
    recordButton->setFocusPolicy(Qt::NoFocus);
    resetButton = new QPushButton(QString("Reset"));
    resetButton->setFocusPolicy(Qt::NoFocus);

    QDialogButtonBox *box = new QDialogButtonBox();
    box->addButton(recordButton, QDialogButtonBox::AcceptRole);
    box->addButton(resetButton, QDialogButtonBox::RejectRole);

    ((QVBoxLayout *)(this->layout()))->addSpacing(12);
    this->layout()->addWidget(box);

    // SET FOCUS POLICY SO ARROW KEYS REACH THIS WIDGET'S KEY HANDLER
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();

    // MAKE SURE WE HAVE A VALID CAMERA
    if (camera && camera->isValid()) {
        // DEFAULT THE DISPLAYED CHANNEL TO ZERO
        glWidget->onSetCamera(0);

        if (camera->sensors() == 1){
            LAUBackgroundGLFilter *filter = new LAUBackgroundGLFilter(camera->depthWidth(), camera->depthHeight(), camera->colorWidth(), camera->colorHeight(), camera->color(), camera->device());
            filter->setMaxDistance(camera->maxDistance() / camera->scaleFactor());
            filter->setFieldsOfView(camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians());
            filter->setJetrVector(0, camera->jetr());
            filter->setCamera(0);

            // CONNECT THE RECORD AND RESET BUTTONS TO THE BACKGROUND FILTER OBJECT
            connect(recordButton, SIGNAL(clicked()), filter, SLOT(onPreserveBackgroundToSettings()));
            connect(resetButton, SIGNAL(clicked()), filter, SLOT(onReset()));

            // CONNECT THE FILTER'S BACKGROUND SIGNAL TO THIS WIDGET'S SLOT
            connect(filter, SIGNAL(emitBackground(LAUMemoryObject)), this, SLOT(onReceiveBackground(LAUMemoryObject)));

            // STORE THE FILTER IN THE LIST
            backgroundFilters.append(filter);

            // INSERT THE FILTER INTO THE SIGNAL CHAIN
            prependFilter(filter);

            // CONNECT THE RECORD BUTTON TO GRAB A SCAN AND SEND IT TO THE USER
            connect(recordButton, SIGNAL(clicked()), this, SLOT(onRecordButtonClicked()));
        } else {
            // ASK USER WHAT CHANNEL TO DISPLAY ON SCREEN
            bool ok = false;
            int val = QInputDialog::getInt(this, QString("Background Widget"), QString("Which channel?"), 0, 0, camera->sensors()-1, 1, &ok);
            if (ok) {
                glWidget->onSetCamera(val);
            }

            // CREATE A LIST OF GLCONTEXT FILTERS
            QList<QObject*> filters;
            for (unsigned int chn = 0; chn < camera->sensors(); chn++){
                LAUBackgroundGLFilter *filter = new LAUBackgroundGLFilter(camera->depthWidth(), camera->depthHeight(), camera->colorWidth(), camera->colorHeight(), camera->color(), camera->device());
                filter->setMaxDistance(camera->maxDistance());
                filter->setFieldsOfView(camera->horizontalFieldOfViewInRadians(), camera->verticalFieldOfViewInRadians());
                if (camera->sensors() == 1){
                    filter->setCamera(-1);
                } else {
                    filter->setCamera(chn);
                }
                filter->setJetrVector(chn, camera->jetr(chn));

                // CONNECT THE RECORD AND RESET BUTTONS TO THE BACKGROUND FILTER OBJECT
                connect(recordButton, SIGNAL(clicked()), filter, SLOT(onPreserveBackgroundToSettings()));
                connect(resetButton, SIGNAL(clicked()), filter, SLOT(onReset()));

                // CONNECT THE FILTER'S BACKGROUND SIGNAL TO THIS WIDGET'S SLOT
                connect(filter, SIGNAL(emitBackground(LAUMemoryObject)), this, SLOT(onReceiveBackground(LAUMemoryObject)));

                // STORE THE FILTER IN THE LIST
                backgroundFilters.append(filter);

                // CONNECT THE NEW FILTER TO THE PREVIOUS FILTER
                if (filters.count() > 0){
                    connect((LAUBackgroundGLFilter*)filters.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), filter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
                }

                // ADD THE NEW BACKGROUND FILTER TO THE LIST OF FILTERS
                filters.append(filter);
            }

            // INSERT THE FILTER INTO THE SIGNAL CHAIN
            prependFilters(filters);

            // CONNECT THE RECORD BUTTON TO GRAB A SCAN AND SEND IT TO THE USER
            connect(recordButton, SIGNAL(clicked()), this, SLOT(onRecordButtonClicked()));
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUBackgroundWidget::LAUBackgroundWidget(QList<LAUVideoPlaybackDevice> devices, LAUVideoPlaybackColor color, QWidget *parent) : LAU3DVideoWidget(color, DeviceUndefined, parent)
{
    // VALIDATE INPUT - need at least one device
    if (devices.isEmpty()) {
        QMessageBox::critical(this, QString("Background Widget"), QString("No camera devices specified."));
        return;
    }

    // CREATE CAMERAS FROM DEVICE LIST AND VALIDATE RESOLUTIONS
    QList<LAU3DCamera*> cameras;
    unsigned int refDepthWidth = 0, refDepthHeight = 0;
    unsigned int refColorWidth = 0, refColorHeight = 0;

    for (int i = 0; i < devices.count(); i++) {
        LAU3DCamera *cam = LAU3DCameras::getCamera(color, devices[i]);

        if (!cam || !cam->isValid()) {
            QString errorMsg = cam ? cam->error() : QString("Failed to create camera");
            QMessageBox::critical(this, QString("Background Widget"),
                                QString("Failed to connect to camera device %1: %2").arg(i).arg(errorMsg));
            // Clean up any cameras we've created
            for (LAU3DCamera *c : cameras) {
                delete c;
            }
            if (cam) delete cam;
            return;
        }

        // Set reference resolution from first camera
        if (i == 0) {
            refDepthWidth = cam->depthWidth();
            refDepthHeight = cam->depthHeight();
            refColorWidth = cam->colorWidth();
            refColorHeight = cam->colorHeight();
            // Use the first camera as the primary camera for the widget
            camera = cam;
        } else {
            // Validate that all cameras have the same resolution
            if (cam->depthWidth() != refDepthWidth || cam->depthHeight() != refDepthHeight ||
                cam->colorWidth() != refColorWidth || cam->colorHeight() != refColorHeight) {
                QMessageBox::critical(this, QString("Background Widget"),
                                    QString("Camera %1 resolution mismatch!\nExpected depth: %2x%3, color: %4x%5\nGot depth: %6x%7, color: %8x%9")
                                    .arg(i)
                                    .arg(refDepthWidth).arg(refDepthHeight)
                                    .arg(refColorWidth).arg(refColorHeight)
                                    .arg(cam->depthWidth()).arg(cam->depthHeight())
                                    .arg(cam->colorWidth()).arg(cam->colorHeight()));
                // Clean up cameras
                for (LAU3DCamera *c : cameras) {
                    delete c;
                }
                delete cam;
                return;
            }
        }

        cameras.append(cam);
    }

    // POPULATE THE INTERFACE WITH A RECORD AND RESET BUTTON
    recordButton = new QPushButton(QString("Record"));
    recordButton->setFocusPolicy(Qt::NoFocus);
    resetButton = new QPushButton(QString("Reset"));
    resetButton->setFocusPolicy(Qt::NoFocus);

    QDialogButtonBox *box = new QDialogButtonBox();
    box->addButton(recordButton, QDialogButtonBox::AcceptRole);
    box->addButton(resetButton, QDialogButtonBox::RejectRole);

    ((QVBoxLayout *)(this->layout()))->addSpacing(12);
    this->layout()->addWidget(box);

    // SET FOCUS POLICY SO ARROW KEYS REACH THIS WIDGET'S KEY HANDLER
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();

    // MAKE SURE WE HAVE A VALID CAMERA
    if (camera && camera->isValid()) {
        // DEFAULT THE DISPLAYED CHANNEL TO ZERO
        glWidget->onSetCamera(0);

        // CREATE A LIST OF GLCONTEXT FILTERS FOR ALL CAMERAS
        QList<QObject*> filters;
        int sensorIndex = 0;

        for (int camIdx = 0; camIdx < cameras.count(); camIdx++) {
            LAU3DCamera *cam = cameras[camIdx];

            // Create filters for each sensor in this camera
            for (unsigned int sensorChn = 0; sensorChn < cam->sensors(); sensorChn++) {
                LAUBackgroundGLFilter *filter = new LAUBackgroundGLFilter(
                    cam->depthWidth(), cam->depthHeight(),
                    cam->colorWidth(), cam->colorHeight(),
                    cam->color(), cam->device());

                filter->setMaxDistance(cam->maxDistance() / cam->scaleFactor());
                filter->setFieldsOfView(cam->horizontalFieldOfViewInRadians(), cam->verticalFieldOfViewInRadians());
                filter->setCamera(sensorIndex);
                filter->setJetrVector(sensorIndex, cam->jetr(sensorChn));

                // CONNECT THE RECORD AND RESET BUTTONS TO THE BACKGROUND FILTER OBJECT
                connect(recordButton, SIGNAL(clicked()), filter, SLOT(onPreserveBackgroundToSettings()));
                connect(resetButton, SIGNAL(clicked()), filter, SLOT(onReset()));

                // CONNECT THE FILTER'S BACKGROUND SIGNAL TO THIS WIDGET'S SLOT
                connect(filter, SIGNAL(emitBackground(LAUMemoryObject)), this, SLOT(onReceiveBackground(LAUMemoryObject)));

                // STORE THE FILTER IN THE LIST
                backgroundFilters.append(filter);

                // CONNECT THE NEW FILTER TO THE PREVIOUS FILTER
                if (filters.count() > 0) {
                    connect((LAUBackgroundGLFilter*)filters.last(), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
                            filter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
                }

                // ADD THE NEW BACKGROUND FILTER TO THE LIST OF FILTERS
                filters.append(filter);
                sensorIndex++;
            }
        }

        // INSERT THE FILTERS INTO THE SIGNAL CHAIN
        prependFilters(filters);

        // CONNECT THE RECORD BUTTON TO GRAB A SCAN AND SEND IT TO THE USER
        connect(recordButton, SIGNAL(clicked()), this, SLOT(onRecordButtonClicked()));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUBackgroundWidget::~LAUBackgroundWidget()
{
    // NOTE: Do NOT delete backgroundFilters here - they are managed by the parent class
    // LAU3DVideoWidget via filterControllers and will be deleted in its destructor.
    // Deleting them here would cause a double-delete crash.

    // Background memory objects are now saved immediately when the Record button is clicked
    // via the onReceiveBackground slot, so no need to export from QSettings on destruction
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUBackgroundWidget::onRecordButtonClicked()
{
    if (camera && camera->isValid()) {
        // GRAB A COPY OF THE PACKET WE INTEND TO COPY INTO
        LAUMemoryObject packet = LAUMemoryObject(camera->depthWidth(), camera->depthHeight(), colors(), sizeof(float), 1);

        // COPY SCAN FROM GLWIDGET AND SEND TO VIDEO SINK OBJECT RUNNING IN A SEPARATE THREAD
        if (glWidget) {
            // FIRST GRAB THE CURRENT POINT CLOUD FROM THE GPU
            glWidget->copyScan((float *)packet.constPointer());

            // NOW COPY THE JETR VECTOR FROM THE GLWIDGET
            packet.setConstJetr(glWidget->jetr(glWidget->camera()));
        }

        // EMIT THE VIDEO FRAME ONLY IF THE PACKET IS VALID
        if (packet.isValid() && packet.constPointer() != nullptr) {
            emit emitVideoFrames(packet);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUBackgroundWidget::onReceiveBackground(LAUMemoryObject background)
{
    // For Mini project (single camera): backgrounds are saved to QSettings only
    // For Background project (multi-camera): collect and concatenate all backgrounds

    if (backgroundFilters.count() == 1) {
        // SINGLE CAMERA - Background is saved to QSettings in onPreserveBackgroundToSettings()
        // The scan is emitted via onRecordButtonClicked() which grabs from glWidget
        // Do nothing here
    } else {
        // MULTIPLE CAMERAS - COLLECT AND CONCATENATE FOR BACKGROUND PROJECT

        // COLLECT THE BACKGROUND FROM THIS SENSOR
        collectedBackgrounds.append(background);

        // CHECK IF WE'VE RECEIVED ALL BACKGROUNDS (one per filter)
        if (collectedBackgrounds.count() == backgroundFilters.count()) {
            // ALL BACKGROUNDS RECEIVED - CONCATENATE THEM VERTICALLY

            // Get dimensions from the first background
            unsigned int width = collectedBackgrounds[0].width();
            unsigned int height = collectedBackgrounds[0].height();
            unsigned int colors = collectedBackgrounds[0].colors();
            unsigned int depth = collectedBackgrounds[0].depth();

            // Calculate total stacked height
            unsigned int totalHeight = height * collectedBackgrounds.count();

            // CREATE A NEW MEMORY OBJECT TO HOLD THE STACKED IMAGES
            // Format: 640 x (480 + 480 + 480) for 3 cameras
            LAUMemoryObject stackedBackground(width, totalHeight, colors, depth, 1);

            // COPY EACH BACKGROUND INTO THE STACKED IMAGE
            for (int i = 0; i < collectedBackgrounds.count(); i++) {
                unsigned int yOffset = i * height;
                unsigned char *srcPtr = (unsigned char*)collectedBackgrounds[i].constPointer();
                unsigned char *dstPtr = (unsigned char*)stackedBackground.constPointer();

                // Calculate bytes per row
                unsigned int bytesPerRow = width * colors * depth;

                // Copy this background to its position in the stacked image
                for (unsigned int row = 0; row < height; row++) {
                    unsigned char *srcRowPtr = srcPtr + (row * bytesPerRow);
                    unsigned char *dstRowPtr = dstPtr + ((yOffset + row) * bytesPerRow);
                    memcpy(dstRowPtr, srcRowPtr, bytesPerRow);
                }
            }

            // SAVE THE STACKED BACKGROUND TO DISK
            // Passing an empty QString will open a file dialog
            stackedBackground.save(QString());

            // CLEAR THE COLLECTED BACKGROUNDS FOR NEXT TIME
            collectedBackgrounds.clear();
        }
    }
}
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUBackgroundGLFilter::~LAUBackgroundGLFilter()
{
    makeCurrent(surface);
    if (maxPixelTextureX) {
        delete maxPixelTextureX;
    }
    if (maxPixelTextureZ) {
        delete maxPixelTextureZ;
    }
    for (int n = 0; n < 4; n++) {
        if (maxPixelFBO[n]) {
            delete maxPixelFBO[n];
        }
    }
    doneCurrent();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUBackgroundGLFilter::initializeGL()
{
    // CREATE TEXTURE FOR HOLDING ALL MAX DEPTH FRAME FOR CALCULATING THE BACKGROUND IMAGE
    maxPixelTextureX = new QOpenGLTexture(QOpenGLTexture::Target2D);
    maxPixelTextureX->setSize(numDepthCols / 4, numDepthRows);
    maxPixelTextureX->setFormat(QOpenGLTexture::RGBA32F);
    maxPixelTextureX->setWrapMode(QOpenGLTexture::ClampToBorder);
    maxPixelTextureX->setMinificationFilter(QOpenGLTexture::Nearest);
    maxPixelTextureX->setMagnificationFilter(QOpenGLTexture::Nearest);
    maxPixelTextureX->allocateStorage();

    // CREATE TEXTURE FOR HOLDING ALL ZERO DEPTH FRAME FOR CALCULATING THE BACKGROUND IMAGE
    maxPixelTextureZ = new QOpenGLTexture(QOpenGLTexture::Target2D);
    maxPixelTextureZ->setSize(numDepthCols / 4, numDepthRows);
    maxPixelTextureZ->setFormat(QOpenGLTexture::RGBA32F);
    maxPixelTextureZ->setWrapMode(QOpenGLTexture::ClampToBorder);
    maxPixelTextureZ->setMinificationFilter(QOpenGLTexture::Nearest);
    maxPixelTextureZ->setMagnificationFilter(QOpenGLTexture::Nearest);
    maxPixelTextureZ->allocateStorage();

    // INITIALIZE ALL PIXELS TO ZERO
    unsigned short *pixels = (unsigned short *)malloc(numDepthCols * numDepthRows * sizeof(unsigned short));
    if (pixels) {
        // SET THE ALL MAX TEXTURE
        memset(pixels, 0xff, numDepthCols * numDepthRows * sizeof(unsigned short));
        maxPixelTextureX->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt16, (const void *)pixels);
        // SET THE ALL ZEROS TEXTURE
        memset(pixels, 0x00, numDepthCols * numDepthRows * sizeof(unsigned short));
        maxPixelTextureZ->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt16, (const void *)pixels);
    }
    free(pixels);

    // CREATE THE FRAME BUFFER OBJECT TO HOLD THE SCAN RESULTS AS A TEXTURE
    QOpenGLFramebufferObjectFormat frameBufferObjectFormat;
    frameBufferObjectFormat.setInternalTextureFormat(GL_RGBA32F);
    for (int n = 0; n < 4; n++) {
        maxPixelFBO[n] = new QOpenGLFramebufferObject(QSize(numDepthCols / 4, numDepthRows), frameBufferObjectFormat);
        maxPixelFBO[n]->release();
    }

    // Override system locale until shaders are compiled
    setlocale(LC_NUMERIC, "C");

    // CREATE SHADER FOR EXTRACTING THE MAXIMUM PIXEL FROM TWO TEXTURES
    minPixelProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/extractMinimumPixel.vert");
    minPixelProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/extractMinimumPixel.frag");
    minPixelProgram.link();

    // CREATE SHADER FOR EXTRACTING THE MAXIMUM PIXEL FROM TWO TEXTURES
    maxPixelProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/extractMaximumPixel.vert");
    maxPixelProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/extractMaximumPixel.frag");
    maxPixelProgram.link();

    // Restore system locale
    setlocale(LC_ALL, "");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUBackgroundGLFilter::updateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    Q_UNUSED(color);
    Q_UNUSED(mapping);

    // CHECK IF DEPTH IS VALID AND HAS A VALID ELAPSED TIME
    // Skip frames where camera didn't update correctly
    if (!depth.isValid() || !depth.isElapsedValid()) {
        return;
    }

    if (makeCurrent(surface)) {
        // FOR EACH AND EVERY FRAME COMING IN FROM THE CAMERA, WE NEED TO APPLY THE
        // MINIMUM PIXEL FILTER IN ORDER TO ACCOUNT FOR GAUSSIAN NOISE ON EACH PIXEL
        if (channel > -1){
            int frameIndex = channel % depth.frames();
            const unsigned short *depthPtr = (const unsigned short *)depth.constFrame(frameIndex);

            textureDepth->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt16, (const void *)depthPtr);
            // NOTE: Do NOT zero the buffer in multi-sensor mode - all filters share the same buffer
            // Only zero in single-sensor mode (channel == -1)
        } else {
            textureDepth->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt16, (const void *)depth.constFrame(0));
            // In single-sensor mode, zero the buffer after reading
            memset(depth.constFrame(0), 0, depth.block());
        }
        if (maxPixelFBO[(frameCounter + 0) % 2]->bind()) {
            // SET THE VIEW PORT AND CLEAR THE SCREEN BUFFER
            glViewport(0, 0, numDepthCols / 4, numDepthRows);

            // BIND THE GLSL PROGRAM THAT WILL DO THE PROCESSING
            if (minPixelProgram.bind()) {
                // BIND VERTEX BUFFER FOR DRAWING TRIANGLES ON SCREEN
                if (quadVertexBuffer.bind()) {
                    // BIND INDICES BUFFER FOR DRAWING TRIANGLES ON SCREEN
                    if (quadIndexBuffer.bind()) {
                        // BIND THE INCOMING DEPTH VIDEO FRAME BUFFER
                        glActiveTexture(GL_TEXTURE0);
                        textureDepth->bind();
                        minPixelProgram.setUniformValue("qt_textureA", 0);

                        // ON EVERY 60TH FRAME, ROTATE IN THE ALL ZERO TEXTURE
                        // OTHERWISE USE THE FBO FROM THE PREVIOUS FRAME AS A WAY
                        // OF ACCUMULATING MAXIMUM PIXELS OVER 60 FRAMES
                        glActiveTexture(GL_TEXTURE1);
                        if ((frameCounter % maxFilterFrameCount) == 0) {
                            // BIND ALL ZERO TEXTURE
                            maxPixelTextureX->bind();
                        } else {
                            // BIND FBO TEXTURE FROM PREVIOUS FRAME
                            glBindTexture(GL_TEXTURE_2D, maxPixelFBO[(frameCounter + 1) % 2]->texture());
                        }
                        // BIND ALL ZERO TEXTURE
                        minPixelProgram.setUniformValue("qt_textureB", 1);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(minPixelProgram.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        minPixelProgram.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE INDICES BUFFERS
                        quadIndexBuffer.release();
                    }
                    // RELEASE THE VERTICES BUFFERS
                    quadVertexBuffer.release();
                }
                // RELEASE THE GLSL PROGRAM
                minPixelProgram.release();
            }
            // RELEASE THE FRAME BUFFER OBJECT
            maxPixelFBO[(frameCounter + 0) % 2]->release();
        }

        // LETS DEFINE A TARGET INDEX AND SET IT TO AN ALTERNATING TEXTURE
        int targetA = (frameCounter / maxFilterFrameCount) % 2 + 2;
        int targetB = (frameCounter / maxFilterFrameCount + 1) % 2 + 2;

        // ON A RESET, WE NEED TO CLEAR THE MAX PIXEL FRAME BUFFER TEXTURES
        // OTHERWISE, WE NEED TO UPDATE THE MAX PIXEL FBO WITH THE MIN PIXEL FBO
        if (frameCounter < 2) {
            // BIND AN ALTERNATING OUTPUT TEXTURE FOR THE MINIMUM PIXEL FILTER
            if (maxPixelFBO[targetA]->bind()) {
                // SET THE VIEW PORT AND CLEAR THE SCREEN BUFFER
                glViewport(0, 0, numDepthCols / 4, numDepthRows);

                // BIND THE GLSL PROGRAM THAT WILL DO THE PROCESSING
                if (maxPixelProgram.bind()) {
                    // BIND VERTEX BUFFER FOR DRAWING TRIANGLES ON SCREEN
                    if (quadVertexBuffer.bind()) {
                        // BIND INDICES BUFFER FOR DRAWING TRIANGLES ON SCREEN
                        if (quadIndexBuffer.bind()) {
                            // RESET THE FBO WITH ALL ZEROS
                            glActiveTexture(GL_TEXTURE0);
                            maxPixelTextureZ->bind();
                            maxPixelProgram.setUniformValue("qt_textureA", 0);
                            maxPixelProgram.setUniformValue("qt_textureB", 0);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(minPixelProgram.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            maxPixelProgram.enableAttributeArray("qt_vertex");
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE INDICES BUFFERS
                            quadIndexBuffer.release();
                        }
                        // RELEASE THE VERTICES BUFFERS
                        quadVertexBuffer.release();
                    }
                    // RELEASE THE GLSL PROGRAM
                    maxPixelProgram.release();
                }
                // RELEASE THE FRAME BUFFER OBJECT
                maxPixelFBO[targetA]->release();
            }
        } else if (frameCounter % maxFilterFrameCount == 0) {
            // BIND AN ALTERNATING OUTPUT TEXTURE FOR THE MINIMUM PIXEL FILTER
            if (maxPixelFBO[targetA]->bind()) {
                // SET THE VIEW PORT AND CLEAR THE SCREEN BUFFER
                glViewport(0, 0, numDepthCols / 4, numDepthRows);

                // BIND THE GLSL PROGRAM THAT WILL DO THE PROCESSING
                if (maxPixelProgram.bind()) {
                    // BIND VERTEX BUFFER FOR DRAWING TRIANGLES ON SCREEN
                    if (quadVertexBuffer.bind()) {
                        // BIND INDICES BUFFER FOR DRAWING TRIANGLES ON SCREEN
                        if (quadIndexBuffer.bind()) {
                            // BIND THE FBO TEXTURE OUTPUT BY THE MAXIMUM PIXEL FILTER
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, maxPixelFBO[(frameCounter + 1) % 2]->texture());
                            maxPixelProgram.setUniformValue("qt_textureA", 0);

                            // BIND FBO TEXTURE FROM PREVIOUS FRAME
                            glActiveTexture(GL_TEXTURE1);
                            glBindTexture(GL_TEXTURE_2D, maxPixelFBO[targetB]->texture());
                            maxPixelProgram.setUniformValue("qt_textureB", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(minPixelProgram.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            maxPixelProgram.enableAttributeArray("qt_vertex");
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE INDICES BUFFERS
                            quadIndexBuffer.release();
                        }
                        // RELEASE THE VERTICES BUFFERS
                        quadVertexBuffer.release();
                    }
                    // RELEASE THE GLSL PROGRAM
                    maxPixelProgram.release();
                }
                // RELEASE THE FRAME BUFFER OBJECT
                maxPixelFBO[targetA]->release();
            }
        }
        // INCREMENT FRAME COUNTER FOR NEXT TIME AROUND
        frameCounter++;

        // NOW COPY FROM CURRENT RESULTING TEXTURE FROM THE FBO AND
        // OVERWRITE THE INCOMING DEPTH VIDEO BUFFER SO WE CAN DRAW
        // IT IN THE CODE COPIED OVER FROM THE PRIME SENSE GLWIDGET
        if (frameCounter < maxFilterFrameCount) {
            glBindTexture(GL_TEXTURE_2D, maxPixelFBO[(frameCounter + 0) % 2]->texture());
        } else {
            glBindTexture(GL_TEXTURE_2D, maxPixelFBO[targetA]->texture());
        }
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        if (channel > -1){
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_SHORT, (void *)depth.constFrame(channel % depth.frames()));
        } else {
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_SHORT, (void *)depth.constFrame(0));
        }

        // RELEASE THE CURRENT CONTEXT
        doneCurrent();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUBackgroundGLFilter::inPaint(LAUMemoryObject object, unsigned short fill)
{
    // MAKE LIST OF PIXELS CORRESPONDING TO HOLES
    int numCols = object.width();
    int numRows = object.height();
    unsigned short *buffer = (unsigned short *)object.constPointer();

#define USE_ZMAX_TO_INPAINT
#ifdef USE_ZMAX_TO_INPAINT
    // ITERATE THROUGH EACH PIXEL AND ADD TO LIST IF ITS EMPTY
    for (int row = 0; row < numRows; row++) {
        for (int col = 0; col < numCols; col++) {
            if (buffer[row * numCols + col] == 0) {
                buffer[row * numCols + col] = fill;
            }
        }
    }
#else
    // MAKE LIST OF PIXELS CORRESPONDING TO HOLES
    QList<QPoint> pixels;

    // ITERATE THROUGH EACH PIXEL AND ADD TO LIST IF ITS EMPTY
    for (int row = 0; row < numRows; row++) {
        for (int col = 0; col < numCols; col++) {
            if (buffer[row * numCols + col] == 0) {
                pixels.append(QPoint(col, row));
            }
        }
    }

    // MAKE SURE WE HAVE AT LEAST ONE VALID PIXEL TO BUILD FROM
    if (pixels.count() < (numRows * numCols)) {
        // CONTINUOUSLY ITERATE THROUGH THE PIXEL LIST UNTIL WE'VE WIDDLED IT AWAY COMPLETELY
        QList<QPoint> inPixels;
        QList<QPoint> otPixels;
        while (pixels.count() > 0) {
            while (pixels.count() > 0) {
                QPoint pixel = pixels.takeFirst();

                // TEST PIXEL TO SEE IF ONE OF ITS FOUR NEIGHBORS EXIST
                unsigned short cumSum = 0xffff;

                // TEST THE LEFT-NEIGHBOR
                if (pixel.x() > 0) {
                    unsigned short val = buffer[(pixel.y() + 0) * numCols + (pixel.x() - 1)];
                    if (val) {
                        cumSum = (unsigned short)qMin(cumSum, val);
                    }
                }

                // TEST THE RIGHT-NEIGHBOR
                if (pixel.x() + 1 < numCols) {
                    unsigned short val = buffer[(pixel.y() + 0) * numCols + (pixel.x() + 1)];
                    if (val) {
                        cumSum = (unsigned short)qMin(cumSum, val);
                    }
                }

                // TEST THE ABOVE-NEIGHBOR
                if (pixel.y() > 0) {
                    unsigned short val = buffer[(pixel.y() - 1) * numCols + (pixel.x() + 0)];
                    if (val) {
                        cumSum = (unsigned short)qMin(cumSum, val);
                    }
                }

                // TEST THE BELOW-NEIGHBOR
                if (pixel.y() + 1 < numRows) {
                    unsigned short val = buffer[(pixel.y() + 1) * numCols + (pixel.x() + 0)];
                    if (val) {
                        cumSum = (unsigned short)qMin(cumSum, val);
                    }
                }

                // IF IT DOES NOT HAVE A NEIGHBOR, ADD IT TO THE END OF THE LIST AND WE WILL TRY AGAIN
                // DURING THE NEXT TIME THROUGH THE PIXEL LIST OTHERWISE UPDATE THE PIXEL IN THE MEMORY OBJECT
                if (cumSum != 0xffff) {
                    otPixels.append(QPoint(pixel.y()*numCols + pixel.x(), (int)cumSum));
                } else {
                    inPixels.append(pixel);
                }
            }

            // NOW THAT WE HAVE A LIST OF PIXELS THAT CAN BE IN-FILLED, LET'S FILL THEM IN
            while (otPixels.count() > 0) {
                QPoint point = otPixels.takeFirst();
                buffer[point.x()] = (unsigned short)point.y();
            }

            // NOW COPY THE LIST OF PIXELS THAT STILL NEED IN-FILLING BACK
            // TO OUR PIXELS LIST AND REMOVE THEM FORM THE SEPARATE LIST
            pixels = inPixels;
            inPixels.clear();
        }
    }
#endif
    //QString tempFileString = QStandardPaths::writableLocation(QStandardPaths::TempLocation).append("/background.tif");
    //object.save(tempFileString);
    //qDebug() << tempFileString;

    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUBackgroundGLFilter::onEmitBackground()
{
    // CHECK IF WE HAVE ENOUGH FRAMES TO CREATE A VALID BACKGROUND
    if (frameCounter < maxFilterFrameCount) {
        qDebug() << "Background not ready - need" << maxFilterFrameCount << "frames, currently have" << frameCounter;
        return;
    }

    // MAKE SURE WE HAVE A VALID TEXTURE TO COPY FROM
    if (maxPixelFBO[3]) {
        // SET THIS CONTEXT AS THE CURRENT CONTEXT
        makeCurrent(surface);

        // CREATE A MEMORY OBJECT AND DUMP THE LAST BACKGROUND BUFFER INTO IT
        LAUMemoryObject object(numDepthCols, numDepthRows, 1, sizeof(unsigned short), 1);
        glBindTexture(GL_TEXTURE_2D, maxPixelFBO[3]->texture());
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_SHORT, object.pointer());

        // PEFORM IN-PLACE INPAINTING
        inPaint(object, (unsigned short)maxDistance);

        // SET THE JETR VECTOR IN THE MEMORY OBJECT
        int localChannel = qMax(0, channel);
        object.setConstJetr(jetr(localChannel));

        // EMIT THE BACKGROUND MEMORY OBJECT WITHOUT SAVING TO SETTINGS
        emit emitBackground(object);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUBackgroundGLFilter::onPreserveBackgroundToSettings()
{
    // CHECK IF WE HAVE ENOUGH FRAMES TO CREATE A VALID BACKGROUND
    if (frameCounter < maxFilterFrameCount) {
        qDebug() << "Background not ready - need" << maxFilterFrameCount << "frames, currently have" << frameCounter;
        return;
    }

    // MAKE SURE WE HAVE A VALID TEXTURE TO COPY FROM
    if (maxPixelFBO[3]) {
        // SET THIS CONTEXT AS THE CURRENT CONTEXT
        makeCurrent(surface);

        // CREATE A MEMORY OBJECT AND DUMP THE LAST BACKGROUND BUFFER INTO IT
        LAUMemoryObject object(numDepthCols, numDepthRows, 1, sizeof(unsigned short), 1);
        glBindTexture(GL_TEXTURE_2D, maxPixelFBO[3]->texture());
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_SHORT, object.pointer());

        // PEFORM IN-PLACE INPAINTING
        inPaint(object, (unsigned short)maxDistance);

        // SET THE JETR VECTOR IN THE MEMORY OBJECT
        int localChannel = qMax(0, channel);
        object.setConstJetr(jetr(localChannel));

        // COPY MEMORY OBJECT INTO BYTE ARRAY FOR SAVING TO SETTINGS
        int length = object.length();
        QByteArray byteArray(length, (char)0);
        memcpy(byteArray.data(), object.constPointer(), length);

        // SAVE THE BACKGROUND BUFFER TO SETTINGS IN CASE WE NEED IT FOR GREEN SCREENING
        QSettings settings;
        QString frameString, jetrvString;
        if (playbackDevice == DeviceRealSense) {
            frameString = QString("LAUBackgroundGLFilter::backgroundTexture::DeviceRealSense::%1").arg(localChannel);
            jetrvString = QString("LAUBackgroundGLFilter::jetrVector::DeviceRealSense::%1").arg(localChannel);
        } else if (playbackDevice == DeviceKinect) {
            frameString = QString("LAUBackgroundGLFilter::backgroundTexture::DeviceKinect::%1").arg(localChannel);
            jetrvString = QString("LAUBackgroundGLFilter::jetrVector::DeviceKinect::%1").arg(localChannel);
        } else if (playbackDevice == DeviceVidu) {
            frameString = QString("LAUBackgroundGLFilter::backgroundTexture::DeviceVidu::%1").arg(localChannel);
            jetrvString = QString("LAUBackgroundGLFilter::jetrVector::DeviceVidu::%1").arg(localChannel);
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
        settings.setValue(frameString, byteArray);

        QVariantList list;
        for (const double& value : jetr(localChannel)) {
            list.append(value);
        }
        settings.setValue(jetrvString, list);

        // EMIT THE BACKGROUND MEMORY OBJECT SO IT CAN BE SAVED TO DISK
        emit emitBackground(object);
    }
}

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUBackgroundDialog::LAUBackgroundDialog(LAUVideoPlaybackColor color, QList<LAU3DCamera*> cameraList, QWidget *parent) : QDialog(parent), widget(nullptr), cameras(cameraList), sensorCount(0)
{
    this->setWindowTitle(QString("Background Filter - Multi-Camera"));
    this->setLayout(new QVBoxLayout());
#ifdef Q_OS_WIN
    this->layout()->setContentsMargins(6, 6, 6, 6);
#else
    this->layout()->setContentsMargins(6, 6, 6, 6);
#endif

    // Calculate total sensor count across all cameras
    for (int cam = 0; cam < cameras.count(); cam++) {
        sensorCount += cameras.at(cam)->sensors();
    }

    if (sensorCount == 0) {
        QLabel *errorLabel = new QLabel(QString("No cameras provided!"));
        this->layout()->addWidget(errorLabel);
        return;
    }

    // Create a LAU3DVideoWidget to display the camera feed
    // We'll use the first camera's device type for the widget
    LAUVideoPlaybackDevice device = DeviceUndefined;
    if (cameras.count() > 0) {
        device = cameras.at(0)->device();
    }

    widget = new LAU3DVideoWidget(color, device);
    widget->setFocusPolicy(Qt::StrongFocus);
    widget->setFocus();

    // Ask user what channel to display on screen if multiple sensors
    if (sensorCount > 1) {
        bool ok = false;
        int val = QInputDialog::getInt(this, QString("Background Filter"), QString("Which sensor channel to display?"), 0, 0, sensorCount - 1, 1, &ok);
        if (ok) {
            widget->onSetCamera(val);
        }
    } else {
        widget->onSetCamera(0);
    }

    // Create background filters for each camera's sensors
    QList<QObject*> filters;
    int globalSensorIndex = 0;

    for (int cam = 0; cam < cameras.count(); cam++) {
        LAU3DCamera *camera = cameras.at(cam);

        for (unsigned int sns = 0; sns < camera->sensors(); sns++) {
            LAUBackgroundGLFilter *filter = new LAUBackgroundGLFilter(
                camera->depthWidth(), camera->depthHeight(),
                camera->colorWidth(), camera->colorHeight(),
                color, camera->device());

            filter->setMaxDistance(camera->maxDistance());
            filter->setFieldsOfView(camera->horizontalFieldOfViewInRadians(),
                                   camera->verticalFieldOfViewInRadians());
            filter->setCamera(globalSensorIndex);
            filter->setJetrVector(globalSensorIndex, camera->jetr(sns));

            // Connect filters in chain
            if (filters.count() > 0) {
                connect((LAUBackgroundGLFilter*)filters.last(),
                       SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
                       filter,
                       SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)));
            }

            filters.append(filter);
            globalSensorIndex++;
        }
    }

    // Insert the filter chain into the video widget
    widget->prependFilters(filters);

    // Connect the first camera to the filter chain
    if (cameras.count() > 0 && filters.count() > 0) {
        connect(cameras.at(0), SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
                (LAUBackgroundGLFilter*)filters.at(0), SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)),
                Qt::QueuedConnection);
    }

    this->layout()->addWidget(widget);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUBackgroundDialog::~LAUBackgroundDialog()
{
    // Widget is deleted automatically as a child widget
    // Cameras are NOT owned by this dialog, so don't delete them
}
#endif
