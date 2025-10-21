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

#include "laucolorizedepthglfilter.h"
#include "lau3dvideoglwidget.h"
#include <locale.h>

#include "qopenglfunctions.h"

using namespace libtiff;

#ifdef ENABLECASCADE
#include "lauobjecthashtable.h"
LAUObjectHashTable *LAUColorizerFromDiskRFIDHashTable = nullptr;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUColorizerFromDiskDialog::LAUColorizerFromDiskDialog(QString dirString, QWidget *parent) : QDialog(parent), filterCount(0), directoryString(dirString), outFile(nullptr)
{
    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->setWindowTitle(QString("Raw Video Processor"));

    QStringList inputStrings;
    QDirIterator it(directoryString, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QString string = it.next();
        if (string.endsWith(".tif") || string.endsWith(".tiff")) {
            inputStrings << string;
        }
    }

    // CREATE A HASH TABLE TO IDENTIFY OBJECTS FROM RFID TAG NUMBERS
    LAUColorizerFromDiskRFIDHashTable = new LAUObjectHashTable(QString("C:/Users/Public/Documents/objectIDList.csv"));

    // MAKE SURE WE HAVE AT LEAST ONE FILE TO PROCESS
    if (inputStrings.isEmpty()) {
        return;
    }
    inputStrings.sort();

    // SAVE THE FILE STRING LIST FOR LATER
    fileStringList = inputStrings;

    // KEEP TRACK OF THE FIRST IMAGE IN OUR PROCESSED LIST
    processedStringList << fileStringList.first();

    // LOAD THE FIRST FRAME OF RAW VIDEO
    LAUMemoryObject frame = LAUMemoryObject(fileStringList.first(), 1);

    // SPLIT THE FRAME OF VIDEO INTO FOUR FRAMES
    object = LAUMemoryObject(frame.width(), frame.height()/4, frame.colors(), frame.depth(), 4);
    memcpy(object.constPointer(), frame.constPointer(), qMin(object.length(), frame.length()));

    // CREATE A LOG FILE TO STORE VIDEO METRICS
    rfdFile.setFileName(QString("%1/RFIDlog.txt").arg(directoryString));
    rfdFile.open(QIODevice::ReadWrite);
    rfdTS.setDevice(&rfdFile);

    // ALLOCATE MEMORY OBJECTS TO HOLD INCOMING VIDEO FRAMES
    LAUModalityObject modal;
    modal.depth = LAUMemoryObject(object.width(), object.height(), object.colors(), object.depth(), object.frames());
    modal.color = LAUMemoryObject(object.width(), object.height(), 1, sizeof(unsigned char), object.frames());
    modal.mappi = LAUMemoryObject();
    framesList << modal;

    // CREATE A GLWIDGET TO DISPLAY GRAYSCALE VIDEO
    LAU3DVideoGLWidget *glWidget = new LAU3DVideoGLWidget(object.width(), object.height(), object.width(), object.height(), ColorRGB, Device2DCamera);
    glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    glWidget->setMaximumIntensityValue(255);
    this->layout()->addWidget(glWidget);

    LAUColorizeDepthGLFilter *colorizerFilter = new LAUColorizeDepthGLFilter(object.width(), object.height(), object.width(), object.height(), ColorXYZRGB, DeviceLucid);
    connect(this, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), colorizerFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
    connect(colorizerFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
    connect(glWidget, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), this, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);

    // CONNECT FILTER TO THIS OBJECT SO WE KNOW WE CAN QUIT
    connect(colorizerFilter, SIGNAL(destroyed()), this, SLOT(onFilterDestroyed()));

    // SET THE RADIUS OF THE SMOOTHING FILTER
    colorizerFilter->setRadius(2);

    // SPIN THE GREEN SCREEN FILTER INTO ITS OWN CONTROLLER
    filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractGLFilter *)colorizerFilter));

    filterCount = filterControllers.count();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUColorizerFromDiskDialog::~LAUColorizerFromDiskDialog()
{
    // DELETE THE RFID HASH TABLE
    if (LAUColorizerFromDiskRFIDHashTable){
        delete LAUColorizerFromDiskRFIDHashTable;
    }

    // DELETE THE FILTER CONTROLLERS
    while (filterControllers.isEmpty() == false) {
        delete filterControllers.takeFirst();
    }

    // WAIT UNTIL ALL THE FILTERS HAVE BEEN DESTROYED
    while (filterCount > 0) {
        qApp->processEvents();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUColorizerFromDiskDialog::accept()
{
    // CLOSE THE LOG FILE
    if (rfdFile.isOpen()) {
        rfdFile.close();
    }

    if (LAUColorizerFromDiskRFIDHashTable){
        LAUColorizerFromDiskRFIDHashTable->save(QString("%1/ObjectIDlog.txt").arg(directoryString));
    }

    // SWAP THE OLD FILES WITH THE NEW
    //while (processedStringList.isEmpty() == false){
    //    QFile::moveToTrash(processedStringList.first());
    //    QFile::copy(newlyCreatedFileList.first(), processedStringList.takeFirst());
    //    QFile::moveToTrash(newlyCreatedFileList.takeFirst());
    //}

    QDialog::accept();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUColorizerFromDiskDialog::onUpdateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    static int numFrames = LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(fileStringList.first());
    static int outFileCounter = 0;
    static int fileIndex = 0;

    // RECORD THE INCOMING DEPTH VIDEO TO DISK IF IT EXISTS
    if (outFile){
        if (depth.isValid()) {
            depth.save(outFile);
        }
        if (color.isValid()) {
            color.save(outFile);
        }
    }

    // CONSTRUCT A VIDEO MEMORY OBJECT FROM THE INCOMING MEMORY OBJECTS
    LAUModalityObject frame(depth, color, mapping);

    // SEE IF WE SHOULD KEEP THIS PARTICULAR FRAME
    if (frame.isAnyValid()) {
        framesList << frame;
    }

    // EMIT THE SIGNAL TO TELL THE FRAME GRABBER TO GRAB THE NEXT SET OF FRAMES
    if (isVisible()) {
        while (framesList.count() > 0) {
            while (fileIndex >= numFrames) {
                // DELETE THE FIRST FILE IN THE LIST SINCE WE ARE DONE WITH IT
                fileStringList.takeFirst();

                // OPEN A NEW TIFF FILE TO RECEIVE THE INCOMING VIDEO
                if (outFile){
                    TIFFClose(outFile);
                    outFile = NULL;
                }

                // SWAP THE OLD FILES WITH THE NEW
                while (processedStringList.isEmpty() == false){
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                    QFile::moveToTrash(processedStringList.first());
                    QFile::copy(newlyCreatedFileList.first(), processedStringList.takeFirst());
                    QFile::moveToTrash(newlyCreatedFileList.takeFirst());
#else
                    QFile::remove(processedStringList.first());
                    QFile::copy(newlyCreatedFileList.first(), processedStringList.takeFirst());
                    QFile::remove(newlyCreatedFileList.takeFirst());
#endif
                }

                // IF WE ARE DONE, THEN SEND THE ACCEPT FLAG
                if (fileStringList.isEmpty()) {
                    accept();
                    return;
                }

                // GET THE NUMBER OF FRAMES FOR THE NEXT FILE TO BE PROCESSED
                numFrames = LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(fileStringList.first());
                fileIndex = 0;
            }

            if (outFile == NULL){
                // CREATE A FILESTRING FOR THE NEW FILE WE ARE CREATING
                QString newFileString;
                for (int n = outFileCounter; n < 100000; n++) {
                    newFileString = QString("%1").arg(n);
                    while (newFileString.length() < 5) {
                        newFileString.prepend("0");
                    }
                    newFileString.prepend(QString("/post"));
                    newFileString.prepend(directoryString);
                    newFileString.append(QString(".tif"));
                    if (QFile::exists(newFileString) == false) {
                        outFileCounter = n;
                        break;
                    }
                }

                // SAVE THE FILE ON OUR NEW FILE LIST
                newlyCreatedFileList << newFileString;

                // OPEN NEW FILE
                outFile = TIFFOpen(newFileString.toLocal8Bit(), "w");
            }

            // LOAD THE NEXT FRAME OF VIDEO FROM DISK
            if (object.loadInto(fileStringList.first(), fileIndex++) == false) {
                if (object.loadInto(fileStringList.first(), fileIndex++) == false) {
                    if (object.loadInto(fileStringList.first(), fileIndex++) == false) {
                        object.loadInto(fileStringList.first(), fileIndex++);
                    }
                }
            }

            // KEEP TRACK OF ALL FILES THAT WE HAVE PROCESSED SO FAR
            if (processedStringList.contains(fileStringList.first()) == false) {
                processedStringList << fileStringList.first();
            }

            // SET THE GREEN SCREEN BUT DON'T SEND THE VIDEO FRAME TO THE FILTER PIPELINE
            if (fileIndex == 1) {
                if (outFile){
                    object.save(outFile);
                }
                continue;
            }

            // GET THE NEXT AVAILABLE FRAME BUFFER FROM OUR BUFFER LIST
            LAUModalityObject frame = framesList.takeFirst();

            // KEEP TRACK OF INCOMING RFID TAG STRING
            static QString previousRFIDString;
            QString rfidString = LAUColorizerFromDiskRFIDHashTable->idString(object.rfid(), QTime(0, 0).addMSecs(object.elapsed()));
            if (previousRFIDString != rfidString) {
                previousRFIDString = rfidString;
                if (rfdFile.isOpen()) {
                    rfdTS << QTime(0, 0).addMSecs(frame.depth.elapsed()).toString(Qt::TextDate) << ", " << rfidString << "\n";
                }
            }

            // COPY THE PIXELS OF THE NEXT AVAILABLE VIDEO FRAME INTO THE DEPTH BUFFER OBJECT
            frame.depth.setConstRFID(rfidString);
            frame.depth.setConstAnchor(object.anchor());
            frame.depth.setConstElapsed(object.elapsed());
            frame.depth.setConstTransform(object.transform());
            memcpy(frame.depth.constPointer(), object.constPointer(), qMin(object.length(), frame.depth.length()));
            memset(frame.color.constPointer(), 0, frame.color.length());

            // EMIT THE OBJECT BUFFERS TO THE VIDEO GLWIDGET
            emit emitBuffer(frame.depth, frame.color, frame.mappi);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUColorizeDepthGLFilter::~LAUColorizeDepthGLFilter()
{
    if (wasInitialized() && makeCurrent(surface)) {
        if (texture) {
            delete texture;
        }
        if (colorFBO) {
            delete colorFBO;
        }
        if (depthFBO) {
            delete depthFBO;
        }
    }

    qDebug() << "LAUColorizeDepthGLFilter()::~LAUColorizeDepthGLFilter()";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUColorizeDepthGLFilter::initializeGL()
{
    texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->setSize(numDepthCols, numDepthRows);
    texture->setFormat(QOpenGLTexture::R32F);
    texture->setWrapMode(QOpenGLTexture::ClampToBorder);
    texture->setMinificationFilter(QOpenGLTexture::Nearest);
    texture->setMagnificationFilter(QOpenGLTexture::Nearest);
    texture->allocateStorage();

    // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE DEPTH TO COLOR VIDEO MAPPING
    QOpenGLFramebufferObjectFormat frameBufferObjectFormat;
    frameBufferObjectFormat.setInternalTextureFormat(GL_RGBA32F);

    colorFBO = new QOpenGLFramebufferObject(numColorCols, numColorRows, frameBufferObjectFormat);
    colorFBO->release();

    depthFBO = new QOpenGLFramebufferObject(numDepthCols, numDepthRows, frameBufferObjectFormat);
    depthFBO->release();

    // OVERRIDE SYSTEM LOCALE UNTIL SHADERS ARE COMPILED
    setlocale(LC_NUMERIC, "C");

    switch (playbackColor) {
        case ColorGray:
        case ColorRGB:
        case ColorXYZ:
        case ColorRGBA:
        case ColorXYZW:
            break;
        case ColorXYZG:
            colorProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/colorizeFilterXYZG.vert");
            colorProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/colorizeFilterXYZG.frag");
            break;
        case ColorXYZRGB:
        case ColorXYZWRGBA:
            colorProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZRGB/XYZRGB/colorizeFilterXYZRGB.vert");
            colorProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/colorizeFilterXYZRGB.frag");
            break;
        default:
            break;
    }
    colorProgram.link();

    switch (playbackColor) {
        case ColorGray:
        case ColorRGB:
        case ColorXYZ:
        case ColorRGBA:
        case ColorXYZW:
            break;
        case ColorXYZG:
            depthProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZG/XYZG/colorizeSmoothingFilterXYZG.vert");
            depthProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZG/XYZG/colorizeSmoothingFilterXYZG.frag");
            break;
        case ColorXYZRGB:
        case ColorXYZWRGBA:
            depthProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/XYZRGB/XYZRGB/colorizeSmoothingFilterXYZRGB.vert");
            depthProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/XYZRGB/XYZRGB/colorizeSmoothingFilterXYZRGB.frag");
            break;
        default:
            break;
    }
    depthProgram.link();

    // RESTORE SYSTEM LOCALE
    setlocale(LC_ALL, "");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUColorizeDepthGLFilter::updateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    Q_UNUSED(mapping);

    if (surface && makeCurrent(surface)) {
        for (unsigned int frm = 0; frm < depth.frames(); frm++){
            // UPDATE THE COLOR TEXTURE
            if (textureColor) {
                if (color.isValid()) {
                    if (color.colors() == 1) {
                        if (color.depth() == sizeof(unsigned char)) {
                            textureColor->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (const void *)color.constFrame(frm % color.frames()));
                        } else if (color.depth() == sizeof(unsigned short)) {
                            textureColor->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, (const void *)color.constFrame(frm % color.frames()));
                        } else if (color.depth() == sizeof(float)) {
                            textureColor->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)color.constFrame(frm % color.frames()));
                        }
                    } else if (color.colors() == 3) {
                        if (color.depth() == sizeof(unsigned char)) {
                            textureColor->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, (const void *)color.constFrame(frm % color.frames()));
                        } else if (color.depth() == sizeof(unsigned short)) {
                            textureColor->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt16, (const void *)color.constFrame(frm % color.frames()));
                        } else if (color.depth() == sizeof(float)) {
                            textureColor->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)color.constFrame(frm % color.frames()));
                        }
                    } else if (color.colors() == 4) {
                        if (color.depth() == sizeof(unsigned char)) {
                            textureColor->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, (const void *)color.constFrame(frm % color.frames()));
                        } else if (color.depth() == sizeof(unsigned short)) {
                            textureColor->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt16, (const void *)color.constFrame(frm % color.frames()));
                        } else if (color.depth() == sizeof(float)) {
                            textureColor->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)color.constFrame(frm % color.frames()));
                        }
                    }
                }
            }

            // UPDATE THE DEPTH AND MAPPING TEXTURES
            if (texture) {
                if (depth.isValid()) {
                    // BIND TEXTURE FOR CONVERTING FROM SPHERICAL TO CARTESIAN
                    if (depth.depth() == sizeof(unsigned char)) {
                        texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (const void *)depth.constFrame(frm % depth.frames()));
                    } else if (depth.depth() == sizeof(unsigned short)) {
                        texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, (const void *)depth.constFrame(frm % depth.frames()));
                    } else if (depth.depth() == sizeof(float)) {
                        texture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)depth.constFrame(frm % depth.frames()));
                    }
                }
            }

            // BIND THE FRAME BUFFER OBJECT ALONG WITH THE GLSL
            // PROGRAMS THAT WILL DO THE PROCESSING
            if (colorFBO && colorFBO->bind()) {
                if (colorProgram.bind()) {
                    // CLEAR THE FRAME BUFFER OBJECT
                    glViewport(0, 0, colorFBO->width(), colorFBO->height());
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                    if (quadVertexBuffer.bind()) {
                        if (quadIndexBuffer.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            if (texture) {
                                glActiveTexture(GL_TEXTURE0);
                                texture->bind();
                                colorProgram.setUniformValue("qt_depth", 0);
                            }

                            if (textureColor) {
                                glActiveTexture(GL_TEXTURE1);
                                textureColor->bind();
                                colorProgram.setUniformValue("qt_color", 1);
                            }

                            // SET THE RADIUS OF THE REGION OF INTEREST
                            colorProgram.setUniformValue("qt_radius", qtRadius);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(colorProgram.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            colorProgram.enableAttributeArray("qt_vertex");
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            quadIndexBuffer.release();
                        }
                        quadVertexBuffer.release();
                    }
                    colorProgram.release();
                }
                colorFBO->release();

                // UPDATE THE COLOR TEXTURE
                if (maskFlag == false){
                    if (color.isValid()) {
                        glBindTexture(GL_TEXTURE_2D, colorFBO->texture());
                        glPixelStorei(GL_PACK_ALIGNMENT, 1);
                        if (color.colors() == 1) {
                            if (color.depth() == sizeof(unsigned char)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, color.constFrame(frm % color.frames()));
                            } else if (color.depth() == sizeof(unsigned short)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_SHORT, color.constFrame(frm % color.frames()));
                            } else if (color.depth() == sizeof(float)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, color.constFrame(frm % color.frames()));
                            }
                        } else if (color.colors() == 3) {
                            if (color.depth() == sizeof(unsigned char)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, color.constFrame(frm % color.frames()));
                            } else if (color.depth() == sizeof(unsigned short)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_SHORT, color.constFrame(frm % color.frames()));
                            } else if (color.depth() == sizeof(float)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, color.constFrame(frm % color.frames()));
                            }
                        } else if (color.colors() == 4) {
                            if (color.depth() == sizeof(unsigned char)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, color.constFrame(frm % color.frames()));
                            } else if (color.depth() == sizeof(unsigned short)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_SHORT, color.constFrame(frm % color.frames()));
                            } else if (color.depth() == sizeof(float)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, color.constFrame(frm % color.frames()));
                            }
                        }
                    }
                }

                for (int cnt = 0; cnt < 1; cnt++){
                    if (cnt > 0){
                        // UPDATE THE DEPTH AND MAPPING TEXTURES
                        if (texture) {
                            if (depth.isValid()) {
                                // BIND TEXTURE FOR CONVERTING FROM SPHERICAL TO CARTESIAN
                                if (depth.depth() == sizeof(unsigned char)) {
                                    texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (const void *)depth.constFrame(frm % depth.frames()));
                                } else if (depth.depth() == sizeof(unsigned short)) {
                                    texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, (const void *)depth.constFrame(frm % depth.frames()));
                                } else if (depth.depth() == sizeof(float)) {
                                    texture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)depth.constFrame(frm % depth.frames()));
                                }
                            }
                        }
                    }

                    // BIND THE FRAME BUFFER OBJECT FOR PROCESSING THE DEPTH BUFFER
                    if (depthFBO && depthFBO->bind()) {
                        if (depthProgram.bind()) {
                            // CLEAR THE FRAME BUFFER OBJECT
                            glViewport(0, 0, depthFBO->width(), depthFBO->height());
                            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                            if (quadVertexBuffer.bind()) {
                                if (quadIndexBuffer.bind()) {
                                    // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                                    if (texture) {
                                        glActiveTexture(GL_TEXTURE0);
                                        texture->bind();
                                        depthProgram.setUniformValue("qt_depth", 0);
                                    }

                                    glActiveTexture(GL_TEXTURE1);
                                    glBindTexture(GL_TEXTURE_2D, colorFBO->texture());
                                    depthProgram.setUniformValue("qt_color", 1);

                                    if (maskFlag && textureColor) {
                                        glActiveTexture(GL_TEXTURE2);
                                        textureColor->bind();
                                        depthProgram.setUniformValue("qt_mask", 2);
                                    } else {
                                        depthProgram.setUniformValue("qt_mask", 1);
                                    }

                                    // SET THE RADIUS OF THE REGION OF INTEREST
                                    depthProgram.setUniformValue("qt_radius", qtRadius);

                                    // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                                    glVertexAttribPointer(depthProgram.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                                    depthProgram.enableAttributeArray("qt_vertex");
                                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

                                    // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                                    quadIndexBuffer.release();
                                }
                                quadVertexBuffer.release();
                            }
                            depthProgram.release();
                        }

                        // UPDATE THE DEPTH TEXTURE PRESERVING THE REGION OF INTEREST
                        unsigned short regionOfInterest[4];
                        memcpy(regionOfInterest, depth.constFrame(frm % depth.frames()), 4 * sizeof(unsigned short));

                        glBindTexture(GL_TEXTURE_2D, depthFBO->texture());
                        glPixelStorei(GL_PACK_ALIGNMENT, 1);
                        if (depth.isValid()) {
                            if (depth.depth() == sizeof(unsigned char)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, depth.constFrame(frm % depth.frames()));
                            } else if (depth.depth() == sizeof(unsigned short)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_SHORT, depth.constFrame(frm % depth.frames()));
                            } else if (depth.depth() == sizeof(float)) {
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, depth.constFrame(frm % depth.frames()));
                            }
                        }

                        // COPY BACK THE REGION OF INTEREST
                        memcpy(depth.constFrame(frm % depth.frames()), regionOfInterest, 4 * sizeof(unsigned short));
                    }
                }
            }
        }
        // RELEASE THE CURRENT CONTEXT
        doneCurrent();
    }
}
#endif
