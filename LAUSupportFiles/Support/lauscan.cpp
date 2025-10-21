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

#include "lauscan.h"

#include <QtMath>
#include <QBuffer>
#include <QSettings>
#include <QDomDocument>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#ifndef HEADLESS
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#ifndef EXCLUDE_LAUSCANINSPECTOR
#include "lauscaninspector.h"
#endif
#endif

#include <stdint.h>
#include <math.h>

using namespace libtiff;
using namespace LAU3DVideoParameters;

#ifdef LAU_LOOKUP_TABLE_SUPPORT
#include "laulookuptable.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::fromRawDepth(const LAUMemoryObject &object, const LAULookUpTable &table)
{
    LAUScan scan(object.width(), object.height(), LAU3DVideoParameters::ColorXYZG);

    float zMin = qMin(table.zLimits().x(), table.zLimits().y());
    float zMax = qMax(table.zLimits().x(), table.zLimits().y());

    for (unsigned int row = 0; row < object.height(); row++) {
        unsigned short *inBuffer = (unsigned short*)(object.constScanLine(row));
        float *otBuffer = (float*)scan.scanLine(row);

        for (unsigned int col = 0; col < object.width(); col++) {
            float *lutVector = (float*)(table.constScanLine(row)) + (table.colors() * col);
            float pixel = inBuffer[col] / 65535.0f;

            // Polynomial depth calculation
            float z = (lutVector[4] * pow(pixel, 4.0f))
                      + (lutVector[5] * pow(pixel, 3.0f))
                      + (lutVector[6] * pow(pixel, 2.0f))
                      + (lutVector[7] * pixel)
                      + lutVector[8];

            // Validate z-range
            z = (z <= zMin || z >= zMax) ? NAN : z;

            // Calculate spatial coordinates
            otBuffer[4*col + 0] = (lutVector[0] * z) + lutVector[1]; // X
            otBuffer[4*col + 1] = (lutVector[2] * z) + lutVector[3]; // Y
            otBuffer[4*col + 2] = z;                                 // Z
            otBuffer[4*col + 3] = pixel;                             // Grayscale
        }
    }
    scan.updateLimits();
    return scan;
}
#endif // LAULOOKUPTABLE_H

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan::LAUScan(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor clr) : LAUMemoryObject(),
    time(QTime::currentTime()), fileString(QString()), makeString(QString()), modelString(QString()),
    serialString(QString()), softwareString(QString()), parentString(QString()), playbackColor(clr),
    xMin(0.0f), xMax(0.0f), yMin(0.0f), yMax(0.0f), zMin(0.0f), zMax(0.0f),
    com(QVector3D(0.0, 0.0, 0.0)), fov(QPointF(0.0, 0.0))
{
    data->numRows = rows;
    data->numCols = cols;
    data->numByts = sizeof(float);
    data->numFrms = 1;

    // SET THE NUMBER OF CHANNELS BASED ON THE DEVICE COLOR SPACE
    switch (playbackColor) {
        case ColorGray:
            data->numChns = 1;
            break;
        case ColorRGB:
        case ColorXYZ:
            data->numChns = 3;
            break;
        case ColorRGBA:
        case ColorXYZG:
        case ColorXYZW:
            data->numChns = 4;
            break;
        case ColorXYZRGB:
            data->numChns = 6;
            break;
        case ColorXYZWRGBA:
            data->numChns = 8;
            break;
        default:
            data->numChns = 0;
    }

    // NOW ALLOCATE SPACE TO HOLD THE SCAN
    data->allocateBuffer();

    // POPULATE FILENAME STRING WITH RANDOM CHARACTERS
    //QByteArray byteArray(30, 0x00);
    //for (int n=0; n<30; n++){
    //    byteArray.data()[n] = (char)(48 + 80 * (float)qrand()/(float)RAND_MAX);
    //}
    //fileString = QString(byteArray);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan::LAUScan(QString filename, int index) : LAUMemoryObject(),
    time(QTime::currentTime()), fileString(QString()), makeString(QString()), modelString(QString()),
    serialString(QString()), softwareString(QString()), parentString(QString()), playbackColor(ColorUndefined),
    xMin(0.0f), xMax(0.0f), yMin(0.0f), yMax(0.0f), zMin(0.0f), zMax(0.0f),
    com(QVector3D(0.0f, 0.0f, 0.0f)), fov(QPointF(0.0f, 0.0f))
{
#ifndef HEADLESS
    // GET A FILE TO OPEN FROM THE USER IF NOT ALREADY PROVIDED ONE
    if (filename.isNull()) {
        QSettings settings;
        QString directory = settings.value("LAUScan::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        filename = QFileDialog::getOpenFileName(0, QString("Load image from disk (*.tif,*.lau)"), directory, QString("*.tif *.tiff *.lau"));
        if (filename.isEmpty() == false) {
            settings.setValue("LAUScan::lastUsedDirectory", QFileInfo(filename).absolutePath());
        } else {
            return;
        }
    }
#endif
    // IF WE HAVE A VALID TIFF FILE, LOAD FROM DISK
    // OTHERWISE TRY TO CONNECT TO SCANNER
    if (QFile::exists(filename)) {
        // OPEN INPUT TIFF FILE FROM DISK
        TIFF *inTiff = TIFFOpen(filename.toLocal8Bit(), "r");
        if (!inTiff) {
            return;
        }
        load(inTiff, index);
        TIFFClose(inTiff);
        if (parentName().isEmpty()) {
            setParentName(fileString);
        }
        fileString = filename;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan::LAUScan(TIFF *inTiff) : LAUMemoryObject(),
    time(QTime::currentTime()), fileString(QString()), makeString(QString()), modelString(QString()),
    serialString(QString()), softwareString(QString()), parentString(QString()), playbackColor(ColorUndefined),
    xMin(0.0f), xMax(0.0f), yMin(0.0f), yMax(0.0f), zMin(0.0f), zMax(0.0f),
    com(QVector3D(0.0, 0.0, 0.0)), fov(QPointF(0.0, 0.0))
{
    load(inTiff);
    if (parentName().isEmpty()) {
        setParentName(fileString);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan::LAUScan(const LAUScan &other) : LAUMemoryObject(other)
{
    time = other.time;
    fileString = other.fileString;
    makeString = other.makeString;
    modelString = other.modelString;
    serialString = other.serialString;
    softwareString = other.softwareString;
    parentString = other.parentString;
    playbackColor = other.playbackColor;

    xMin = other.xMin;
    xMax = other.xMax;
    yMin = other.yMin;
    yMax = other.yMax;
    zMin = other.zMin;
    zMax = other.zMax;
    com = other.com;
    fov = other.fov;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan::LAUScan(const LAUMemoryObject &other, LAUVideoPlaybackColor clr) : LAUMemoryObject(other),
    time(QTime::currentTime()), fileString(QString()), makeString(QString()), modelString(QString()),
    serialString(QString()), softwareString(QString()), parentString(QString()), playbackColor(clr),
    xMin(0.0f), xMax(0.0f), yMin(0.0f), yMax(0.0f), zMin(0.0f), zMax(0.0f),
    com(QVector3D(0.0, 0.0, 0.0)), fov(QPointF(0.0, 0.0))
{
    // UPDATE THE XYZ LIMITS
    updateLimits();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan &LAUScan::operator = (const LAUScan &other)
{
    if (this != &other) {
        *((LAUMemoryObject *)this) = (LAUMemoryObject)other;

        time = other.time;
        fileString = other.fileString;
        makeString = other.makeString;
        modelString = other.modelString;
        serialString = other.serialString;
        softwareString = other.softwareString;
        parentString = other.parentString;
        playbackColor = other.playbackColor;

        xMin = other.xMin;
        xMax = other.xMax;
        yMin = other.yMin;
        yMax = other.yMax;
        zMin = other.zMin;
        zMax = other.zMax;
        com = other.com;
        fov = other.fov;
    }
    return (*this);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan &LAUScan::operator + (const LAUScan &other)
{
    if (this != &other) {
        time = other.time;
        fileString = other.fileString;
        makeString = other.makeString;
        serialString = other.serialString;
        modelString = other.modelString;
        softwareString = other.softwareString;
        parentString = other.parentString;

        xMin = other.xMin;
        xMax = other.xMax;
        yMin = other.yMin;
        yMax = other.yMax;
        zMin = other.zMin;
        zMax = other.zMax;

        com = other.com;
        fov = other.fov;
    }
    return (*this);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUScan::save(QString filename)
{
#ifndef HEADLESS
    QSettings settings;
    QString directory = settings.value(QString("LAUScan::lastUsedDirectory"), QString()).toString();

    if (filename.isNull()) {
        int counter = 0;
        do {
            if (counter == 0) {
                filename = QString("%1/Untitled.tif").arg(directory);
            } else {
                filename = QString("%1/Untitled%2.tif").arg(directory).arg(counter);
            }
            counter++;
        } while (QFile::exists(filename));

        filename = QFileDialog::getSaveFileName(0, QString("Save image to disk (*.tif)"), filename, QString("*.tif;*.tiff"));
        if (!filename.isNull()) {
            settings.setValue(QString("LAUScan::lastUsedDirectory"), QFileInfo(filename).absolutePath());
            if (!filename.toLower().endsWith(QString(".tiff"))) {
                if (!filename.toLower().endsWith(QString(".tif"))) {
                    filename = QString("%1.tif").arg(filename);
                }
            }
        } else {
            return (false);
        }
    }
#endif
    // OPEN TIFF FILE FOR SAVING THE IMAGE
    TIFF *outputTiff = TIFFOpen(filename.toLocal8Bit(), "w8");
    if (!outputTiff) {
        return (false);
    } else if (save(outputTiff)) {
        setFilename(filename);
    }

    // CLOSE TIFF FILE
    TIFFClose(outputTiff);

    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUScan::saveAsUint8(QString filename)
{
#ifndef HEADLESS
    QSettings settings;
    QString directory = settings.value(QString("LAUScan::lastUsedDirectory"), QString()).toString();

    if (filename.isNull()) {
        int counter = 0;
        do {
            if (counter == 0) {
                filename = QString("%1/Untitled.tif").arg(directory);
            } else {
                filename = QString("%1/Untitled%2.tif").arg(directory).arg(counter);
            }
            counter++;
        } while (QFile::exists(filename));

        filename = QFileDialog::getSaveFileName(0, QString("Save image to disk (*.tif)"), filename, QString("*.tif;*.tiff"));
        if (!filename.isNull()) {
            settings.setValue(QString("LAUScan::lastUsedDirectory"), QFileInfo(filename).absolutePath());
            if (!filename.toLower().endsWith(QString(".tiff"))) {
                if (!filename.toLower().endsWith(QString(".tif"))) {
                    filename = QString("%1.tif").arg(filename);
                }
            }
        } else {
            return (false);
        }
    }
#endif
    // OPEN TIFF FILE FOR SAVING THE IMAGE
    TIFF *otTiff = TIFFOpen(filename.toLocal8Bit(), "w");
    if (!otTiff) {
        return (false);
    }

    // CREATE THE XML DATA PACKET USING QT'S XML STREAM OBJECTS
    QByteArray xmlByteArray;
    QBuffer buffer(&xmlByteArray);
    buffer.open(QIODevice::WriteOnly);

    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("scan");

    // WRITE THE COLOR SPACE AND RANGE LIMITS FOR EACH DIMENSION
    switch (playbackColor) {
        case ColorGray:
            writer.writeTextElement("playbackcolor", "ColorGray");
            writer.writeTextElement("minimumvalues", QString("%1").arg(0.0f));
            writer.writeTextElement("maximumvalues", QString("%1").arg(1.0f));
            break;
        case ColorRGB:
            writer.writeTextElement("playbackcolor", "ColorRGB");
            writer.writeTextElement("minimumvalues", QString("%1,%2,%3").arg(0.0f).arg(0.0f).arg(0.0f));
            writer.writeTextElement("maximumvalues", QString("%1,%2,%3").arg(1.0f).arg(1.0f).arg(1.0f));
            break;
        case ColorRGBA:
            writer.writeTextElement("playbackcolor", "ColorRGBA");
            writer.writeTextElement("minimumvalues", QString("%1,%2,%3,%4").arg(0.0f).arg(0.0f).arg(0.0f).arg(0.0f));
            writer.writeTextElement("maximumvalues", QString("%1,%2,%3,%4").arg(1.0f).arg(1.0f).arg(1.0f).arg(1.0f));
            break;
        case ColorXYZ:
            writer.writeTextElement("playbackcolor", "ColorXYZ");
            writer.writeTextElement("minimumvalues", QString("%1,%2,%3").arg(xMin).arg(yMin).arg(zMin));
            writer.writeTextElement("maximumvalues", QString("%1,%2,%3").arg(xMax).arg(yMax).arg(zMax));
            break;
        case ColorXYZG:
            writer.writeTextElement("playbackcolor", "ColorXYZG");
            writer.writeTextElement("minimumvalues", QString("%1,%2,%3,%4").arg(xMin).arg(yMin).arg(zMin).arg(0.0f));
            writer.writeTextElement("maximumvalues", QString("%1,%2,%3,%4").arg(xMax).arg(yMax).arg(zMax).arg(1.0f));
            break;
        case ColorXYZW:
            writer.writeTextElement("playbackcolor", "ColorXYZW");
            writer.writeTextElement("minimumvalues", QString("%1,%2,%3,%4").arg(xMin).arg(yMin).arg(zMin).arg(0.0f));
            writer.writeTextElement("maximumvalues", QString("%1,%2,%3,%4").arg(xMax).arg(yMax).arg(zMax).arg(1.0f));
            break;
        case ColorXYZRGB:
            writer.writeTextElement("playbackcolor", "ColorXYZRGB");
            writer.writeTextElement("minimumvalues", QString("%1,%2,%3,%4,%5,%6").arg(xMin).arg(yMin).arg(zMin).arg(0.0f).arg(0.0f).arg(0.0f));
            writer.writeTextElement("maximumvalues", QString("%1,%2,%3,%4,%5,%6").arg(xMax).arg(yMax).arg(zMax).arg(1.0f).arg(1.0f).arg(1.0f));
            break;
        case ColorXYZWRGBA:
            writer.writeTextElement("playbackcolor", "ColorXYZWRGBA");
            writer.writeTextElement("minimumvalues", QString("%1,%2,%3,%4,%5,%6,%7,%8").arg(xMin).arg(yMin).arg(zMin).arg(0.0f).arg(0.0f).arg(0.0f).arg(0.0f).arg(0.0f));
            writer.writeTextElement("maximumvalues", QString("%1,%2,%3,%4,%5,%6,%7,%8").arg(xMax).arg(yMax).arg(zMax).arg(1.0f).arg(1.0f).arg(1.0f).arg(1.0f).arg(1.0f));
            break;
        case ColorUndefined:
            writer.writeTextElement("playbackcolor", "ColorUndefined");
    }

    // WRITE THE CAMERA FIELD OF VIEW AND CENTER OF MASS
    writer.writeTextElement("fieldofview", QString("%1,%2").arg(fov.x()).arg(fov.y()));
    writer.writeTextElement("centerofmass", QString("%1,%2,%3").arg(com.x()).arg(com.y()).arg(com.z()));

    // WRITE THE TRANSFORM MATRIX OUT TO THE XML FIELD
    QMatrix4x4 mat = this->transform();
    if (mat.isIdentity() == false){
        writer.writeTextElement("transform", QString("A = [ %1, %2, %3, %4; %5, %6, %7, %8; %9, %10, %11, %12; %13, %14, %15, %16 ];").arg(mat(0, 0)).arg(mat(0, 1)).arg(mat(0, 2)).arg(mat(0, 3)).arg(mat(1, 0)).arg(mat(1, 1)).arg(mat(1, 2)).arg(mat(1, 3)).arg(mat(2, 0)).arg(mat(2, 1)).arg(mat(2, 2)).arg(mat(2, 3)).arg(mat(3, 0)).arg(mat(3, 1)).arg(mat(3, 2)).arg(mat(3, 3)));
    }

    // WRITE THE PROJECTION MATRIX OUT TO THE XML FIELD
    mat = this->projection();
    if (mat.isIdentity() == false){
        writer.writeTextElement("projection", QString("B = [ %1, %2, %3, %4; %5, %6, %7, %8; %9, %10, %11, %12; %13, %14, %15, %16 ];").arg(mat(0, 0)).arg(mat(0, 1)).arg(mat(0, 2)).arg(mat(0, 3)).arg(mat(1, 0)).arg(mat(1, 1)).arg(mat(1, 2)).arg(mat(1, 3)).arg(mat(2, 0)).arg(mat(2, 1)).arg(mat(2, 2)).arg(mat(2, 3)).arg(mat(3, 0)).arg(mat(3, 1)).arg(mat(3, 2)).arg(mat(3, 3)));
    }

    // CLOSE OUT THE XML BUFFER
    writer.writeEndElement();
    writer.writeEndDocument();
    buffer.close();

    // EXPORT THE XML BUFFER TO THE XMLPACKET FIELD OF THE TIFF IMAGE
    setXML(xmlByteArray);

    // SAVE CUSTOM FILESTRINGS FROM THE DOCUMENT NAME TIFFTAG
    TIFFSetField(otTiff, TIFFTAG_DOCUMENTNAME, fileString.toLocal8Bit().data());
    TIFFSetField(otTiff, TIFFTAG_PAGENAME, parentString.toLocal8Bit().data());
    TIFFSetField(otTiff, TIFFTAG_CAMERASERIALNUMBER, serialString.toLocal8Bit().data());
    TIFFSetField(otTiff, TIFFTAG_SOFTWARE, softwareString.toLocal8Bit().data());
    TIFFSetField(otTiff, TIFFTAG_MODEL, modelString.toLocal8Bit().data());
    TIFFSetField(otTiff, TIFFTAG_MAKE, makeString.toLocal8Bit().data());

    // WRITE FORMAT PARAMETERS TO CURRENT TIFF DIRECTORY
    TIFFSetField(otTiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
    TIFFSetField(otTiff, TIFFTAG_IMAGEWIDTH, (unsigned long)width());
    TIFFSetField(otTiff, TIFFTAG_IMAGELENGTH, (unsigned long)(height()*frames()));
    TIFFSetField(otTiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    TIFFSetField(otTiff, TIFFTAG_XRESOLUTION, 72.0);
    TIFFSetField(otTiff, TIFFTAG_YRESOLUTION, 72.0);
    TIFFSetField(otTiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(otTiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(otTiff, TIFFTAG_SAMPLESPERPIXEL, (unsigned short)colors());
    TIFFSetField(otTiff, TIFFTAG_BITSPERSAMPLE, (unsigned short)16);
    if (colors() == 1) {
        TIFFSetField(otTiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    } else if (colors() == 3) {
        TIFFSetField(otTiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    } else if (colors() == 4) {
        TIFFSetField(otTiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_SEPARATED);
    } else {
        TIFFSetField(otTiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    }
    TIFFSetField(otTiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    TIFFSetField(otTiff, TIFFTAG_XPOSITION, qMax(0.0f, (float)anchor().x()));
    TIFFSetField(otTiff, TIFFTAG_YPOSITION, qMax(0.0f, (float)anchor().y()));
    TIFFSetField(otTiff, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
    TIFFSetField(otTiff, TIFFTAG_ROWSPERSTRIP, 1);

    // MAKE TEMPORARY BUFFER TO HOLD CURRENT ROW BECAUSE COMPRESSION DESTROYS
    // WHATS EVER INSIDE THE BUFFER
    unsigned short *toBuffer = (unsigned short *)malloc(width() * colors() * sizeof(unsigned short));
    for (unsigned int row = 0; row < height()*frames(); row++) {
        float *fmBuffer = (float *)constScanLine(row);
        unsigned int index = 0;
        for (unsigned int col = 0; col < width(); col++) {
            for (unsigned int chn = 0; chn < colors(); chn++) {
                toBuffer[index] = (unsigned short)qMax(0, qMin(65535, qFloor(fmBuffer[index] * 65535.0f)));
                index++;
            }
        }
        TIFFWriteScanline(otTiff, toBuffer, row, 0);
    }
    free(toBuffer);

    // WRITE THE CURRENT DIRECTORY AND PREPARE FOR THE NEW ONE
    TIFFWriteDirectory(otTiff);

    // WRITE THE ELAPSED TIME STAMP TO AN EXIF TAG
    uint64_t dir_offset;
    TIFFCreateEXIFDirectory(otTiff);
    TIFFSetField(otTiff, EXIFTAG_SUBSECTIME, QString("%1").arg(elapsed()).toLocal8Bit().data());
    TIFFWriteCustomDirectory(otTiff, &dir_offset);

    TIFFSetDirectory(otTiff, 0);
    TIFFSetField(otTiff, TIFFTAG_EXIFIFD, dir_offset);
    TIFFRewriteDirectory(otTiff);

    // CLOSE TIFF FILE
    TIFFClose(otTiff);

    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUScan::save(TIFF *otTiff, int index)
{
    // GRAB THE CURRENT XML FIELD IN HASH TABLE FORM
    QHash<QString, QString> hashTable = xmlToHash(xml());

    // OVERWRITE LIST OF QSTRINGS THAT YOU WANT TO WRITE TO YOUR XML ARRAY
    switch (playbackColor) {
    case ColorGray:
        hashTable["playbackcolor"] = QString("ColorGray");
        hashTable["minimumvalues"] = QString("%1").arg(0.0f);
        hashTable["maximumvalues"] = QString("%1").arg(1.0f);
        break;
    case ColorRGB:
        hashTable["playbackcolor"] = QString("ColorRGB");
        hashTable["minimumvalues"] = QString("%1,%2,%3").arg(0.0f).arg(0.0f).arg(0.0f);
        hashTable["maximumvalues"] = QString("%1,%2,%3").arg(1.0f).arg(1.0f).arg(1.0f);
        break;
    case ColorRGBA:
        hashTable["playbackcolor"] = QString("ColorRGBA");
        hashTable["minimumvalues"] = QString("%1,%2,%3,%4").arg(0.0f).arg(0.0f).arg(0.0f).arg(0.0f);
        hashTable["maximumvalues"] = QString("%1,%2,%3,%4").arg(1.0f).arg(1.0f).arg(1.0f).arg(1.0f);
        break;
    case ColorXYZ:
        hashTable["playbackcolor"] = QString("ColorXYZ");
        hashTable["minimumvalues"] = QString("%1,%2,%3").arg(xMin).arg(yMin).arg(zMin);
        hashTable["maximumvalues"] = QString("%1,%2,%3").arg(xMax).arg(yMax).arg(zMax);
        break;
    case ColorXYZG:
        hashTable["playbackcolor"] = QString("ColorXYZG");
        hashTable["minimumvalues"] = QString("%1,%2,%3,%4").arg(xMin).arg(yMin).arg(zMin).arg(0.0f);
        hashTable["maximumvalues"] = QString("%1,%2,%3,%4").arg(xMax).arg(yMax).arg(zMax).arg(1.0f);
        break;
    case ColorXYZW:
        hashTable["playbackcolor"] = QString("ColorXYZW");
        hashTable["minimumvalues"] = QString("%1,%2,%3,%4").arg(xMin).arg(yMin).arg(zMin).arg(0.0f);
        hashTable["maximumvalues"] = QString("%1,%2,%3,%4").arg(xMax).arg(yMax).arg(zMax).arg(1.0f);
        break;
    case ColorXYZRGB:
        hashTable["playbackcolor"] = QString("ColorXYZRGB");
        hashTable["minimumvalues"] = QString("%1,%2,%3,%4,%5,%6").arg(xMin).arg(yMin).arg(zMin).arg(0.0f).arg(0.0f).arg(0.0f);
        hashTable["maximumvalues"] = QString("%1,%2,%3,%4,%5,%6").arg(xMax).arg(yMax).arg(zMax).arg(1.0f).arg(1.0f).arg(1.0f);
        break;
    case ColorXYZWRGBA:
        hashTable["playbackcolor"] = QString("ColorXYZWRGBA");
        hashTable["minimumvalues"] = QString("%1,%2,%3,%4,%5,%6,%7,%8").arg(xMin).arg(yMin).arg(zMin).arg(0.0f).arg(0.0f).arg(0.0f).arg(0.0f).arg(0.0f);
        hashTable["maximumvalues"] = QString("%1,%2,%3,%4,%5,%6,%7,%8").arg(xMax).arg(yMax).arg(zMax).arg(1.0f).arg(1.0f).arg(1.0f).arg(1.0f).arg(1.0f);
        break;
    case ColorUndefined:
        hashTable["playbackcolor"] = QString("ColorUndefined");
    }

    // WRITE THE CAMERA FIELD OF VIEW AND CENTER OF MASS
    hashTable["fieldofview"] = QString("%1,%2").arg(fov.x()).arg(fov.y());
    hashTable["centerofmass"] = QString("%1,%2,%3").arg(com.x()).arg(com.y()).arg(com.z());

    // WRITE THE TRANSFORM MATRIX OUT TO THE XML FIELD
    QMatrix4x4 mat = this->transform();
    if (mat.isIdentity() == false){
        hashTable["transform"] = QString("A = [ %1, %2, %3, %4; %5, %6, %7, %8; %9, %10, %11, %12; %13, %14, %15, %16 ];").arg(mat(0, 0)).arg(mat(0, 1)).arg(mat(0, 2)).arg(mat(0, 3)).arg(mat(1, 0)).arg(mat(1, 1)).arg(mat(1, 2)).arg(mat(1, 3)).arg(mat(2, 0)).arg(mat(2, 1)).arg(mat(2, 2)).arg(mat(2, 3)).arg(mat(3, 0)).arg(mat(3, 1)).arg(mat(3, 2)).arg(mat(3, 3));
    }

    // WRITE THE TRANSFORM MATRIX OUT TO THE XML FIELD
    mat = this->projection();
    if (mat.isIdentity() == false){
        hashTable["projection"] = QString("B = [ %1, %2, %3, %4; %5, %6, %7, %8; %9, %10, %11, %12; %13, %14, %15, %16 ];").arg(mat(0, 0)).arg(mat(0, 1)).arg(mat(0, 2)).arg(mat(0, 3)).arg(mat(1, 0)).arg(mat(1, 1)).arg(mat(1, 2)).arg(mat(1, 3)).arg(mat(2, 0)).arg(mat(2, 1)).arg(mat(2, 2)).arg(mat(2, 3)).arg(mat(3, 0)).arg(mat(3, 1)).arg(mat(3, 2)).arg(mat(3, 3));
    }

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
    setXML(xmlByteArray);

    // SAVE CUSTOM FILESTRINGS FROM THE DOCUMENT NAME TIFFTAG
    TIFFSetField(otTiff, TIFFTAG_DOCUMENTNAME, fileString.toLocal8Bit().data());
    TIFFSetField(otTiff, TIFFTAG_PAGENAME, parentString.toLocal8Bit().data());
    TIFFSetField(otTiff, TIFFTAG_SOFTWARE, softwareString.toLocal8Bit().data());
    TIFFSetField(otTiff, TIFFTAG_CAMERASERIALNUMBER, serialString.toLocal8Bit().data());
    TIFFSetField(otTiff, TIFFTAG_MODEL, modelString.toLocal8Bit().data());
    TIFFSetField(otTiff, TIFFTAG_MAKE, makeString.toLocal8Bit().data());

    return (LAUMemoryObject::save(otTiff, index));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUVideoPlaybackColor LAUScan::whatColorIsThisTiffFile(QString filename, int frame)
{
    // CREATE A VARIABLE TO HOLD THE RETURN VALUE
    LAUVideoPlaybackColor playbackColor = LAU3DVideoParameters::ColorUndefined;

    // MAKE SURE WE HAVE A VALID FILE TO OPEN
    if (filename.isNull() == false && QFile::exists(filename)) {
        // OPEN INPUT TIFF FILE FROM DISK
        TIFF *inTiff = TIFFOpen(filename.toLatin1(), "r");
        if (inTiff) {
            int numDirectories = TIFFNumberOfDirectories(inTiff);
            if (frame > -1 && frame < numDirectories) {
                // SET THE DIRECTORY TO THE USER SUPPLIED FRAME
                TIFFSetDirectory(inTiff, (unsigned short)frame);

                // LOAD THE XML FIELD OF THE TIFF FILE, IF PROVIDED
                int dataLength;
                char *dataString;
                if (TIFFGetField(inTiff, TIFFTAG_XMLPACKET, &dataLength, &dataString)) {
                    QXmlStreamReader reader(QByteArray(dataString, dataLength));
                    while (!reader.atEnd()) {
                        if (reader.readNext()) {
                            if (reader.name().toString() == "playbackcolor") {
                                QString colorString = reader.readElementText();
                                if (colorString == QString("ColorGray")) {
                                    playbackColor = ColorGray;
                                } else if (colorString == QString("ColorRGB")) {
                                    playbackColor = ColorRGB;
                                } else if (colorString == QString("ColorRGBA")) {
                                    playbackColor = ColorRGBA;
                                } else if (colorString == QString("ColorXYZ")) {
                                    playbackColor = ColorXYZ;
                                } else if (colorString == QString("ColorXYZG")) {
                                    playbackColor = ColorXYZG;
                                } else if (colorString == QString("ColorXYZW")) {
                                    playbackColor = ColorXYZW;
                                } else if (colorString == QString("ColorXYZRGB")) {
                                    playbackColor = ColorXYZRGB;
                                } else if (colorString == QString("ColorXYZWRGBA")) {
                                    playbackColor = ColorXYZWRGBA;
                                } else {
                                    playbackColor = ColorUndefined;
                                }
                            }
                        }
                    }
                    reader.clear();
                }
            }
            TIFFClose(inTiff);
        }
    }
    return (playbackColor);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUScan::load(TIFF *inTiff, int index)
{
    char *dataString;
    QMatrix4x4 transform;
    QMatrix4x4 projection;

    // SEE IF USER WANTS TO LOAD A PARTICULAR DIRECTORY WITHIN A MULTIDIRECTORY TIFF FILE
    if (index > -1) {
        TIFFSetDirectory(inTiff, (unsigned short)index);
    }

    // LOAD A CUSTOM FILESTRING FROM THE DOCUMENT NAME TIFFTAG
    if (TIFFGetField(inTiff, TIFFTAG_DOCUMENTNAME, &dataString)) {
        fileString = QString(QByteArray(dataString));
    }

    if (TIFFGetField(inTiff, TIFFTAG_PAGENAME, &dataString)) {
        if (QString(QByteArray(dataString)) != QString("not defined")) {
            parentString = QString(QByteArray(dataString));
        }
    }

    // MAKE SURE PARENT STRING ISN'T EMPTY AND IF IT IS REPLACE WITH FILESTRING
    if (parentString.isEmpty()) {
        parentString = fileString;
    }

    if (TIFFGetField(inTiff, TIFFTAG_CAMERASERIALNUMBER, &dataString)) {
        serialString = QByteArray(dataString);
    }

    if (TIFFGetField(inTiff, TIFFTAG_SOFTWARE, &dataString)) {
        softwareString = QByteArray(dataString);
    }

    if (TIFFGetField(inTiff, TIFFTAG_MODEL, &dataString)) {
        modelString = QByteArray(dataString);
    }

    if (TIFFGetField(inTiff, TIFFTAG_MAKE, &dataString)) {
        makeString = QByteArray(dataString);
    }

    playbackColor = ColorXYZG;

    // SO FAR SO GOOD, NOW LET'S SEE IF WE CAN
    // LOAD THE MEMORY OBJECT WITHOUT ERROR
    if (LAUMemoryObject::load(inTiff)) {
        // LOAD THE XML FIELD OF THE TIFF FILE, IF PROVIDED
        QByteArray xmlByteArray = xml();

        QHash<QString, QString> hashTable = LAUMemoryObject::xmlToHash(xml());
        if (hashTable["playbackcolor"].isNull() == false){
            QString colorString = hashTable["playbackcolor"];
            if (colorString == QString("ColorGray")) {
                playbackColor = ColorGray;
            } else if (colorString == QString("ColorRGB")) {
                playbackColor = ColorRGB;
            } else if (colorString == QString("ColorRGBA")) {
                playbackColor = ColorRGBA;
            } else if (colorString == QString("ColorXYZ")) {
                playbackColor = ColorXYZ;
            } else if (colorString == QString("ColorXYZG")) {
                playbackColor = ColorXYZG;
            } else if (colorString == QString("ColorXYZW")) {
                playbackColor = ColorXYZW;
            } else if (colorString == QString("ColorXYZRGB")) {
                playbackColor = ColorXYZRGB;
            } else if (colorString == QString("ColorXYZWRGBA")) {
                playbackColor = ColorXYZWRGBA;
            } else {
                playbackColor = ColorUndefined;
            }
        }

        if (hashTable["minimumvalues"].isNull() == false){
            QString rangeString = hashTable["minimumvalues"];
            QStringList floatList = rangeString.split(",");
            if (floatList.count() >= 3) {
                xMin = floatList.at(0).toFloat();
                yMin = floatList.at(1).toFloat();
                zMin = floatList.at(2).toFloat();
            }
        }

        if (hashTable["maximumvalues"].isNull() == false){
            QString rangeString = hashTable["maximumvalues"];
            QStringList floatList = rangeString.split(",");
            if (floatList.count() >= 3) {
                xMax = floatList.at(0).toFloat();
                yMax = floatList.at(1).toFloat();
                zMax = floatList.at(2).toFloat();
            }
        }

        if (hashTable["fieldofview"].isNull() == false){
            QString fovString = hashTable["fieldofview"];
            QStringList floatList = fovString.split(",");
            if (floatList.count() == 2) {
                fov.setX(floatList.at(0).toFloat());
                fov.setY(floatList.at(1).toFloat());
            }
        }

        if (hashTable["centerofmass"].isNull() == false){
            QString fovString = hashTable["centerofmass"];
            QStringList floatList = fovString.split(",");
            if (floatList.count() == 3) {
                com.setX(floatList.at(0).toFloat());
                com.setY(floatList.at(1).toFloat());
                com.setZ(floatList.at(2).toFloat());
            }
        }

        if (hashTable["transform"].isNull() == false){
            QString matrixString = hashTable["transform"];
            matrixString.remove(0, 5);
            matrixString.chop(2);

            QStringList rowStringList = matrixString.split(";");
            QString rowString = rowStringList.takeFirst();
            QStringList floatList = rowString.split(",");

            transform(0, 0) = floatList.at(0).toFloat();
            transform(0, 1) = floatList.at(1).toFloat();
            transform(0, 2) = floatList.at(2).toFloat();
            transform(0, 3) = floatList.at(3).toFloat();

            rowString = rowStringList.takeFirst();
            floatList = rowString.split(",");

            transform(1, 0) = floatList.at(0).toFloat();
            transform(1, 1) = floatList.at(1).toFloat();
            transform(1, 2) = floatList.at(2).toFloat();
            transform(1, 3) = floatList.at(3).toFloat();

            rowString = rowStringList.takeFirst();
            floatList = rowString.split(",");

            transform(2, 0) = floatList.at(0).toFloat();
            transform(2, 1) = floatList.at(1).toFloat();
            transform(2, 2) = floatList.at(2).toFloat();
            transform(2, 3) = floatList.at(3).toFloat();

            rowString = rowStringList.takeFirst();
            floatList = rowString.split(",");

            transform(3, 0) = floatList.at(0).toFloat();
            transform(3, 1) = floatList.at(1).toFloat();
            transform(3, 2) = floatList.at(2).toFloat();
            transform(3, 3) = floatList.at(3).toFloat();
        }

        if (hashTable["projection"].isNull() == false){
            QString matrixString = hashTable["projection"];
            matrixString.remove(0, 5);
            matrixString.chop(2);

            QStringList rowStringList = matrixString.split(";");
            QString rowString = rowStringList.takeFirst();
            QStringList floatList = rowString.split(",");

            projection(0, 0) = floatList.at(0).toFloat();
            projection(0, 1) = floatList.at(1).toFloat();
            projection(0, 2) = floatList.at(2).toFloat();
            projection(0, 3) = floatList.at(3).toFloat();

            rowString = rowStringList.takeFirst();
            floatList = rowString.split(",");

            projection(1, 0) = floatList.at(0).toFloat();
            projection(1, 1) = floatList.at(1).toFloat();
            projection(1, 2) = floatList.at(2).toFloat();
            projection(1, 3) = floatList.at(3).toFloat();

            rowString = rowStringList.takeFirst();
            floatList = rowString.split(",");

            projection(2, 0) = floatList.at(0).toFloat();
            projection(2, 1) = floatList.at(1).toFloat();
            projection(2, 2) = floatList.at(2).toFloat();
            projection(2, 3) = floatList.at(3).toFloat();

            rowString = rowStringList.takeFirst();
            floatList = rowString.split(",");

            projection(3, 0) = floatList.at(0).toFloat();
            projection(3, 1) = floatList.at(1).toFloat();
            projection(3, 2) = floatList.at(2).toFloat();
            projection(3, 3) = floatList.at(3).toFloat();
        }

        // THIS IS NOT A SCAN FILE, SO RETURN FALSE
        if (playbackColor == ColorUndefined) {
            return (false);
        }

        // TEMPORARY FIX FOR AN ERROR IN THE SCANNING CODE
        if (colors() == 1){
            playbackColor = ColorGray;
        }

        // STICK THE TRANSFORM INSIDE THE BUFFER OBJECT
        setConstTransform(transform);
        setConstProjection(projection);

        switch (playbackColor) {
            case ColorGray:
                return (this->colors() == 1);
            case ColorRGB:
                return (this->colors() == 3);
            case ColorRGBA:
                return (this->colors() == 4);
            case ColorXYZ:
                updateLimits();
                return (this->colors() == 3);
            case ColorXYZG:
                updateLimits();
                return (this->colors() == 4);
            case ColorXYZW:
                updateLimits();
                return (this->colors() == 4);
            case ColorXYZRGB:
                updateLimits();
                return (this->colors() == 6);
            case ColorXYZWRGBA:
                updateLimits();
                return (this->colors() == 8);
            case ColorUndefined:
                return (false);
        }
    }

    return (false);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector<float> LAUScan::pixel(QPoint point) const
{
    return (pixel(point.x(), point.y()));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector<float> LAUScan::pixel(int col, int row) const
{
    QVector<float> pix(colors());
    if (col < 0 || col >= (int)width() || row < 0 || row >= (int)height()) {
        memset(pix.data(), 0xff, colors()*sizeof(float));
    } else {
        memcpy(pix.data(), ((float *)constScanLine(row) + colors()*col), colors()*sizeof(float));
    }
    return (pix);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUScan::updateLimits()
{
    if (playbackColor == ColorGray || playbackColor == ColorRGB || playbackColor == ColorRGBA) {
        return;
    }

    __m128 minVec = _mm_set1_ps(1e9f);
    __m128 maxVec = _mm_set1_ps(-1e9f);
    __m128 menVec = _mm_set1_ps(0.0f);

    int index = 0;
    int pixelCount = 0;
    float *buffer = (float *)constScanLine(0);
    if (colors() % 4 == 0) {
        for (unsigned int row = 0; row < height(); row++) {
            for (unsigned int col = 0; col < width(); col++) {
                __m128 pixVec = _mm_load_ps(buffer + index);
                __m128 mskVec = _mm_cmpeq_ps(pixVec, pixVec);
                if (_mm_test_all_ones(_mm_castps_si128(mskVec))) {
                    pixelCount++;
                    minVec = _mm_min_ps(minVec, pixVec);
                    maxVec = _mm_max_ps(maxVec, pixVec);
                    menVec = _mm_add_ps(menVec, pixVec);
                }
                index += colors();
            }
        }
    } else {
        for (unsigned int row = 0; row < height(); row++) {
            for (unsigned int col = 0; col < width(); col++) {
                __m128 pixVec = _mm_loadu_ps(buffer + index);
                __m128 mskVec = _mm_cmpeq_ps(pixVec, pixVec);
                if (_mm_test_all_ones(_mm_castps_si128(mskVec))) {
                    pixelCount++;
                    minVec = _mm_min_ps(minVec, pixVec);
                    maxVec = _mm_max_ps(maxVec, pixVec);
                    menVec = _mm_add_ps(menVec, pixVec);
                }
                index += colors();
            }
        }
    }

    *(int *)&xMin = _mm_extract_ps(minVec, 0);
    *(int *)&xMax = _mm_extract_ps(maxVec, 0);
    *(int *)&yMin = _mm_extract_ps(minVec, 1);
    *(int *)&yMax = _mm_extract_ps(maxVec, 1);
    *(int *)&zMin = _mm_extract_ps(minVec, 2);
    *(int *)&zMax = _mm_extract_ps(maxVec, 2);

    float val = 0.0;
    *(int *)&val = _mm_extract_ps(menVec, 0);
    com.setX(val / (float)pixelCount);
    *(int *)&val = _mm_extract_ps(menVec, 1);
    com.setY(val / (float)pixelCount);
    *(int *)&val = _mm_extract_ps(menVec, 2);
    com.setZ(val / (float)pixelCount);

    if (qFabs(zMin) > qFabs(zMax)) {
        float z = zMin;
        zMin = zMax;
        zMax = z;
    }

    if (zMin == 0.0f) {
        zMin = 1.0f;
    }

    // CALCULATE THE FIELD OF VIEW OF THE BOUNDING BOX FROM THE ORIGIN
    float deltaX = qMax(xMin, xMax) - qMin(xMin, xMax);
    float deltaY = qMax(yMin, yMax) - qMin(yMin, yMax);
    fov = QPointF(2.0f * qAtan2(deltaX / 2.0f, qFabs(zMax)), 2.0f * qAtan2(deltaY / 2.0f, qFabs(zMax)));

    return;
}

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QImage> LAUScan::nearestNeighborMap()
{
    QList<QImage> images;

    QImage imageA(941, 941, QImage::Format_ARGB32);
    imageA.fill(Qt::black);

    updateLimits();
    float xn = qMin(minX(), maxX());
    float xx = qMax(minX(), maxX()) - xn;
    float yn = qMin(minY(), maxY());
    float yx = qMax(minY(), maxY()) - yn;
    float zn = qMin(minZ(), maxZ());
    float zx = qMax(minZ(), maxZ()) - zn;

    for (unsigned int row = 0; row < height(); row++) {
        for (unsigned int col = 0; col < width(); col++) {
            QVector<float> vector = pixel(col, row);
            if (!qIsNaN(vector[0]) && !qIsNaN(vector[1]) && !qIsNaN(vector[2])) {
                int xi = qRound((vector[0] - xn) / xx * 95);
                int yi = qRound((vector[1] - yn) / yx * 95);
                int zi = qRound((vector[2] - zn) / zx * 95);

                int index = zi * 96 * 96 + yi * 96 + xi;
                imageA.setPixel(index % 941, index / 941, qRgb(255, 255, 255));
            }
        }
    }
    images << imageA;

    QImage imageB(182, 182, QImage::Format_ARGB32);
    imageB.fill(Qt::black);
    for (unsigned int chn = 0; chn < 96; chn++) {
        for (unsigned int row = 0; row < 96; row++) {
            for (unsigned int col = 0; col < width(); col++) {
                ;
            }
        }
    }
    images << imageB;

    return (images);
}
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::mergeScans(QList<LAUScan> scans)
{
    if (scans.count() == 0) {
        return (LAUScan());
    }

    // KEEP A COPY OF THE NUMBER OF SCANS IN THE LIST
    int N = scans.count();

    // CREATE A COPY OF THE FIRST SCAN IN THE LIST
    LAUScan scan = scans.takeFirst();

    // NOW ITERATE THROUGH ENTIRE LIST MAKING SURE THEY ARE ALL THE SAME SIZE AND COLOR
    while (scans.count() > 0) {
        LAUScan scnp = scans.takeFirst();

        // MAKE SURE NEW SCAN IS SAME COLOR AS REFERENCE SCAN
        if (scnp.color() != scan.color()) {
            scnp = scnp.convertToColor(scan.color());
        }
        // MAKE SURE NEW SCAN IS SAME SIZE AND REFERENCE SCAN
        if (scnp.width() != scan.width() || scnp.height() != scan.height()) {
            scnp = scnp.resize(scan.width(), scan.height());
        }
        // ADD NEW SCAN'S BUFFER TO EXIST BUFFER
        for (unsigned int i = 0; i < scan.length(); i += 16) {
            // LOAD THE NEXT 16 BYTES OF DATA FROM THE TWO BUFFERS
            __m128 inVector = _mm_load_ps((float *)(scan.constPointer() + i));
            __m128 otVector = _mm_load_ps((float *)(scnp.constPointer() + i));

            // SAVE THE SUM OF THE TWO VECTORS BACK INTO THE REFERENCE BUFFER
            _mm_store_ps((float *)(scan.constPointer() + i), _mm_add_ps(inVector, otVector));
        }
    }

    // NOW WE NEED TO DIVIDE THE BUFFER BY THE NUMBER OF IMAGES
    __m128 otVector = _mm_set1_ps(1.0f / (float)N);
    for (unsigned int i = 0; i < scan.length(); i += 16) {
        // LOAD THE NEXT 16 BYTES OF DATA FROM THE TWO BUFFERS
        __m128 inVector = _mm_load_ps((float *)(scan.constPointer() + i));

        // SAVE THE SUM OF THE TWO VECTORS BACK INTO THE REFERENCE BUFFER
        _mm_store_ps((float *)(scan.constPointer() + i), _mm_mul_ps(inVector, otVector));
    }
    return (scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::maskChannel(unsigned int chn, float threshold) const
{
    LAUScan image = LAUScan(width(), height(), color()) + *this;

    unsigned int channels = colors();
    for (unsigned int row = 0; row < height(); row++) {
        float *fmBuffer = (float *)constScanLine(row);
        float *toBuffer = (float *)image.scanLine(row);
        for (unsigned int col = 0; col < width(); col++) {
            if (fmBuffer[col * channels + chn] > threshold) {
                memcpy(&toBuffer[col * channels], &fmBuffer[col * channels], channels * sizeof(float));
            } else {
                memset(&toBuffer[col * channels], 0xFF, channels * sizeof(float));
            }
        }
    }
    return (image);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::flipLeftRight()
{
    // MAKE A NEW IMAGE TO HOLD THE FLIPPED IMAGE
    LAUScan image = LAUScan(width(), height(), color()) + *this;

    // FLIP THE IMAGE ROW BY ROW
    int chunk = image.colors() * image.depth();
    for (unsigned int row = 0; row < image.height(); row++) {
        unsigned char *fmBuffer = this->constScanLine(row);
        unsigned char *toBuffer = image.constScanLine(row);
        for (unsigned int col = 0; col < image.width(); col++) {
            memcpy(&toBuffer[col * chunk], &fmBuffer[(image.width() - col - 1) * chunk], chunk);
        }
    }

    // RETURN THE FLIPPED IMAGE TO THE USER
    return (image);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
const QVector3D LAUScan::center()
{
    QVector<float> bounds = boundingBox();
    return (QVector3D(bounds[0] + bounds[1], bounds[2] + bounds[3], bounds[4] + bounds[5]) / 2.0f);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
const QVector<float> LAUScan::boundingBox()
{
    QVector<float> bounds(6, NAN);

    // DERIVE THE TRANSFORM FROM SCAN SPACE TO VOXEL MAP SPACE
    bounds[0] = qMin(maxX(), minX());
    bounds[1] = qMax(maxX(), minX());
    bounds[2] = qMin(maxY(), minY());
    bounds[3] = qMax(maxY(), minY());
    bounds[4] = qMin(maxZ(), minZ());
    bounds[5] = qMax(maxZ(), minZ());

    return (bounds);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUScan::channelsToFrames() const
{
    LAUMemoryObject image = LAUMemoryObject(width(), height(), 1, sizeof(float), colors());

    for (unsigned int clr = 0; clr < colors(); clr++){
        for (unsigned int row = 0; row < height(); row++){
            float *fmBuffer = (float*)constScanLine(row);
            float *toBuffer = (float*)image.constScanLine(row, clr);
            for (unsigned int col = 0; col < width(); col++){
                toBuffer[col] = fmBuffer[col * colors() + clr];
            }
        }
    }
    return(image);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::extractChannel(unsigned int channel) const
{
    LAUScan image = LAUScan(width(), height(), ColorGray) + *this;

    for (unsigned int row = 0; row < height(); row++) {
        float *toBuffer = (float *)constScanLine(row);
        float *inBuffer = (float *)image.scanLine(row);
        for (unsigned int col = 0; col < width(); col++) {
            inBuffer[col] = toBuffer[col * colors() + channel];
        }
    }
    return (image);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::crop(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    if ((x + w) > width()) {
        w = width() - x;
    }
    if ((y + h) > height()) {
        h = height() - y;
    }

    LAUScan image = LAUScan(w, h, playbackColor) + *this;

    for (unsigned int r = 0; r < image.height(); r++) {
        float *toBuffer = (float *)image.scanLine(r);
        float *inBuffer = (float *) & (((float *)constScanLine(y + r))[colors() * x]);
        memcpy(toBuffer, inBuffer, w * colors()*sizeof(float));
    }

    return (image);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::rotate()
{
    // CREATE AN IMAGE TO HOLD THE OUTPUT SCAN
    return (LAUScan(((LAUMemoryObject *)this)->rotate(), color()) + *this);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::resize(unsigned int cols, unsigned int rows)
{
    // CREATE AN IMAGE TO HOLD THE OUTPUT SCAN
    LAUScan image = LAUScan(cols, rows, color()) + *this;

    // ITERATE THROUGH EACH ROW OF THE OUTPUT IMAGE
    for (unsigned int r = 0; r < rows; r++) {
        // CALCULATE THE INTERPOLATED ROW IN THE INPUT BUFFER
        int row = qFloor((double)r / (double)rows * (double)this->height());

        // GET POINTERS TO THE SCAN ROWS FROM THE INPUT AND OUTPUT SCANS
        float *inBuffer = (float *)constScanLine(row);
        float *ogBuffer = (float *)image.scanLine(r);

        // ITERATE ACROSS A ROW OF PIXELS
        for (unsigned int c = 0; c < cols; c++) {
            int col = qFloor((double)c / (double)cols * (double)this->width());

            // COPY A COMPLETE PIXEL FROM THE IN BUFFER TO THE OUT BUFFER
            memcpy(&ogBuffer[c * colors()], &inBuffer[col * colors()], nugget());
        }
    }
    return (image);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAUScan::extractXYZWVertices(float *toBuffer, int downSampleFactor) const
{
    // CREATE POINTER TO OUTPUT BUFFER TO KEEP TRACK OF NUMBER OF VALID POINTS
    int indexOt = 0;

    // CREATE A VECTOR TO HOLD ONES THAT WE CAN INSERT INTO XYZW VECTOR
    if (toBuffer) {
        if (color() == ColorGray) {
            return (0);
        } else if (color() == ColorRGB) {
            return (0);
        } else if (color() == ColorRGBA) {
            return (0);
        } else if (color() == ColorXYZ) {
            __m128 mskVec = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
            __m128 oneVec = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
            for (unsigned int row = 0; row < height(); row += downSampleFactor) {
                float *inBuffer = (float *)constScanLine(row);
                for (unsigned int col = 0; col < width(); col += downSampleFactor) {
                    // GRAB COPY OF CURRENT PIXEL'S XYZW COORDINATES
                    __m128 vecIn = _mm_loadu_ps(inBuffer + 3 * col);

                    // INSERT A ONE IN THE W COORDINATE
                    vecIn = _mm_add_ps(oneVec, _mm_and_ps(vecIn, mskVec));

                    // TEST TO SEE IF ANY ELEMENTS IN THE INCOMING VECTOR ARE NANS
                    // AND IF NOT THEN COPY THE VALID PIXEL INTO THE OUTPUT BUFFER
                    if (_mm_test_all_ones(_mm_castps_si128(_mm_cmpeq_ps(vecIn, vecIn)))) {
                        _mm_store_ps(toBuffer + 4 * (indexOt++), vecIn);
                    }
                }
            }
        } else if (color() == ColorXYZW) {
            for (unsigned int row = 0; row < height(); row += downSampleFactor) {
                float *inBuffer = (float *)constScanLine(row);
                for (unsigned int col = 0; col < width(); col += downSampleFactor) {
                    // GRAB COPY OF CURRENT PIXEL'S XYZW COORDINATES
                    __m128 vecIn = _mm_load_ps(inBuffer + 4 * col);

                    // TEST TO SEE IF ANY ELEMENTS IN THE INCOMING VECTOR ARE NANS
                    // AND IF NOT THEN COPY THE VALID PIXEL INTO THE OUTPUT BUFFER
                    if (_mm_test_all_ones(_mm_castps_si128(_mm_cmpeq_ps(vecIn, vecIn)))) {
                        _mm_store_ps(toBuffer + 4 * (indexOt++), vecIn);
                    }
                }
            }
        } else if (color() == ColorXYZG) {
            __m128 mskVec = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
            __m128 oneVec = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
            for (unsigned int row = 0; row < height(); row += downSampleFactor) {
                float *fmBuffer = (float *)constScanLine(row);
                for (unsigned int col = 0; col < width(); col += downSampleFactor) {
                    // GRAB COPY OF CURRENT PIXEL'S XYZW COORDINATES
                    __m128 vecIn = _mm_load_ps(fmBuffer + 4 * col);

                    // TEST TO SEE IF ANY ELEMENTS IN THE INCOMING VECTOR ARE NANS
                    // AND IF NOT THEN COPY THE VALID PIXEL INTO THE OUTPUT BUFFER
                    if (_mm_test_all_ones(_mm_castps_si128(_mm_cmpeq_ps(vecIn, vecIn)))) {
                        // INSERT A ONE IN THE W COORDINATE AND STORE
                        _mm_store_ps(toBuffer + 4 * (indexOt++), _mm_add_ps(oneVec, _mm_and_ps(vecIn, mskVec)));
                    }
                }
            }
        } else if (color() == ColorXYZRGB) {
            __m128 mskVec = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
            __m128 oneVec = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
            for (unsigned int row = 0; row < height(); row += downSampleFactor) {
                float *inBuffer = (float *)constScanLine(row);
                for (unsigned int col = 0; col < width(); col += downSampleFactor) {
                    // GRAB COPY OF CURRENT PIXEL'S XYZW COORDINATES
                    __m128 vecIn = _mm_loadu_ps(inBuffer + 6 * col);

                    // TEST TO SEE IF ANY ELEMENTS IN THE INCOMING VECTOR ARE NANS
                    // AND IF NOT THEN COPY THE VALID PIXEL INTO THE OUTPUT BUFFER
                    if (_mm_test_all_ones(_mm_castps_si128(_mm_cmpeq_ps(vecIn, vecIn)))) {
                        // INSERT A ONE IN THE W COORDINATE AND STORE
                        _mm_store_ps(toBuffer + 4 * (indexOt++), _mm_add_ps(oneVec, _mm_and_ps(vecIn, mskVec)));
                    }
                }
            }
        } else if (color() == ColorXYZWRGBA) {
            __m128 mskVec = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
            __m128 oneVec = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
            for (unsigned int row = 0; row < height(); row += downSampleFactor) {
                float *inBuffer = (float *)constScanLine(row);
                for (unsigned int col = 0; col < width(); col += downSampleFactor) {
                    // GRAB COPY OF CURRENT PIXEL'S XYZW COORDINATES
                    __m128 vecIn = _mm_load_ps(inBuffer + 8 * col);

                    // TEST TO SEE IF ANY ELEMENTS IN THE INCOMING VECTOR ARE NANS
                    // AND IF NOT THEN COPY THE VALID PIXEL INTO THE OUTPUT BUFFER
                    if (_mm_test_all_ones(_mm_castps_si128(_mm_cmpeq_ps(vecIn, vecIn)))) {
                        _mm_store_ps(toBuffer + 4 * (indexOt++), _mm_add_ps(oneVec, _mm_and_ps(vecIn, mskVec)));
                    }
                }
            }
        }
    }
    // RETURN THE NUMBER OF VALID POINTS COPIED INTO THE OUTPUT BUFFER
    return (indexOt);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAUScan::pointCount() const
{
    // CREATE POINTER TO OUTPUT BUFFER TO KEEP TRACK OF NUMBER OF VALID POINTS
    int indexOt = 0;

    // CREATE A VECTOR TO HOLD ONES THAT WE CAN INSERT INTO XYZW VECTOR
    if (color() == ColorGray) {
        return (0);
    } else if (color() == ColorRGB) {
        return (0);
    } else if (color() == ColorRGBA) {
        return (0);
    } else if (color() == ColorXYZ) {
        __m128 mskVec = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
        __m128 oneVec = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
        for (unsigned int row = 0; row < height(); row++) {
            float *inBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < width(); col++) {
                // GRAB COPY OF CURRENT PIXEL'S XYZW COORDINATES
                __m128 vecIn = _mm_loadu_ps(inBuffer + 3 * col);

                // INSERT A ONE IN THE W COORDINATE
                vecIn = _mm_add_ps(oneVec, _mm_and_ps(vecIn, mskVec));

                // TEST TO SEE IF ANY ELEMENTS IN THE INCOMING VECTOR ARE NANS
                // AND IF NOT THEN COPY THE VALID PIXEL INTO THE OUTPUT BUFFER
                if (_mm_test_all_ones(_mm_castps_si128(_mm_cmpeq_ps(vecIn, vecIn)))) {
                    indexOt++;
                }
            }
        }
    } else if (color() == ColorXYZW) {
        for (unsigned int row = 0; row < height(); row++) {
            float *inBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < width(); col++) {
                // GRAB COPY OF CURRENT PIXEL'S XYZW COORDINATES
                __m128 vecIn = _mm_load_ps(inBuffer + 4 * col);

                // TEST TO SEE IF ANY ELEMENTS IN THE INCOMING VECTOR ARE NANS
                // AND IF NOT THEN COPY THE VALID PIXEL INTO THE OUTPUT BUFFER
                if (_mm_test_all_ones(_mm_castps_si128(_mm_cmpeq_ps(vecIn, vecIn)))) {
                    indexOt++;
                }
            }
        }
    } else if (color() == ColorXYZG) {
        for (unsigned int row = 0; row < height(); row++) {
            float *fmBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < width(); col++) {
                // GRAB COPY OF CURRENT PIXEL'S XYZW COORDINATES
                __m128 vecIn = _mm_load_ps(fmBuffer + 4 * col);

                // TEST TO SEE IF ANY ELEMENTS IN THE INCOMING VECTOR ARE NANS
                // AND IF NOT THEN COPY THE VALID PIXEL INTO THE OUTPUT BUFFER
                if (_mm_test_all_ones(_mm_castps_si128(_mm_cmpeq_ps(vecIn, vecIn)))) {
                    // INSERT A ONE IN THE W COORDINATE AND STORE
                    indexOt++;
                }
            }
        }
    } else if (color() == ColorXYZRGB) {
        for (unsigned int row = 0; row < height(); row++) {
            float *inBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < width(); col++) {
                // GRAB COPY OF CURRENT PIXEL'S XYZW COORDINATES
                __m128 vecIn = _mm_loadu_ps(inBuffer + 6 * col);

                // TEST TO SEE IF ANY ELEMENTS IN THE INCOMING VECTOR ARE NANS
                // AND IF NOT THEN COPY THE VALID PIXEL INTO THE OUTPUT BUFFER
                if (_mm_test_all_ones(_mm_castps_si128(_mm_cmpeq_ps(vecIn, vecIn)))) {
                    // INSERT A ONE IN THE W COORDINATE AND STORE
                    indexOt++;
                }
            }
        }
    } else if (color() == ColorXYZWRGBA) {
        for (unsigned int row = 0; row < height(); row++) {
            float *inBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < width(); col++) {
                // GRAB COPY OF CURRENT PIXEL'S XYZW COORDINATES
                __m128 vecIn = _mm_load_ps(inBuffer + 8 * col);

                // TEST TO SEE IF ANY ELEMENTS IN THE INCOMING VECTOR ARE NANS
                // AND IF NOT THEN COPY THE VALID PIXEL INTO THE OUTPUT BUFFER
                if (_mm_test_all_ones(_mm_castps_si128(_mm_cmpeq_ps(vecIn, vecIn)))) {
                    indexOt++;
                }
            }
        }
    }
    // RETURN THE NUMBER OF VALID POINTS COPIED INTO THE OUTPUT BUFFER
    return (indexOt);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::convertToColor(LAUVideoPlaybackColor clr) const
{
    LAUScan image = LAUScan(width(), height(), clr) + *this;

    if (playbackColor == ColorGray) {
        if (clr == ColorGray) {
            memcpy(image.scanLine(0), constScanLine(0), image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[3 * col + 0] = ((float *)(constScanLine(row)))[col];
                    ((float *)(image.scanLine(row)))[3 * col + 1] = ((float *)(constScanLine(row)))[col];
                    ((float *)(image.scanLine(row)))[3 * col + 2] = ((float *)(constScanLine(row)))[col];
                }
            }
        } else if (clr == ColorRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[col];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[col];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[col];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = 1.0f;
                }
            }
        } else if (clr == ColorXYZ) {
            memset(image.scanLine(0), 0xff, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZW) {
            memset(image.scanLine(0), 0xff, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZG) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = (float)col / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[4 * col + 1] = (float)row / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[4 * col + 2] = -100.0f;
                    ((float *)(image.scanLine(row)))[4 * col + 3] = ((float *)(constScanLine(row)))[col];
                }
            }
        } else if (clr == ColorXYZRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[6 * col + 0] = (float)col / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[6 * col + 1] = (float)row / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[6 * col + 2] = -100.0f;
                    ((float *)(image.scanLine(row)))[6 * col + 3] = ((float *)(constScanLine(row)))[col];
                    ((float *)(image.scanLine(row)))[6 * col + 4] = ((float *)(constScanLine(row)))[col];
                    ((float *)(image.scanLine(row)))[6 * col + 5] = ((float *)(constScanLine(row)))[col];
                }
            }
        } else if (clr == ColorXYZWRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[8 * col + 0] = (float)col / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[8 * col + 1] = (float)row / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[8 * col + 2] = -100.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 3] = 1.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 4] = ((float *)(constScanLine(row)))[col];
                    ((float *)(image.scanLine(row)))[8 * col + 5] = ((float *)(constScanLine(row)))[col];
                    ((float *)(image.scanLine(row)))[8 * col + 6] = ((float *)(constScanLine(row)))[col];
                    ((float *)(image.scanLine(row)))[8 * col + 7] = 1.0f;
                }
            }
        }
    } else if (playbackColor == ColorRGB) {
        if (clr == ColorGray) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[col] = 0.21 * ((float *)(constScanLine(row)))[3 * col + 0] + 0.72 * ((float *)(constScanLine(row)))[3 * col + 1] + 0.07 * ((float *)(constScanLine(row)))[3 * col + 2];
                }
            }
        } else if (clr == ColorRGB) {
            memcpy(image.scanLine(0), constScanLine(0), image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[3 * col + 0];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[3 * col + 1];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[3 * col + 2];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = 1.0f;
                }
            }
        } else if (clr == ColorXYZ) {
            memset(image.scanLine(0), 0xff, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZW) {
            memset(image.scanLine(0), 0xff, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZG) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = (float)col / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[4 * col + 1] = (float)row / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[4 * col + 2] = -100.0f;
                    ((float *)(image.scanLine(row)))[4 * col + 3] = 0.21 * ((float *)(constScanLine(row)))[3 * col + 0] + 0.72 * ((float *)(constScanLine(row)))[3 * col + 1] + 0.07 * ((float *)(constScanLine(row)))[3 * col + 2];
                }
            }
        } else if (clr == ColorXYZRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[6 * col + 0] = (float)col / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[6 * col + 1] = (float)row / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[6 * col + 2] = -100.0f;
                    ((float *)(image.scanLine(row)))[6 * col + 3] = ((float *)(constScanLine(row)))[3 * col + 0];
                    ((float *)(image.scanLine(row)))[6 * col + 4] = ((float *)(constScanLine(row)))[3 * col + 1];
                    ((float *)(image.scanLine(row)))[6 * col + 5] = ((float *)(constScanLine(row)))[3 * col + 2];
                }
            }
        } else if (clr == ColorXYZWRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[8 * col + 0] = (float)col / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[8 * col + 1] = (float)row / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[8 * col + 2] = -100.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 3] = 1.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 4] = ((float *)(constScanLine(row)))[3 * col + 0];
                    ((float *)(image.scanLine(row)))[8 * col + 5] = ((float *)(constScanLine(row)))[3 * col + 1];
                    ((float *)(image.scanLine(row)))[8 * col + 6] = ((float *)(constScanLine(row)))[3 * col + 2];
                    ((float *)(image.scanLine(row)))[8 * col + 7] = 1.0f;
                }
            }
        }
    } else if (playbackColor == ColorRGBA) {
        if (clr == ColorGray) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[col] = (0.21 * ((float *)(constScanLine(row)))[4 * col + 0] + 0.72 * ((float *)(constScanLine(row)))[4 * col + 1] + 0.07 * ((float *)(constScanLine(row)))[4 * col + 2]) * ((float *)(constScanLine(row)))[4 * col + 3];
                }
            }
        } else if (clr == ColorRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[3 * col + 0] = ((float *)(constScanLine(row)))[4 * col + 0] * ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[3 * col + 1] = ((float *)(constScanLine(row)))[4 * col + 1] * ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[3 * col + 2] = ((float *)(constScanLine(row)))[4 * col + 2] * ((float *)(constScanLine(row)))[4 * col + 3];
                }
            }
        } else if (clr == ColorRGBA) {
            memcpy(image.scanLine(0), constScanLine(0), image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZ) {
            memset(image.scanLine(0), 0xff, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZW) {
            memset(image.scanLine(0), 0xff, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZG) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = (float)col / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[4 * col + 1] = (float)row / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[4 * col + 2] = -100.0f;
                    ((float *)(image.scanLine(row)))[4 * col + 3] = (0.21 * ((float *)(constScanLine(row)))[4 * col + 0] + 0.72 * ((float *)(constScanLine(row)))[4 * col + 1] + 0.07 * ((float *)(constScanLine(row)))[4 * col + 2]) * ((float *)(constScanLine(row)))[4 * col + 3];
                }
            }
        } else if (clr == ColorXYZRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[6 * col + 0] = (float)col / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[6 * col + 1] = (float)row / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[6 * col + 2] = -100.0f;
                    ((float *)(image.scanLine(row)))[6 * col + 3] = ((float *)(constScanLine(row)))[4 * col + 0] * ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[6 * col + 4] = ((float *)(constScanLine(row)))[4 * col + 1] * ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[6 * col + 5] = ((float *)(constScanLine(row)))[4 * col + 2] * ((float *)(constScanLine(row)))[4 * col + 3];
                }
            }
        } else if (clr == ColorXYZWRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[8 * col + 0] = (float)col / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[8 * col + 1] = (float)row / (float)qMax(image.width(), image.height()) - 0.5f;
                    ((float *)(image.scanLine(row)))[8 * col + 2] = -100.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 3] = 1.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 4] = ((float *)(constScanLine(row)))[4 * col + 0];
                    ((float *)(image.scanLine(row)))[8 * col + 5] = ((float *)(constScanLine(row)))[4 * col + 1];
                    ((float *)(image.scanLine(row)))[8 * col + 6] = ((float *)(constScanLine(row)))[4 * col + 2];
                    ((float *)(image.scanLine(row)))[8 * col + 7] = ((float *)(constScanLine(row)))[4 * col + 3];
                }
            }
        }
    } else if (playbackColor == ColorXYZ) {
        if (clr == ColorGray) {
            memset(image.scanLine(0), 0, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorRGB) {
            memset(image.scanLine(0), 0, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorRGBA) {
            memset(image.scanLine(0), 0, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZ) {
            memcpy(image.scanLine(0), constScanLine(0), image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZW) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[3 * col + 0];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[3 * col + 1];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[3 * col + 2];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = 1.0f - (float)qIsNaN(((float *)(constScanLine(row)))[3 * col + 0]);
                }
            }
        } else if (clr == ColorXYZG) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[3 * col + 0];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[3 * col + 1];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[3 * col + 2];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = 0.0f;
                }
            }
        } else if (clr == ColorXYZRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[6 * col + 0] = ((float *)(constScanLine(row)))[3 * col + 0];
                    ((float *)(image.scanLine(row)))[6 * col + 1] = ((float *)(constScanLine(row)))[3 * col + 1];
                    ((float *)(image.scanLine(row)))[6 * col + 2] = ((float *)(constScanLine(row)))[3 * col + 2];
                    ((float *)(image.scanLine(row)))[6 * col + 3] = 0.0f;
                    ((float *)(image.scanLine(row)))[6 * col + 4] = 0.0f;
                    ((float *)(image.scanLine(row)))[6 * col + 5] = 0.0f;
                }
            }
        } else if (clr == ColorXYZWRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[8 * col + 0] = ((float *)(constScanLine(row)))[3 * col + 0];
                    ((float *)(image.scanLine(row)))[8 * col + 1] = ((float *)(constScanLine(row)))[3 * col + 1];
                    ((float *)(image.scanLine(row)))[8 * col + 2] = ((float *)(constScanLine(row)))[3 * col + 2];
                    ((float *)(image.scanLine(row)))[8 * col + 3] = 1.0f - (float)qIsNaN(((float *)(constScanLine(row)))[3 * col + 0]);
                    ((float *)(image.scanLine(row)))[8 * col + 4] = 0.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 5] = 0.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 6] = 0.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 7] = 1.0f;
                }
            }
        }
    } else if (playbackColor == ColorXYZW) {
        if (clr == ColorGray) {
            memset(image.scanLine(0), 0, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorRGB) {
            memset(image.scanLine(0), 0, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorRGBA) {
            memset(image.scanLine(0), 0, image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZ) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[3 * col + 0] = ((float *)(constScanLine(row)))[4 * col + 0] / ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[3 * col + 1] = ((float *)(constScanLine(row)))[4 * col + 1] / ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[3 * col + 2] = ((float *)(constScanLine(row)))[4 * col + 2] / ((float *)(constScanLine(row)))[4 * col + 3];
                }
            }
        } else if (clr == ColorXYZW) {
            memcpy(image.scanLine(0), constScanLine(0), image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZG) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[4 * col + 0] / ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[4 * col + 1] / ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[4 * col + 2] / ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = 0.0f;
                }
            }
        } else if (clr == ColorXYZRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[6 * col + 0] = ((float *)(constScanLine(row)))[4 * col + 0] / ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[6 * col + 1] = ((float *)(constScanLine(row)))[4 * col + 1] / ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[6 * col + 2] = ((float *)(constScanLine(row)))[4 * col + 2] / ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[6 * col + 3] = 0.0f;
                    ((float *)(image.scanLine(row)))[6 * col + 4] = 0.0f;
                    ((float *)(image.scanLine(row)))[6 * col + 5] = 0.0f;
                }
            }
        } else if (clr == ColorXYZWRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[8 * col + 0] = ((float *)(constScanLine(row)))[4 * col + 0];
                    ((float *)(image.scanLine(row)))[8 * col + 1] = ((float *)(constScanLine(row)))[4 * col + 1];
                    ((float *)(image.scanLine(row)))[8 * col + 2] = ((float *)(constScanLine(row)))[4 * col + 2];
                    ((float *)(image.scanLine(row)))[8 * col + 3] = ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[8 * col + 4] = 0.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 5] = 0.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 6] = 0.0f;
                    ((float *)(image.scanLine(row)))[8 * col + 7] = 1.0f;
                }
            }
        }
    } else if (playbackColor == ColorXYZG) {
        if (clr == ColorGray) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[col] = ((float *)(constScanLine(row)))[4 * col + 3];
                }
            }
        } else if (clr == ColorRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    float red = ((float *)(constScanLine(row)))[3 * col + 0];
                    float grn = ((float *)(constScanLine(row)))[3 * col + 1];
                    float blu = ((float *)(constScanLine(row)))[3 * col + 2];

                    ((float *)(image.scanLine(row)))[col] = 0.2126 * red + 0.7152 * grn + 0.0722 * blu;
                }
            }
        } else if (clr == ColorRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    float red = ((float *)(constScanLine(row)))[4 * col + 0];
                    float grn = ((float *)(constScanLine(row)))[4 * col + 1];
                    float blu = ((float *)(constScanLine(row)))[4 * col + 2];
                    float alp = ((float *)(constScanLine(row)))[4 * col + 3];

                    ((float *)(image.scanLine(row)))[col] = (0.2126 * red + 0.7152 * grn + 0.0722 * blu) / alp;
                }
            }
        } else if (clr == ColorXYZ) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[3 * col + 0] = ((float *)(constScanLine(row)))[4 * col + 0];
                    ((float *)(image.scanLine(row)))[3 * col + 1] = ((float *)(constScanLine(row)))[4 * col + 1];
                    ((float *)(image.scanLine(row)))[3 * col + 2] = ((float *)(constScanLine(row)))[4 * col + 2];
                }
            }
        } else if (clr == ColorXYZW) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[4 * col + 0];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[4 * col + 1];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[4 * col + 2];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = 1.0f - (float)qIsNaN(((float *)(constScanLine(row)))[4 * col + 0]);;
                }
            }
        } else if (clr == ColorXYZG) {
            memcpy(image.scanLine(0), constScanLine(0), image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[6 * col + 0] = ((float *)(constScanLine(row)))[4 * col + 0];
                    ((float *)(image.scanLine(row)))[6 * col + 1] = ((float *)(constScanLine(row)))[4 * col + 1];
                    ((float *)(image.scanLine(row)))[6 * col + 2] = ((float *)(constScanLine(row)))[4 * col + 2];
                    ((float *)(image.scanLine(row)))[6 * col + 3] = ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[6 * col + 4] = ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[6 * col + 5] = ((float *)(constScanLine(row)))[4 * col + 3];
                }
            }
        } else if (clr == ColorXYZWRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[8 * col + 0] = ((float *)(constScanLine(row)))[4 * col + 0];
                    ((float *)(image.scanLine(row)))[8 * col + 1] = ((float *)(constScanLine(row)))[4 * col + 1];
                    ((float *)(image.scanLine(row)))[8 * col + 2] = ((float *)(constScanLine(row)))[4 * col + 2];
                    ((float *)(image.scanLine(row)))[8 * col + 3] = 1.0f - (float)qIsNaN(((float *)(constScanLine(row)))[4 * col + 0]);
                    ((float *)(image.scanLine(row)))[8 * col + 4] = ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[8 * col + 5] = ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[8 * col + 6] = ((float *)(constScanLine(row)))[4 * col + 3];
                    ((float *)(image.scanLine(row)))[8 * col + 7] = 1.0f;
                }
            }
        }
    } else if (playbackColor == ColorXYZRGB) {
        if (clr == ColorGray) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[col] = (0.21 * ((float *)(constScanLine(row)))[6 * col + 3] + 0.72 * ((float *)(constScanLine(row)))[6 * col + 4] + 0.07 * ((float *)(constScanLine(row)))[6 * col + 5]);
                }
            }
        } else if (clr == ColorRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[3 * col + 0] = ((float *)(constScanLine(row)))[6 * col + 3];
                    ((float *)(image.scanLine(row)))[3 * col + 1] = ((float *)(constScanLine(row)))[6 * col + 4];
                    ((float *)(image.scanLine(row)))[3 * col + 2] = ((float *)(constScanLine(row)))[6 * col + 5];
                }
            }
        } else if (clr == ColorRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[6 * col + 3];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[6 * col + 4];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[6 * col + 5];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = 1.0f;
                }
            }
        } else if (clr == ColorXYZ) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[3 * col + 0] = ((float *)(constScanLine(row)))[6 * col + 0];
                    ((float *)(image.scanLine(row)))[3 * col + 1] = ((float *)(constScanLine(row)))[6 * col + 1];
                    ((float *)(image.scanLine(row)))[3 * col + 2] = ((float *)(constScanLine(row)))[6 * col + 2];
                }
            }
        } else if (clr == ColorXYZW) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[6 * col + 0];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[6 * col + 1];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[6 * col + 2];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = 1.0f - (float)qIsNaN(((float *)(constScanLine(row)))[6 * col + 0]);
                }
            }
        } else if (clr == ColorXYZG) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[6 * col + 0];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[6 * col + 1];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[6 * col + 2];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = 0.21 * ((float *)(constScanLine(row)))[6 * col + 3] + 0.72 * ((float *)(constScanLine(row)))[6 * col + 4] + 0.07 * ((float *)(constScanLine(row)))[6 * col + 5];
                }
            }
        } else if (clr == ColorXYZRGB) {
            memcpy(image.scanLine(0), constScanLine(0), image.width()*image.height()*image.colors()*sizeof(float));
        } else if (clr == ColorXYZWRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[8 * col + 0] = ((float *)(constScanLine(row)))[6 * col + 0];
                    ((float *)(image.scanLine(row)))[8 * col + 1] = ((float *)(constScanLine(row)))[6 * col + 1];
                    ((float *)(image.scanLine(row)))[8 * col + 2] = ((float *)(constScanLine(row)))[6 * col + 2];
                    ((float *)(image.scanLine(row)))[8 * col + 3] = 1.0f - (float)qIsNaN(((float *)(constScanLine(row)))[6 * col + 0]);
                    ((float *)(image.scanLine(row)))[8 * col + 4] = ((float *)(constScanLine(row)))[6 * col + 3];
                    ((float *)(image.scanLine(row)))[8 * col + 5] = ((float *)(constScanLine(row)))[6 * col + 4];
                    ((float *)(image.scanLine(row)))[8 * col + 6] = ((float *)(constScanLine(row)))[6 * col + 5];
                    ((float *)(image.scanLine(row)))[8 * col + 7] = 1.0f;
                }
            }
        }
    } else if (playbackColor == ColorXYZWRGBA) {
        if (clr == ColorGray) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[col] = (0.21 * ((float *)(constScanLine(row)))[8 * col + 4] + 0.72 * ((float *)(constScanLine(row)))[8 * col + 5] + 0.07 * ((float *)(constScanLine(row)))[8 * col + 6]) * ((float *)(constScanLine(row)))[8 * col + 7];
                }
            }
        } else if (clr == ColorRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[3 * col + 0] = ((float *)(constScanLine(row)))[8 * col + 4] * ((float *)(constScanLine(row)))[8 * col + 7];
                    ((float *)(image.scanLine(row)))[3 * col + 1] = ((float *)(constScanLine(row)))[8 * col + 5] * ((float *)(constScanLine(row)))[8 * col + 7];
                    ((float *)(image.scanLine(row)))[3 * col + 2] = ((float *)(constScanLine(row)))[8 * col + 6] * ((float *)(constScanLine(row)))[8 * col + 7];
                }
            }
        } else if (clr == ColorRGBA) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[8 * col + 4];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[8 * col + 5];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[8 * col + 6];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = ((float *)(constScanLine(row)))[8 * col + 7];
                }
            }
        } else if (clr == ColorXYZ) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[3 * col + 0] = ((float *)(constScanLine(row)))[8 * col + 0] / ((float *)(constScanLine(row)))[8 * col + 3];
                    ((float *)(image.scanLine(row)))[3 * col + 1] = ((float *)(constScanLine(row)))[8 * col + 1] / ((float *)(constScanLine(row)))[8 * col + 3];
                    ((float *)(image.scanLine(row)))[3 * col + 2] = ((float *)(constScanLine(row)))[8 * col + 2] / ((float *)(constScanLine(row)))[8 * col + 3];
                }
            }
        } else if (clr == ColorXYZW) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[8 * col + 0];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[8 * col + 1];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[8 * col + 2];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = ((float *)(constScanLine(row)))[8 * col + 3];
                }
            }
        } else if (clr == ColorXYZG) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[4 * col + 0] = ((float *)(constScanLine(row)))[8 * col + 0] / ((float *)(constScanLine(row)))[8 * col + 3];
                    ((float *)(image.scanLine(row)))[4 * col + 1] = ((float *)(constScanLine(row)))[8 * col + 1] / ((float *)(constScanLine(row)))[8 * col + 3];
                    ((float *)(image.scanLine(row)))[4 * col + 2] = ((float *)(constScanLine(row)))[8 * col + 2] / ((float *)(constScanLine(row)))[8 * col + 3];
                    ((float *)(image.scanLine(row)))[4 * col + 3] = (0.21 * ((float *)(constScanLine(row)))[8 * col + 4] + 0.72 * ((float *)(constScanLine(row)))[8 * col + 5] + 0.07 * ((float *)(constScanLine(row)))[8 * col + 6]) * ((float *)(constScanLine(row)))[8 * col + 7];
                }
            }
        } else if (clr == ColorXYZRGB) {
            for (unsigned int row = 0; row < image.height(); row++) {
                for (unsigned int col = 0; col < image.width(); col++) {
                    ((float *)(image.scanLine(row)))[6 * col + 0] = ((float *)(constScanLine(row)))[8 * col + 0] / ((float *)(constScanLine(row)))[8 * col + 3];
                    ((float *)(image.scanLine(row)))[6 * col + 1] = ((float *)(constScanLine(row)))[8 * col + 1] / ((float *)(constScanLine(row)))[8 * col + 3];
                    ((float *)(image.scanLine(row)))[6 * col + 2] = ((float *)(constScanLine(row)))[8 * col + 2] / ((float *)(constScanLine(row)))[8 * col + 3];
                    ((float *)(image.scanLine(row)))[6 * col + 3] = ((float *)(constScanLine(row)))[8 * col + 4] * ((float *)(constScanLine(row)))[8 * col + 7];
                    ((float *)(image.scanLine(row)))[6 * col + 4] = ((float *)(constScanLine(row)))[8 * col + 5] * ((float *)(constScanLine(row)))[8 * col + 7];
                    ((float *)(image.scanLine(row)))[6 * col + 5] = ((float *)(constScanLine(row)))[8 * col + 6] * ((float *)(constScanLine(row)))[8 * col + 7];
                }
            }
        } else if (clr == ColorXYZWRGBA) {
            memcpy(image.scanLine(0), constScanLine(0), image.width()*image.height()*image.colors()*sizeof(float));
        }
    }

    // UPDATE THE SCAN OBJECT'S LIMITS
    image.updateLimits();

    return (image);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUScan::transformScanInPlace(QMatrix4x4 mat)
{
    if (playbackColor == ColorGray || playbackColor == ColorRGB || playbackColor == ColorRGBA) {
        return;
    }

    __m128 colVec0 = _mm_set_ps(mat(3, 0), mat(2, 0), mat(1, 0), mat(0, 0));
    __m128 colVec1 = _mm_set_ps(mat(3, 1), mat(2, 1), mat(1, 1), mat(0, 1));
    __m128 colVec2 = _mm_set_ps(mat(3, 2), mat(2, 2), mat(1, 2), mat(0, 2));
    __m128 colVec3 = _mm_set_ps(mat(3, 3), mat(2, 3), mat(1, 3), mat(0, 3));
    if (playbackColor == ColorXYZG) {
        for (unsigned int row = 0; row < this->height(); row++) {
            float *toBuffer = (float *)this->scanLine(row);
            float *inBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < this->width(); col++) {
                __m128 inVecA = _mm_load_ps(&inBuffer[4 * col]);
                __m128 inVecB = _mm_mul_ps(colVec0, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(0, 0, 0, 0))); // MULTIPLY X COORDINATE BY FIRST COLUMN VECTOR OF OUR MATRIX
                __m128 inVecC = _mm_mul_ps(colVec1, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(1, 1, 1, 1))); // MULTIPLY Y COORDINATE BY SECOND COLUMN VECTOR OF OUR MATRIX
                __m128 inVecD = _mm_mul_ps(colVec2, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(2, 2, 2, 2))); // MULTIPLY Z COORDINATE BY THIRD COLUMN VECTOR OF OUR MATRIX

                // NOW MULTIPLY WITH THE ROWS OF THE TRANFORM MATRIX
                inVecB = _mm_add_ps(_mm_add_ps(inVecB, inVecC), _mm_add_ps(inVecD, colVec3));

                // SWAP BACK IN T IN PLACE OF W
                inVecB = _mm_insert_ps(inVecB, inVecA, 0xF0);

                // STORE RESULTING COORDINATE INTO OUTPUT SCAN
                _mm_store_ps(&toBuffer[4 * col], inVecB);
            }
        }
    } else if (playbackColor == ColorXYZWRGBA) {
        for (unsigned int row = 0; row < this->height(); row++) {
            float *toBuffer = (float *)this->scanLine(row);
            float *inBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < this->width(); col++) {
                __m128 inVecA = _mm_load_ps(&inBuffer[8 * col]);
                __m128 inVecB = _mm_mul_ps(colVec0, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(0, 0, 0, 0))); // MULTIPLY X COORDINATE BY FIRST COLUMN VECTOR OF OUR MATRIX
                __m128 inVecC = _mm_mul_ps(colVec1, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(1, 1, 1, 1))); // MULTIPLY Y COORDINATE BY SECOND COLUMN VECTOR OF OUR MATRIX
                __m128 inVecD = _mm_mul_ps(colVec2, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(2, 2, 2, 2))); // MULTIPLY Z COORDINATE BY THIRD COLUMN VECTOR OF OUR MATRIX

                // NOW MULTIPLY WITH THE ROWS OF THE TRANFORM MATRIX
                inVecB = _mm_add_ps(_mm_add_ps(inVecB, inVecC), _mm_add_ps(inVecD, colVec3));

                // STORE RESULTING COORDINATE INTO OUTPUT SCAN
                _mm_store_ps(&toBuffer[8 * col], inVecB);

                // COPY THE RGBA VECTOR
                _mm_store_ps(&toBuffer[8 * col + 4], _mm_load_ps(&inBuffer[8 * col + 4]));
            }
        }
    } else if (playbackColor == ColorXYZRGB) {
        __m128 mskVec = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
        __m128 oneVec = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
        for (unsigned int row = 0; row < this->height(); row++) {
            float *toBuffer = (float *)this->scanLine(row);
            float *inBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < this->width(); col++) {
                __m128 inVecA = _mm_add_ps(oneVec, _mm_and_ps(_mm_loadu_ps(&inBuffer[6 * col]), mskVec));
                __m128 inVecB = _mm_mul_ps(colVec0, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(0, 0, 0, 0))); // MULTIPLY X COORDINATE BY FIRST COLUMN VECTOR OF OUR MATRIX
                __m128 inVecC = _mm_mul_ps(colVec1, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(1, 1, 1, 1))); // MULTIPLY Y COORDINATE BY SECOND COLUMN VECTOR OF OUR MATRIX
                __m128 inVecD = _mm_mul_ps(colVec2, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(2, 2, 2, 2))); // MULTIPLY Z COORDINATE BY THIRD COLUMN VECTOR OF OUR MATRIX

                // NOW MULTIPLY WITH THE ROWS OF THE TRANFORM MATRIX
                inVecB = _mm_add_ps(_mm_add_ps(inVecB, inVecC), _mm_add_ps(inVecD, colVec3));

                // SWAP BACK IN T IN PLACE OF W
                inVecB = _mm_insert_ps(inVecB, inVecA, 0xF0);

                // STORE RESULTING COORDINATE INTO OUTPUT SCAN
                _mm_storeu_ps(&toBuffer[6 * col], inVecB);

                // COPY THE RGB VECTOR
                _mm_storeu_ps(&toBuffer[6 * col + 3], _mm_loadu_ps(&inBuffer[6 * col + 3]));
            }
        }
    }
    this->setConstTransform(QMatrix4x4());
    this->updateLimits();

    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::transformScan(QMatrix4x4 mat)
{
    if (playbackColor == ColorGray || playbackColor == ColorRGB || playbackColor == ColorRGBA) {
        return (LAUScan());
    }

    // MAKE A NEW IMAGE TO HOLD THE TRANSFORMED IMAGE
    LAUScan image = LAUScan(width(), height(), color()) + *this;

    __m128 colVec0 = _mm_set_ps(mat(3, 0), mat(2, 0), mat(1, 0), mat(0, 0));
    __m128 colVec1 = _mm_set_ps(mat(3, 1), mat(2, 1), mat(1, 1), mat(0, 1));
    __m128 colVec2 = _mm_set_ps(mat(3, 2), mat(2, 2), mat(1, 2), mat(0, 2));
    __m128 colVec3 = _mm_set_ps(mat(3, 3), mat(2, 3), mat(1, 3), mat(0, 3));
    if (playbackColor == ColorXYZG) {
        for (unsigned int row = 0; row < image.height(); row++) {
            float *toBuffer = (float *)image.scanLine(row);
            float *inBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < image.width(); col++) {
                __m128 inVecA = _mm_load_ps(&inBuffer[4 * col]);
                __m128 inVecB = _mm_mul_ps(colVec0, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(0, 0, 0, 0))); // MULTIPLY X COORDINATE BY FIRST COLUMN VECTOR OF OUR MATRIX
                __m128 inVecC = _mm_mul_ps(colVec1, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(1, 1, 1, 1))); // MULTIPLY Y COORDINATE BY SECOND COLUMN VECTOR OF OUR MATRIX
                __m128 inVecD = _mm_mul_ps(colVec2, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(2, 2, 2, 2))); // MULTIPLY Z COORDINATE BY THIRD COLUMN VECTOR OF OUR MATRIX

                // NOW MULTIPLY WITH THE ROWS OF THE TRANFORM MATRIX
                inVecB = _mm_add_ps(_mm_add_ps(inVecB, inVecC), _mm_add_ps(inVecD, colVec3));

                // SWAP BACK IN T IN PLACE OF W
                inVecB = _mm_insert_ps(inVecB, inVecA, 0xF0);

                // STORE RESULTING COORDINATE INTO OUTPUT SCAN
                _mm_store_ps(&toBuffer[4 * col], inVecB);
            }
        }
    } else if (playbackColor == ColorXYZWRGBA) {
        for (unsigned int row = 0; row < image.height(); row++) {
            float *toBuffer = (float *)image.scanLine(row);
            float *inBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < image.width(); col++) {
                __m128 inVecA = _mm_load_ps(&inBuffer[8 * col]);
                __m128 inVecB = _mm_mul_ps(colVec0, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(0, 0, 0, 0))); // MULTIPLY X COORDINATE BY FIRST COLUMN VECTOR OF OUR MATRIX
                __m128 inVecC = _mm_mul_ps(colVec1, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(1, 1, 1, 1))); // MULTIPLY Y COORDINATE BY SECOND COLUMN VECTOR OF OUR MATRIX
                __m128 inVecD = _mm_mul_ps(colVec2, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(2, 2, 2, 2))); // MULTIPLY Z COORDINATE BY THIRD COLUMN VECTOR OF OUR MATRIX

                // NOW MULTIPLY WITH THE ROWS OF THE TRANFORM MATRIX
                inVecB = _mm_add_ps(_mm_add_ps(inVecB, inVecC), _mm_add_ps(inVecD, colVec3));

                // STORE RESULTING COORDINATE INTO OUTPUT SCAN
                _mm_store_ps(&toBuffer[8 * col], inVecB);

                // COPY THE RGBA VECTOR
                _mm_store_ps(&toBuffer[8 * col + 4], _mm_load_ps(&inBuffer[8 * col + 4]));
            }
        }
    } else if (playbackColor == ColorXYZRGB) {
        __m128 mskVec = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
        __m128 oneVec = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
        for (unsigned int row = 0; row < image.height(); row++) {
            float *toBuffer = (float *)image.scanLine(row);
            float *inBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < image.width(); col++) {
                __m128 inVecA = _mm_add_ps(oneVec, _mm_and_ps(_mm_loadu_ps(&inBuffer[6 * col]), mskVec));
                __m128 inVecB = _mm_mul_ps(colVec0, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(0, 0, 0, 0))); // MULTIPLY X COORDINATE BY FIRST COLUMN VECTOR OF OUR MATRIX
                __m128 inVecC = _mm_mul_ps(colVec1, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(1, 1, 1, 1))); // MULTIPLY Y COORDINATE BY SECOND COLUMN VECTOR OF OUR MATRIX
                __m128 inVecD = _mm_mul_ps(colVec2, _mm_shuffle_ps(inVecA, inVecA, _MM_SHUFFLE(2, 2, 2, 2))); // MULTIPLY Z COORDINATE BY THIRD COLUMN VECTOR OF OUR MATRIX

                // NOW MULTIPLY WITH THE ROWS OF THE TRANFORM MATRIX
                inVecB = _mm_add_ps(_mm_add_ps(inVecB, inVecC), _mm_add_ps(inVecD, colVec3));

                // SWAP BACK IN T IN PLACE OF W
                inVecB = _mm_insert_ps(inVecB, inVecA, 0xF0);

                // STORE RESULTING COORDINATE INTO OUTPUT SCAN
                _mm_storeu_ps(&toBuffer[6 * col], inVecB);

                // COPY THE RGB VECTOR
                _mm_storeu_ps(&toBuffer[6 * col + 3], _mm_loadu_ps(&inBuffer[6 * col + 3]));
            }
        }
    }
    image.setConstTransform(QMatrix4x4());
    image.updateLimits();

    return (image);
}

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QImage LAUScan::preview(QSize size, Qt::AspectRatioMode aspectRatioMode)
{
    // TRANSFORM ROW BY ROW FROM THIS IMAGE BUFFER DIRECTLY TO THE SCAN LINES OF THE QIMAGE OBJECT
    QImage image(width(), height(), QImage::Format_RGB888);
    if (playbackColor == ColorGray) {
        for (unsigned int row = 0; row < height(); row++) {
            unsigned char *toBuffer = image.scanLine(row);
            unsigned char *fromBuffer = constScanLine(row);
            for (unsigned int col = 0; col < width(); col++) {
                toBuffer[3 * col + 0] = (unsigned char)(((float *)fromBuffer)[col] * 255.0f);
                toBuffer[3 * col + 1] = (unsigned char)(((float *)fromBuffer)[col] * 255.0f);
                toBuffer[3 * col + 2] = (unsigned char)(((float *)fromBuffer)[col] * 255.0f);
            }
        }
    } else if (playbackColor == ColorRGB) {
        for (unsigned int row = 0; row < height(); row++) {
            unsigned char *toBuffer = image.scanLine(row);
            unsigned char *fromBuffer = constScanLine(row);
            for (unsigned int col = 0; col < width(); col++) {
                toBuffer[3 * col + 0] = (unsigned char)(((float *)fromBuffer)[3 * col + 0] * 255.0f);
                toBuffer[3 * col + 1] = (unsigned char)(((float *)fromBuffer)[3 * col + 1] * 255.0f);
                toBuffer[3 * col + 2] = (unsigned char)(((float *)fromBuffer)[3 * col + 2] * 255.0f);
            }
        }
    } else if (playbackColor == ColorRGBA) {
        for (unsigned int row = 0; row < height(); row++) {
            unsigned char *toBuffer = image.scanLine(row);
            unsigned char *fromBuffer = constScanLine(row);
            for (unsigned int col = 0; col < width(); col++) {
                toBuffer[3 * col + 0] = (unsigned char)(((float *)fromBuffer)[4 * col + 0] * ((float *)fromBuffer)[4 * col + 3] * 255.0f);
                toBuffer[3 * col + 1] = (unsigned char)(((float *)fromBuffer)[4 * col + 1] * ((float *)fromBuffer)[4 * col + 3] * 255.0f);
                toBuffer[3 * col + 2] = (unsigned char)(((float *)fromBuffer)[4 * col + 2] * ((float *)fromBuffer)[4 * col + 3] * 255.0f);
            }
        }
    } else if (playbackColor == ColorXYZ) {
        image.fill(Qt::black);
    } else if (playbackColor == ColorXYZW) {
        image.fill(Qt::black);
    } else if (playbackColor == ColorXYZG) {
        for (unsigned int row = 0; row < height(); row++) {
            unsigned char *toBuffer = image.scanLine(row);
            unsigned char *fromBuffer = constScanLine(row);
            for (unsigned int col = 0; col < width(); col++) {
                toBuffer[3 * col + 0] = (unsigned char)(((float *)fromBuffer)[4 * col + 3] * 255.0f);
                toBuffer[3 * col + 1] = (unsigned char)(((float *)fromBuffer)[4 * col + 3] * 255.0f);
                toBuffer[3 * col + 2] = (unsigned char)(((float *)fromBuffer)[4 * col + 3] * 255.0f);
            }
        }
    } else if (playbackColor == ColorXYZRGB) {
        for (unsigned int row = 0; row < height(); row++) {
            unsigned char *toBuffer = image.scanLine(row);
            unsigned char *fromBuffer = constScanLine(row);
            for (unsigned int col = 0; col < width(); col++) {
                toBuffer[3 * col + 0] = (unsigned char)(((float *)fromBuffer)[6 * col + 3] * 255.0f);
                toBuffer[3 * col + 1] = (unsigned char)(((float *)fromBuffer)[6 * col + 4] * 255.0f);
                toBuffer[3 * col + 2] = (unsigned char)(((float *)fromBuffer)[6 * col + 5] * 255.0f);
            }
        }
    } else if (playbackColor == ColorXYZWRGBA) {
        for (unsigned int row = 0; row < height(); row++) {
            unsigned char *toBuffer = image.scanLine(row);
            unsigned char *fromBuffer = constScanLine(row);
            for (unsigned int col = 0; col < width(); col++) {
                toBuffer[3 * col + 0] = (unsigned char)(((float *)fromBuffer)[8 * col + 4] * ((float *)fromBuffer)[8 * col + 7] * 255.0f);
                toBuffer[3 * col + 1] = (unsigned char)(((float *)fromBuffer)[8 * col + 5] * ((float *)fromBuffer)[8 * col + 7] * 255.0f);
                toBuffer[3 * col + 2] = (unsigned char)(((float *)fromBuffer)[8 * col + 6] * ((float *)fromBuffer)[8 * col + 7] * 255.0f);
            }
        }
    }

    return (image.scaled(size, aspectRatioMode));
}
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
const QMatrix4x4 LAUScan::lookAt()
{
    // MAKE SURE LIMITS ARE UP TO DATA
    updateLimits();

    // GET THE CENTER OF THE BOUNDING BOX HOLDING SCAN
    QVector3D point((maxX() + minX()) / 2.0f, (maxY() + minY()) / 2.0f, 0.0f);

    // CREATE A MATRIX USING THE LOOK AT METHOD
    QMatrix4x4 transform;
    transform.translate(-point);

    return (transform);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::loadFromSKW(QString filename)
{
    // CREATE A SCAN TO HOLD THE INCOMING SKW FILE
    LAUScan scan;
    QMatrix4x4 tMatrix;

#ifndef HEADLESS
    // GET A FILE TO OPEN FROM THE USER IF NOT ALREADY PROVIDED ONE
    if (filename.isNull()) {
        QSettings settings;
        QString directory = settings.value("LAUScan::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        filename = QFileDialog::getOpenFileName(0, QString("Load scan from disk (*.skw)"), directory, QString("*.skw"));
        if (!filename.isNull()) {
            settings.setValue("LAUScan::lastUsedDirectory", QFileInfo(filename).absolutePath());
        } else {
            return (scan);
        }
    }
#endif
    // CREATE VARIABLES TO HOLD THE SIZE OF THE SCAN
    int cols = 0;
    int rows = 0;

    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray header;
        while (file.atEnd() == false) {
            QByteArray byteArray = file.readLine();
            header.append(byteArray);
            if (byteArray.contains(QByteArray("</skw>"))) {
                break;
            }
        }

        // SAVE THE CURRENT POSITION IN THE FILE FOR LATTER
        int dataStartPos = file.pos();

        qDebug() << header;

        QDomDocument xml;
        xml.setContent(header);

        QByteArray xArray, yArray, zArray, gArray, tArray;

        QDomElement root = xml.documentElement();
        if (root.tagName() == "skw") {
            QDomElement next = root.firstChildElement();
            while (!next.isNull()) {
                QDomElement element = next.toElement();
                if (!element.isNull()) {
                    if (element.tagName() == "image") {
                        QDomElement tag = element.firstChildElement();
                        while (!tag.isNull()) {
                            if (tag.tagName() == "imageRows") {
                                rows = tag.text().toInt();
                            } else if (tag.tagName() == "imageColumns") {
                                cols = tag.text().toInt();
                            }
                            tag = tag.nextSiblingElement();
                        }
                    } else if (element.tagName() == "xBuffer") {
                        int size = -1, offset = -1;
                        QDomElement tag = element.firstChildElement();
                        while (!tag.isNull()) {
                            if (tag.tagName() == "size") {
                                size = tag.text().toInt();
                            } else if (tag.tagName() == "offset") {
                                offset = tag.text().toInt();
                            }
                            tag = tag.nextSiblingElement();
                        }
                        // READ IN XBUFFER TO A QBYTEARRAY
                        if (size > 0 && offset >= 0) {
                            file.seek(dataStartPos + offset);
                            xArray = qUncompress(file.read(size));
                        }
                    } else if (element.tagName() == "yBuffer") {
                        int size = -1, offset = -1;
                        QDomElement tag = element.firstChildElement();
                        while (!tag.isNull()) {
                            if (tag.tagName() == "size") {
                                size = tag.text().toInt();
                            } else if (tag.tagName() == "offset") {
                                offset = tag.text().toInt();
                            }
                            tag = tag.nextSiblingElement();
                        }
                        // READ IN YBUFFER TO A QBYTEARRAY
                        if (size > 0 && offset >= 0) {
                            file.seek(dataStartPos + offset);
                            yArray = qUncompress(file.read(size));
                        }
                    } else if (element.tagName() == "zBuffer") {
                        int size = -1, offset = -1;
                        QDomElement tag = element.firstChildElement();
                        while (!tag.isNull()) {
                            if (tag.tagName() == "size") {
                                size = tag.text().toInt();
                            } else if (tag.tagName() == "offset") {
                                offset = tag.text().toInt();
                            }
                            tag = tag.nextSiblingElement();
                        }
                        // READ IN ZBUFFER TO A QBYTEARRAY
                        if (size > 0 && offset >= 0) {
                            file.seek(dataStartPos + offset);
                            zArray = qUncompress(file.read(size));
                        }
                    } else if (element.tagName() == "transform") {
                        int size = -1, offset = -1;
                        QDomElement tag = element.firstChildElement();
                        while (!tag.isNull()) {
                            if (tag.tagName() == "size") {
                                size = tag.text().toInt();
                            } else if (tag.tagName() == "offset") {
                                offset = tag.text().toInt();
                            }
                            tag = tag.nextSiblingElement();
                        }
                        // READ IN YBUFFER TO A QBYTEARRAY
                        if (size > 0 && offset >= 0) {
                            file.seek(dataStartPos + offset);
                            tArray = qUncompress(file.read(size));
                            if (tArray.length() == 16 * sizeof(float)) {
                                int index = 0;
                                for (int row = 0; row < 4; row++) {
                                    for (int col = 0; col < 4; col++) {
                                        tMatrix(row, col) = reinterpret_cast<float *>(tArray.data())[index++];
                                    }
                                }
                            }
                        }
                    } else if (element.tagName() == "texture") {
                        int size = -1, offset = -1;
                        QDomElement tag = element.firstChildElement();
                        while (!tag.isNull()) {
                            if (tag.tagName() == "size") {
                                size = tag.text().toInt();
                            } else if (tag.tagName() == "offset") {
                                offset = tag.text().toInt();
                            }
                            tag = tag.nextSiblingElement();
                        }
                        // READ IN TEXTURE TO A QBYTEARRAY
                        if (size > 0 && offset >= 0) {
                            file.seek(dataStartPos + offset);
                            gArray = qUncompress(file.read(size));
                        }
                    }
                }
                next = next.nextSiblingElement();
            }
        }

        // CLOSE THE FILE NOW THAT WE HAVE WHAT WE NEED
        file.close();

        // MAKE SURE WE HAVE VALID IMAGE SIZE
        if (cols > 0 && rows > 0) {
            // LETS MAKE SURE WE HAVE SUFFICIENTLY LARGE X, Y, Z, AND T BUFFERS
            int expectedBufferLength = cols * rows * sizeof(float);

            float *xBuffer = (float *)xArray.data();
            float *yBuffer = (float *)yArray.data();
            float *zBuffer = (float *)zArray.data();
            float *gBuffer = (float *)gArray.data();

            if (xArray.length() == expectedBufferLength && yArray.length() == expectedBufferLength && zArray.length() == expectedBufferLength) {
                int grayScaleBufferLength = cols * rows * sizeof(float);
                if (gArray.length() == 1 * grayScaleBufferLength) {
                    scan = LAUScan(cols, rows, ColorXYZG);

                    for (int r = 0; r < rows; r++) {
                        float *buffer = (float *)scan.constScanLine(r);
                        for (int c = 0; c < cols; c++) {
                            buffer[4 * c + 0] = xBuffer[r * cols + c];
                            buffer[4 * c + 1] = yBuffer[r * cols + c];
                            buffer[4 * c + 2] = zBuffer[r * cols + c];
                            buffer[4 * c + 3] = gBuffer[r * cols + c];
                        }
                    }
                } else if (gArray.length() == 3 * grayScaleBufferLength) {
                    scan = LAUScan(cols, rows, ColorXYZRGB);

                    int step = rows * cols;
                    for (int r = 0; r < rows; r++) {
                        float *buffer = (float *)scan.constScanLine(r);
                        for (int c = 0; c < cols; c++) {
                            buffer[6 * c + 0] = xBuffer[r * cols + c];
                            buffer[6 * c + 1] = yBuffer[r * cols + c];
                            buffer[6 * c + 2] = zBuffer[r * cols + c];
                            buffer[6 * c + 3] = gBuffer[0 * step + (r * cols + c)];
                            buffer[6 * c + 4] = gBuffer[1 * step + (r * cols + c)];
                            buffer[6 * c + 5] = gBuffer[2 * step + (r * cols + c)];
                        }
                    }
                } else if (gArray.length() == 4 * grayScaleBufferLength) {
                    scan = LAUScan(cols, rows, ColorXYZWRGBA);

                    int step = rows * cols;
                    for (int r = 0; r < rows; r++) {
                        float *buffer = (float *)scan.constScanLine(r);
                        for (int c = 0; c < cols; c++) {
                            buffer[8 * c + 0] = xBuffer[r * cols + c];
                            buffer[8 * c + 1] = yBuffer[r * cols + c];
                            buffer[8 * c + 2] = zBuffer[r * cols + c];
                            buffer[8 * c + 3] = 1.0f - (float)qIsNaN(xBuffer[r * cols + c] * yBuffer[r * cols + c] * zBuffer[r * cols + c]);
                            buffer[8 * c + 3] = gBuffer[0 * step + (r * cols + c)];
                            buffer[8 * c + 4] = gBuffer[1 * step + (r * cols + c)];
                            buffer[8 * c + 5] = gBuffer[2 * step + (r * cols + c)];
                            buffer[8 * c + 3] = gBuffer[3 * step + (r * cols + c)];
                        }
                    }
                }
            }
            scan.setFilename(filename);
            scan.updateLimits();
        }
    }
    return (scan); //.transformScan(tMatrix));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::loadFromCSV(QString filename)
{
    // CREATE A SCAN TO HOLD THE INCOMING SKW FILE
    LAUScan scan;
#ifndef HEADLESS
    // GET A FILE TO OPEN FROM THE USER IF NOT ALREADY PROVIDED ONE
    if (filename.isNull()) {
        QSettings settings;
        QString directory = settings.value("LAUScan::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        filename = QFileDialog::getOpenFileName(0, QString("Load scan from disk (*.csv)"), directory, QString("*.csv"));
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        if (!filename.isNull()) {
            settings.setValue("LAUScan::lastUsedDirectory", QFileInfo(filename).absolutePath());
        } else {
            return (scan);
        }
    }
#endif
    // OPEN THE FILE FOR READING
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        // READ THE FIRST LINE TO GET THE NUMBER OF SCAN COLUMNS
        QString string = QString(file.readLine());
        string.remove(",\r\n");
        string.remove(",\n");
        string.remove("\r");
        string.remove("\n");
        QStringList strings = string.split(",");

        float radius = strings.takeFirst().toFloat();
        int cols = strings.count();

        // ITERATE THROUGH THE REST OF THE FILE TO GET THE NUMBER OF ROWS
        int rows = 0;
        while (file.atEnd() == false) {
            file.readLine();
            rows++;
        }
        file.seek(0);
        file.readLine();

        // CREATE A LAUSCAN TO HOLD THE INCOMING SCAN
        scan = LAUScan(cols, rows, ColorXYZG);

        // GET THE X-COORDINATES FROM THE FIRST ROW OF DATA
        QVector<float> x(cols);
        for (int n = 0; n < cols; n++) {
            x[n] = strings.at(n).toFloat();
        }

        // GET THE Y-COORDINATES FROM THE FIRST COLUMN OF DATA
        QVector<float> y(rows);
        for (int row = 0; row < rows; row++) {
            QStringList strings = QString(file.readLine()).split(",");
            y[row] = strings.takeFirst().toFloat();
            double theta = (double)y[row] / (double)radius;

            float *buffer = (float *)scan.scanLine(row);
            for (int c = 0; c < strings.count() && c < cols; c++) {
                double distance = radius + strings.at(c).toFloat();
                buffer[c * 4 + 0] =  x[c];
                buffer[c * 4 + 1] =  sin(theta) * distance;
                buffer[c * 4 + 2] =  cos(theta) * distance - 2.0 * radius;
            }
        }
        scan = scan.transformScan(scan.lookAt());
        scan.updateLimits();

        // NOW FILTER THROUGH IMAGE AND CREATE A TEXTURE EQUAL TO THE NORMALIZED Z VALUE
        float zMin = scan.minZ();
        float zMax = scan.maxZ();
        for (unsigned int row = 0; row < scan.height(); row++) {
            float *buffer = (float *)scan.scanLine(row);
            for (unsigned int col = 0; col < scan.width(); col++) {
                buffer[4 * col + 3] = (buffer[4 * col + 2] - zMin) / (zMax - zMin);
            }
        }

        scan.setFilename(filename);
        file.close();
    }
    return (scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUScan::loadFromTIFF(QString filename)
{
    // CREATE A SCAN TO HOLD THE INCOMING SKW FILE
    LAUScan scan;
#ifndef HEADLESS
    // GET A FILE TO OPEN FROM THE USER IF NOT ALREADY PROVIDED ONE
    if (filename.isNull()) {
        QSettings settings;
        QString directory = settings.value("LAUScan::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        filename = QFileDialog::getOpenFileName(0, QString("Load scan from disk (*.tiff)"), directory, QString("*.tif;*.tiff"));
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        if (!filename.isNull()) {
            settings.setValue("LAUScan::lastUsedDirectory", QFileInfo(filename).absolutePath());
        } else {
            return (scan);
        }
    }
#endif
    // OPEN TIFF FILE FOR LOADING THE IMAGE FROM DISK
    TIFF *tiff = TIFFOpen(filename.toLocal8Bit(), "r");
    if (tiff) {
        // CHOP OFF FILE EXTENSION FROM FILENAME
        filename = filename.left(filename.indexOf("."));

        // LOAD THE CURRENT DIRECTORY INTO A NEW IMAGE AND ADD TO LIST
        LAUMemoryObject object(tiff);

        // MAKE SURE OBJECT IS FLOATING POINT
        if (object.depth() != sizeof(float)) {
            object = object.toFloat();
        }

        // SEE IF THIS IS A VALID SCAN FORMAT
        if (object.colors() == 1) {
            scan = LAUScan(object, ColorGray);
        } else if (object.colors() == 3) {
            scan = LAUScan(object, ColorRGB);
        } else if (object.colors() == 4) {
            scan = LAUScan(object, ColorRGBA);
        } else if (object.colors() == 6) {
            scan = LAUScan(object, ColorXYZRGB);
        } else if (object.colors() == 8) {
            scan = LAUScan(object, ColorXYZWRGBA);
        }

        if (scan.isValid()) {
            scan.setFilename(filename);
        }

        // CLOSE TIFF FILE
        TIFFClose(tiff);
    } else {
#ifndef HEADLESS
        QMessageBox::warning(nullptr, QString("Import Document"), QString("Error opening tiff file: %1").arg(LAUMemoryObject::lastTiffErrorString));
#endif
    }

    return (scan);
}

#ifndef HEADLESS
#ifndef EXCLUDE_LAUSCANINSPECTOR
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUScan::approveImage(bool *doNotShowAgainCheckBoxEnabled, QWidget *parent) const
{
    if (doNotShowAgainCheckBoxEnabled) {
        LAUScanInspector dialog(*this, true, *doNotShowAgainCheckBoxEnabled, parent);
        dialog.setWindowTitle(this->parentName());

        int ret = dialog.exec();
        *doNotShowAgainCheckBoxEnabled = !dialog.doNotShowAgainChecked();
        return (ret == QDialog::Accepted);
    } else {
        LAUScanInspector dialog(*this, true, false, parent);
        dialog.setWindowTitle(this->parentName());
        return (dialog.exec() == QDialog::Accepted);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAUScan::inspectImage() const
{
    LAUScanInspector dialog(*this, false);
    dialog.setWindowTitle(parentName());
    return (dialog.exec());
}
#endif
#endif
