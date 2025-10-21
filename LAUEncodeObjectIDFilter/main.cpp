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

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QTextStream>
#include <QHash>
#include <QSet>
#include <QXmlStreamWriter>
#include <QBuffer>
#include <QList>
#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "laumemoryobject.h"
#include "lauobjecthashtable.h"
#include "lauconstants.h"

QTextStream console(stdout);

// INPUT VALIDATION FUNCTION FOR FUZZING PROTECTION
struct ValidationResult {
    bool valid = false;
    QString errorMessage;
    int errorCode = 0;
};

ValidationResult validatePathString(const QString &path, const QString &parameterName, bool checkExists = false, bool mustBeDirectory = false)
{
    ValidationResult result;

    // CHECK LENGTH
    if (path.length() == 0) {
        result.errorMessage = QString("Error: %1 is empty").arg(parameterName);
        result.errorCode = 5;
        return result;
    }

    if (path.length() > 4096) {
        result.errorMessage = QString("Error: %1 exceeds maximum length (4096 characters)").arg(parameterName);
        result.errorCode = 4;
        return result;
    }

    // CHECK FOR PATH TRAVERSAL
    if (path.contains("..")) {
        result.errorMessage = QString("Error: Path traversal detected in %1 (contains '..')").arg(parameterName);
        result.errorCode = 6;
        return result;
    }

    // CHECK FILE EXTENSION IF APPLICABLE
    if (!mustBeDirectory && path.contains('.')) {
        QString extension = QFileInfo(path).suffix().toLower();
        QStringList validExtensions;
        validExtensions << "tif" << "tiff" << "lau" << "csv" << "json";

        if (!validExtensions.contains(extension)) {
            result.errorMessage = QString("Error: Invalid file extension for %1: .%2").arg(parameterName).arg(extension);
            result.errorCode = 7;
            return result;
        }
    }

    // CHECK EXISTENCE IF REQUIRED
    if (checkExists) {
        if (mustBeDirectory) {
            QDir dir(path);
            if (!dir.exists()) {
                result.errorMessage = QString("Error: Directory does not exist: %1").arg(path);
                result.errorCode = 2;
                return result;
            }
        } else {
            QFileInfo fileInfo(path);
            if (!fileInfo.exists()) {
                result.errorMessage = QString("Error: File does not exist: %1").arg(path);
                result.errorCode = 2;
                return result;
            }
        }
    }

    result.valid = true;
    return result;
}

bool LAUMemoryObject_LessThan(const LAUMemoryObject &s1, const LAUMemoryObject &s2)
{
    return (s1.elapsed() < s2.elapsed());
}

QByteArray createXMLStringWithObjectID(QByteArray inXml, const QString &objectID, int originalFrameOrder)
{
    // GRAB THE CURRENT XML FIELD IN HASH TABLE FORM
    QHash<QString, QString> hashTable = LAUMemoryObject::xmlToHash(inXml);

    // ADD OBJECT ID AND ORIGINAL FRAME ORDER TO METADATA
    // Pad object ID to 15 digits for consistent field length (enables in-place editing later)
    QString paddedObjectID = objectID.leftJustified(15, ' ');
    hashTable["ObjectID"] = paddedObjectID;
    hashTable["OriginalFrameOrder"] = QString::number(originalFrameOrder);

    // CREATE THE XML DATA PACKET USING QT'S XML STREAM OBJECTS
    QByteArray xmlByteArray;
    QBuffer buffer(&xmlByteArray);
    buffer.open(QIODevice::WriteOnly);

    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("scan");

    QHashIterator<QString, QString> i(hashTable);
    while (i.hasNext()) {
        i.next();
        writer.writeTextElement(i.key(), i.value());
    }

    // CLOSE OUT THE XML BUFFER
    writer.writeEndElement();
    writer.writeEndDocument();
    buffer.close();

    // EXPORT THE XML BUFFER TO THE XMLPACKET FIELD OF THE TIFF IMAGE
    return(xmlByteArray);
}

QString findMostFrequentObjectID(const QStringList &objectIDs)
{
    QHash<QString, int> frequency;

    // Count frequency of each object ID
    for (const QString &id : objectIDs) {
        frequency[id]++;
    }

    // Find the most frequent object ID
    QString mostFrequent;
    int maxCount = 0;
    QHashIterator<QString, int> i(frequency);
    while (i.hasNext()) {
        i.next();
        if (i.value() > maxCount) {
            maxCount = i.value();
            mostFrequent = i.key();
        }
    }

    return mostFrequent;
}

bool undoDataFile(const QString &filePath, int &restoredCount, int &skippedCount)
{
    console << "Undoing file: " << QFileInfo(QFileInfo(filePath).absolutePath()).fileName() << "/" << QFileInfo(filePath).fileName() << "\n";
    console.flush();

    try {
        // Check if file has object ID metadata to remove
        LAUMemoryObject firstFrame(filePath, 0);
        QHash<QString, QString> metadata = LAUMemoryObject::xmlToHash(firstFrame.xml());

        if (!metadata.contains("ObjectID") && !metadata.contains("OriginalFrameOrder")) {
            console << "  Skipping: No object ID metadata found\n";
            skippedCount++;
            return false;
        }

        // Get number of frames
        int numDirectories = LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(filePath);
        if (numDirectories <= 1) {
            console << "  Warning: File has only " << numDirectories << " frames, skipping\n";
            skippedCount++;
            return false;
        }

        // Load all frames with their original order information
        QList<QPair<int, LAUMemoryObject>> frameOrder;

        for (int frameNum = 0; frameNum < numDirectories; frameNum++) {
            LAUMemoryObject frame(filePath, frameNum);
            QHash<QString, QString> frameMeta = LAUMemoryObject::xmlToHash(frame.xml());

            // Get original frame order if it exists, otherwise use current order
            int originalOrder = frameNum;
            if (frameMeta.contains("OriginalFrameOrder")) {
                originalOrder = frameMeta["OriginalFrameOrder"].toInt();
            }

            // Remove ObjectID and OriginalFrameOrder from metadata
            frameMeta.remove("ObjectID");
            frameMeta.remove("OriginalFrameOrder");

            // Rebuild XML without these fields
            QByteArray xmlByteArray;
            QBuffer buffer(&xmlByteArray);
            buffer.open(QIODevice::WriteOnly);

            QXmlStreamWriter writer(&buffer);
            writer.setAutoFormatting(true);
            writer.writeStartDocument();
            writer.writeStartElement("scan");

            QHashIterator<QString, QString> i(frameMeta);
            while (i.hasNext()) {
                i.next();
                writer.writeTextElement(i.key(), i.value());
            }

            writer.writeEndElement();
            writer.writeEndDocument();
            buffer.close();

            frame.setXML(xmlByteArray);
            frameOrder.append(QPair<int, LAUMemoryObject>(originalOrder, frame));
        }

        // Sort frames back to original order
        std::sort(frameOrder.begin(), frameOrder.end(),
            [](const QPair<int, LAUMemoryObject> &a, const QPair<int, LAUMemoryObject> &b) {
                return a.first < b.first;
            });

        // Write frames back in original order
        libtiff::TIFF *outputTiff = libtiff::TIFFOpen(filePath.toLocal8Bit(), "w");
        if (!outputTiff) {
            console << "  Error: Failed to open file for writing\n";
            return false;
        }

        for (int frameNum = 0; frameNum < frameOrder.count(); frameNum++) {
            frameOrder[frameNum].second.save(outputTiff, frameNum);
        }

        TIFFClose(outputTiff);
        restoredCount++;
        console << "  Success: Removed object ID metadata and restored original frame order\n";
        return true;

    } catch (const std::exception &e) {
        console << "  Error: " << e.what() << "\n";
        return false;
    }
}

int undoDirectoryProcessing(const QString &directoryPath)
{
    QDir directory(directoryPath);
    if (!directory.exists()) {
        console << "Error: Directory does not exist: " << directoryPath << "\n";
        return 1;
    }

    console << "\n================================================\n";
    console << "UNDO MODE - Reverting object ID processing\n";
    console << "================================================\n\n";
    console << "Directory: " << directoryPath << "\n\n";

    int renamedFilesRestored = 0;
    int dataFilesRestored = 0;
    int skippedFiles = 0;

    // Step 1: Restore renamed files (noTag*.tif, badFile*.tif, and noCal*.tif back to data*.tif)
    console << "Step 1: Restoring renamed files...\n";
    QStringList renamedPatterns;
    renamedPatterns << "noTag*.tif" << "noTag*.tiff" << "noTag*.lau";
    renamedPatterns << "badFile*.tif" << "badFile*.tiff" << "badFile*.lau";
    renamedPatterns << "noCal*.tif" << "noCal*.tiff" << "noCal*.lau";

    for (const QString &pattern : renamedPatterns) {
        QFileInfoList renamedFiles = directory.entryInfoList(QStringList() << pattern, QDir::Files);

        for (const QFileInfo &fileInfo : renamedFiles) {
            QString fileName = fileInfo.baseName();
            QString dataNumber;

            if (fileName.startsWith("noTag")) {
                dataNumber = fileName.mid(5); // Extract number after "noTag"
            } else if (fileName.startsWith("badFile")) {
                dataNumber = fileName.mid(7); // Extract number after "badFile"
            } else if (fileName.startsWith("noCal")) {
                dataNumber = fileName.mid(5); // Extract number after "noCal"
            }

            QString newFileName = QString("data%1.%2").arg(dataNumber).arg(fileInfo.suffix());
            QString newFilePath = fileInfo.dir().absoluteFilePath(newFileName);

            // Check if target file already exists
            if (QFile::exists(newFilePath)) {
                console << "  Warning: Cannot rename " << fileInfo.fileName()
                        << " - target " << newFileName << " already exists\n";
                continue;
            }

            QFile file(fileInfo.absoluteFilePath());
            if (file.rename(newFilePath)) {
                console << "  Renamed: " << fileInfo.fileName() << " -> " << newFileName << "\n";
                renamedFilesRestored++;
            } else {
                console << "  Error: Failed to rename " << fileInfo.fileName() << "\n";
            }
        }
    }

    // Step 2: Process all data*.tif files to remove object ID metadata and restore frame order
    console << "\nStep 2: Removing object ID metadata and restoring frame order...\n";
    QStringList dataPatterns;
    dataPatterns << "data*.tif" << "data*.tiff" << "data*.lau";
    QFileInfoList dataFiles = directory.entryInfoList(dataPatterns, QDir::Files, QDir::Name);

    for (const QFileInfo &fileInfo : dataFiles) {
        undoDataFile(fileInfo.absoluteFilePath(), dataFilesRestored, skippedFiles);
    }

    // Summary
    console << "\n================================================\n";
    console << "Undo Processing Summary:\n";
    console << "  Renamed files restored: " << renamedFilesRestored << "\n";
    console << "  Data files restored: " << dataFilesRestored << "\n";
    console << "  Files skipped (no metadata): " << skippedFiles << "\n";
    console << "  Total files processed: " << (renamedFilesRestored + dataFilesRestored + skippedFiles) << "\n";
    console << "================================================\n";

    return 0;
}

struct ProcessingResult {
    bool success = false;
    QString objectID;
    QString newFilePath;
    QString status; // "success", "duplicate", "bad_file", "already_processed", "error"
};

ProcessingResult processDataFileWithResult(const QString &filePath, const LAUObjectHashTable &rfidTable, QHash<QString, QString> &objectIDToFile, int &renamedCount, int &badFileCount)
{
    console << "Processing file: " << QFileInfo(QFileInfo(filePath).absolutePath()).fileName() << "/" << QFileInfo(filePath).fileName() << "\n";
    console.flush();

    try {
        // CHECK IF FILE ALREADY HAS A OBJECT ID IN METADATA (FRAME 0)
        LAUMemoryObject firstFrame(filePath, 0);

        // VALIDATE JETR VECTORS IN FIRST FRAME (BACKGROUND)
        // First frame should contain complete JETR calibration vectors from background
        QVector<double> jetrVector = firstFrame.jetr();
        if (jetrVector.isEmpty() || jetrVector.size() < LAU_JETR_VECTOR_SIZE) {
            // No valid JETR vectors found - this file was recorded without proper calibration
            QFileInfo fileInfo(filePath);
            QString fileName = fileInfo.baseName();
            QString dataNumber = fileName.mid(4);
            QString newFileName = QString("noCal%1.%2").arg(dataNumber).arg(fileInfo.suffix());
            QString newFilePath = fileInfo.dir().absoluteFilePath(newFileName);

            console << "  Warning: Missing or incomplete JETR calibration (" << jetrVector.size() << " elements), renaming to: " << newFileName << "\n";
            console << "  (Hint: Run LAUBackgroundFilter to create calibration, then re-record videos)\n";

            // Rename the file
            ProcessingResult result;
            QFile file(filePath);
            if (file.rename(newFilePath)) {
                badFileCount++;
                console << "  Success: Renamed file with invalid JETR\n";
                result.success = true;
                result.status = "no_calibration";
                result.newFilePath = newFilePath;
            } else {
                console << "  Error: Failed to rename file\n";
                result.success = false;
                result.status = "error";
                result.newFilePath = filePath;
            }
            return result;
        }

        // Validate JETR vector size is a multiple of LAU_JETR_VECTOR_SIZE (LAU_JETR_VECTOR_SIZE elements per camera)
        if (jetrVector.size() % LAU_JETR_VECTOR_SIZE != 0) {
            QFileInfo fileInfo(filePath);
            QString fileName = fileInfo.baseName();
            QString dataNumber = fileName.mid(4);
            QString newFileName = QString("noCal%1.%2").arg(dataNumber).arg(fileInfo.suffix());
            QString newFilePath = fileInfo.dir().absoluteFilePath(newFileName);

            int numCameras = jetrVector.size() / LAU_JETR_VECTOR_SIZE;
            int remainder = jetrVector.size() % LAU_JETR_VECTOR_SIZE;
            console << "  Warning: Invalid JETR vector size (" << jetrVector.size()
                    << " elements = " << numCameras << " cameras + " << remainder
                    << " extra), renaming to: " << newFileName << "\n";

            // Rename the file
            ProcessingResult result;
            QFile file(filePath);
            if (file.rename(newFilePath)) {
                badFileCount++;
                console << "  Success: Renamed file with malformed JETR\n";
                result.success = true;
                result.status = "no_calibration";
                result.newFilePath = newFilePath;
            } else {
                console << "  Error: Failed to rename file\n";
                result.success = false;
                result.status = "error";
                result.newFilePath = filePath;
            }
            return result;
        }

        // JETR size is valid - now check calibration quality for each camera
        int numCameras = jetrVector.size() / LAU_JETR_VECTOR_SIZE;
        bool hasValidCalibration = true;
        QString calibrationIssue;

        for (int cam = 0; cam < numCameras; cam++) {
            int offset = cam * LAU_JETR_VECTOR_SIZE;

            // Check transform matrix (elements 12-27: 4x4 matrix)
            // Identity matrix check: diagonal should be 1,1,1,1 and off-diagonal should be 0
            // Format: T00,T10,T20,T30, T01,T11,T21,T31, T02,T12,T22,T32, T03,T13,T23,T33
            bool isIdentity = true;
            // Check diagonal elements (positions 0,5,10,15 in the 16-element matrix)
            if (qAbs(jetrVector[offset + 12] - 1.0) > 0.001 ||  // T00
                qAbs(jetrVector[offset + 17] - 1.0) > 0.001 ||  // T11
                qAbs(jetrVector[offset + 22] - 1.0) > 0.001 ||  // T22
                qAbs(jetrVector[offset + 27] - 1.0) > 0.001) {  // T33
                isIdentity = false;
            }
            // Check if off-diagonal elements are near zero (identity matrix)
            for (int i = 12; i < 28 && isIdentity; i++) {
                int matrixPos = i - 12;
                int row = matrixPos % 4;
                int col = matrixPos / 4;
                if (row != col && qAbs(jetrVector[offset + i]) > 0.001) {
                    isIdentity = false;
                }
            }

            if (isIdentity) {
                hasValidCalibration = false;
                calibrationIssue = QString("Camera %1 has identity transform matrix (not calibrated)").arg(cam + 1);
                break;
            }

            // Check bounding box (elements 28-33: xMin,xMax,yMin,yMax,zMin,zMax)
            double xMin = jetrVector[offset + 28];
            double xMax = jetrVector[offset + 29];
            double yMin = jetrVector[offset + 30];
            double yMax = jetrVector[offset + 31];
            double zMin = jetrVector[offset + 32];
            double zMax = jetrVector[offset + 33];

            // Check if bounding box is infinite or too large (uncalibrated)
            if (qIsInf(xMin) || qIsInf(xMax) || qIsInf(yMin) ||
                qIsInf(yMax) || qIsInf(zMin) || qIsInf(zMax)) {
                hasValidCalibration = false;
                calibrationIssue = QString("Camera %1 has infinite bounding box (not calibrated)").arg(cam + 1);
                break;
            }

            // Check if bounding box is unreasonably large (likely default/uncalibrated)
            // Typical object region should be < 10 meters in any dimension
            if ((xMax - xMin) > 10000 || (yMax - yMin) > 10000 || (zMax - zMin) > 10000) {
                hasValidCalibration = false;
                calibrationIssue = QString("Camera %1 has unreasonably large bounding box: %.0fx%.0fx%.0f mm (not calibrated)")
                    .arg(cam + 1).arg(xMax - xMin).arg(yMax - yMin).arg(zMax - zMin);
                break;
            }
        }

        // If calibration is incomplete, rename file
        if (!hasValidCalibration) {
            QFileInfo fileInfo(filePath);
            QString fileName = fileInfo.baseName();
            QString dataNumber = fileName.mid(4);
            QString newFileName = QString("noCal%1.%2").arg(dataNumber).arg(fileInfo.suffix());
            QString newFilePath = fileInfo.dir().absoluteFilePath(newFileName);

            console << "  Warning: Incomplete calibration - " << calibrationIssue << ", renaming to: " << newFileName << "\n";
            console << "  (Hint: Load a sample video in LAUJetrStandalone to set transforms and bounding box)\n";

            // Rename the file
            ProcessingResult result;
            QFile file(filePath);
            if (file.rename(newFilePath)) {
                badFileCount++;
                console << "  Success: Renamed file with incomplete calibration\n";
                result.success = true;
                result.status = "no_calibration";
                result.newFilePath = newFilePath;
            } else {
                console << "  Error: Failed to rename file\n";
                result.success = false;
                result.status = "error";
                result.newFilePath = filePath;
            }
            return result;
        }

        qDebug() << "  JETR validation passed:" << numCameras << "camera(s), calibrated with transforms and bounding boxes";

        QHash<QString, QString> metadata = LAUMemoryObject::xmlToHash(firstFrame.xml());

        if (metadata.contains("ObjectID") && !metadata["ObjectID"].isEmpty()) {
            QString existingObjectID = metadata["ObjectID"].trimmed(); // Remove padding for comparison

            // CHECK FOR DUPLICATES
            if (objectIDToFile.contains(existingObjectID)) {
                // This object ID was already processed by another file, rename this one
                QFileInfo fileInfo(filePath);
                QString fileName = fileInfo.baseName();
                QString dataNumber = fileName.mid(4);
                QString newFileName = QString("noTag%1.%2").arg(dataNumber).arg(fileInfo.suffix());
                QString newFilePath = fileInfo.dir().absoluteFilePath(newFileName);

                console << "  Info: Object ID " << existingObjectID << " already processed by " << QFileInfo(objectIDToFile[existingObjectID]).fileName() << "\n";
                console << "  Renaming duplicate to: " << newFileName << "\n";
                
                ProcessingResult result;
                QFile file(filePath);
                if (file.rename(newFilePath)) {
                    renamedCount++;
                    console << "  Success: Renamed duplicate file\n";
                    result.success = true;
                    result.status = "duplicate";
                    result.newFilePath = newFilePath;
                    result.objectID = existingObjectID;
                } else {
                    console << "  Error: Failed to rename file\n";
                    result.success = false;
                    result.status = "error";
                    result.newFilePath = filePath;
                }
                return result;
            } else {
                // Add to processed list and return immediately
                objectIDToFile[existingObjectID] = filePath;
                console << "  Info: File already has object ID " << existingObjectID << " in metadata, skipping\n";
                ProcessingResult result;
                result.success = true;
                result.status = "already_processed";
                result.objectID = existingObjectID;
                result.newFilePath = filePath;
                return result;
            }
        }
        // HOW MANY DIRECTORIES DOES THIS TIFF FILE HAVE
        int numDirectories = LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(filePath);
        if (numDirectories <= 1) {
            console << "  Warning: File has only " << numDirectories << " frames, skipping\n";
            ProcessingResult result;
            result.success = false;
            result.status = "error";
            result.newFilePath = filePath;
            return result;
        }

        // GENERATE A LIST OF VALID VIDEO FRAMES AND EXTRACT OBJECT IDs
        QStringList objectIDs;
        QList<LAUMemoryObject> frames;
        bool headerFrame = false;

        for (int frameNum = 0; frameNum < numDirectories; frameNum++) {
            // LOAD THE CURRENT FRAME OF VIDEO FROM DISK
            LAUMemoryObject frame(filePath, frameNum);

            // ALWAYS KEEP THE FIRST FRAME OF VIDEO OTHERWISE ONLY KEEP IF VALID
            if (frameNum > 0 && !headerFrame) {
                if (frame.elapsed() < frames.last().elapsed()) {
                    headerFrame = true;
                } else {
                    // Extract object ID from RFID
                    QString rfid = frame.rfid();
                    int objectIndex = rfidTable.id(rfid);
                    QString objectID;

                    if (objectIndex > -1) {
                        // Use mapped object ID from CSV file
                        objectID = QString::number(objectIndex);
                    } else {
                        // Use raw RFID tag as object ID (normal for remote operation)
                        objectID = rfid;
                    }

                    if (!objectID.isEmpty()) {
                        objectIDs << objectID;
                    }
                }
            }
            frames << frame;
        }

        // CHECK IF WE HAVE ENOUGH VALID OBJECT ID READINGS
        if (objectIDs.count() < 5) {
            // Extract the data number from the current file (e.g., "00023" from "data00023.tif")
            QFileInfo fileInfo(filePath);
            QString fileName = fileInfo.baseName(); // "data00023"
            QString dataNumber = fileName.mid(4); // "00023"
            
            // Create new filename: badFile00023.tif
            QString newFileName = QString("badFile%1.%2").arg(dataNumber).arg(fileInfo.suffix());
            QString newFilePath = fileInfo.dir().absoluteFilePath(newFileName);
            
            console << "  Warning: Not enough valid object ID readings (" << objectIDs.count() << "), renaming to: " << newFileName << "\n";
            
            // Rename the file
            ProcessingResult result;
            QFile file(filePath);
            if (file.rename(newFilePath)) {
                badFileCount++;
                console << "  Success: Renamed bad file\n";
                result.success = true;
                result.status = "bad_file";
                result.newFilePath = newFilePath;
            } else {
                console << "  Error: Failed to rename bad file\n";
                result.success = false;
                result.status = "error";
                result.newFilePath = filePath;
            }
            return result;
        }

        // FIND THE MOST FREQUENT OBJECT ID (MODE)
        QString finalObjectID = findMostFrequentObjectID(objectIDs);
        if (finalObjectID.isEmpty()) {
            console << "  Warning: Could not determine object ID, skipping\n";
            ProcessingResult result;
            result.success = false;
            result.status = "error";
            result.newFilePath = filePath;
            return result;
        }

        // CHECK FOR DUPLICATES
        if (objectIDToFile.contains(finalObjectID)) {
            // Extract the data number from the current file (e.g., "00042" from "data00042.tif")
            QFileInfo fileInfo(filePath);
            QString fileName = fileInfo.baseName(); // "data00042"
            QString dataNumber = fileName.mid(4); // "00042"

            // Create new filename: noTag00042.tif
            QString newFileName = QString("noTag%1.%2").arg(dataNumber).arg(fileInfo.suffix());
            QString newFilePath = fileInfo.dir().absoluteFilePath(newFileName);

            console << "  Info: Object ID " << finalObjectID << " already processed by " << QFileInfo(objectIDToFile[finalObjectID]).fileName() << "\n";
            console << "  Renaming duplicate to: " << newFileName << "\n";

            // Rename the file
            ProcessingResult result;
            QFile file(filePath);
            if (file.rename(newFilePath)) {
                renamedCount++;
                console << "  Success: Renamed duplicate file\n";
                result.success = true;
                result.status = "duplicate";
                result.objectID = finalObjectID;
                result.newFilePath = newFilePath;
            } else {
                console << "  Error: Failed to rename file\n";
                result.success = false;
                result.status = "error";
                result.newFilePath = filePath;
            }
            return result;
        }

        // MARK THIS OBJECT ID AS PROCESSED AND TRACK THE FILE
        objectIDToFile[finalObjectID] = filePath;

        // Show whether object ID is from mapping or raw RFID (before padding)
        if (finalObjectID.length() > 10) {
            // Likely a raw RFID tag (they are typically 15+ digits)
            console << "  Identified object ID (RFID tag): " << finalObjectID << " (will be padded to 15 digits)\n";
        } else {
            console << "  Identified object ID: " << finalObjectID << " (will be padded to 15 digits)\n";
        }

        // ADD OBJECT ID AND ORIGINAL FRAME ORDER TO ALL FRAMES
        for (int frameNum = 0; frameNum < frames.count(); frameNum++) {
            LAUMemoryObject &frame = frames[frameNum];
            QByteArray newXml = createXMLStringWithObjectID(frame.xml(), finalObjectID, frameNum);
            frame.setXML(newXml);
        }

        // SORT FRAMES IN CHRONOLOGICAL ORDER ACCORDING TO THE ELAPSED TIME
        std::sort(frames.begin(), frames.end(), LAUMemoryObject_LessThan);

        // OPEN OUTPUT TIFF FILE (OVERWRITE ORIGINAL)
        libtiff::TIFF *outputTiff = libtiff::TIFFOpen(filePath.toLocal8Bit(), "w");
        if (!outputTiff) {
            console << "  Error: Failed to open file for writing\n";
            ProcessingResult result;
            result.success = false;
            result.status = "error";
            result.newFilePath = filePath;
            return result;
        }

        // WRITE THE FRAMES OUT TO DISK IN CHRONOLOGICAL ORDER
        for (int frameNum = 0; frameNum < frames.count(); frameNum++) {
            frames.at(frameNum).save(outputTiff, frameNum);
        }

        // CLOSE THE TIFF FILE
        TIFFClose(outputTiff);

        console << "  Success: Encoded object ID " << finalObjectID << " and reordered " << frames.count() << " frames\n";
        ProcessingResult result;
        result.success = true;
        result.status = "success";
        result.objectID = finalObjectID;
        result.newFilePath = filePath;
        return result;

    } catch (const std::exception &e) {
        console << "  Error: " << e.what() << "\n";
        ProcessingResult result;
        result.success = false;
        result.status = "error";
        result.newFilePath = filePath;
        return result;
    }
}

bool processDataFile(const QString &filePath, const LAUObjectHashTable &rfidTable, QHash<QString, QString> &objectIDToFile, int &renamedCount, int &badFileCount)
{
    ProcessingResult result = processDataFileWithResult(filePath, rfidTable, objectIDToFile, renamedCount, badFileCount);
    return result.success;
}

int processManifestMode(const QString &manifestPath, const QString &rfidMappingFile, bool dryRun = false)
{
    // Load manifest JSON file
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) {
        console << "Error: Cannot open manifest file: " << manifestPath << "\n";
        return 1;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        console << "Error: Invalid manifest JSON format\n";
        return 1;
    }
    
    QJsonObject rootObj = doc.object();
    QJsonArray entries;
    
    // Support direct array or object with "entries" array
    if (rootObj.contains("entries") && rootObj["entries"].isArray()) {
        entries = rootObj["entries"].toArray();
    } else {
        console << "Error: No 'entries' array found in manifest\n";
        return 1;
    }
    
    // Extract base path from manifest file location
    QFileInfo manifestInfo(manifestPath);
    QString basePath = manifestInfo.absolutePath();
    
    // Find all unique directories containing files without object IDs
    QSet<QString> directoriesToProcess;
    QHash<QString, QList<QJsonObject>> directoryEntries; // Map directory to entries
    int filesNeedingProcessing = 0;
    int totalFiles = 0;

    console << "Analyzing manifest for files needing object ID processing...\n";

    for (const QJsonValue &value : entries) {
        if (!value.isObject()) continue;

        QJsonObject entry = value.toObject();
        totalFiles++;

        // Check if file needs object ID processing
        bool hasObjectId = entry["has_object_id"].toBool();
        QString objectId = entry["object_id"].toString();

        // Process files that either don't have object ID flag OR have flag but empty object ID
        bool needsProcessing = !hasObjectId || (hasObjectId && objectId.isEmpty());

        if (needsProcessing) {
            // Extract directory path from video file path
            QJsonObject videoFile = entry["video_file"].toObject();
            QString filePath = videoFile["path"].toString();

            if (!filePath.isEmpty()) {
                QFileInfo fileInfo(filePath);
                QString dirPath = fileInfo.absolutePath();
                directoriesToProcess.insert(dirPath);
                directoryEntries[dirPath].append(entry);
                filesNeedingProcessing++;
            }
        }
    }

    if (dryRun) {
        // DRY-RUN MODE: Generate detailed report
        console << "\n========================================\n";
        console << "DRY-RUN REPORT - No changes will be made\n";
        console << "========================================\n\n";
        console << "Manifest Summary:\n";
        console << "  Total entries in manifest: " << totalFiles << "\n";
        console << "  Files with object ID: " << (totalFiles - filesNeedingProcessing) << "\n";
        console << "  Files without object ID: " << filesNeedingProcessing << "\n";
        console << "  Unique directories to process: " << directoriesToProcess.count() << "\n";

        if (directoriesToProcess.isEmpty()) {
            console << "\nNo files found that need object ID encoding.\n";
            return 0;
        }

        console << "\nDirectories and files that would be processed:\n";
        console << "================================================\n";

        // Sort directories for consistent output
        QList<QString> sortedDirs = directoriesToProcess.values();
        std::sort(sortedDirs.begin(), sortedDirs.end());

        for (const QString &dirPath : sortedDirs) {
            console << "\nDirectory: " << dirPath << "\n";
            console << "  Files without object ID in this directory: " << directoryEntries[dirPath].size() << "\n";

            // Check if directory exists
            QDir dir(dirPath);
            if (!dir.exists()) {
                console << "  WARNING: Directory does not exist!\n";
                continue;
            }

            // List the actual data files in the directory
            QStringList nameFilters;
            nameFilters << "data*.tif" << "data*.tiff" << "data*.lau";
            QFileInfoList fileList = dir.entryInfoList(nameFilters, QDir::Files, QDir::Name);

            console << "  Total data files in directory: " << fileList.size() << "\n";

            // Show which specific files need processing
            console << "  Files needing object ID:\n";
            for (const QJsonObject &entry : directoryEntries[dirPath]) {
                QString dataNumber = entry["data_number"].toString();
                QJsonObject videoFile = entry["video_file"].toObject();
                QString fileName = QFileInfo(videoFile["path"].toString()).fileName();
                console << "    - " << fileName << " (data number: " << dataNumber << ")\n";
            }

            // Check for special files that might be skipped
            QStringList specialFiles;
            specialFiles << "noTag*.tif" << "badFile*.tif" << "noCal*.tif";
            QFileInfoList skipList = dir.entryInfoList(specialFiles, QDir::Files);
            if (!skipList.isEmpty()) {
                console << "  Special files (would be skipped):\n";
                for (const QFileInfo &file : skipList) {
                    console << "    - " << file.fileName() << "\n";
                }
            }
        }

        console << "\n========================================\n";
        console << "Processing Strategy:\n";
        console << "  1. Each directory would be processed sequentially\n";
        console << "  2. All data*.tif files would be analyzed for RFID\n";
        console << "  3. Files with < 5 valid RFID readings would be renamed to badFile#####.tif\n";
        console << "  4. Duplicate object IDs would be renamed to noTag#####.tif\n";
        console << "  5. Valid files would have object ID embedded in metadata\n";
        console << "\nRFID Mapping:\n";
        if (!rfidMappingFile.isEmpty() && QFile::exists(rfidMappingFile)) {
            console << "  Would use RFID mapping from: " << rfidMappingFile << "\n";
        } else {
            console << "  No RFID mapping file - would use raw RFID tags as object IDs\n";
            console << "  (This is normal for remote computer operation)\n";
        }
        console << "\nTo execute these changes, run without --dry-run flag\n";
        console << "========================================\n";

        return 0;
    }

    if (directoriesToProcess.isEmpty()) {
        console << "No files found that need object ID encoding\n";
        return 0;
    }

    console << "Found " << filesNeedingProcessing << " files needing processing in "
            << directoriesToProcess.count() << " directories\n\n";
    
    // Load RFID mapping
    LAUObjectHashTable rfidTable;
    if (!rfidMappingFile.isEmpty() && QFile::exists(rfidMappingFile)) {
        console << "Loading RFID mapping from: " << rfidMappingFile << "\n";
        if (!rfidTable.load(rfidMappingFile)) {
            console << "Info: Failed to load RFID mapping file, using raw RFID tags as object IDs\n";
        } else {
            console << "Info: RFID mapping loaded successfully\n";
        }
    } else {
        // This is normal for remote operation - use RFID tags directly
        if (rfidMappingFile.isEmpty()) {
            console << "Info: No RFID mapping file specified, using raw RFID tags as object IDs\n";
        } else {
            console << "Info: RFID mapping file not found, using raw RFID tags as object IDs\n";
        }
        console << "      (This is normal for remote computer operation)\n";
    }
    
    // Process each directory and track manifest updates
    int totalSuccess = 0;
    int totalSkipped = 0;
    int totalRenamed = 0;
    int totalBadFiles = 0;
    QHash<QString, ProcessingResult> fileResults; // Map original file path to result

    foreach (const QString &dirPath, directoriesToProcess) {
        console << "\nProcessing directory: " << dirPath << "\n";
        console << "========================================\n";

        QDir directory(dirPath);
        if (!directory.exists()) {
            console << "Error: Directory does not exist: " << dirPath << "\n";
            continue;
        }

        // Find all data*.tif files in this directory
        QStringList nameFilters;
        nameFilters << "data*.tif" << "data*.tiff" << "data*.lau";
        QFileInfoList fileList = directory.entryInfoList(nameFilters, QDir::Files, QDir::Name);

        QHash<QString, QString> objectIDToFile;
        int successCount = 0;
        int skipCount = 0;
        int renamedCount = 0;
        int badFileCount = 0;

        for (const QFileInfo &fileInfo : fileList) {
            ProcessingResult result = processDataFileWithResult(fileInfo.absoluteFilePath(), rfidTable, objectIDToFile, renamedCount, badFileCount);

            // Store result for manifest updating
            fileResults[fileInfo.absoluteFilePath()] = result;

            if (result.success) {
                successCount++;
            } else {
                skipCount++;
            }
        }

        console << "Directory summary: " << successCount << " processed, "
                << skipCount << " skipped, " << renamedCount << " duplicates, "
                << badFileCount << " bad files\n";

        totalSuccess += successCount;
        totalSkipped += skipCount;
        totalRenamed += renamedCount;
        totalBadFiles += badFileCount;
    }

    // Update manifest with processing results
    console << "\nUpdating manifest with processing results...\n";
    bool manifestModified = false;

    for (int i = 0; i < entries.size(); i++) {
        QJsonObject entry = entries[i].toObject();

        // Check if this entry was processed (either no object ID flag OR empty object ID)
        bool hasObjectId = entry["has_object_id"].toBool();
        QString objectId = entry["object_id"].toString();
        bool needsProcessing = !hasObjectId || (hasObjectId && objectId.isEmpty());

        if (needsProcessing) {
            QJsonObject videoFile = entry["video_file"].toObject();
            QString originalFilePath = videoFile["path"].toString();

            if (fileResults.contains(originalFilePath)) {
                ProcessingResult result = fileResults[originalFilePath];
                manifestModified = true;

                if (result.status == "success" || result.status == "already_processed") {
                    // Successfully processed - update manifest entry
                    entry["has_object_id"] = true;
                    entry["object_id"] = result.objectID;

                    // Update file path if it changed
                    if (result.newFilePath != originalFilePath) {
                        videoFile["path"] = result.newFilePath;
                        entry["video_file"] = videoFile;
                    }

                    console << "  Updated entry " << entry["data_number"].toString()
                            << " - object ID: " << result.objectID << "\n";

                } else if (result.status == "duplicate" || result.status == "bad_file") {
                    // File was renamed - update path but keep has_object_id false
                    videoFile["path"] = result.newFilePath;
                    entry["video_file"] = videoFile;

                    // Add processing status for tracking
                    if (result.status == "duplicate") {
                        entry["processing_status"] = "duplicate_object_id";
                        entry["object_id"] = result.objectID; // Keep the object ID that caused the duplicate
                    } else {
                        entry["processing_status"] = "insufficient_rfid_readings";
                    }

                    console << "  Updated entry " << entry["data_number"].toString()
                            << " - renamed to: " << QFileInfo(result.newFilePath).fileName()
                            << " (" << result.status << ")\n";
                }

                entries[i] = entry;
            }
        }
    }

    // Write updated manifest back to disk
    if (manifestModified) {
        rootObj["entries"] = entries;
        QJsonDocument updatedDoc(rootObj);

        QFile outputFile(manifestPath);
        if (outputFile.open(QIODevice::WriteOnly)) {
            outputFile.write(updatedDoc.toJson(QJsonDocument::Indented));
            outputFile.close();
            console << "\nManifest updated successfully: " << manifestPath << "\n";
        } else {
            console << "\nWarning: Failed to write updated manifest to: " << manifestPath << "\n";
        }
    } else {
        console << "\nNo manifest updates needed\n";
    }
    
    // Print overall summary
    console << "\n========================================================\n";
    console << "Overall Processing Summary:\n";
    console << "  Directories processed: " << directoriesToProcess.count() << "\n";
    console << "  Successfully processed: " << totalSuccess << "\n";
    console << "  Skipped (errors): " << totalSkipped << "\n";
    console << "  Renamed (duplicates): " << totalRenamed << "\n";
    console << "  Renamed (bad files): " << totalBadFiles << "\n";
    
    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setOrganizationName("Lau Consulting Inc");
    app.setOrganizationDomain("drhalftone.com");
    app.setApplicationName("LAUEncodeObjectIDFilter");

    // SET THE TIFF ERROR AND WARNING HANDLERS
    libtiff::TIFFSetErrorHandler(myTIFFErrorHandler);
    libtiff::TIFFSetWarningHandler(myTIFFWarningHandler);

    // CHECK FOR HELP ARGUMENT FIRST
    if (argc >= 2) {
        QString arg1 = QString::fromUtf8(argv[1]);
        if (arg1 == "-h" || arg1 == "--help" || arg1 == "-?" || arg1.toLower() == "help") {
            console << "LAUEncodeObjectIDFilter - Object ID Metadata Encoding Tool\n";
            console << "========================================================\n";
            console << "Compiled: " << __DATE__ << " " << __TIME__ << "\n\n";
            console << "DESCRIPTION:\n";
            console << "  This utility processes 3D video recordings from object tracking systems and\n";
            console << "  encodes object identification metadata into TIFF files. It reads RFID tag data\n";
            console << "  embedded in video frames and associates it with individual objects.\n\n";
            console << "  The tool can operate in several modes:\n";
            console << "  - Directory Mode: Process all data*.tif files in a folder\n";
            console << "  - Manifest Mode: Process files listed in a JSON manifest\n";
            console << "  - Dry-Run Mode: Preview changes without modifying files\n";
            console << "  - Undo Mode: Remove object ID metadata and restore original state\n\n";
            console << "USAGE:\n";
            console << "  Directory mode:  LAUEncodeObjectIDFilter <directory_path> [rfid_mapping.csv]\n";
            console << "  Manifest mode:   LAUEncodeObjectIDFilter --manifest <manifest.json> [rfid_mapping.csv]\n";
            console << "  Dry-run mode:    LAUEncodeObjectIDFilter --dry-run --manifest <manifest.json> [rfid_mapping.csv]\n";
            console << "  Undo mode:       LAUEncodeObjectIDFilter --undo <directory_path>\n\n";
            console << "ARGUMENTS:\n";
            console << "  directory_path    Directory containing data####.tif files to process\n";
            console << "                    Must be an existing directory\n";
            console << "                    Maximum path length: 4096 characters\n\n";
            console << "  rfid_mapping.csv  Optional CSV file mapping RFID tags to object ID numbers\n";
            console << "                    If not provided, uses raw RFID tags as object IDs\n";
            console << "                    Maximum path length: 4096 characters\n\n";
            console << "  manifest.json     JSON file listing entries that need processing\n";
            console << "                    Maximum path length: 4096 characters\n\n";
            console << "  --dry-run         Analyze manifest without making any changes\n";
            console << "                    Shows detailed preview of what would be done\n\n";
            console << "  --undo            Remove object ID metadata and restore original frame order\n";
            console << "                    Renames noTag*.tif, badFile*.tif, and noCal*.tif back to data*.tif\n\n";
            console << "PROCESSING BEHAVIOR:\n";
            console << "  - Analyzes RFID readings from video frames\n";
            console << "  - Requires minimum 5 valid RFID readings per file\n";
            console << "  - Uses most frequent (mode) RFID as the object ID\n";
            console << "  - Sorts frames chronologically by timestamp\n";
            console << "  - Embeds object ID in TIFF metadata (padded to 15 digits)\n";
            console << "  - Stores original frame order for undo capability\n\n";
            console << "FILE RENAMING:\n";
            console << "  Files are automatically renamed based on processing results:\n";
            console << "  - data#####.tif   -> Original file (processed successfully)\n";
            console << "  - noTag#####.tif  -> Duplicate object ID detected\n";
            console << "  - badFile#####.tif-> Insufficient RFID readings (< 5)\n";
            console << "  - noCal#####.tif  -> Missing or incomplete calibration\n\n";
            console << "EXAMPLES:\n";
            console << "  # Remote mode (no CSV, uses raw RFID tags as IDs)\n";
            console << "  LAUEncodeObjectIDFilter /path/to/data/folder\n\n";
            console << "  # Server mode (with RFID-to-ID mapping)\n";
            console << "  LAUEncodeObjectIDFilter /path/to/data/folder /path/to/rfid_mapping.csv\n\n";
            console << "  # Process files from manifest without CSV\n";
            console << "  LAUEncodeObjectIDFilter --manifest /path/to/manifest.json\n\n";
            console << "  # Preview changes without modifying files\n";
            console << "  LAUEncodeObjectIDFilter --dry-run --manifest ~/OneDrive/Videos/manifest.json\n\n";
            console << "  # Undo previous processing\n";
            console << "  LAUEncodeObjectIDFilter --undo /path/to/data/folder\n\n";
            console << "RETURN CODES:\n";
            console << "  0  - Success (all files processed)\n";
            console << "  1  - Insufficient arguments or invalid usage\n";
            console << "  2  - Directory does not exist\n";
            console << "  3  - No data files found in directory\n";
            console << "  4  - Path string too long (>4096 characters)\n";
            console << "  5  - Path string empty\n";
            console << "  6  - Path traversal detected (security violation)\n";
            console << "  7  - Invalid file extension\n\n";
            console << "SUPPORTED PLATFORMS:\n";
#if defined(Q_OS_WIN)
            console << "  - Windows\n";
#endif
#if defined(Q_OS_MACOS)
            console << "  - macOS\n";
#endif
#if defined(Q_OS_LINUX)
            console << "  - Linux\n";
#endif
            console << "\nFor more information, visit: drhalftone.com\n";
            console << "Copyright (c) 2017, Lau Consulting Inc\n";
            return 0;
        }
    }

    console << "LAUEncodeObjectIDFilter - Encode Object IDs into TIFF metadata\n";
    console << "========================================================\n\n";

    // CHECK COMMAND LINE ARGUMENTS
    if (argc < 2) {
        console << "LAUEncodeObjectIDFilter - Object ID Metadata Encoding Tool\n";
        console << "========================================================\n";
        console << "Compiled: " << __DATE__ << " " << __TIME__ << "\n\n";
        
        console << "DESCRIPTION:\n";
        console << "  This tool processes 3D video recordings from object tracking systems and\n";
        console << "  encodes object identification metadata directly into TIFF file headers.\n";
        console << "  It detects RFID tags in video frames and associates them with object IDs.\n\n";
        
        console << "KEY FEATURES:\n";
        console << "  - Processes data*.tif files containing 3D point cloud data\n";
        console << "  - Detects RFID tags embedded in corner pixels of frames\n";
        console << "  - Maps RFID tags to object IDs using CSV lookup files\n";
        console << "  - Updates TIFF metadata without recompressing image data\n";
        console << "  - Supports batch processing and undo operations\n\n";
        
        console << "USAGE MODES:\n";
        console << "  1. Directory mode:  LAUEncodeObjectIDFilter <directory_path> [rfid_mapping.csv]\n";
        console << "     Process all data*.tif files in the specified directory\n\n";
        
        console << "  2. Manifest mode:   LAUEncodeObjectIDFilter --manifest <manifest.json> [rfid_mapping.csv]\n";
        console << "     Process files listed in a JSON manifest file\n\n";
        
        console << "  3. Dry-run mode:    LAUEncodeObjectIDFilter --dry-run --manifest <manifest.json> [rfid_mapping.csv]\n";
        console << "     Preview what would be done without modifying files\n\n";
        
        console << "  4. Undo mode:       LAUEncodeObjectIDFilter --undo <directory_path>\n";
        console << "     Remove object ID metadata from previously processed files\n\n";
        
        console << "EXAMPLES:\n";
        console << "  LAUEncodeObjectIDFilter C:\\VideoData\\Recording001\n";
        console << "  LAUEncodeObjectIDFilter /data/recordings/system1 custom_rfid_map.csv\n";
        console << "  LAUEncodeObjectIDFilter --manifest recordings.json\n";
        console << "  LAUEncodeObjectIDFilter --undo C:\\VideoData\\Recording001\n\n";
        
        console << "For detailed help and all options, run: LAUEncodeObjectIDFilter --help\n";
        return 1;
    }

    // DETECT DRY-RUN, UNDO, AND MANIFEST MODE VS DIRECTORY MODE
    bool dryRun = false;
    bool undoMode = false;
    int argIndex = 1;

    // Check for --dry-run flag
    if (QString::fromUtf8(argv[argIndex]) == "--dry-run") {
        dryRun = true;
        argIndex++;
        if (argc <= argIndex) {
            console << "Error: Expected --manifest after --dry-run\n";
            return 1;
        }
    }

    // Check for --undo flag
    if (QString::fromUtf8(argv[argIndex]) == "--undo") {
        undoMode = true;
        argIndex++;
        if (argc <= argIndex) {
            console << "Error: Directory path required after --undo\n";
            return 1;
        }

        // VALIDATE DIRECTORY PATH
        QString directoryPath = QString::fromUtf8(argv[argIndex]);
        ValidationResult validation = validatePathString(directoryPath, "directory path", true, true);
        if (!validation.valid) {
            console << validation.errorMessage << "\n";
            return validation.errorCode;
        }

        return undoDirectoryProcessing(directoryPath);
    }

    QString firstArg = QString::fromUtf8(argv[argIndex]);
    if (firstArg == "--manifest") {
        // MANIFEST MODE
        argIndex++;
        if (argc <= argIndex) {
            console << "Error: Manifest file path required when using --manifest\n";
            return 1;
        }

        // VALIDATE MANIFEST PATH
        QString manifestPath = QString::fromUtf8(argv[argIndex]);
        ValidationResult validation = validatePathString(manifestPath, "manifest path", false, false);
        if (!validation.valid) {
            console << validation.errorMessage << "\n";
            return validation.errorCode;
        }

        QString rfidMappingFile;
        argIndex++;
        if (argc > argIndex) {
            rfidMappingFile = QString::fromUtf8(argv[argIndex]);

            // VALIDATE RFID MAPPING FILE PATH
            ValidationResult rfidValidation = validatePathString(rfidMappingFile, "RFID mapping file", false, false);
            if (!rfidValidation.valid) {
                console << rfidValidation.errorMessage << "\n";
                return rfidValidation.errorCode;
            }
        }

        return processManifestMode(manifestPath, rfidMappingFile, dryRun);
    }

    if (dryRun) {
        console << "Error: --dry-run can only be used with --manifest mode\n";
        return 1;
    }

    // DIRECTORY MODE (original behavior)
    // VALIDATE DIRECTORY PATH
    QString directoryPath = firstArg;
    ValidationResult dirValidation = validatePathString(directoryPath, "directory path", true, true);
    if (!dirValidation.valid) {
        console << dirValidation.errorMessage << "\n";
        return dirValidation.errorCode;
    }

    QDir directory(directoryPath);

    // GET RFID MAPPING FILE
    QString rfidMappingFile;
    if (argc >= 3) {
        rfidMappingFile = QString::fromUtf8(argv[2]);

        // VALIDATE RFID MAPPING FILE PATH
        ValidationResult rfidValidation = validatePathString(rfidMappingFile, "RFID mapping file", false, false);
        if (!rfidValidation.valid) {
            console << rfidValidation.errorMessage << "\n";
            return rfidValidation.errorCode;
        }
    } else {
        // Look for objectID.csv in the directory
        rfidMappingFile = directory.absoluteFilePath("objectID.csv");
    }

    // LOAD RFID TO OBJECT ID MAPPING TABLE
    LAUObjectHashTable rfidTable;
    if (!rfidMappingFile.isEmpty() && QFile::exists(rfidMappingFile)) {
        console << "Loading RFID mapping from: " << rfidMappingFile << "\n";
        if (!rfidTable.load(rfidMappingFile)) {
            console << "Info: Failed to load RFID mapping file, using raw RFID tags as object IDs\n";
        } else {
            console << "Info: RFID mapping loaded successfully\n";
        }
    } else {
        // This is normal for remote operation - use RFID tags directly
        if (rfidMappingFile.isEmpty()) {
            console << "Info: No RFID mapping file specified, using raw RFID tags as object IDs\n";
        } else {
            console << "Info: RFID mapping file not found (" << rfidMappingFile << "), using raw RFID tags as object IDs\n";
        }
        console << "      (This is normal for remote computer operation)\n";
    }

    // FIND ALL DATA*.TIF FILES IN THE DIRECTORY
    QStringList nameFilters;
    nameFilters << "data*.tif" << "data*.tiff" << "data*.lau";

    QFileInfoList fileList = directory.entryInfoList(nameFilters, QDir::Files, QDir::Name);

    if (fileList.isEmpty()) {
        console << "Error: No data*.tif files found in directory: " << directoryPath << "\n";
        return 3;
    }

    console << "Found " << fileList.count() << " data files to process\n\n";

    // PROCESS EACH FILE
    QHash<QString, QString> objectIDToFile; // Map object ID to first file that had it
    int successCount = 0;
    int skipCount = 0;
    int renamedCount = 0;
    int badFileCount = 0;

    for (const QFileInfo &fileInfo : fileList) {
        bool success = processDataFile(fileInfo.absoluteFilePath(), rfidTable, objectIDToFile, renamedCount, badFileCount);

        if (success) {
            successCount++;
        } else {
            skipCount++;
        }
    }

    // PRINT SUMMARY
    console << "\n========================================================\n";
    console << "Processing Summary:\n";
    console << "  Total files found: " << fileList.count() << "\n";
    console << "  Successfully processed: " << successCount << "\n";
    console << "  Skipped (errors): " << skipCount << "\n";
    console << "  Renamed (duplicates): " << renamedCount << "\n";
    console << "  Renamed (bad files): " << badFileCount << "\n";
    console << "  Unique object IDs found: " << objectIDToFile.count() << "\n";
    console << "\nProcessed object IDs: ";

    QStringList sortedObjectIDs = objectIDToFile.keys();
    sortedObjectIDs.sort();
    console << sortedObjectIDs.join(", ") << "\n";

    return 0;
}