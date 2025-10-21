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

#include "laucascadeclassifierglfilter.h"
#include "lauglwidget.h"
#include <locale.h>

#ifdef ORBBEC
#include "lauorbbeccamera.h"
#endif

#ifdef LUCID
#include "laulucidcamera.h"
#endif

#ifndef HEADLESS
#include "laugreenscreenglfilter.h"
#ifdef ENABLEFILTERS
#include "laucolorizedepthglfilter.h"
#include "lausavetodiskfilter.h"
#endif

LAUMemoryObject sharedObject;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCascadeClassifierFromDiskDialog::LAUCascadeClassifierFromDiskDialog(QString dirString, QWidget *parent) : QDialog(parent), directoryString(dirString), rfidHashTable(nullptr), saveToDiskFilter(nullptr), filterCount(0)
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

    //#define ALLOWUSERINPUT
#ifdef ALLOWUSERINPUT
    if (inputStrings.isEmpty()) {
        QSettings settings;
        QString directory = settings.value("LAUCascadeClassifierFromDiskWidget::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        inputStrings = QFileDialog::getOpenFileNames(this, QString("Load image from disk (*.skw, *.csv, *.tif)"), directory, QString("*.skw;*.csv;*.tif;*.tiff"));
        if (inputStrings.isEmpty()) {
            return;
        }
        settings.setValue(QString("LAUCascadeClassifierFromDiskWidget::lastUsedDirectory"), QFileInfo(inputStrings.first()).absolutePath());
    }
    rfidHashTable = new LAUObjectHashTable(QString());
#else
#ifdef ENABLEFILTERS
    rfidHashTable = new LAUObjectHashTable(QString("C:/Users/Public/Documents/objectIDList.csv"));
#endif
#endif

    // MAKE SURE WE HAVE AT LEAST ONE FILE TO PROCESS
    if (inputStrings.isEmpty()) {
        return;
    }
    inputStrings.sort();

    // SAVE THE FILE STRING LIST FOR LATER
    fileStringList = inputStrings;

    // LOAD THE FIRST FRAME OF VIDEO AS THE BACKGROUND IMAGE
    LAUMemoryObject background = LAUMemoryObject(fileStringList.first(), 0).minAreaFilter(2);

    if (background.isValid()) {
        // LOAD THE FIRST FRAME OF RAW VIDEO
        LAUMemoryObject frame = LAUMemoryObject(fileStringList.first(), 1);

        // SPLIT THE FRAME OF VIDEO INTO FOUR FRAMES
        object = LAUMemoryObject(frame.width(), frame.height() / 4, frame.colors(), frame.depth(), 4);
        memcpy(object.constPointer(), frame.constPointer(), qMin(object.length(), frame.length()));

#ifdef ENABLEFILTERS
        // CREATE A LOG FILE TO STORE VIDEO METRICS
        logFile.setFileName(QString("%1/LAUCascadeClassifierFromDiskDialog.txt").arg(directoryString));
        logFile.open(QIODevice::WriteOnly);
        logTS.setDevice(&logFile);
#endif
        // CREATE A LOG FILE TO STORE VIDEO METRICS
        rfdFile.setFileName(QString("%1/RFIDlog.txt").arg(directoryString));
        rfdFile.open(QIODevice::WriteOnly);
        rfdTS.setDevice(&rfdFile);

        // ALLOCATE MEMORY OBJECTS TO HOLD INCOMING VIDEO FRAMES
        for (int n = 0; n < NUMFRAMESINBUFFER; n++) {
            LAUModalityObject frame;
            frame.depth = LAUMemoryObject(object.width(), object.height(), object.colors(), object.depth(), object.frames());
            frame.color = LAUMemoryObject(object.width(), object.height(), 3, sizeof(unsigned char));
            frame.mappi = LAUMemoryObject();
            framesList << frame;
        }

        // CREATE A GLWIDGET TO DISPLAY GRAYSCALE VIDEO
        LAU3DVideoGLWidget *glWidget;
        glWidget = new LAU3DVideoGLWidget(object.width(), object.height(), object.width(), object.height(), ColorRGB, Device2DCamera);
        glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        glWidget->setMaximumIntensityValue(255);
        this->layout()->addWidget(glWidget);

        // CREATE A TEMPORARY FILE ON DISK TO HOLD THE CLASSIFIER
        QFile xmlFile(QString("%1/LAUCascadeFilterTool.xml").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
        if (xmlFile.open(QIODevice::WriteOnly)) {
            QFile file(":/CLASSIFIERS/cascade.xml");
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray byteArray = file.readAll();
                xmlFile.write(byteArray);
                xmlFile.close();
                file.close();
            } else {
                qDebug() << file.errorString();
            }
            xmlFile.close();
        }

#ifndef ALLOWUSERINPUT
#ifdef ENABLEFILTERS
        // CREATE A GLCONTEXT FILTER
        saveToDiskFilter = new LAUSaveToDiskFilter(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        saveToDiskFilter->setHeader(background);
#endif
#endif
        LAUCascadeClassifierGLFilter *classifierFilter = nullptr;
#define LOOKFORTAILS
#ifdef LOOKFORTAILS
        classifierFilter = new LAUCascadeClassifierGLFilter(QFileInfo(xmlFile).absoluteFilePath(), object.width(), object.height(), object.width(), object.height(), ColorXYZRGB, DeviceLucid);
#endif
#ifdef ENABLEFILTERS
        LAUColorizeDepthGLFilter *colorizerFilter = new LAUColorizeDepthGLFilter(object.width(), object.height(), object.width(), object.height(), ColorXYZRGB, DeviceLucid);
        LAUGreenScreenGLFilter *greenScreenFilter = new LAUGreenScreenGLFilter(object.width(), object.height(), object.width(), object.height(), ColorXYZRGB, DeviceLucid);
        greenScreenFilter->onSetBackgroundTexture(background);
        greenScreenFilter->setSensitivity(0.20f);

        if (classifierFilter) {
            connect(this, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), classifierFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(classifierFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), colorizerFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        } else {
            connect(this, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), colorizerFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        }
        connect(colorizerFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), greenScreenFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        if (saveToDiskFilter) {
            connect(greenScreenFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), saveToDiskFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(saveToDiskFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        } else {
            connect(greenScreenFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
        }
        connect(glWidget, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), this, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);

        if (saveToDiskFilter) {
            connect(saveToDiskFilter, SIGNAL(destroyed()), this, SLOT(onFilterDestroyed()));
        }

        if (classifierFilter) {
            connect(classifierFilter, SIGNAL(destroyed()), this, SLOT(onFilterDestroyed()));
        }
        connect(colorizerFilter, SIGNAL(destroyed()), this, SLOT(onFilterDestroyed()));
        connect(greenScreenFilter, SIGNAL(destroyed()), this, SLOT(onFilterDestroyed()));
#else
        if (classifierFilter) {
            connect(this, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), classifierFilter, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(classifierFilter, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), glWidget, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
            connect(classifierFilter, SIGNAL(destroyed()), this, SLOT(onFilterDestroyed()));
        }
        connect(glWidget, SIGNAL(emitBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), this, SLOT(onUpdateBuffer(LAUMemoryObject, LAUMemoryObject, LAUMemoryObject)), Qt::QueuedConnection);
#endif
        // SPIN THE FILTERS INTO THEIR OWN THREAD CONTROLLERS
        if (classifierFilter) {
            filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractGLFilter *)classifierFilter));
        }
#ifdef ENABLEFILTERS
        if (saveToDiskFilter) {
            filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractFilter *)saveToDiskFilter));
        }
        filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractGLFilter *)colorizerFilter));
        filterControllers.prepend(new LAUAbstractFilterController((LAUAbstractGLFilter *)greenScreenFilter));
#endif
        filterCount = filterControllers.count();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCascadeClassifierFromDiskDialog::~LAUCascadeClassifierFromDiskDialog()
{
#ifdef ENABLEFILTERS
    // DELETE THE RFID HASH TABLE
    if (rfidHashTable) {
        delete rfidHashTable;
    }
#endif

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
void LAUCascadeClassifierFromDiskDialog::accept()
{
#ifdef ENABLEFILTERS
    // CLOSE THE LOG FILE
    if (logFile.isOpen()) {
        logFile.close();
    }

    // CLOSE THE LOG FILE
    if (rfdFile.isOpen()) {
        rfidHashTable->save(QString("%1/ObjectIDlog.txt").arg(directoryString));
        rfdFile.close();
    }

    // COPY THE OUTPUT FILES OVER TO THE INPUT FILES DIRECTORY
    if (saveToDiskFilter) {
        // GET A LIST OF ALL THE NEW FILES THAT WE JUST CREATED
        QStringList newFiles = saveToDiskFilter->newFiles();

        //#define DELETEFILES
#ifdef DELETEFILES
        // DELETE ALL THE INPUT FILES
        while (processedStringList.count() > 0) {
            // DELETE THE EXISTING INPUT IMAGE
            if (QFile::moveToTrash(processedStringList.takeFirst())) {
                ;
            } else {
                ;
            }
        }
#endif
        // KEEP TRACK OF A LOCAL COUNTER SO WE CAN KEEP TRACK OF THE NEXT AVAILABLE FILE STRING
        int counter = 1;

        // COPY THE NEW FILES INTO THE INPUT DIRECTORY THAT HAVE AT LEAST 20 FRAMES OF VIDEO
        while (newFiles.count() > 0) {
            int fileLength = LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(newFiles.first());
            if (fileLength > 20) {
                // CREATE A FILESTRING FOR THE NEW FILE WE ARE CREATING
                QString newFileString;
                for (int n = counter; n < 100000; n++) {
                    newFileString = QString("%1").arg(n);
                    while (newFileString.length() < 5) {
                        newFileString.prepend("0");
                    }
                    newFileString.prepend(QString("/post"));
                    newFileString.prepend(directoryString);
                    newFileString.append(QString(".tif"));
                    if (QFile::exists(newFileString) == false) {
                        counter = n;
                        break;
                    }
                }

                // COPY THE INPUT FILE FROM THE TEMPORARY DIRECTORY TO ITS FINAL LOCATION
                if (QFile::copy(newFiles.first(), newFileString)) {
                    ;
                }
            }

            // DELETE THE EXISTING INPUT IMAGE
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
            if (QFile::moveToTrash(newFiles.takeFirst())) {
#else
            if (QFile::remove(newFiles.takeFirst())) {
#endif
                ;
            } else {
                ;
            }
        }
    }
#endif
    QDialog::accept();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCascadeClassifierFromDiskDialog::onUpdateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    static int numFrames = LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(fileStringList.first());
    static int fileIndex = 0;

    // CONSTRUCT A VIDEO MEMORY OBJECT FROM THE INCOMING MEMORY OBJECTS
    LAUModalityObject frame(depth, color, mapping);

    // SEE IF WE SHOULD KEEP THIS PARTICULAR FRAME
    if (frame.isAnyValid()) {
        framesList << frame;

#ifdef ENABLEFILTERS
        // EXPORT VIDEO METRICS TO THE LOG FILE
        if (logFile.isOpen() && depth.isValid()) {
            logTS << depth.elapsed() << ", " << depth.rfid() << ", " << depth.anchor().x() << ", " << depth.anchor().y() << "\n";
        }
#endif
    }

    // EMIT THE SIGNAL TO TELL THE FRAME GRABBER TO GRAB THE NEXT SET OF FRAMES
    if (isVisible()) {
        while (framesList.count() > 0) {
            while (fileIndex >= numFrames) {
                // DELETE THE FIRST FILE IN THE LIST SINCE WE ARE DONE WITH IT
                fileStringList.takeFirst();

                // IF WE ARE DONE, THEN SEND THE ACCEPT FLAG
                if (fileStringList.isEmpty()) {
                    accept();
                    return;
                }

                // GET THE NUMBER OF FRAMES FOR THE NEXT FILE TO BE PROCESSED
                numFrames = LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(fileStringList.first());
                fileIndex = 0;
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
                continue;
            }

            // GET THE NEXT AVAILABLE FRAME BUFFER FROM OUR BUFFER LIST
            LAUModalityObject frame = framesList.takeFirst();

#ifdef ENABLEFILTERS
            // KEEP TRACK OF INCOMING RFID TAG STRING
            static QString previousRFIDString;
            QString rfidString = rfidHashTable->idString(object.rfid(), QTime(0, 0).addMSecs(object.elapsed()));
            if (previousRFIDString != rfidString) {
                previousRFIDString = rfidString;
                if (rfdFile.isOpen()) {
                    rfdTS << QTime(0, 0).addMSecs(frame.depth.elapsed()).toString(Qt::TextDate) << ", " << rfidString << "\n";
                }
            }
            frame.depth.setConstRFID(rfidString);
#endif
            // COPY THE PIXELS OF THE NEXT AVAILABLE VIDEO FRAME INTO THE DEPTH BUFFER OBJECT
            frame.depth.setConstAnchor(object.anchor());
            frame.depth.setConstElapsed(object.elapsed());
            frame.depth.setConstTransform(object.transform());
            memcpy(frame.depth.constPointer(), object.constPointer(), qMin(object.length(), frame.depth.length()));
            memset(frame.color.constPointer(), 0, frame.color.length());

            qDebug() << "RFID:" << frame.depth.rfid() << object.elapsed();

            // EMIT THE OBJECT BUFFERS TO THE VIDEO GLWIDGET
            emit emitBuffer(frame.depth, frame.color, frame.mappi);
        }
    }
}
#ifndef EXCLUDE_LAU3DVIDEOWIDGET
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCascadeClassifierWidget::LAUCascadeClassifierWidget(LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent) : LAU3DVideoRecordingWidget(color, device, parent)
{
    if (camera && camera->isValid()) {
        // GET A FILE TO OPEN FROM THE USER IF NOT ALREADY PROVIDED ONE
        QSettings settings;
        QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        directory = settings.value(QString("LAUCascadeClassifierWidget::lastSaveDirectory"), directory).toString();
        QString filestring = QFileDialog::getOpenFileName(0, QString("Load cascade classifier from disk..."), directory, QString("*.xml;*.dat"));
        if (filestring.isEmpty() == false) {
            settings.setValue(QString("LAUCascadeClassifierWidget::lastSaveDirectory"), QFileInfo(filestring).absolutePath());
        }

        // CREATE A GLCONTEXT FILTER
        LAUAbstractGLFilter *classifierFilter = new LAUCascadeClassifierGLFilter(filestring, camera->depthWidth(), camera->depthHeight(), camera->colorWidth(), camera->colorHeight(), camera->color(), camera->device());

        // CREATE GREEN SCREEN FILTER (always enabled for background removal)
        LAUAbstractGLFilter *greenScreenFilter = new LAUGreenScreenGLFilter(camera->depthWidth(), camera->depthHeight(), camera->colorWidth(), camera->colorHeight(), camera->color(), camera->device());
        ((LAUGreenScreenGLFilter *)greenScreenFilter)->setSensitivity(0.20);

#if defined(LUCID)
        // MAKE CONNECTION SO THAT CAMERA OBJECT CAN UPDATE THE GREENSCREEN WHEN READING VIDEO FROM DISK
        if (dynamic_cast<LAULucidCamera *>(camera) != nullptr) {
            connect(dynamic_cast<LAULucidCamera *>(camera), SIGNAL(emitBackgroundTexture(LAUMemoryObject)), greenScreenFilter, SLOT(onSetBackgroundTexture(LAUMemoryObject)));
        }
#endif

#if defined(ORBBEC)
        // MAKE CONNECTION SO THAT CAMERA OBJECT CAN UPDATE THE GREENSCREEN WHEN READING VIDEO FROM DISK
        if (dynamic_cast<LAUOrbbecCamera *>(camera) != nullptr) {
            connect(dynamic_cast<LAUOrbbecCamera *>(camera), SIGNAL(emitBackgroundTexture(LAUMemoryObject)), greenScreenFilter, SLOT(onSetBackgroundTexture(LAUMemoryObject)));
        }
#endif

        // INSERT THE FILTER INTO THE SIGNAL CHAIN
        prependFilter(greenScreenFilter);
        prependFilter(classifierFilter);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCascadeClassifierWidget::~LAUCascadeClassifierWidget()
{
    ;
}
#endif
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCascadeClassifierGLFilter::~LAUCascadeClassifierGLFilter()
{
    if (makeCurrent(surface)){
        if (texture){
            delete texture;
        }

        if (frameBufferObjectA){
            delete frameBufferObjectA;
        }

        if (frameBufferObjectB){
            delete frameBufferObjectB;
        }

        doneCurrent();
    }
    qDebug() << "LAUCascadeClassifierGLFilter()::~LAUCascadeClassifierGLFilter()";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCascadeClassifierGLFilter::initializeGL()
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

    frameBufferObjectA = new QOpenGLFramebufferObject(numDepthCols, numDepthRows, frameBufferObjectFormat);
    frameBufferObjectA->release();

    frameBufferObjectB = new QOpenGLFramebufferObject(numDepthCols, numDepthRows, frameBufferObjectFormat);
    frameBufferObjectB->release();

    // OVERRIDE SYSTEM LOCALE UNTIL SHADERS ARE COMPILED
    setlocale(LC_NUMERIC, "C");
    fillHolesProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/MEDIAN/MedianFilters/filterFillHolesDilate.vert");
    fillHolesProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/MEDIAN/MedianFilters/filterFillHolesDilate.frag");
    fillHolesProgram.link();

    // RESTORE SYSTEM LOCALE
    setlocale(LC_ALL, "");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCascadeClassifierGLFilter::updateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    // CREATE THE SHARED OBJECT TO BE THE SAME SIZE OF THE DEPTH OBJECT
    if (sharedObject.isNull()){
        sharedObject = depth;
        sharedObject.triggerDeepCopy();
    }

#ifndef DONTCOMPILE
    if (depth.isValid() && makeCurrent(surface)) {
        // UPDATE THE DEPTH TEXTURE
        if (texture) {
            if (depth.isValid()) {
                // BIND TEXTURE FOR CONVERTING FROM SPHERICAL TO CARTESIAN
                if (depth.depth() == sizeof(unsigned char)) {
                    texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (const void *)depth.constFrame(channel % depth.frames()));
                } else if (depth.depth() == sizeof(unsigned short)) {
                    texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, (const void *)depth.constFrame(channel % depth.frames()));
                } else if (depth.depth() == sizeof(float)) {
                    texture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)depth.constFrame(channel % depth.frames()));
                }
            }
        }

        if (frameBufferObjectA->bind()) {
            // SET THE VIEW PORT AND CLEAR THE SCREEN BUFFER
            glViewport(0, 0, frameBufferObjectA->width(), frameBufferObjectA->height());
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // BIND THE GLSL PROGRAM THAT WILL DO THE PROCESSING
            if (fillHolesProgram.bind()) {
                // BIND VERTEX BUFFER FOR DRAWING TRIANGLES ON SCREEN
                if (quadVertexBuffer.bind()) {
                    // BIND INDICES BUFFER FOR DRAWING TRIANGLES ON SCREEN
                    if (quadIndexBuffer.bind()) {
                        // BIND THE INCOMING DEPTH VIDEO FRAME BUFFER
                        glActiveTexture(GL_TEXTURE0);
                        texture->bind();
                        fillHolesProgram.setUniformValue("qt_texture", 0);

                        // SET THE SIGMA VALUE
                        fillHolesProgram.setUniformValue("qt_radius", 1);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(fillHolesProgram.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        fillHolesProgram.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE INDICES BUFFERS
                        quadIndexBuffer.release();
                    }
                    // RELEASE THE VERTICES BUFFERS
                    quadVertexBuffer.release();
                }
                // RELEASE THE GLSL PROGRAM
                fillHolesProgram.release();
            }
            // RELEASE THE FRAME BUFFER OBJECT
            frameBufferObjectA->release();
        }

        if (frameBufferObjectB->bind()) {
            // SET THE VIEW PORT AND CLEAR THE SCREEN BUFFER
            glViewport(0, 0, frameBufferObjectB->width(), frameBufferObjectB->height());
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // BIND THE GLSL PROGRAM THAT WILL DO THE PROCESSING
            if (fillHolesProgram.bind()) {
                // BIND VERTEX BUFFER FOR DRAWING TRIANGLES ON SCREEN
                if (quadVertexBuffer.bind()) {
                    // BIND INDICES BUFFER FOR DRAWING TRIANGLES ON SCREEN
                    if (quadIndexBuffer.bind()) {
                        // BIND THE INCOMING DEPTH VIDEO FRAME BUFFER
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectA->texture());
                        fillHolesProgram.setUniformValue("qt_depthTexture", 0);

                        // SET THE SIGMA VALUE
                        fillHolesProgram.setUniformValue("qt_radius", 1);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(fillHolesProgram.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        fillHolesProgram.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE INDICES BUFFERS
                        quadIndexBuffer.release();
                    }
                    // RELEASE THE VERTICES BUFFERS
                    quadVertexBuffer.release();
                }
                // RELEASE THE GLSL PROGRAM
                fillHolesProgram.release();
            }
            // RELEASE THE FRAME BUFFER OBJECT
            frameBufferObjectB->release();
        }

        if (frameBufferObjectA->bind()) {
            // SET THE VIEW PORT AND CLEAR THE SCREEN BUFFER
            glViewport(0, 0, frameBufferObjectA->width(), frameBufferObjectA->height());
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // BIND THE GLSL PROGRAM THAT WILL DO THE PROCESSING
            if (fillHolesProgram.bind()) {
                // BIND VERTEX BUFFER FOR DRAWING TRIANGLES ON SCREEN
                if (quadVertexBuffer.bind()) {
                    // BIND INDICES BUFFER FOR DRAWING TRIANGLES ON SCREEN
                    if (quadIndexBuffer.bind()) {
                        // BIND THE INCOMING DEPTH VIDEO FRAME BUFFER
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectB->texture());
                        fillHolesProgram.setUniformValue("qt_depthTexture", 0);

                        // SET THE SIGMA VALUE
                        fillHolesProgram.setUniformValue("qt_radius", 1);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(fillHolesProgram.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        fillHolesProgram.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE INDICES BUFFERS
                        quadIndexBuffer.release();
                    }
                    // RELEASE THE VERTICES BUFFERS
                    quadVertexBuffer.release();
                }
                // RELEASE THE GLSL PROGRAM
                fillHolesProgram.release();
            }
            // RELEASE THE FRAME BUFFER OBJECT
            frameBufferObjectA->release();

            glBindTexture(GL_TEXTURE_2D, frameBufferObjectA->texture());
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            if (sharedObject.isValid()) {
                if (sharedObject.depth() == sizeof(unsigned char)) {
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, sharedObject.constFrame(qMax(0, channel) % sharedObject.frames()));
                } else if (sharedObject.depth() == sizeof(unsigned short)) {
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_SHORT, sharedObject.constFrame(qMax(0, channel) % sharedObject.frames()));
                } else if (sharedObject.depth() == sizeof(float)) {
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, sharedObject.constFrame(qMax(0, channel) % sharedObject.frames()));
                }
            }
        }

        // RELEASE THE CURRENT CONTEXT
        doneCurrent();
    }
#endif
    // CALL THE FILTER OBJECT TO EMBED THE ROI INTO THE MEMORY OBJECT
    filter->onUpdateBuffer(depth, color, mapping);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCascadeClassifierFilter::updateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    Q_UNUSED(mapping);

    // CHOOSE THE USE AS INPUT THE SHARED OBJECT OR THE INCOMING DEPTH OBJECT
    LAUMemoryObject localObject = (sharedObject.isValid()) ? sharedObject : depth;

#ifdef ENABLECASCADE
    // PROCESS WITH CLASSIFIER
    if (localObject.isValid() && frame.data) {
        // COPY THE INCOMING DEPTH VIDEO INTO THE FRAME MATRIX COVERTING FROM 16 TO 8 BITS PER PIXEL
        // FOR VZENSE PIXELS MULTIPLY BY 10 SO THEY MATCH LUCID
        for (unsigned int r = 0; r < localObject.height(); r++) {
            unsigned short *inBuffer = (unsigned short *)localObject.constScanLine(r);
            unsigned char  *otBuffer = frame.data + r * frame.step;
            for (unsigned int c = 0; c < localObject.width() - 16; c += 16) {
                __m128i inVecA = _mm_load_si128((__m128i *)&inBuffer[c + 0]);
                inVecA = _mm_srli_epi16(inVecA, 8);
                __m128i inVecB = _mm_load_si128((__m128i *)&inBuffer[c + 8]);
                inVecB = _mm_srli_epi16(inVecB, 8);
                __m128i otVec  = _mm_packus_epi16(inVecA, inVecB);
                _mm_storeu_si128((__m128i *)&otBuffer[c], otVec);
            }
        }

        //for (unsigned int r = 0; r < depth.height(); r += 2) {
        //    unsigned short *inBuffer = (unsigned short *)depth.constScanLine(r);
        //    unsigned char  *otBuffer = frame.data + r / 2 * frame.step;
        //    for (unsigned int c = 0; c < depth.width(); c += 2) {
        //        otBuffer[c / 2] = ((inBuffer[c] >> 6) & 0x00ff);
        //    }
        //}

        // FILL LOCAL MEMORY OBJECT WITH INPAINTING TO FILL HOLES
        for (int row = 0; row < frame.rows; row++) {
            unsigned char  *buffer = frame.data + row * frame.step;
            for (int col = 1; col < frame.cols; col++) {
                if (buffer[col] == 255) {
                    buffer[col] = buffer[col - 1];
                }
            }
            for (int col = frame.cols - 2; col >= 0; col--) {
                if (buffer[col] == 255) {
                    buffer[col] = buffer[col + 1];
                }
            }
        }

        // LOOK FOR THE PATTERN TEMPLATE
        std::vector<cv::Rect> rois;
        classifier.detectMultiScale(frame, rois, 1.1, 3, 0, cv::Size(80, 100), cv::Size(150, 300));
        //classifier.detectMultiScale(frame, rois, 1.1, 3, 0, cv::Size(30, 40), cv::Size(70, 140));

        // COPY THE LEFT-MOST DETECTED TEMPLATE TO THE DEPTH BUFFER USING ITS TRANSFORM FIELD
        if (rois.size() > 0) {
            if (depth.isValid()) {
                unsigned short *buffer = (unsigned short *)depth.constPointer();
                for (int chn = 0; chn < (int)rois.size(); chn++) {
                    cv::Rect roi = rois.at(chn);
                    buffer[4 * chn + 0] = (unsigned short)roi.x;
                    buffer[4 * chn + 1] = (unsigned short)roi.y;
                    buffer[4 * chn + 2] = (unsigned short)roi.width;
                    buffer[4 * chn + 3] = (unsigned short)roi.height;
                    buffer[4 * chn + 4] = 0xffff;
                }
            }

            if (color.isValid()) {
                // FILTER THE RGB IMAGE TO INCLUDE ROI TO HIGHLIGHT CLASSIFIER OUTPUT
                for (int chn = 0; chn < (int)rois.size(); chn++) {
                    cv::Rect roi = rois.at(chn);
                    //roi.x *= 2;
                    //roi.y *= 2;
                    //roi.width *= 2;
                    //roi.height *= 2;
                    for (int row = 0; row < roi.height; row++) {
                        unsigned char *buffer = color.constScanLine(roi.y + row);
                        for (int col = 0; col < roi.width; col++) {
                            buffer[3 * (roi.x + col) + 0] = 0;
                            buffer[3 * (roi.x + col) + 1] = 255;
                            buffer[3 * (roi.x + col) + 2] = 0;
                        }
                    }
                }
            }

            // COPY THE LEFT-MOST ROI INTO THE FIRST ROI IN THE VECTOR
            //for (unsigned int n = 1; n < rois.size(); n++) {
            //    if (rois[n].x < rois[0].x) {
            //        rois[0] = rois[n];
            //    }
            //}

            // WRITE THE ROI TO TERMINAL FOR DEBUGGING
            qDebug() << "ROI:" << rois[0].x << rois[0].y << rois[0].width << rois[0].height;

            // STORE THE TAIL-HEAD COORDINATE TO THE ANCHOR PIXEL OF THE DEPTH BUFFER
            depth.setConstAnchor(QPoint(rois[0].x + rois[0].width / 2, rois[0].y + rois[0].height / 2));
        } else {
            // STORE THE TAIL-HEAD COORDINATE TO THE ANCHOR PIXEL OF THE DEPTH BUFFER
            depth.setConstAnchor(QPoint(-1, -1));
        }
    }
#endif
}
