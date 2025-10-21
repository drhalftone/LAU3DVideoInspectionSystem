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

#include "lausavetodiskfilter.h"

#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QIODevice>

using namespace libtiff;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUSaveToDiskFilter::LAUSaveToDiskFilter(QString dirString, QObject *parent) : LAUAbstractFilter(0, 0, parent), frameCounter(0), file(nullptr), directoryString(dirString), recordFlag(false)
{
    if (directoryString.isEmpty()) {
        QSettings settings;
        QString directory = settings.value("LAUSaveToDiskFilter::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        directoryString = QFileDialog::getExistingDirectory(nullptr, QString("Select directory to save video..."), directory);
        if (directoryString.isEmpty()) {
            return;
        }
        settings.setValue(QString("LAUSaveToDiskFilter::lastUsedDirectory"), directoryString);        
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUSaveToDiskFilter::onStart()
{
    // CREATE A LOG FILE TO STORE VIDEO METRICS APPEDNING TO ANY EXISTING FILE
    if (directoryString.isEmpty() == false) {
        logFile.setFileName(QString("%1/LAUSaveToDiskFilter.txt").arg(directoryString));
        if (logFile.exists()) {
            QFileInfo fileInfo(logFile);
            QDateTime creationTime = fileInfo.birthTime(); // or fileInfo.created() in older Qt versions
            QDateTime currentTime = QDateTime::currentDateTime();

            // Check if file was created more than 12 hours ago
            qint64 hoursOld = creationTime.secsTo(currentTime) / 3600;

            if (hoursOld > 12) {
                // File is older than 12 hours, overwrite it
                if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    // File opened for overwriting
                    // Write your data here
                }
            } else {
                // File is less than 12 hours old, append to it
                if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
                    // File opened for appending
                    // Write your data here
                }
            }
        } else {
            // File doesn't exist, create it
            if (logFile.open(QIODevice::WriteOnly)) {
                // New file created
                // Write your data here
            }
        }

        if (logFile.isOpen()){
            logTS.setDevice(&logFile);
        }
    }

    // OUTPUT TO STANDARD OUTPUT
    logTS << "Starting save to disk filter.\n";
    logTS.flush();

    if (isValid()) {
        ;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUSaveToDiskFilter::onFinish()
{
    if (file) {
        closeOldFile(frameCounter);
    }

    // OUTPUT TO STANDARD OUTPUT
    logTS << "LAUSaveToDiskFilter::onFinish()\n";
    logTS.flush();

    // CLOSE THE LOG FILE
    if (logFile.isOpen()) {
        logFile.close();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUSaveToDiskFilter::updateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    // SAVE SOME DEBUG MESSAGES
    static int updateBufferCounter = 0;
    if ((updateBufferCounter++)%1000 == 0){
        logTS << "Inside LAUSaveToDiskFilter::updateBuffer()" << updateBufferCounter << "\n";
        logTS.flush();
    }

#ifdef SAVE_HEADER_FRAMES
    // SINCE THIS IS THE FIRST TIME AROUND, WE CAN DETERMINE IF WE HAVE DEPTH
    // AND/OR COLOR VIDEO THAT WE NEED TO RECORD TO DISK SO USE THE INCOMING
    // BUFFERS TO RESERVE SPACE IN OUR HEADER FRAMES LIST
    while (headerFrames.count() < NUMBER_HEADER_FRAMES){
        LAUFrame frame;
        if (depth.isValid()){
            frame.depth = LAUMemoryObject(depth.width(), depth.height(), depth.colors(), depth.depth(), depth.frames());
        }
        if (color.isValid()){
            frame.color = LAUMemoryObject(color.width(), color.height(), color.colors(), color.depth(), color.frames());
        }
        headerFrames << frame;
    }

    while (trailerFrames.count() < NUMBER_HEADER_FRAMES){
        LAUFrame frame;
        if (depth.isValid()){
            frame.depth = LAUMemoryObject(depth.width(), depth.height(), depth.colors(), depth.depth(), depth.frames());
        }
        if (color.isValid()){
            frame.color = LAUMemoryObject(color.width(), color.height(), color.colors(), color.depth(), color.frames());
        }
        trailerFrames << frame;
    }

    // GRAB THE OLDEST FRAME IN OUR TRAILER LIST
    LAUFrame frame = trailerFrames.takeFirst();

    // WE ALWAYS WANT TO HAVE A COPY OF THE MOST RECENT FRAMES OF VIDEO FOR WHEN WE CLOSE A FILE AND START A NEW RECORDING
    if (depth.isValid()){
        // COPY OVER THE DEPTH MEMORY OBJECT
        frame.depth.setConstRFID(depth.rfid());
        frame.depth.setConstXML(depth.xml());
        frame.depth.setConstTransform(depth.transform());
        frame.depth.setConstAnchor(depth.anchor());
        frame.depth.setConstElapsed(depth.elapsed());
        memcpy(frame.depth.constPointer(), depth.constPointer(), qMin(frame.depth.length(), depth.length()));
    }

    if (color.isValid()){
        // COPY OVER THE COLOR MEMORY OBJECT
        frame.color.setConstRFID(color.rfid());
        frame.color.setConstXML(color.xml());
        frame.color.setConstTransform(color.transform());
        frame.color.setConstAnchor(color.anchor());
        frame.color.setConstElapsed(color.elapsed());
        memcpy(frame.color.constPointer(), color.constPointer(), qMin(frame.color.length(), color.length()));
    }

    // ADD THE FRAME TO THE END OF THE LIST AS THE NEWEST FRAME
    trailerFrames << frame;
#endif

#ifdef RECORDRAWVIDEOTODISK
    // SEE IF THE RECORDING FLAG IS TRUE
    if (recordFlag) {
        // IF WE DON'T HAVE AN OPEN FILE HERE, THEN SKIP THE REST OF THIS LOOP
        if (file == nullptr) {
            if (openNewFile()) {
                ;
            }
        }

        // RECORD THE INCOMING DEPTH VIDEO TO DISK IF IT EXISTS
        if (depth.isValid()) {
            depth.save(file, frameCounter);
            frameCounter++;
        }

        // RECORD THE INCOMING RGB VIDEO TO DISK IF IT EXISTS
        if (color.isValid()) {
            color.save(file, frameCounter);
            frameCounter++;
        }

        // RECORD THE INCOMING MAPPING VIDEO TO DISK IF IT EXISTS
        if (mapping.isValid()) {
            mapping.save(file, frameCounter);
            frameCounter++;
        }

        // CHECK TO SEE IF WE NEED TO SWITCH TO A NEW RAW DATA FILE
        if (frameCounter >= 500) {
            if (closeOldFile()) {
                ;
            }
        }
    } else {
        // IF WE DON'T HAVE AN OPEN FILE HERE, THEN SKIP THE REST OF THIS LOOP
        if (file != nullptr) {
            if (closeOldFile()) {
                ;
            }
        }
    }
#else
    static int numUpdateBufferCalls = 0;
    static int noObjectFrameCounter = 0;
    static QPointF previousAnchor = QPointF(-1.0f, -1.0f);

#ifdef RECORDRAWVIDEO
    // MAKE SURE WE HAVE AN OPEN TIFF FILE TO SAVE INTO
    if (file == nullptr) {
        // ONLY START A NEW FILE IF THE TAIL IS IN THE LEFT HALF OF THE FRAME
        if (previousAnchor.x() < 300) {
            if (openNewFile()) {
                ;
            }
        }
    }

    // IF WE DON'T HAVE AN OPEN FILE HERE, THEN SKIP THE REST OF THIS LOOP
    if (file != nullptr) {
        // RECORD THE INCOMING DEPTH VIDEO TO DISK IF IT EXISTS
        if (depth.isValid()) {
            depth.save(file, frameCounter);
            frameCounter++;
        }

        // RECORD THE INCOMING RGB VIDEO TO DISK IF IT EXISTS
        if (color.isValid()) {
            color.save(file, frameCounter);
            frameCounter++;
        }

        // RECORD THE INCOMING MAPPING VIDEO TO DISK IF IT EXISTS
        if (mapping.isValid()) {
            mapping.save(file, frameCounter);
            frameCounter++;
        }

        // CHECK TO SEE IF WE NEED TO SWITCH TO A NEW RAW DATA FILE
        if (frameCounter >= 500) {
            if (closeOldFile()) {
                ;
            }
        }
    }
#else

    // int xPt = (5 * numUpdateBufferCalls++) % 800;
    // if (xPt > 600){
    //     xPt = -1;
    // }
    // depth.setConstAnchor(QPoint((float)xPt, 0.0));

    // SEE IF WE SHOULD KEEP THIS PARTICULAR FRAME
    // if (depth.isValid()) {
    //     // EXPORT VIDEO METRICS TO THE LOG FILE
    //     if (logFile.isOpen()) {
    //         logTS << depth.elapsed() << ", " << depth.rfid() << ", " << depth.anchor().x() << ", " << depth.anchor().y() << "\n";
    //     }
    // }

    // ONLY SAVE THE VIDEO FRAMES IF THERE IS A VISIBLE TAIL
    if (depth.anchor().x() >= 100.0f) {
        logTS << "Tail location is greater than 100, " << depth.anchor().x() << ", " << noObjectFrameCounter << ", " << previousAnchor.x() << "\n";
        logTS.flush();

        noObjectFrameCounter = 0;
        float delta = depth.anchor().x() - previousAnchor.x();
        if (delta > 2.0f) {
            logTS << "delta is greater than 2.0 ," << delta << ", " << depth.anchor().x() << ", " << depth.anchor().y() << "\n";
            logTS.flush();

            previousAnchor = depth.anchor();

            // MAKE SURE WE HAVE AN OPEN TIFF FILE TO SAVE INTO
            if (file == nullptr) {
                logTS << "file pointer is NULL\n";
                logTS.flush();

                // ONLY START A NEW FILE IF THE TAIL IS IN THE LEFT HALF OF THE FRAME
                if (previousAnchor.x() < 300) {
                    logTS << "Trying to open new file.\n";
                    logTS.flush();
                    if (openNewFile()) {
                        ;
                    } else {
                        logTS << "OPENING NEW FILE FAILED.\n";
                        logTS.flush();
                    }
                }
            }

            // IF WE DON'T HAVE AN OPEN FILE HERE, THEN SKIP THE REST OF THIS LOOP
            if (file != nullptr) {
                logTS << "file pointer is valid\n";
                logTS.flush();

                // RECORD THE INCOMING DEPTH VIDEO TO DISK IF IT EXISTS
                if (depth.isValid()) {
                    depth.save(file, frameCounter);
                    frameCounter++;
                }

                // RECORD THE INCOMING RGB VIDEO TO DISK IF IT EXISTS
                if (color.isValid()) {
                    color.save(file, frameCounter);
                    frameCounter++;
                }

                // RECORD THE INCOMING MAPPING VIDEO TO DISK IF IT EXISTS
                if (mapping.isValid()) {
                    mapping.save(file, frameCounter);
                    frameCounter++;
                }

                // CHECK TO SEE IF WE NEED TO SWITCH TO A NEW RAW DATA FILE
                if (frameCounter >= 500) {
                    if (closeOldFile()) {
                        ;
                    }
                }
            }
        } else {
            ;
        }
    } else {
        // IF THERE IS NO OBJECT, INCREMENT THE FRAME COUNTER
        noObjectFrameCounter = noObjectFrameCounter + 1;

        // IF WE ARE IN THE MIDDLE OF A RECORDING, WE SHOULD CLOSE THE FILE
        if (frameCounter > (int)header.frames() && noObjectFrameCounter > 5) {
            if (closeOldFile(frameCounter)) {
                ;
            }
            previousAnchor = QPointF(-1.0f, -1.0f);
        } else {
#ifdef SAVE_HEADER_FRAMES
            // CHECK IF WE ARE IN THE MIDDLE OF A RECORDING
            if (file == nullptr){
                // GRAB THE OLDEST FRAME IN OUR HEADER LIST
                LAUFrame frame = headerFrames.takeFirst();

                // IF WE ARE NOT RECORDING, THEN COPY THE INCOMING MEMORY OBJECTS TO THE HEADER FRAMES LIST
                if (depth.isValid()){
                    // COPY OVER THE DEPTH MEMORY OBJECT
                    frame.depth.setConstRFID(depth.rfid());
                    frame.depth.setConstXML(depth.xml());
                    frame.depth.setConstTransform(depth.transform());
                    frame.depth.setConstAnchor(depth.anchor());
                    frame.depth.setConstElapsed(depth.elapsed());
                    memcpy(frame.depth.constPointer(), depth.constPointer(), qMin(frame.depth.length(), depth.length()));
                }
                if (color.isValid()){
                    // COPY OVER THE COLOR MEMORY OBJECT
                    frame.color.setConstRFID(color.rfid());
                    frame.color.setConstXML(color.xml());
                    frame.color.setConstTransform(color.transform());
                    frame.color.setConstAnchor(color.anchor());
                    frame.color.setConstElapsed(color.elapsed());
                    memcpy(frame.color.constPointer(), color.constPointer(), qMin(frame.color.length(), color.length()));
                }

                // ADD THE FRAME TO THE END OF THE LIST AS THE NEWEST FRAME
                headerFrames << frame;
            }
#endif
        }
    }
#endif
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUSaveToDiskFilter::openNewFile()
{
    // SEE IF WE NEED TO CLOSE THE CURRENT FILE
    if (file) {
        closeOldFile();
    }

    // LET'S CREATE A FILE TO START RECORDING INTO
    QString filename = getNextFilestring();

    // OPEN TIFF FILE FOR SAVING THE IMAGE
    if (filename.isEmpty() == false) {
        currentFileString = filename;
        file = TIFFOpen(filename.toLocal8Bit(), "w");
        if (file) {
            newFileList << filename;
            logTS << "Opening new file:" << filename << "\n";
        } else {
            logTS << "Failed to open file:" << filename << "\n";
        }

        // SAVE ANY HEADER FRAMES
        if (header.isValid()) {
            header.save(file, 0);
        }
        frameCounter = (header.frames() > 0);
    } else {
        logTS << "New filename is empty:" << filename << "\n";
    }
    logTS.flush();

#ifdef SAVE_HEADER_FRAMES
    // SWAP THE HEADER AND TRAILER FRAMES
    for (int n = 0; n < NUMBER_HEADER_FRAMES; n++){
        headerFrames << trailerFrames.takeFirst();
        trailerFrames << headerFrames.takeFirst();
    }
#endif

    return ((bool)file);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUSaveToDiskFilter::closeOldFile(int frames)
{
    // SEE IF WE NEED TO CLOSE THE CURRENT FILE
    if (file) {
        // OUTPUT TO STANDARD OUTPUT
        logTS << "Closing file\n";
        logTS.flush();

        // IF THE USER PROVIDED A VALID FRAME COUNT, CHECK TO SEE
        // IF WE SHOULD DELETE THE FILE BECAUSE ITS TOO SMALL
        if (frames > -1){
            if (frames < 3) {
                // CLOSE TIFF FILE
                TIFFClose(file);

                // SEE IF WE NEED TO DELETE THIS FILE
                if (QFile::exists(currentFileString)) {
#if QT_VERSION >= 0x060000
                    QFile(currentFileString).moveToTrash();
#else
                    QFile(currentFileString).remove();
#endif
                    logTS << "Deleting file:" << currentFileString << "\n";
                    logTS.flush();
                }
            } else {
#ifdef SAVE_HEADER_FRAMES
                // SAVE THE HEADER FRAMES AT THE END OF THE VIDEO JUST BEFORE CLOSING
                // AND CLEAR THE ELAPSED TIME FIELD SO WE KNOW THE FRAME IS NOT VALID
                for (int n = 0; n < headerFrames.count(); n++){
                    LAUFrame frame = headerFrames.takeFirst();
                    if (frame.depth.isValid() && frame.depth.isElapsedValid()){
                        frame.depth.save(file, frameCounter++);
                    }
                    frame.depth.constMakeElapsedInvalid();
                    if (frame.color.isValid() && frame.color.isElapsedValid()){
                        frame.color.save(file, frameCounter++);
                    }
                    frame.color.constMakeElapsedInvalid();

                    // APPEND THIS NOW INVALID VIDEO FRAME BACK TO OUR HEADER LIST
                    headerFrames << frame;
                }
#endif
                TIFFClose(file);
            }
        }
        file = nullptr;
        return (true);
    }
    return (false);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUSaveToDiskFilter::getNextFilestring()
{
    // KEEP TRACK OF A STATIC COUNTER SO WE DON'T REDO FILENAMES
    static int localCounter = 0;

    for (int n = 0; n < 100000; n++) {
        QString string = QString("%1").arg(localCounter);
        while (string.length() < 5) {
            string.prepend("0");
        }
        string.prepend(QString("/data"));
        string.prepend(directoryString);
        string.append(QString(".tif"));

        if (QFile::exists(string)) {
            localCounter++;
        } else {
            emit emitNewRecordingOpened(localCounter);
            return (string);
        }
    }

    // IF WE MAKE IT THIS FAR, SOMETHING IS REALLY WRONG
    return (QString());
}
