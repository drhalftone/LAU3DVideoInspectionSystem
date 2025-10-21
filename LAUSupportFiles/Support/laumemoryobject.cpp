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

#include <stdio.h>

#include <QFile>
#include <QtMath>
#include <QBuffer>
#include <QDateTime>
#include <QXmlStreamReader>

#ifndef HEADLESS
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#endif

#include "laumemoryobject.h"

using namespace libtiff;

int LAUMemoryObjectData::instanceCounter = 0;

QString LAUMemoryObject::lastTiffWarningString;
QString LAUMemoryObject::lastTiffErrorString;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void myTIFFWarningHandler(const char *stringA, const char *stringB, va_list args)
{
    Q_UNUSED(stringA);

    char buffer[1024];
#ifdef Q_OS_WIN
    int length = vsprintf_s(buffer, (char *)stringB, args);
#else
    int length = vsprintf(buffer, (char *)stringB, args);
#endif
    LAUMemoryObject::lastTiffWarningString = QString(QByteArray(buffer, length));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void myTIFFErrorHandler(const char *stringA, const char *stringB, va_list args)
{
    Q_UNUSED(stringA);

    char buffer[1024];
#ifdef Q_OS_WIN
    int length = vsprintf_s(buffer, (char *)stringB, args);
#else
    int length = vsprintf(buffer, (char *)stringB, args);
#endif
    LAUMemoryObject::lastTiffErrorString = QString(QByteArray(buffer, length));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAU3DVideoParameters::colors(LAUVideoPlaybackColor clr)
{
    switch (clr) {
        case ColorUndefined:
            return (0);
        case ColorGray:
            return (1);
        case ColorRGB:
            return (3);
        case ColorRGBA:
            return (4);
        case ColorXYZ:
            return (3);
        case ColorXYZW:
            return (4);
        case ColorXYZG:
            return (4);
        case ColorXYZRGB:
            return (6);
        case ColorXYZWRGBA:
            return (8);
        default:
            return (0);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAU3DVideoParameters::isMachineVision(LAUVideoPlaybackDevice dvc)
{
    if (dvc == DeviceProsilicaGRY || dvc == DeviceProsilicaIOS || dvc == DeviceProsilicaLCG ||
        dvc == DeviceProsilicaAST || dvc == DeviceProsilicaPST || dvc == DeviceProsilicaDPR || dvc == DeviceProsilicaTOF ||
        dvc == DeviceProsilicaRGB) {
        return (true);
    }
    return (false);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObjectData::LAUMemoryObjectData()
{
    numRows = 0;
    numCols = 0;
    numChns = 0;
    numByts = 0;
    numFrms = 0;

    stepBytes = 0;
    frameBytes = 0;

    buffer = nullptr;
    xmlByteArray = nullptr;
    rfidString = nullptr;
    transformMatrix = nullptr;
    projectionMatrix = nullptr;
    anchorPt = nullptr;
    elapsedTime = nullptr;
    jetr = nullptr;

    numBytesTotal = 0;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObjectData::LAUMemoryObjectData(unsigned int cols, unsigned int rows, unsigned int chns, unsigned int byts, unsigned int frms)
{
    numRows = rows;
    numCols = cols;
    numChns = chns;
    numByts = byts;
    numFrms = frms;

    buffer = nullptr;
    xmlByteArray = nullptr;
    rfidString = nullptr;
    transformMatrix = nullptr;
    projectionMatrix = nullptr;
    anchorPt = nullptr;
    elapsedTime = nullptr;
    jetr = nullptr;

    stepBytes = 0;
    frameBytes = 0;
    numBytesTotal = 0;

    allocateBuffer();

    // FOR A BRAND NEW MEMORY OBJECT, INITIALIZE THE MEMORY TO ALL ZEROS
    if (buffer) {
        memset(buffer, 0, numBytesTotal);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObjectData::LAUMemoryObjectData(unsigned long long bytes)
{
    numRows = bytes;
    numCols = 1;
    numChns = 1;
    numByts = 1;
    numFrms = 1;

    buffer = nullptr;
    xmlByteArray = nullptr;
    rfidString = nullptr;
    transformMatrix = nullptr;
    projectionMatrix = nullptr;
    anchorPt = nullptr;
    elapsedTime = nullptr;
    jetr = nullptr;

    stepBytes = 0;
    frameBytes = 0;
    numBytesTotal = 0;

    allocateBuffer();

    // FOR A BRAND NEW MEMORY OBJECT, INITIALIZE THE MEMORY TO ALL ZEROS
    if (buffer) {
        memset(buffer, 0, numBytesTotal);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObjectData::LAUMemoryObjectData(const LAUMemoryObjectData &other) : LAUMemoryObjectData()
{
    // COPY OVER THE SIZE PARAMETERS FROM THE SUPPLIED OBJECT
    numRows = other.numRows;
    numCols = other.numCols;
    numChns = other.numChns;
    numByts = other.numByts;
    numFrms = other.numFrms;

    // SET THESE VARIABLES TO ZERO AND LET THEM BE MODIFIED IN THE ALLOCATION METHOD
    buffer = nullptr;
    xmlByteArray = nullptr;
    rfidString = nullptr;
    transformMatrix = nullptr;
    projectionMatrix = nullptr;
    anchorPt = nullptr;
    elapsedTime = nullptr;
    jetr = nullptr;

    stepBytes = 0;
    frameBytes = 0;
    numBytesTotal = 0;

    allocateBuffer();
    if (buffer) {
        memcpy(buffer, other.buffer, numBytesTotal);
        xmlByteArray->clear();
        xmlByteArray->append(*(other.xmlByteArray));
        rfidString->clear();
        rfidString->append(*(other.rfidString));
        memcpy((void *)transformMatrix->constData(), (void *)other.transformMatrix->constData(), sizeof(QMatrix4x4));
        memcpy((void *)projectionMatrix->constData(), (void *)other.projectionMatrix->constData(), sizeof(QMatrix4x4));
        anchorPt->setX(other.anchorPt->x());
        anchorPt->setY(other.anchorPt->y());
        *elapsedTime = *other.elapsedTime;
        *jetr = *other.jetr;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObjectData::~LAUMemoryObjectData()
{
    if (buffer != NULL) {
        instanceCounter = instanceCounter - 1;
        qDebug() << QString("LAUMemoryObjectData::~LAUMemoryObjectData() %1").arg(instanceCounter) << numRows << numCols << numChns << numByts << numFrms << numBytesTotal;
        _mm_free(buffer);

        delete rfidString;
        delete transformMatrix;
        delete projectionMatrix;
        delete anchorPt;
        delete elapsedTime;
        delete jetr;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMemoryObjectData::allocateBuffer()
{
    // ALLOCATE SPACE FOR HOLDING PIXEL DATA BASED ON NUMBER OF CHANNELS AND BYTES PER PIXEL
    numBytesTotal  = (unsigned long long)numRows;
    numBytesTotal *= (unsigned long long)numCols;
    numBytesTotal *= (unsigned long long)numChns;
    numBytesTotal *= (unsigned long long)numByts;
    numBytesTotal *= (unsigned long long)numFrms;

    if (numBytesTotal > 0) {
        instanceCounter = instanceCounter + 1;
        qDebug() << QString("LAUMemoryObjectData::allocateBuffer() %1").arg(instanceCounter) << numRows << numCols << numChns << numByts << numFrms << numBytesTotal;

        stepBytes  = numCols * numChns * numByts;
        frameBytes = numCols * numChns * numByts * numRows;
        buffer     = _mm_malloc(numBytesTotal + 128, 16);
        if (buffer == nullptr) {
            qDebug() << QString("LAUVideoBufferData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
            qDebug() << QString("LAUVideoBufferData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
            qDebug() << QString("LAUVideoBufferData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
        } else {
            xmlByteArray = new QByteArray();
            rfidString = new QString();
            transformMatrix = new QMatrix4x4();
            projectionMatrix = new QMatrix4x4();
            anchorPt = new QPoint(-1, -1);
            elapsedTime = new unsigned int;
            *elapsedTime = 0;
            jetr = new QVector<double>;
            *jetr = QVector<double>(37,NAN);
        }
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject::LAUMemoryObject(QString filename, int index) : data(new LAUMemoryObjectData())
{
    // GET A FILE TO OPEN FROM THE USER IF NOT ALREADY PROVIDED ONE
    if (filename.isNull()) {
#ifndef HEADLESS
        QSettings settings;
        QString directory = settings.value("LAUMemoryObject::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        filename = QFileDialog::getOpenFileName(0, QString("Load scan from disk (*.tif)"), directory, QString("*.tif;*.tiff"));
        if (filename.isEmpty() == false) {
            settings.setValue("LAUMemoryObject::lastUsedDirectory", QFileInfo(filename).absolutePath());
        } else {
            return;
        }
#else
        return;
#endif
    }

    // IF WE HAVE A VALID TIFF FILE, LOAD FROM DISK
    // OTHERWISE TRY TO CONNECT TO SCANNER
    if (QFile::exists(filename)) {
        // OPEN INPUT TIFF FILE FROM DISK
        TIFF *inTiff = TIFFOpen(filename.toLatin1(), "r");
        if (!inTiff) {
            return;
        }
        load(inTiff, index);
        TIFFClose(inTiff);
    }
}

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject::LAUMemoryObject(QImage image) : data(new LAUMemoryObjectData())
{
    qDebug() << image.format();
    if (image.isNull() == false) {
        if (image.format() == QImage::Format_ARGB32 || image.format() == QImage::Format_RGB32) {
            data->numCols = (unsigned int)image.width();
            data->numRows = (unsigned int)image.height();
            data->numByts = sizeof(float);
            data->numChns = 4;
            data->numFrms = 1;
            data->allocateBuffer();

            for (unsigned int row = 0; row < height(); row++) {
                float *toBuffer = reinterpret_cast<float *>(constScanLine(row));
                for (unsigned int col = 0; col < width(); col++) {
                    QRgb pixel = image.pixel(static_cast<int>(col), static_cast<int>(row));
                    toBuffer[4 * col + 0] = (float)qRed(pixel) / 255.0f;
                    toBuffer[4 * col + 1] = (float)qGreen(pixel) / 255.0f;
                    toBuffer[4 * col + 2] = (float)qBlue(pixel) / 255.0f;
                    toBuffer[4 * col + 3] = 1.0f;
                }
            }
        } else if (image.format() == QImage::Format_Grayscale8 || image.format() == QImage::Format_Mono  || image.format() == QImage::Format_Indexed8) {
            data->numCols = (unsigned int)image.width();
            data->numRows = (unsigned int)image.height();
            data->numByts = sizeof(unsigned char);
            data->numChns = 1;
            data->numFrms = 1;
            data->allocateBuffer();

            for (unsigned int row = 0; row < height(); row++) {
                memcpy(constScanLine(row), image.constScanLine(row), step());
            }
#if QT_VERSION >= 0x060000
        } else if (image.format() == QImage::Format_Grayscale16) {
            data->numCols = (unsigned int)image.width();
            data->numRows = (unsigned int)image.height();
            data->numByts = sizeof(unsigned short);
            data->numChns = 1;
            data->numFrms = 1;
            data->allocateBuffer();

            for (unsigned int row = 0; row < height(); row++) {
                memcpy(constScanLine(row), image.constScanLine(row), step());
            }
#endif
        }
    }
}
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject::LAUMemoryObject(TIFF *inTiff, int index) : data(new LAUMemoryObjectData())
{
    load(inTiff, index);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUMemoryObject::save(QString filename, QString *savedFilePath) const
{
    // LET THE USER SELECT A FILE FROM THE FILE DIALOG
    if (filename.isNull()) {
#ifndef HEADLESS
        QSettings settings;
        QString directory = settings.value(QString("LAUMemoryObject::lastUsedDirectory"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        int counter = 0;
        do {
            if (counter == 0) {
                filename = QString("%1/Untitled.tif").arg(directory);
            } else {
                filename = QString("%1/Untitled%2.tif").arg(directory).arg(counter);
            }
            counter++;
        } while (QFile::exists(filename));

        filename = QFileDialog::getSaveFileName(nullptr, QString("Save image to disk (*.tif)"), filename, QString("*.tif;*.tiff"));
        if (!filename.isNull()) {
            settings.setValue(QString("LAUMemoryObject::lastUsedDirectory"), QFileInfo(filename).absolutePath());
            if (!filename.toLower().endsWith(QString(".tiff"))) {
                if (!filename.toLower().endsWith(QString(".tif"))) {
                    filename = QString("%1.tif").arg(filename);
                }
            }
        } else {
            return (false);
        }
#else
        return (false);
#endif
    }

    // Store the saved file path if caller provided an output parameter
    if (savedFilePath) {
        *savedFilePath = filename;
    }

    // OPEN TIFF FILE FOR SAVING THE IMAGE USING BIGTIFF FOR LARGE FILES
    TIFF *outputTiff = (this->length() > 100000000) ? TIFFOpen(filename.toLatin1(), "w8") : TIFFOpen(filename.toLatin1(), "w");
    if (!outputTiff) {
        return (false);
    }

    // CREATE A RETURN FLAG
    bool flag = true;

    if (frames() == 1) {
        // WRITE IMAGE TO CURRENT DIRECTORY AND SAVE RESULT TO BOOL FLAG
        flag = save(outputTiff);
    } else {
        // CREATE A MEMORY OBJECT TO HOLD A SINGLE FRAME OF VIDEO
        LAUMemoryObject object(width(), height(), colors(), depth(), 1);
        for (unsigned int n = 0; n < frames(); n++) {
            // COPY THE CURRENT FRAME INTO THE TEMPORARY FRAME BUFFER OBJECT
            memcpy(object.constPointer(), constFrame(n), block());

            // SAVE THE CURRENT FRAME INTO ITS OWN DIRECTORY INSIDE THE NEW TIFF FILE
            if (object.save(outputTiff, n) == false) {
                flag = false;
                break;
            }
        }
#ifdef DONTCOMPILE
        LAUMemoryObject object(width(), height(), colors()*frames(), depth(), 1);

        // COPY THE CURRENT FRAME INTO THE TEMPORARY FRAME BUFFER OBJECT
        memcpy(object.constPointer(), constPointer(), length());

        // SAVE THE CURRENT FRAME INTO ITS OWN DIRECTORY INSIDE THE NEW TIFF FILE
        if (object.save(outputTiff, 0) == false) {
            flag = false;
        }
#endif
    }

    // CLOSE TIFF FILE
    TIFFClose(outputTiff);
    return (flag);
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUMemoryObject::saveObjectsToDisk(QList<LAUMemoryObject> objects, QString filename)
{
    // LET THE USER SELECT A FILE FROM THE FILE DIALOG
    if (filename.isNull()) {
#ifndef HEADLESS
        QSettings settings;
        QString directory = settings.value(QString("LAUMemoryObject::lastUsedDirectory"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        int counter = 0;
        do {
            if (counter == 0) {
                filename = QString("%1/Untitled.tif").arg(directory);
            } else {
                filename = QString("%1/Untitled%2.tif").arg(directory).arg(counter);
            }
            counter++;
        } while (QFile::exists(filename));

        filename = QFileDialog::getSaveFileName(nullptr, QString("Save image to disk (*.tif)"), filename, QString("*.tif;*.tiff"));
        if (!filename.isNull()) {
            settings.setValue(QString("LAUMemoryObject::lastUsedDirectory"), QFileInfo(filename).absolutePath());
            if (!filename.toLower().endsWith(QString(".tiff"))) {
                if (!filename.toLower().endsWith(QString(".tif"))) {
                    filename = QString("%1.tif").arg(filename);
                }
            }
        } else {
            return (false);
        }
#else
        return (false);
#endif
    }

    // OPEN TIFF FILE FOR SAVING THE IMAGE USING BIGTIFF FOR LARGE FILES
    TIFF *outputTiff = (objects.first().length() > 100000000) ? TIFFOpen(filename.toLatin1(), "w8") : TIFFOpen(filename.toLatin1(), "w");
    if (!outputTiff) {
        return (false);
    }

    for (int n = 0; n < objects.count(); n++){
        if (objects.at(n).save(outputTiff, n) == false){
            TIFFClose(outputTiff);
            return(false);
        }
    }

    // CLOSE TIFF FILE
    TIFFClose(outputTiff);
    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUMemoryObject::save(TIFF *otTiff, int index) const
{
    TIFFSetField(otTiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
    TIFFSetField(otTiff, TIFFTAG_DATETIME, QDateTime::currentDateTime().toString("yyyy:MM:dd hh:mm:ss").toLatin1().data());
    TIFFSetField(otTiff, TIFFTAG_IMAGEWIDTH, (unsigned long)width());
    TIFFSetField(otTiff, TIFFTAG_IMAGELENGTH, (unsigned long)(height()*frames()));
    TIFFSetField(otTiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    TIFFSetField(otTiff, TIFFTAG_XRESOLUTION, 72.0);
    TIFFSetField(otTiff, TIFFTAG_YRESOLUTION, 72.0);
    TIFFSetField(otTiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(otTiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(otTiff, TIFFTAG_SAMPLESPERPIXEL, (unsigned short)colors());
    TIFFSetField(otTiff, TIFFTAG_BITSPERSAMPLE, (unsigned short)(8 * depth()));
    if (colors() == 1) {
        TIFFSetField(otTiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    } else if (colors() == 3) {
        TIFFSetField(otTiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    } else if (colors() == 4) {
        TIFFSetField(otTiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_SEPARATED);
    } else {
        TIFFSetField(otTiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        unsigned short *smples = new unsigned short [colors()];
        for (unsigned int n = 0; n < colors(); n++) {
            smples[n] = EXTRASAMPLE_UNSPECIFIED;
        }
        TIFFSetField(otTiff, TIFFTAG_EXTRASAMPLES, colors() - 1, smples);
        delete [] smples;
    }
    TIFFSetField(otTiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    TIFFSetField(otTiff, TIFFTAG_XPOSITION, qMax(0.0f, (float)anchor().x()));
    TIFFSetField(otTiff, TIFFTAG_YPOSITION, qMax(0.0f, (float)anchor().y()));
    TIFFSetField(otTiff, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
    TIFFSetField(otTiff, TIFFTAG_ROWSPERSTRIP, 1);

    // LET'S SEE IF THERE ARE ANY VALID ENTRIES IN THE JETR VECTOR
    QVector<double> jetrVector = this->jetr();
    for (int n = 0; n < jetrVector.length(); n++){
        if (qIsNaN(jetrVector[n]) == false){
            // GRAB THE CURRENT XML FIELD IN HASH TABLE FORM
            QHash<QString, QString> hashTable = xmlToHash(xml());

            // OVERWRITE LIST OF QSTRINGS THAT YOU WANT TO WRITE TO YOUR XML ARRAY
            QString jetrString;

            // APPEND JETR DOUBLES TO STRING
            jetrString = QString("%1").arg(jetrVector[0]);
            for (int n = 1; n < jetrVector.length(); n++){
                jetrString.append(QString(",%1").arg(jetrVector[n], 0, 'f', 5));
            }
            hashTable["jetrVector"] = jetrString;

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

            setConstXML(xmlByteArray);

            break;
        }
    }

    // WRITE XML DATA IF IT EXISTS
    QByteArray xmlByteArray = xml();
    if (xmlByteArray.length() > 0) {
        TIFFSetField(otTiff, TIFFTAG_XMLPACKET, xmlByteArray.length(), xmlByteArray.data());
    }

    // WRITE THE RFID TAG STRING TO THE PAGE NAME TAG
    if (rfid().isEmpty()) {
        TIFFSetField(otTiff, TIFFTAG_IMAGEDESCRIPTION, QString("not defined").toLatin1().data());
    } else {
        TIFFSetField(otTiff, TIFFTAG_IMAGEDESCRIPTION, rfid().toLatin1().data());
    }

    if (depth() == sizeof(float) || depth() == sizeof(double)) {
        // SEE IF WE HAVE TO TELL THE TIFF READER THAT WE ARE STORING
        // PIXELS IN 32-BIT FLOATING POINT FORMAT
        TIFFSetField(otTiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    }

    // MAKE TEMPORARY BUFFER TO HOLD CURRENT ROW BECAUSE COMPRESSION DESTROYS
    // WHATS EVER INSIDE THE BUFFER
    unsigned int cnt = 0;
    unsigned char *tempBuffer = (unsigned char *)malloc(step());
    for (unsigned int frm = 0; frm < frames(); frm++) {
        for (unsigned int row = 0; row < height(); row++) {
            memcpy(tempBuffer, constScanLine(row, frm), step());
            TIFFWriteScanline(otTiff, tempBuffer, cnt++, 0);
        }
    }
    free(tempBuffer);

    // WRITE THE CURRENT DIRECTORY AND PREPARE FOR THE NEW ONE
    TIFFWriteDirectory(otTiff);

    // WRITE THE ELAPSED TIME STAMP TO AN EXIF TAG
    if (elapsed() != 0){
        uint64_t dir_offset;
        TIFFCreateEXIFDirectory(otTiff);
        TIFFSetField(otTiff, EXIFTAG_SUBSECTIME, QString("%1").arg(elapsed()).toLatin1().data());
        TIFFWriteCustomDirectory(otTiff, &dir_offset);

        TIFFSetDirectory(otTiff, index);
        TIFFSetField(otTiff, TIFFTAG_EXIFIFD, dir_offset);
        TIFFRewriteDirectory(otTiff);
    }
    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUMemoryObject::load(TIFF *inTiff, int index)
{
    // LOAD INPUT TIFF FILE PARAMETERS IMPORTANT TO RESAMPLING THE IMAGE
    unsigned long uLongVariable;
    unsigned short uShortVariable;

    // SEE IF USER WANTS TO LOAD A PARTICULAR DIRECTORY WITHIN A MULTIDIRECTORY TIFF FILE
    if (index > -1) {
        TIFFSetDirectory(inTiff, (unsigned short)index);
    }

    // NUMBER OF FRAMES IS ALWAYS EQUAL TO ONE
    data->numFrms = 1;

    // GET THE HEIGHT AND WIDTH OF INPUT IMAGE IN PIXELS
    TIFFGetField(inTiff, TIFFTAG_IMAGEWIDTH, &uLongVariable);
    data->numCols = uLongVariable;
    TIFFGetField(inTiff, TIFFTAG_IMAGELENGTH, &uLongVariable);
    data->numRows = uLongVariable;
    TIFFGetField(inTiff, TIFFTAG_SAMPLESPERPIXEL, &uShortVariable);
    data->numChns = uShortVariable;
    TIFFGetField(inTiff, TIFFTAG_BITSPERSAMPLE, &uShortVariable);
    data->numByts = uShortVariable / 8;

    // READ AND CHECK THE PHOTOMETRIC INTERPRETATION FIELD AND MAKE SURE ITS WHAT WE EXPECT
    TIFFGetField(inTiff, TIFFTAG_PHOTOMETRIC, &uShortVariable);
    //if (uShortVariable != PHOTOMETRIC_SEPARATED && uShortVariable != PHOTOMETRIC_MINISBLACK) {
    //    return (false);
    //}

    // ALLOCATE SPACE TO HOLD IMAGE DATA
    data->allocateBuffer();

    // LOAD THE ANCHOR POINT
    float xPos = -1.0f, yPos = -1.0f;
    TIFFGetField(inTiff, TIFFTAG_XPOSITION, &xPos);
    TIFFGetField(inTiff, TIFFTAG_YPOSITION, &yPos);
    setConstAnchor(QPoint(qRound(xPos), qRound(yPos)));

    // READ DATA AS EITHER CHUNKY OR PLANAR FORMAT
    if (data->buffer) {
        short shortVariable;
        TIFFGetField(inTiff, TIFFTAG_PLANARCONFIG, &shortVariable);
        if (shortVariable == PLANARCONFIG_SEPARATE) {
            unsigned char *tempBuffer = new unsigned char [step()];
            for (unsigned int chn = 0; chn < colors(); chn++) {
                for (unsigned int row = 0; row < height(); row++) {
                    unsigned char *pBuffer = scanLine(row);
                    TIFFReadScanline(inTiff, tempBuffer, (int)row, (int)chn);
                    for (unsigned int col = 0; col < width(); col++) {
                        ((float *)pBuffer)[col * colors() + chn] = ((float *)tempBuffer)[col];
                    }
                }
            }
            delete [] tempBuffer;
        } else if (shortVariable == PLANARCONFIG_CONTIG) {
            for (unsigned int row = 0; row < height(); row++) {
                TIFFReadScanline(inTiff, (unsigned char *)scanLine(row), static_cast<unsigned int>(row));
            }
        }
    }

    // LOAD THE XML FIELD OF THE TIFF FILE, IF PROVIDED
    int dataLength;
    char *dataString = nullptr;
    bool dataPresent = TIFFGetField(inTiff, TIFFTAG_XMLPACKET, &dataLength, &dataString);
    if (dataPresent) {
        QByteArray byteArray(dataString, dataLength);
        int index = byteArray.lastIndexOf(QByteArray("\n"));
        if (index > -1) {
            setXML(byteArray.left(index + 1));
        }

        // PARSE THE XML LOOKING FOR A JETR VECTOR OF DOUBLES
        // LOAD THE XML FIELD OF THE TIFF FILE, IF PROVIDED
        QByteArray xmlByteArray = xml();

        QHash<QString, QString> hashTable = LAUMemoryObject::xmlToHash(xml());
        if (hashTable["jetrVector"].isNull() == false){
            QString jetrString = hashTable["jetrVector"];
            QStringList doubleList = jetrString.split(",");
            QVector<double> jetrVector(doubleList.count(), NAN);
            for (int n = 0; n < doubleList.count() && n < jetrVector.length(); n++){
                jetrVector[n] = doubleList.at(n).toDouble();
            }
            setConstJetr(jetrVector);
        }
    }

    // LOAD A CUSTOM FILESTRING FROM THE DOCUMENT NAME TIFFTAG
    dataString = nullptr;
    if (TIFFGetField(inTiff, TIFFTAG_IMAGEDESCRIPTION, &dataString)) {
        setConstRFID(QString(QByteArray(dataString)));
    } else {
        setConstRFID(QString());
    }

    // GET THE ELAPSED TIME VALUE FROM THE EXIF TAG FOR SUBSECOND TIME
    uint64_t directoryOffset;
    if (TIFFGetField(inTiff, TIFFTAG_EXIFIFD, &directoryOffset)) {
        char *byteArray;
        TIFFReadEXIFDirectory(inTiff, directoryOffset);
        if (TIFFGetField(inTiff, EXIFTAG_SUBSECTIME, &byteArray)) {
            setConstElapsed(QString(QByteArray(byteArray)).toUInt());
        }
    }

    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUMemoryObject::loadInto(QString filename, int index)
{
    // CREATE A RETURN VARIABLE AND INITIALIZE AS FALSE
    bool flag = false;

    // IF WE HAVE A VALID TIFF FILE, LOAD FROM DISK
    // OTHERWISE TRY TO CONNECT TO SCANNER
    if (QFile::exists(filename)) {
        // OPEN INPUT TIFF FILE FROM DISK
        TIFF *inTiff = TIFFOpen(filename.toLatin1(), "r");
        if (!inTiff) {
            return (false);
        }
        flag = loadInto(inTiff, index);
        TIFFClose(inTiff);
    }
    return (flag);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUMemoryObject::loadInto(TIFF *inTiff, int index)
{
    // LOAD INPUT TIFF FILE PARAMETERS IMPORTANT TO RESAMPLING THE IMAGE
    unsigned short uShortVariable;

    // SEE IF USER WANTS TO LOAD A PARTICULAR DIRECTORY WITHIN A MULTIDIRECTORY TIFF FILE
    if (index > -1) {
        TIFFSetDirectory(inTiff, (unsigned short)index);
    }

    // GET THE HEIGHT AND WIDTH OF INPUT IMAGE IN PIXELS
    TIFFGetField(inTiff, TIFFTAG_IMAGEWIDTH, &uShortVariable);
    if (data->numCols != uShortVariable) {
        return (false);
    }

    TIFFGetField(inTiff, TIFFTAG_IMAGELENGTH, &uShortVariable);
    if ((data->numFrms * data->numRows) != uShortVariable) {
        return (false);
    }

    TIFFGetField(inTiff, TIFFTAG_SAMPLESPERPIXEL, &uShortVariable);
    if (data->numChns != uShortVariable) {
        return (false);
    }

    TIFFGetField(inTiff, TIFFTAG_BITSPERSAMPLE, &uShortVariable);
    if (data->numByts != uShortVariable / 8) {
        return (false);
    }

    // READ AND CHECK THE PHOTOMETRIC INTERPRETATION FIELD AND MAKE SURE ITS WHAT WE EXPECT
    //TIFFGetField(inTiff, TIFFTAG_PHOTOMETRIC, &uShortVariable);
    //if (uShortVariable != PHOTOMETRIC_SEPARATED && uShortVariable != PHOTOMETRIC_MINISBLACK) {
    //    return (false);
    //}

    // LOAD THE ANCHOR POINT
    float xPos = -1.0f, yPos = -1.0f;
    TIFFGetField(inTiff, TIFFTAG_XPOSITION, &xPos);
    TIFFGetField(inTiff, TIFFTAG_YPOSITION, &yPos);
    setConstAnchor(QPoint(qRound(xPos), qRound(yPos)));

    // READ DATA AS EITHER CHUNKY OR PLANAR FORMAT
    if (data->buffer) {
        short shortVariable;
        TIFFGetField(inTiff, TIFFTAG_PLANARCONFIG, &shortVariable);
        if (shortVariable == PLANARCONFIG_SEPARATE) {
            unsigned char *tempBuffer = new unsigned char [step()];
            for (unsigned int chn = 0; chn < colors(); chn++) {
                for (unsigned int row = 0; row < height() * frames(); row++) {
                    unsigned char *pBuffer = scanLine(row);
                    TIFFReadScanline(inTiff, tempBuffer, (int)row, (int)chn);
                    for (unsigned int col = 0; col < width(); col++) {
                        ((float *)pBuffer)[col * colors() + chn] = ((float *)tempBuffer)[col];
                    }
                }
            }
            delete [] tempBuffer;
        } else if (shortVariable == PLANARCONFIG_CONTIG) {
            for (unsigned int row = 0; row < height() * frames(); row++) {
                TIFFReadScanline(inTiff, (unsigned char *)scanLine(row), static_cast<unsigned int>(row));
            }
        }
    }

    // LOAD THE XML FIELD OF THE TIFF FILE, IF PROVIDED
    int dataLength;
    char *dataString = nullptr;
    bool dataPresent = TIFFGetField(inTiff, TIFFTAG_XMLPACKET, &dataLength, &dataString);
    if (dataPresent) {
        QByteArray byteArray(dataString, dataLength);
        int index = byteArray.lastIndexOf(QByteArray("\n"));
        if (index > -1) {
            setXML(byteArray.left(index + 1));
        }

        // PARSE THE XML LOOKING FOR A JETR VECTOR OF DOUBLES
        // LOAD THE XML FIELD OF THE TIFF FILE, IF PROVIDED
        QByteArray xmlByteArray = xml();

        QHash<QString, QString> hashTable = LAUMemoryObject::xmlToHash(xml());
        if (hashTable["jetrVector"].isNull() == false){
            QString jetrString = hashTable["jetrVector"];
            QStringList doubleList = jetrString.split(",");
            if (doubleList.count() > 0){
                QVector<double> jetrVector(doubleList.count(), NAN);
                for (int n = 0; n < doubleList.count(); n++){
                    jetrVector[n] = doubleList.at(n).toDouble();
                }
                setConstJetr(jetrVector);
            }
        }
    }

    // LOAD A CUSTOM FILESTRING FROM THE DOCUMENT NAME TIFFTAG
    if (TIFFGetField(inTiff, TIFFTAG_IMAGEDESCRIPTION, &dataString)) {
        setConstRFID(QString(QByteArray(dataString)));
    } else {
        setConstRFID(QString());
    }

    // GET THE ELAPSED TIME VALUE FROM THE EXIF TAG FOR SUBSECOND TIME
    uint64_t directoryOffset;
    if (TIFFGetField(inTiff, TIFFTAG_EXIFIFD, &directoryOffset)) {
        char *byteArray;
        TIFFReadEXIFDirectory(inTiff, directoryOffset);
        if (TIFFGetField(inTiff, EXIFTAG_SUBSECTIME, &byteArray)) {
            setConstElapsed(QString(QByteArray(byteArray)).toUInt());
        }
    }

    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUMemoryObject::peakEnvelope(float dx, float dy, float radius) const
{
    // GET THE RADIUS IN TERMS OF PIXELS
    int pixelRadius = (int)qFloor(radius / dx);

    // CREATE A NEW COPY OF THIS MEMORY OBJECT
    LAUMemoryObject object = *this;
    object.triggerDeepCopy();

    // COPY METADATA
    object.setXML(xml());
    object.setRFID(rfid());
    object.setTransform(transform());
    object.setAnchor(anchor());
    object.setElapsed(elapsed());

    // FIGURE OUT HOW MANY COLORS WE NEED TO HANDLE
    if (colors() == 1){
        // SEE WHAT FORMAT OUR PXELS ARE
        if (depth() == sizeof(unsigned short)){
            // ITERATE THROUGH EACH FRAME OF VIDEO
            for (unsigned int frm = 0; frm < frames(); frm++){
                // DEFINE VERTICAL LINE AND THEN GET THE CORRESPONDING Y COORDINATE FOR EACH POINT
                // IN THE SCAN, SORTING THEM AS YOU CALCULATE THEM, AND THEN JUST KEEP THE TOP
                // Y COORDINATE AS THE CENTER OF THE CIRCLE. AND THEN GIVEN THE SET OF CIRCLES,
                // PROJECT EACH RADIUS ONTO THE VERTICAL AXIS AND SORT AS YOU
                // GO ALONG. TAKE THE FIRST RADIUS FROM THE BOTTOM.

                // ITERATURE THROUGH EACH ROW OF THE CURRENT VIDEO FRAME
                for (unsigned int row = 0; row < height(); row++){
                    // GRAB THE POINTER TO THE SCAN BUFFER THAT WE INTEND TO READ FROM
                    unsigned short *fmBuffer = (unsigned short*)constScanLine(row, frm);

                    // CREATE A VECTOR TO HOLD ALL THE CIRCLE CENTERS
                    QVector<float> yVector((int)width(), -1e6);

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (unsigned int col = 0; col < width(); col++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (fmBuffer[col] == 0 || fmBuffer[col] == 65535){
                            continue;
                        }

                        // GET THE SCAN COORDINATE OF THE CURRENT PIXEL
                        QPointF pointA((float)col*dx, (float)fmBuffer[col]*dy);

                        // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                        for (int deltaP = -pixelRadius; deltaP < pixelRadius; deltaP++){
                            // DEFINE THE COLUMN PRIME
                            unsigned int colP = qMin(width()-1, (unsigned int)qMax(0, (int)col + deltaP));

                            // GET THE LINE COORDINATE OF THE CURRENT COLUMN
                            float deltaX = (float)((int)colP - (int)col)*dx;

                            // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                            // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                            float yCoord = pointA.y() + qSqrt(radius * radius - deltaX * deltaX);

                            // SEE IF THIS Y COORDINATE IS THE HIGHEST SO FAR
                            yVector[colP] = qMax(yVector[colP], yCoord);
                        }
                    }

                    // NOW THAT WE HAVE A LIST OF CIRCLES, WE NEED TO PROJECT THEM BACK
                    // ONTO THE LINE SO LETS CREATE A BUFFER TO HOLD OUR RECONSTRUCTED SCAN
                    QVector<float> xVector((int)width(), +1e6);

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (unsigned int col = 0; col < width(); col++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (yVector[col] < -1e5){
                            continue;
                        }

                        // GET THE CENTER COORDINATE OF THE CURRENT CIRCLE
                        QPointF pointA((float)col*dx, yVector[col]);

                        // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                        for (int deltaP = -pixelRadius; deltaP < pixelRadius; deltaP++){
                            // DEFINE THE COLUMN PRIME
                            unsigned int colP = qMin(width()-1, (unsigned int)qMax(0, (int)col + deltaP));

                            // GET THE X COORDINATE OF THE CURRENT COLUMN
                            float deltaX = (float)((int)colP - (int)col)*dx;

                            // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                            // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                            float yCoord = pointA.y() - qSqrt(radius * radius - deltaX * deltaX);

                            // SEE IF THIS Y COORDINATE IS THE LOWEST SO FAR
                            xVector[colP] = qMin(xVector[colP], yCoord);
                        }
                    }

                    // GRAB A POINTER TO THE INPUT AND OUTPUT BUFFER ROWS
                    unsigned short *toBuffer = (unsigned short*)object.scanLine(row, frm);

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (unsigned int col = 0; col < width(); col++){
                        // CONVERT THE SCAN COORDINATE BACK TO A PIXEL VALUE
                        if (toBuffer[col] != 0 && toBuffer[col] != 65535){
                            toBuffer[col] = (unsigned short)qMax(0.0f, qMin(65535.0f, xVector[col] / dy));
                        }
                    }
                }
            }
        }
    }
    return(object);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMemoryObject::peakEnvelopeInPlace(float dx, float dy, float radius)
{
    // GET THE RADIUS IN TERMS OF PIXELS
    int pixelRadius = 0;

    // FIGURE OUT HOW MANY COLORS WE NEED TO HANDLE
    if (colors() == 1){
        // SEE WHAT FORMAT OUR PXELS ARE
        if (depth() == sizeof(unsigned short)){
            // ITERATE THROUGH EACH FRAME OF VIDEO
            for (unsigned int frm = 0; frm < frames(); frm++){
                // DEFINE VERTICAL LINE AND THEN GET THE CORRESPONDING Y COORDINATE FOR EACH POINT
                // IN THE SCAN, SORTING THEM AS YOU CALCULATE THEM, AND THEN JUST KEEP THE TOP
                // Y COORDINATE AS THE CENTER OF THE CIRCLE. AND THEN GIVEN THE SET OF CIRCLES,
                // PROJECT EACH RADIUS ONTO THE VERTICAL AXIS AND SORT AS YOU
                // GO ALONG. TAKE THE FIRST RADIUS FROM THE BOTTOM.

                // CREATE A VECTOR TO HOLD THE INPUT PIXELS DEPENDING ON ROWWISE, COLUMNWISE, OR DIAGONAL
                QVector<unsigned short> wVector;

                // CREATE A VECTOR TO HOLD ALL THE CIRCLE CENTERS
                QVector<float> yVector((int)width());
                QVector<float> xVector((int)width());

                // GET THE RADIUS IN TERMS OF PIXELS
                pixelRadius = (int)qFloor(radius / dx);

                // RESIZE THE W VECTOR TO HOLD EACH ROW OF THE INPUT IMAGE
                wVector.resize(width());

                // ITERATURE THROUGH EACH ROW OF THE CURRENT VIDEO FRAME
                for (unsigned int row = 0; row < height(); row++){
                    // GRAB THE POINTER TO THE SCAN BUFFER THAT WE INTEND TO READ FROM
                    unsigned short *fmBuffer = (unsigned short*)constScanLine(row, frm);

                    // COPY A ROW OF IMAGE PIXELS TO THE W VECTOR
                    for (unsigned int col = 0; col < width(); col++){
                        wVector[col] = fmBuffer[col];
                    }

                    // RESET OUR STORAGE VECTORS FOR A NEW ROW
                    yVector.fill(-1e6);
                    xVector.fill(+1e6);

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (wVector[ind] != 0 && wVector[ind] != 65535){
                            // GET THE SCAN COORDINATE OF THE CURRENT PIXEL
                            QPointF pointA((float)ind * dx, (float)wVector[ind] * dy);

                            // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                            for (int deltaP = -pixelRadius; deltaP <= pixelRadius; deltaP++){
                                // DEFINE THE COLUMN PRIME
                                unsigned int indP = (unsigned int)qMin(wVector.count()-1, qMax(0, (int)ind + deltaP));

                                // GET THE LINE COORDINATE OF THE CURRENT COLUMN
                                float deltaX = (float)((int)indP - (int)ind) * dx;

                                // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                                // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                                float yCoord = pointA.y() + qSqrt((radius * radius) - (deltaX * deltaX));

                                // SEE IF THIS Y COORDINATE IS THE HIGHEST SO FAR
                                yVector[indP] = qMax(yVector[indP], yCoord);
                            }
                        }
                    }

                    // NOW THAT WE HAVE A LIST OF CIRCLES, WE NEED TO PROJECT THEM BACK
                    // ONTO THE LINE SO LETS CREATE A BUFFER TO HOLD OUR RECONSTRUCTED SCAN
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (yVector[ind] > -1e5){
                            // GET THE CENTER COORDINATE OF THE CURRENT CIRCLE
                            QPointF pointA((float)ind * dx, yVector[ind]);

                            // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                            for (int deltaP = -pixelRadius; deltaP <= pixelRadius; deltaP++){
                                // DEFINE THE COLUMN PRIME
                                unsigned int indP = qMin(width()-1, (unsigned int)qMax(0, (int)ind + deltaP));

                                // GET THE X COORDINATE OF THE CURRENT COLUMN
                                float deltaX = (float)((int)indP - (int)ind) * dx;

                                // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                                // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                                float yCoord = pointA.y() - qSqrt((radius * radius) - (deltaX * deltaX));

                                // SEE IF THIS Y COORDINATE IS THE LOWEST SO FAR
                                xVector[indP] = qMin(xVector[indP], yCoord);
                            }
                        }
                    }

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // CONVERT THE SCAN COORDINATE BACK TO A PIXEL VALUE
                        if (wVector[ind] != 0 && wVector[ind] != 65535){
                            wVector[ind] = (unsigned short)qMax(0.0f, qMin(65535.0f, xVector[ind] / dy));
                        }
                    }

                    // COPY THE W VECTOR TO A ROW OF IMAGE PIXELS TO THE
                    for (unsigned int col = 0; col < width(); col++){
                        fmBuffer[col] = wVector[col];
                    }
                }

                // RESIZE THE W VECTOR TO HOLD EACH COLUMN OF THE INPUT IMAGE
                wVector.resize(height());

                // ITERATURE THROUGH EACH ROW OF THE CURRENT VIDEO FRAME
                for (unsigned int col = 0; col < width(); col++){
                    // GRAB THE POINTER TO THE SCAN BUFFER THAT WE INTEND TO READ FROM
                    unsigned short *fmBuffer = (unsigned short*)constScanLine(0, frm);

                    // COPY A COLUMN OF IMAGE PIXELS TO THE W VECTOR
                    for (unsigned int row = 0; row < height(); row++){
                        wVector[row] = fmBuffer[row * step() + col];
                    }

                    // RESET OUR STORAGE VECTORS FOR A NEW ROW
                    yVector.fill(-1e6);
                    xVector.fill(+1e6);

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (wVector[ind] != 0 && wVector[ind] != 65535){
                            // GET THE SCAN COORDINATE OF THE CURRENT PIXEL
                            QPointF pointA((float)ind * dx, (float)wVector[ind] * dy);

                            // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                            for (int deltaP = -pixelRadius; deltaP <= pixelRadius; deltaP++){
                                // DEFINE THE COLUMN PRIME
                                unsigned int indP = (unsigned int)qMin(wVector.count()-1, qMax(0, (int)ind + deltaP));

                                // GET THE LINE COORDINATE OF THE CURRENT COLUMN
                                float deltaX = (float)((int)indP - (int)ind) * dx;

                                // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                                // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                                float yCoord = pointA.y() + qSqrt((radius * radius) - (deltaX * deltaX));

                                // SEE IF THIS Y COORDINATE IS THE HIGHEST SO FAR
                                yVector[indP] = qMax(yVector[indP], yCoord);
                            }
                        }
                    }

                    // NOW THAT WE HAVE A LIST OF CIRCLES, WE NEED TO PROJECT THEM BACK
                    // ONTO THE LINE SO LETS CREATE A BUFFER TO HOLD OUR RECONSTRUCTED SCAN
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (yVector[ind] > -1e5){
                            // GET THE CENTER COORDINATE OF THE CURRENT CIRCLE
                            QPointF pointA((float)ind * dx, yVector[ind]);

                            // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                            for (int deltaP = -pixelRadius; deltaP <= pixelRadius; deltaP++){
                                // DEFINE THE COLUMN PRIME
                                unsigned int indP = qMin(width()-1, (unsigned int)qMax(0, (int)ind + deltaP));

                                // GET THE X COORDINATE OF THE CURRENT COLUMN
                                float deltaX = (float)((int)indP - (int)ind) * dx;

                                // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                                // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                                float yCoord = pointA.y() - qSqrt((radius * radius) - (deltaX * deltaX));

                                // SEE IF THIS Y COORDINATE IS THE LOWEST SO FAR
                                xVector[indP] = qMin(xVector[indP], yCoord);
                            }
                        }
                    }

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // CONVERT THE SCAN COORDINATE BACK TO A PIXEL VALUE
                        if (wVector[ind] != 0 && wVector[ind] != 65535){
                            wVector[ind] = (unsigned short)qMax(0.0f, qMin(65535.0f, xVector[ind] / dy));
                        }
                    }

                    // COPY THE W VECTOR TO A COLUMN OF IMAGE PIXELS TO THE
                    for (unsigned int row = 0; row < height(); row++){
                        fmBuffer[row * step() + col] = wVector[row];
                    }
                }
#ifdef DONTCOMPILE
                // GET THE RADIUS IN TERMS OF PIXELS
                pixelRadius = (int)qFloor(radius / (dx * qSqrt(2.0)));

                // RESIZE THE W VECTOR TO HOLD EACH DIAGONAL OF THE INPUT IMAGE
                wVector.resize(height());

                // ITERATURE THROUGH EACH ROW OF THE CURRENT VIDEO FRAME
                for (int col = -(int)height(); col < (int)width(); col++){
                    // RESET OUR STORAGE VECTORS FOR A NEW ROW
                    yVector.fill(-1e6);
                    xVector.fill(+1e6);

                    // COPY A DIAGONAL OF IMAGE PIXELS TO THE W VECTOR
                    for (int row = 0; row < (int)height(); row++){
                        int pixelRow = row;
                        int pixelCol = col + row;

                        if (pixelCol >= 0 && pixelCol < (int)width()){
                            wVector[row] = fmBuffer[pixelRow * (int)step() + pixelCol];
                        } else {
                            wVector[row] = 65535;
                        }
                    }

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (wVector[ind] != 0 && wVector[ind] != 65535){
                            // GET THE SCAN COORDINATE OF THE CURRENT PIXEL
                            QPointF pointA((float)ind * dx * qSqrt(2.0), (float)wVector[ind] * dy);

                            // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                            for (int deltaP = -pixelRadius; deltaP <= pixelRadius; deltaP++){
                                // DEFINE THE COLUMN PRIME
                                unsigned int indP = (unsigned int)qMin(wVector.count()-1, qMax(0, (int)ind + deltaP));

                                // GET THE LINE COORDINATE OF THE CURRENT COLUMN
                                float deltaX = (float)((int)indP - (int)ind) * dx * qSqrt(2.0);

                                // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                                // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                                float yCoord = pointA.y() + qSqrt((radius * radius) - (deltaX * deltaX));

                                // SEE IF THIS Y COORDINATE IS THE HIGHEST SO FAR
                                yVector[indP] = qMax(yVector[indP], yCoord);
                            }
                        }
                    }

                    // NOW THAT WE HAVE A LIST OF CIRCLES, WE NEED TO PROJECT THEM BACK
                    // ONTO THE LINE SO LETS CREATE A BUFFER TO HOLD OUR RECONSTRUCTED SCAN
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (yVector[ind] > -1e5){
                            // GET THE CENTER COORDINATE OF THE CURRENT CIRCLE
                            QPointF pointA((float)ind * dx * qSqrt(2.0), yVector[ind]);

                            // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                            for (int deltaP = -pixelRadius; deltaP <= pixelRadius; deltaP++){
                                // DEFINE THE COLUMN PRIME
                                unsigned int indP = qMin(width()-1, (unsigned int)qMax(0, (int)ind + deltaP));

                                // GET THE X COORDINATE OF THE CURRENT COLUMN
                                float deltaX = (float)((int)indP - (int)ind) * dx * qSqrt(2.0);

                                // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                                // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                                float yCoord = pointA.y() - qSqrt((radius * radius) - (deltaX * deltaX));

                                // SEE IF THIS Y COORDINATE IS THE LOWEST SO FAR
                                xVector[indP] = qMin(xVector[indP], yCoord);
                            }
                        }
                    }

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // CONVERT THE SCAN COORDINATE BACK TO A PIXEL VALUE
                        if (wVector[ind] != 0 && wVector[ind] != 65535){
                            wVector[ind] = (unsigned short)qMax(0.0f, qMin(65535.0f, xVector[ind] / dy));
                        }
                    }

                    // COPY THE W VECTOR TO A DIAGONAL OF IMAGE PIXELS
                    for (int row = 0; row < (int)height(); row++){
                        int pixelRow = row;
                        int pixelCol = col + row;

                        if (pixelCol >= 0 && pixelCol < (int)width()){
                            fmBuffer[pixelRow * (int)step() + pixelCol] = wVector[row];
                        }
                    }
                }

                // ITERATURE THROUGH EACH ROW OF THE CURRENT VIDEO FRAME
                for (int col = 0; col < (int)(width() + height()); col++){
                    // RESET OUR STORAGE VECTORS FOR A NEW ROW
                    yVector.fill(-1e6);
                    xVector.fill(+1e6);

                    // COPY A DIAGONAL OF IMAGE PIXELS TO THE W VECTOR
                    for (int row = 0; row < (int)height(); row++){
                        int pixelRow = row;
                        int pixelCol = col - row;

                        if (pixelCol >= 0 && pixelCol < (int)width()){
                            wVector[row] = fmBuffer[pixelRow * (int)step() + pixelCol];
                        } else {
                            wVector[row] = 65535;
                        }
                    }

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (wVector[ind] != 0 && wVector[ind] != 65535){
                            // GET THE SCAN COORDINATE OF THE CURRENT PIXEL
                            QPointF pointA((float)ind * dx * qSqrt(2.0), (float)wVector[ind] * dy);

                            // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                            for (int deltaP = -pixelRadius; deltaP <= pixelRadius; deltaP++){
                                // DEFINE THE COLUMN PRIME
                                unsigned int indP = (unsigned int)qMin(wVector.count()-1, qMax(0, (int)ind + deltaP));

                                // GET THE LINE COORDINATE OF THE CURRENT COLUMN
                                float deltaX = (float)((int)indP - (int)ind) * dx * qSqrt(2.0);

                                // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                                // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                                float yCoord = pointA.y() + qSqrt((radius * radius) - (deltaX * deltaX));

                                // SEE IF THIS Y COORDINATE IS THE HIGHEST SO FAR
                                yVector[indP] = qMax(yVector[indP], yCoord);
                            }
                        }
                    }

                    // NOW THAT WE HAVE A LIST OF CIRCLES, WE NEED TO PROJECT THEM BACK
                    // ONTO THE LINE SO LETS CREATE A BUFFER TO HOLD OUR RECONSTRUCTED SCAN
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (yVector[ind] > -1e5){
                            // GET THE CENTER COORDINATE OF THE CURRENT CIRCLE
                            QPointF pointA((float)ind * dx * qSqrt(2.0), yVector[ind]);

                            // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                            for (int deltaP = -pixelRadius; deltaP <= pixelRadius; deltaP++){
                                // DEFINE THE COLUMN PRIME
                                unsigned int indP = qMin(width()-1, (unsigned int)qMax(0, (int)ind + deltaP));

                                // GET THE X COORDINATE OF THE CURRENT COLUMN
                                float deltaX = (float)((int)indP - (int)ind) * dx * qSqrt(2.0);

                                // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                                // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                                float yCoord = pointA.y() - qSqrt((radius * radius) - (deltaX * deltaX));

                                // SEE IF THIS Y COORDINATE IS THE LOWEST SO FAR
                                xVector[indP] = qMin(xVector[indP], yCoord);
                            }
                        }
                    }

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (int ind = 0; ind < wVector.count(); ind++){
                        // CONVERT THE SCAN COORDINATE BACK TO A PIXEL VALUE
                        if (wVector[ind] != 0 && wVector[ind] != 65535){
                            wVector[ind] = (unsigned short)qMax(0.0f, qMin(65535.0f, xVector[ind] / dy));
                        }
                    }

                    // COPY THE W VECTOR TO A DIAGONAL OF IMAGE PIXELS
                    for (int row = 0; row < (int)height(); row++){
                        int pixelRow = row;
                        int pixelCol = col - row;

                        if (pixelCol >= 0 && pixelCol < (int)width()){
                            fmBuffer[pixelRow * (int)step() + pixelCol] = wVector[row];
                        }
                    }
                }
#endif
            }
        }
    }
#ifdef DONTCOMPILE
                    // RESET OUR STORAGE VECTORS FOR A NEW ROW
                    yVector.fill(-1e6);
                    xVector.fill(+1e6);


                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (unsigned int col = 0; col < width(); col++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (fmBuffer[col] != 0 && fmBuffer[col] != 65535){
                            // GET THE SCAN COORDINATE OF THE CURRENT PIXEL
                            QPointF pointA((float)col * dx, (float)fmBuffer[col] * dy);

                            // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                            for (int deltaP = -pixelRadius; deltaP <= pixelRadius; deltaP++){
                                // DEFINE THE COLUMN PRIME
                                unsigned int colP = qMin(width()-1, (unsigned int)qMax(0, (int)col + deltaP));

                                // GET THE LINE COORDINATE OF THE CURRENT COLUMN
                                float deltaX = (float)((int)colP - (int)col) * dx;

                                // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                                // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                                float yCoord = pointA.y() + qSqrt((radius * radius) - (deltaX * deltaX));

                                // SEE IF THIS Y COORDINATE IS THE HIGHEST SO FAR
                                yVector[colP] = qMax(yVector[colP], yCoord);
                            }
                        }
                    }

                    // NOW THAT WE HAVE A LIST OF CIRCLES, WE NEED TO PROJECT THEM BACK
                    // ONTO THE LINE SO LETS CREATE A BUFFER TO HOLD OUR RECONSTRUCTED SCAN
                    for (unsigned int col = 0; col < width(); col++){
                        // SEE IF WE HAVE A VALID PIXEL
                        if (yVector[col] > -1e5){
                            // GET THE CENTER COORDINATE OF THE CURRENT CIRCLE
                            QPointF pointA((float)col * dx, yVector[col]);

                            // ONLY ITERATE THROUGH PIXELS WITHIN REACH OF THE RADIUS
                            for (int deltaP = -pixelRadius; deltaP <= pixelRadius; deltaP++){
                                // DEFINE THE COLUMN PRIME
                                unsigned int colP = qMin(width()-1, (unsigned int)qMax(0, (int)col + deltaP));

                                // GET THE X COORDINATE OF THE CURRENT COLUMN
                                float deltaX = (float)((int)colP - (int)col) * dx;

                                // FIND THE POINT ON THE LINE THAT IS THE CENTER OF
                                // THE CIRCLE GOING THROUGH OUR CURRENT SCAN POINT
                                float yCoord = pointA.y() - qSqrt((radius * radius) - (deltaX * deltaX));

                                // SEE IF THIS Y COORDINATE IS THE LOWEST SO FAR
                                xVector[colP] = qMin(xVector[colP], yCoord);
                            }
                        }
                    }

                    // ITERATE COLUMN BY COLUMN FROM LEFT TO RIGHT
                    for (unsigned int col = 0; col < width(); col++){
                        // CONVERT THE SCAN COORDINATE BACK TO A PIXEL VALUE
                        if (fmBuffer[col] != 0 && fmBuffer[col] != 65535){
                            fmBuffer[col] = (unsigned short)qMax(0.0f, qMin(65535.0f, xVector[col] / dy));
                        }
                    }
                }
            }
        }
    }
#endif
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUMemoryObject::getFrame(int frm) const
{
    // CREATE MEMORY OBJECT TO RETURN TO USER
    LAUMemoryObject object(width(), height(), colors(), depth(), 1);
    memcpy(object.constPointer(), constFrame(frm), object.length());

    // COPY METADATA
    object.setXML(xml());
    object.setRFID(rfid());
    object.setTransform(transform());
    object.setAnchor(anchor());
    object.setElapsed(elapsed());

    return(object);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUMemoryObject::crop(QRect rect) const
{
    // CREATE MEMORY OBJECT TO RETURN TO USER
    LAUMemoryObject object(rect.width(), rect.height(), colors(), depth(), frames());

    // COPY METADATA
    object.setXML(xml());
    object.setRFID(rfid());
    object.setXML(xml());
    object.setTransform(transform());
    object.setAnchor(anchor());
    object.setElapsed(elapsed());

    // ITERATE THROUGH EVERY PIXEL IN THE USER SUPPLIED RECTANGLE
    // MAKING SURE WE STAY IN THE BOUNDS OF THE SOURCE AND DESTINATION IMAGES
    for (unsigned int row = 0; row < (unsigned int)rect.height() && row < height(); row++) {
        for (unsigned int col = 0; col < (unsigned int)rect.width() && col < width(); col++) {
            // CALCULATE COORDINATE INSIDE THE SOURCE IMAGE
            int rowP = (int)row + rect.top();
            int colP = (int)col + rect.left();

            // MAKE SURE THIS PIXEL IS INSIDE THE SOURCE IMAGE OTHERWISE SET THE PIXEL TO ZERO
            if (rowP >= 0 && rowP < (int)height() && colP >= 0 && colP < (int)width()) {
                for (unsigned int frm = 0; frm < frames(); frm++) {
                    memcpy(object.pixel(col, row, frm), constPixel(colP, rowP, frm), nugget());
                }
            } else {
                if (rowP >= 0 && rowP < (int)height() && colP >= 0 && colP < (int)width()) {
                    for (unsigned int frm = 0; frm < frames(); frm++) {
                        memset(object.pixel(col, row, frm), 0x00, nugget());
                    }
                }
            }
        }
    }
    return (object);
}

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QImage LAUMemoryObject::toImage(int frame) const
{
    QImage image;
    if (colors() == 1) {
        if (depth() == sizeof(unsigned char)) {
            image = QImage(width(), height(), QImage::Format_Grayscale8);
            for (int row = 0; row < image.height(); row++) {
                memcpy((void *)image.constScanLine(row), constScanLine(row, frame), step());
            }
        } else if (depth() == sizeof(unsigned short)) {
            image = QImage(width(), height(), QImage::Format_Grayscale16);
            for (int row = 0; row < image.height(); row++) {
                memcpy((void *)image.constScanLine(row), constScanLine(row, frame), step());
            }
        } else if (depth() == sizeof(float)) {
            image = QImage(width(), height(), QImage::Format_Grayscale8);
            for (unsigned int row = 0; row < height(); row++){
                float* fmBuffer = (float*)constScanLine(row, frame);
                unsigned char *toBuffer = image.scanLine(row);
                for (unsigned int col = 0; col < width(); col++){
                    toBuffer[col] = (unsigned char)qRound(fmBuffer[col] * 255.0);
                }
            }
        }
    } else if (colors() == 3) {
        if (depth() == sizeof(unsigned char)) {
            image = QImage(width(), height(), QImage::Format_RGB888);
            for (int row = 0; row < image.height(); row++) {
                memcpy((void *)image.constScanLine(row), constScanLine(row, frame), step());
            }
        } else if (depth() == sizeof(unsigned short)) {
            image = QImage(width(), height(), QImage::Format_RGB888);
            for (unsigned int row = 0; row < height(); row++){
                unsigned short* fmBuffer = (unsigned short*)constScanLine(row, frame);
                unsigned char *toBuffer = image.scanLine(row);
                for (unsigned int col = 0; col < width(); col++){
                    toBuffer[3 * col + 0] = (unsigned char)((fmBuffer[3 * col + 0] >> 8) & 0x00ff);
                    toBuffer[3 * col + 1] = (unsigned char)((fmBuffer[3 * col + 1] >> 8) & 0x00ff);
                    toBuffer[3 * col + 2] = (unsigned char)((fmBuffer[3 * col + 2] >> 8) & 0x00ff);
                }
            }
        } else if (depth() == sizeof(float)) {
            image = QImage(width(), height(), QImage::Format_RGB888);
            for (unsigned int row = 0; row < height(); row++){
                float* fmBuffer = (float*)constScanLine(row, frame);
                unsigned char *toBuffer = image.scanLine(row);
                for (unsigned int col = 0; col < width(); col++){
                    toBuffer[3 * col + 0] = (unsigned char)qRound(fmBuffer[3 * col + 0] * 255.0);
                    toBuffer[3 * col + 1] = (unsigned char)qRound(fmBuffer[3 * col + 1] * 255.0);
                    toBuffer[3 * col + 2] = (unsigned char)qRound(fmBuffer[3 * col + 2] * 255.0);
                }
            }
        }
    } else if (colors() == 4) {
        if (depth() == sizeof(unsigned char)) {
            image = QImage(width(), height(), QImage::Format_ARGB32);
            for (int row = 0; row < image.height(); row++) {
                memcpy((void *)image.constScanLine(row), constScanLine(row, frame), step());
            }
        } else if (depth() == sizeof(unsigned short)) {
            image = QImage(width(), height(), QImage::Format_ARGB32);
            for (unsigned int row = 0; row < height(); row++){
                unsigned short* fmBuffer = (unsigned short*)constScanLine(row, frame);
                unsigned char *toBuffer = image.scanLine(row);
                for (unsigned int col = 0; col < width(); col++){
                    toBuffer[4 * col + 0] = (unsigned char)((fmBuffer[4 * col + 0] >> 8) & 0x00ff);
                    toBuffer[4 * col + 1] = (unsigned char)((fmBuffer[4 * col + 1] >> 8) & 0x00ff);
                    toBuffer[4 * col + 2] = (unsigned char)((fmBuffer[4 * col + 2] >> 8) & 0x00ff);
                    toBuffer[4 * col + 3] = (unsigned char)((fmBuffer[4 * col + 3] >> 8) & 0x00ff);
                }
            }
        } else if (depth() == sizeof(float)) {
            image = QImage(width(), height(), QImage::Format_ARGB32);
            for (unsigned int row = 0; row < height(); row++){
                float* fmBuffer = (float*)constScanLine(row, frame);
                unsigned char *toBuffer = image.scanLine(row);
                for (unsigned int col = 0; col < width(); col++){
                    toBuffer[4 * col + 0] = (unsigned char)qRound(fmBuffer[4 * col + 0] * 255.0);
                    toBuffer[4 * col + 1] = (unsigned char)qRound(fmBuffer[4 * col + 1] * 255.0);
                    toBuffer[4 * col + 2] = (unsigned char)qRound(fmBuffer[4 * col + 2] * 255.0);
                    toBuffer[4 * col + 3] = (unsigned char)qRound(fmBuffer[4 * col + 3] * 255.0);
                }
            }
        }
    } else if (colors() == 6) {
        if (depth() == sizeof(unsigned char)) {
            image = QImage(width(), height(), QImage::Format_RGB888);
            for (unsigned int row = 0; row < height(); row++){
                unsigned char *fmBuffer = image.scanLine(row);
                unsigned char *toBuffer = image.scanLine(row);
                for (unsigned int col = 0; col < width(); col++){
                    toBuffer[3 * col + 0] = fmBuffer[6 * col + 3];
                    toBuffer[3 * col + 1] = fmBuffer[6 * col + 4];
                    toBuffer[3 * col + 2] = fmBuffer[6 * col + 5];
                }
            }
        } else if (depth() == sizeof(unsigned short)) {
            image = QImage(width(), height(), QImage::Format_RGB888);
            for (unsigned int row = 0; row < height(); row++){
                unsigned short* fmBuffer = (unsigned short*)constScanLine(row, frame);
                unsigned char *toBuffer = image.scanLine(row);
                for (unsigned int col = 0; col < width(); col++){
                    toBuffer[3 * col + 0] = (unsigned char)((fmBuffer[6 * col + 3] >> 8) & 0x00ff);
                    toBuffer[3 * col + 1] = (unsigned char)((fmBuffer[6 * col + 4] >> 8) & 0x00ff);
                    toBuffer[3 * col + 2] = (unsigned char)((fmBuffer[6 * col + 5] >> 8) & 0x00ff);
                }
            }
        } else if (depth() == sizeof(float)) {
            image = QImage(width(), height(), QImage::Format_RGB888);
            for (unsigned int row = 0; row < height(); row++){
                float* fmBuffer = (float*)constScanLine(row, frame);
                unsigned char *toBuffer = image.scanLine(row);
                for (unsigned int col = 0; col < width(); col++){
                    toBuffer[3 * col + 0] = (unsigned char)qRound(fmBuffer[6 * col + 3] * 255.0);
                    toBuffer[3 * col + 1] = (unsigned char)qRound(fmBuffer[6 * col + 4] * 255.0);
                    toBuffer[3 * col + 2] = (unsigned char)qRound(fmBuffer[6 * col + 5] * 255.0);
                }
            }
        }
    } else if (colors() == 8) {
        if (depth() == sizeof(unsigned char)) {
            image = QImage(width(), height(), QImage::Format_ARGB32);
            for (unsigned int row = 0; row < height(); row++){
                unsigned char *fmBuffer = image.scanLine(row);
                unsigned char *toBuffer = image.scanLine(row);
                for (unsigned int col = 0; col < width(); col++){
                    toBuffer[4 * col + 0] = fmBuffer[8 * col + 4];
                    toBuffer[4 * col + 1] = fmBuffer[8 * col + 5];
                    toBuffer[4 * col + 2] = fmBuffer[8 * col + 6];
                    toBuffer[4 * col + 3] = fmBuffer[8 * col + 7];
                }
            }
        } else if (depth() == sizeof(unsigned short)) {
            image = QImage(width(), height(), QImage::Format_ARGB32);
            for (unsigned int row = 0; row < height(); row++){
                unsigned short* fmBuffer = (unsigned short*)constScanLine(row, frame);
                unsigned char *toBuffer = image.scanLine(row);
                for (unsigned int col = 0; col < width(); col++){
                    toBuffer[4 * col + 0] = (unsigned char)((fmBuffer[8 * col + 4] >> 8) & 0x00ff);
                    toBuffer[4 * col + 1] = (unsigned char)((fmBuffer[8 * col + 5] >> 8) & 0x00ff);
                    toBuffer[4 * col + 2] = (unsigned char)((fmBuffer[8 * col + 6] >> 8) & 0x00ff);
                    toBuffer[4 * col + 3] = (unsigned char)((fmBuffer[8 * col + 7] >> 8) & 0x00ff);
                }
            }
        } else if (depth() == sizeof(float)) {
            image = QImage(width(), height(), QImage::Format_ARGB32);
            for (unsigned int row = 0; row < height(); row++){
                float* fmBuffer = (float*)constScanLine(row, frame);
                unsigned char *toBuffer = image.scanLine(row);
                for (unsigned int col = 0; col < width(); col++){
                    toBuffer[4 * col + 0] = (unsigned char)qRound(fmBuffer[8 * col + 4] * 255.0);
                    toBuffer[4 * col + 1] = (unsigned char)qRound(fmBuffer[8 * col + 5] * 255.0);
                    toBuffer[4 * col + 2] = (unsigned char)qRound(fmBuffer[8 * col + 6] * 255.0);
                    toBuffer[4 * col + 3] = (unsigned char)qRound(fmBuffer[8 * col + 7] * 255.0);
                }
            }
        }
    }
    return (image);
}
#endif
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
unsigned int LAUMemoryObject::nonZeroPixelsCount(int chn) const
{
    // CREATE A REGISTER TO HOLD THE PIXEL COUNT
    unsigned int pixels = 0;

    // CREATE A VECTOR TO HOLD THE ACCUMULATED SUM OF PIXELS
    __m128i acSum = _mm_set1_epi32(0);

    if (depth() == sizeof(unsigned char)) {
        // CREATE A ZERO VECTOR FOR THE COMPARE OPERATION
        __m128i zeros = _mm_set1_epi8(0);

        // GRAB THE POINTER TO THE MEMORY OBJECT DATA
        unsigned char *buffer = (unsigned char *)constFrame(chn % frames());

        // ITERATE THROUGH THE BUFFER 16 BYTES AT A TIME
        for (unsigned int n = 0; n < length(); n += 16) {
            __m128i pixels = _mm_cmpeq_epi8(_mm_load_si128((const __m128i *)(buffer + n)), zeros);

            // HORIZONTAL SUM THE BYTES INTO 64-BIT INTEGERS AND
            __m128i vecL = _mm_sad_epu8(pixels, zeros);

            // ACCUMULATE THE SUM OF THE VECTORS TO FORM A SUM OF INTS
            acSum = _mm_add_epi32(acSum, vecL);
        }
        acSum = _mm_hadd_epi32(acSum, acSum);
        acSum = _mm_hadd_epi32(acSum, acSum);

        // EXTRACT THE INTEGER AND DIVIDE BY 255 TO GET UNITS OF PIXELS
        pixels = _mm_extract_epi32(acSum, 0) / 255;
    } else if (depth() == sizeof(unsigned short)) {
        // CREATE A ZERO VECTOR FOR THE COMPARE OPERATION
        __m128i zeros = _mm_set1_epi16(0);

        // GRAB THE POINTER TO THE MEMORY OBJECT DATA
        unsigned char *buffer = (unsigned char *)constFrame(chn % frames());

        // ITERATE THROUGH THE BUFFER 16 BYTES AT A TIME
        for (unsigned int n = 0; n < length(); n += 16) {
            __m128i pixels = _mm_cmpeq_epi16(_mm_load_si128((const __m128i *)(buffer + n)), zeros);

            // UNPACK FROM UNSIGNED SHORTS TO UNSIGNED INTS
            __m128i vecL = _mm_hadds_epi16(pixels, zeros);
            __m128i vecH = _mm_cvtepi16_epi32(vecL);

            // ACCUMULATE THE SUM OF THE VECTORS TO FORM A SUM OF INTS
            acSum = _mm_add_epi32(acSum, vecH);
        }
        acSum = _mm_hadd_epi32(acSum, acSum);
        acSum = _mm_hadd_epi32(acSum, acSum);

        // EXTRACT THE INTEGER AND DIVIDE BY 65535 TO GET UNITS OF PIXELS
        pixels = (unsigned int)(-1 * _mm_extract_epi32(acSum, 0));
    } else if (depth() == sizeof(float)) {
        // CREATE A ZERO VECTOR FOR THE COMPARE OPERATION
        __m128 zeros = _mm_set1_ps(0.0f);

        // GRAB THE POINTER TO THE MEMORY OBJECT DATA
        unsigned char *buffer = (unsigned char *)constFrame(chn % frames());

        // ITERATE THROUGH THE BUFFER 16 BYTES AT A TIME
        for (unsigned int n = 0; n < length(); n += 16) {
            __m128i pixels = _mm_castps_si128(_mm_cmpeq_ps(_mm_load_ps((const float *)(buffer + n)), zeros));

            // ACCUMULATE THE SUM OF THE VECTORS TO FORM A SUM OF INTS
            acSum = _mm_add_epi32(acSum, pixels);
        }
        acSum = _mm_hadd_epi32(acSum, acSum);
        acSum = _mm_hadd_epi32(acSum, acSum);

        // EXTRACT THE INTEGER AND DIVIDE BY 65535 TO GET UNITS OF PIXELS
        pixels = (unsigned int)(-1 * _mm_extract_epi32(acSum, 0));
    }

    // AT THIS POINT, THE SUM OF ZEROS RESULTS IN ADDING -1S TOGETHER
    // SO WE JUST NEED TO ADD THE NUMBER OF PIXELS TO GET THE NUMBER
    // OF NON-ZERO PIXELS IN THE BUFFER
    pixels = (unsigned int)((int)(width() * height() * colors()) - (int)pixels);

    // RETURN THE NUMBER OF NON-ZERO PIXELS
    return (pixels);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUMemoryObject::rotate()
{
    LAUMemoryObject object(height(), width(), colors(), depth(), frames());

    // COPY METADATA
    object.setXML(xml());
    object.setRFID(rfid());
    object.setTransform(transform());
    object.setAnchor(anchor());
    object.setElapsed(elapsed());

    for (unsigned int frm = 0; frm < frames(); frm++) {
        for (unsigned int row = 0; row < height(); row++) {
            for (unsigned int col = 0; col < width(); col++) {
                memcpy(object.pixel(height() - row - 1, col, frm), constPixel(col, row, frm), nugget());
            }
        }
    }
    return (object);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUMemoryObject::toFloat()
{
    LAUMemoryObject object(width(), height(), colors(), sizeof(float), frames());

    // COPY METADATA
    object.setXML(xml());
    object.setRFID(rfid());
    object.setTransform(transform());
    object.setAnchor(anchor());
    object.setElapsed(elapsed());

    unsigned char *fmBuffer = (unsigned char *)constPointer();
    unsigned char *toBuffer = (unsigned char *)object.constPointer();

    if (depth() == sizeof(unsigned char)) {
        __m128i vecA = _mm_set_epi8(-1, -1, -1, 3, -1, -1, -1, 2, -1, -1, -1, 1, -1, -1, -1, 0);
        __m128i vecB = _mm_set_epi8(-1, -1, -1, 7, -1, -1, -1, 6, -1, -1, -1, 5, -1, -1, -1, 4);
        __m128i vecC = _mm_set_epi8(-1, -1, -1, 11, -1, -1, -1, 10, -1, -1, -1, 9, -1, -1, -1, 8);
        __m128i vecD = _mm_set_epi8(-1, -1, -1, 15, -1, -1, -1, 14, -1, -1, -1, 13, -1, -1, -1, 12);
        __m128 vecE = _mm_set1_ps(1.0f / 255.0f);
        for (unsigned long long pix = 0; pix < length(); pix += 16) {
            __m128i inVec = _mm_loadu_si128((const __m128i *)&fmBuffer[pix]);
            _mm_store_ps((float *)&toBuffer[4 * pix + 0], _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(inVec, vecA)), vecE));
            _mm_store_ps((float *)&toBuffer[4 * pix + 16], _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(inVec, vecB)), vecE));
            _mm_store_ps((float *)&toBuffer[4 * pix + 32], _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(inVec, vecC)), vecE));
            _mm_store_ps((float *)&toBuffer[4 * pix + 48], _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(inVec, vecD)), vecE));
        }
    } else if (depth() == sizeof(unsigned short)) {
        __m128i vecA = _mm_set_epi8(-1, -1, 7, 6, -1, -1, 5, 4, -1, -1, 3, 2, -1, -1, 1, 0);
        __m128i vecB = _mm_set_epi8(-1, -1, 15, 14, -1, -1, 13, 12, -1, -1, 11, 10, -1, -1, 9, 8);
        __m128 vecC = _mm_set1_ps(1.0f / 65535.0f);
        for (unsigned long long pix = 0; pix < length(); pix += 16) {
            __m128i inVec = _mm_loadu_si128((const __m128i *)&fmBuffer[pix]);
            _mm_store_ps((float *)&toBuffer[2 * pix + 0], _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(inVec, vecA)), vecC));
            _mm_store_ps((float *)&toBuffer[2 * pix + 16], _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(inVec, vecB)), vecC));
        }
    } else if (depth() == sizeof(float)) {
        memcpy(toBuffer, fmBuffer, length());
    }

    return (object);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QHash<QString, QString> LAUMemoryObject::xmlToHash(QByteArray byteArray)
{
    // CREATE HASH TABLE TO RETURN TO USER
    QHash<QString, QString> hashTable;

    // LOAD THE XML FIELD OF THE TIFF FILE, IF PROVIDED
    if (byteArray.isEmpty() == false){
        QXmlStreamReader reader(byteArray);

        // IGNORE THE FIRST TEXT ELEMENT
        if (reader.readNextStartElement()) {
            while (!reader.atEnd()) {
                if (reader.readNextStartElement()) {
                    QString nameText = reader.name().toString();
                    if (nameText.isEmpty() == false){
                        QString elementText = reader.readElementText();
                        if (elementText.isEmpty() == false){
                            hashTable[nameText] = elementText;
                        }
                    }
                }
            }
        }
        reader.clear();
    }

    // RETURN THE HASH TABLE TO THE USER
    return(hashTable);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(QString filename)
{
    // GET A FILE TO OPEN FROM THE USER IF NOT ALREADY PROVIDED ONE
    if (filename.isNull()) {
        return (-1);
    }

    // IF WE HAVE A VALID TIFF FILE, LOAD FROM DISK
    // OTHERWISE TRY TO CONNECT TO SCANNER
    if (QFile::exists(filename)) {
        // OPEN INPUT TIFF FILE FROM DISK
        TIFF *inTiff = TIFFOpen(filename.toLatin1(), "r");
        if (!inTiff) {
            return (-2);
        }
        int numDirectories = TIFFNumberOfDirectories(inTiff);
        TIFFClose(inTiff);
        return (numDirectories);
    }
    return (-3);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAUMemoryObject::howManyChannelsDoesThisTiffFileHave(QString filename, int frame)
{
    // CREATE A VARIABLE TO HOLD THE RETURN VALUE
    int numChannels = -1;

    // MAKE SURE WE HAVE A VALID FILE TO OPEN
    if (filename.isNull() == false && QFile::exists(filename)) {
        // OPEN INPUT TIFF FILE FROM DISK
        TIFF *inTiff = TIFFOpen(filename.toLatin1(), "r");
        if (!inTiff) {
            numChannels = -2;
        } else {
            int numDirectories = TIFFNumberOfDirectories(inTiff);
            if (frame < 0) {
                numChannels = -3;
            } else if (frame >= numDirectories) {
                numChannels = -4;
            } else {
                // LOAD INPUT TIFF FILE PARAMETERS IMPORTANT TO RESAMPLING THE IMAGE
                unsigned short uShortVariable;

                // SET THE DIRECTORY TO THE USER SUPPLIED FRAME
                TIFFSetDirectory(inTiff, (unsigned short)frame);
                TIFFGetField(inTiff, TIFFTAG_SAMPLESPERPIXEL, &uShortVariable);

                // COPY THE NUMBER OF CHANNELS OVER TO THE RETURN VARIABLE
                numChannels = (int)uShortVariable;
            }
            TIFFClose(inTiff);
        }
    }
    return (numChannels);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAUMemoryObject::howManyColumnsDoesThisTiffFileHave(QString filename, int frame)
{
    // CREATE A VARIABLE TO HOLD THE RETURN VALUE
    int numColumns = -1;

    // MAKE SURE WE HAVE A VALID FILE TO OPEN
    if (filename.isNull() == false && QFile::exists(filename)) {
        // OPEN INPUT TIFF FILE FROM DISK
        TIFF *inTiff = TIFFOpen(filename.toLatin1(), "r");
        if (!inTiff) {
            numColumns = -2;
        } else {
            int numDirectories = TIFFNumberOfDirectories(inTiff);
            if (frame < 0) {
                numColumns = -3;
            } else if (frame >= numDirectories) {
                numColumns = -4;
            } else {
                // LOAD INPUT TIFF FILE PARAMETERS IMPORTANT TO RESAMPLING THE IMAGE
                unsigned short uShortVariable;

                // SET THE DIRECTORY TO THE USER SUPPLIED FRAME
                TIFFSetDirectory(inTiff, (unsigned short)frame);
                TIFFGetField(inTiff, TIFFTAG_IMAGEWIDTH, &uShortVariable);

                // COPY THE NUMBER OF CHANNELS OVER TO THE RETURN VARIABLE
                numColumns = (int)uShortVariable;
            }
            TIFFClose(inTiff);
        }
    }
    return (numColumns);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAUMemoryObject::howManyRowsDoesThisTiffFileHave(QString filename, int frame)
{
    // CREATE A VARIABLE TO HOLD THE RETURN VALUE
    int numRows = -1;

    // MAKE SURE WE HAVE A VALID FILE TO OPEN
    if (filename.isNull() == false && QFile::exists(filename)) {
        // OPEN INPUT TIFF FILE FROM DISK
        TIFF *inTiff = TIFFOpen(filename.toLatin1(), "r");
        if (!inTiff) {
            numRows = -2;
        } else {
            int numDirectories = TIFFNumberOfDirectories(inTiff);
            if (frame < 0) {
                numRows = -3;
            } else if (frame >= numDirectories) {
                numRows = -4;
            } else {
                // LOAD INPUT TIFF FILE PARAMETERS IMPORTANT TO RESAMPLING THE IMAGE
                unsigned short uShortVariable;

                // SET THE DIRECTORY TO THE USER SUPPLIED FRAME
                TIFFSetDirectory(inTiff, (unsigned short)frame);
                TIFFGetField(inTiff, TIFFTAG_IMAGELENGTH, &uShortVariable);

                // COPY THE NUMBER OF CHANNELS OVER TO THE RETURN VARIABLE
                numRows = (int)uShortVariable;
            }
            TIFFClose(inTiff);
        }
    }
    return (numRows);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QDateTime LAUMemoryObject::getTiffDateTime(QString filename, int directory)
{
    // CREATE A RETURN VARIABLE WITH INVALID DATE/TIME
    QDateTime dateTime;

    // MAKE SURE WE HAVE A VALID FILE TO OPEN
    if (filename.isNull() == false && QFile::exists(filename)) {
        // OPEN INPUT TIFF FILE FROM DISK
        TIFF *inTiff = TIFFOpen(filename.toLatin1(), "r");
        if (inTiff) {
            int numDirectories = TIFFNumberOfDirectories(inTiff);
            if (directory >= 0 && directory < numDirectories) {
                // SET THE DIRECTORY TO THE USER SUPPLIED INDEX
                TIFFSetDirectory(inTiff, (unsigned short)directory);

                // TRY TO READ THE DATETIME TAG
                char *dateTimeString = nullptr;
                if (TIFFGetField(inTiff, TIFFTAG_DATETIME, &dateTimeString)) {
                    // TIFF DateTime format is "yyyy:MM:dd hh:mm:ss"
                    dateTime = QDateTime::fromString(QString(dateTimeString), "yyyy:MM:dd hh:mm:ss");
                }
            }
            TIFFClose(inTiff);
        }
    }
    return dateTime;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObjectManager::~LAUMemoryObjectManager()
{
    framesAvailable.clear();
    qDebug() << QString("LAUMemoryObjectManager::~LAUMemoryObjectManager()");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMemoryObjectManager::onGetFrame()
{
    // CREATE NEW BUFFER IF NONE AVAILABLE, OTHERWISE TAKE FIRST AVAILABLE
    if (framesAvailable.isEmpty()) {
        emit emitFrame(LAUMemoryObject(numCols, numRows, numChns, numByts));
    } else {
        emit emitFrame(framesAvailable.takeFirst());
    }

    // NOW CREATE TWO NEW BUFFERS TO EVERY BUFFER RELEASED UP UNTIL
    // WE HAVE A MINIMUM NUMBER OF BUFFERS AVAILABLE FOR SUBSEQUENT CALLS
    if (framesAvailable.count() < MINNUMBEROFFRAMESAVAILABLE) {
        framesAvailable << LAUMemoryObject(numCols, numRows, numChns, numByts);
        framesAvailable << LAUMemoryObject(numCols, numRows, numChns, numByts);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMemoryObjectManager::onReleaseFrame(LAUMemoryObject frame)
{
    // WE CAN EITHER ADD THE BUFFER BACK TO THE AVAILABLE LIST, IF THERE IS SPACE
    // OR OTHERWISE FREE THE MEMORY
    if (framesAvailable.count() < MAXNUMBEROFFRAMESAVAILABLE) {
        framesAvailable << frame;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObjectWriter::LAUMemoryObjectWriter(QString filename, LAUMemoryObject obj, QObject *parent) : QThread(parent), tiff(nullptr), object(obj)
{
    if (object.isValid()) {
        // LET THE USER SELECT A FILE FROM THE FILE DIALOG
        if (filename.isNull()) {
#ifndef HEADLESS
            QSettings settings;
            QString directory = settings.value("LAUMemoryObject::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
            filename = QFileDialog::getSaveFileName(nullptr, QString("Save image to disk (*.tif)"), directory);
            if (filename.isEmpty() == false) {
                if (filename.toLower().endsWith(".tif") == false && filename.toLower().endsWith(".tiff") == false) {
                    filename.append(".tif");
                }
                settings.setValue("LAUMemoryObject::lastUsedDirectory", QFileInfo(filename).absolutePath());
            } else {
                return;
            }
#else
            return;
#endif
        }

        // OPEN TIFF FILE FOR SAVING THE IMAGE USING BIGTIFF FOR LARGE FILES
        tiff = (object.length() > 1000000000) ? TIFFOpen(filename.toLatin1(), "w8") : TIFFOpen(filename.toLatin1(), "w");
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObjectWriter::~LAUMemoryObjectWriter()
{
    qDebug() << QString("LAUMemoryObjectWriter::~LAUMemoryObjectWriter()");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMemoryObjectWriter::run()
{
    // MAKE SURE WE HAVE A VALID FILE TO WRITE TO
    if (tiff) {
        for (unsigned int frm = 0; frm < object.frames(); frm++) {
            qDebug() << "LAUMemoryObjectWriter::run()" << frm << object.frames();

            // WRITE FORMAT PARAMETERS TO CURRENT TIFF DIRECTORY
            TIFFSetField(tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
            TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, (unsigned long)object.width());
            TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, (unsigned long)object.height());
            TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
            TIFFSetField(tiff, TIFFTAG_XRESOLUTION, 72.0);
            TIFFSetField(tiff, TIFFTAG_YRESOLUTION, 72.0);
            TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
            TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, (unsigned short)object.colors());
            TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, (unsigned short)(8 * object.depth()));
            TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
            TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
            TIFFSetField(tiff, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
            TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, 1);

            if (object.depth() == sizeof(float)) {
                // SEE IF WE HAVE TO TELL THE TIFF READER THAT WE ARE STORING
                // PIXELS IN 32-BIT FLOATING POINT FORMAT
                TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
            }

            // MAKE TEMPORARY BUFFER TO HOLD CURRENT ROW BECAUSE COMPRESSION DESTROYS
            // WHATS EVER INSIDE THE BUFFER
            unsigned char *tempBuffer = (unsigned char *)malloc(object.step());
            for (unsigned int row = 0; row < object.height(); row++) {
                memcpy(tempBuffer, object.constScanLine(row, frm), object.step());
                TIFFWriteScanline(tiff, tempBuffer, row, 0);
            }
            free(tempBuffer);

            // WRITE THE CURRENT DIRECTORY AND PREPARE FOR THE NEW ONE
            TIFFWriteDirectory(tiff);

            // WRITE THE ELAPSED TIME STAMP TO AN EXIF TAG
            uint64_t dir_offset;
            TIFFCreateEXIFDirectory(tiff);
            TIFFSetField(tiff, EXIFTAG_SUBSECTIME, QString("%1").arg(object.elapsed()).toLatin1().data());
            TIFFWriteCustomDirectory(tiff, &dir_offset);

            TIFFSetDirectory(tiff, (unsigned short)frm);
            TIFFSetField(tiff, TIFFTAG_EXIFIFD, dir_offset);
            TIFFRewriteDirectory(tiff);
        }
        TIFFClose(tiff);
    }
    emit emitSaveComplete();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMemoryObject::flipUpDownInPlace() const
{
    // ALLOCATE A BLOCK OF MEMORY FOR HOLDING AN INTERMEDIATE NUGGET OF DATA
    unsigned char *mdBuffer = (unsigned char *)_mm_malloc(step(), 16);

    // ITERATE THROUGH EVERY ROW OF EVERY FRAME
    for (unsigned int frm = 0; frm < frames(); frm++) {
        for (unsigned int row = 0; row < height() / 2; row++) {
            // GRAB POINTERS TO THE BEGINNING AND END OF THE CURRENT ROW
            unsigned char *toBuffer = constScanLine(height() - 1 - row, frm);
            unsigned char *fmBuffer = constScanLine(row, frm);

            // SWAP IMAGE ROWS
            memcpy(mdBuffer, fmBuffer, step());
            memcpy(fmBuffer, toBuffer, step());
            memcpy(toBuffer, mdBuffer, step());
        }
    }

    // FREE UP THE ALLOCATED SPACE
    _mm_free(mdBuffer);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMemoryObject::flipLeftRightInPlace() const
{
    // ALLOCATE A BLOCK OF MEMORY FOR HOLDING AN INTERMEDIATE NUGGET OF DATA
    unsigned char mdBuffer[1024];

    // ITERATE THROUGH EVERY ROW OF EVERY FRAME
    for (unsigned int frm = 0; frm < frames(); frm++) {
        for (unsigned int row = 0; row < height(); row++) {
            // GRAB POINTERS TO THE BEGINNING AND END OF THE CURRENT ROW
            unsigned char *toBuffer = constScanLine(row + 1, frm);
            unsigned char *fmBuffer = constScanLine(row, frm);

            // ITERATE ACROSS THE ROW ONE NUGGET AT A TIME
            for (unsigned int col = 0; col < width() / 2; col++) {
                toBuffer -= nugget();
                memcpy(mdBuffer, fmBuffer, nugget());
                memcpy(fmBuffer, toBuffer, nugget());
                memcpy(toBuffer, mdBuffer, nugget());
                fmBuffer += nugget();
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMemoryObject::rotate180InPlace() const
{
    // ITERATE THROUGH EVERY ROW OF EVERY FRAME
    for (unsigned int frm = 0; frm < frames(); frm++) {
        rotateFrame180InPlace(frm);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMemoryObject::rotateFrame180InPlace(int frm) const
{
    // ALLOCATE A BLOCK OF MEMORY FOR HOLDING AN INTERMEDIATE NUGGET OF DATA
    unsigned char mdBuffer[1024];

    // GRAB POINTERS TO THE BEGINNING AND END OF THE CURRENT FRAME
    unsigned char *toBuffer = constScanLine(0, frm + 1);
    unsigned char *fmBuffer = constScanLine(0, frm);

    // ITERATE THROUGH ALL PIXELS FROM LEFT TO RIGHT AND TOP TO BOTTOM
    for (unsigned int pxl = 0; pxl < height()*width() / 2; pxl++) {
        toBuffer -= nugget();
        memcpy(mdBuffer, fmBuffer, nugget());
        memcpy(fmBuffer, toBuffer, nugget());
        memcpy(toBuffer, mdBuffer, nugget());
        fmBuffer += nugget();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUMemoryObject::flipLeftRight() const
{
    // CREATE MEMORY OBJECT WE CAN RETURN TO USER
    LAUMemoryObject object(*this);

    // ITERATE THROUGH EVERY ROW OF EVERY FRAME
    for (unsigned int frm = 0; frm < frames(); frm++) {
        for (unsigned int row = 0; row < height(); row++) {
            // GRAB POINTERS TO THE BEGINNING AND END OF THE CURRENT ROW
            unsigned char *toBuffer = object.scanLine(row + 1, frm);
            unsigned char *fmBuffer = constScanLine(row, frm);

            // ITERATE ACROSS THE ROW ONE NUGGET AT A TIME
            for (unsigned int col = 0; col < width(); col++) {
                toBuffer -= nugget();
                memcpy(toBuffer, fmBuffer, nugget());
                fmBuffer += nugget();
            }
        }
    }

    // RETURN THE LEFT-RIGHT FLIPPED MEMORY OBJECT TO THE USER
    return (object);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUMemoryObject::minAreaFilter(int rad) const
{
    LAUMemoryObject object(*this);

    if (depth() == sizeof(unsigned char)) {
        for (unsigned int frm = 0; frm < frames(); frm++) {
            for (int row = 0; row < (int)height(); row++) {
                unsigned char *toBuffer = object.scanLine(row, frm);
                for (int col = 0; col < (int)width(); col++) {
                    for (int dy = -rad; dy <= rad; dy++) {
                        int rwp = qMin((int)height() - 1, qMax(0, row + dy));
                        unsigned char *fmBuffer = constScanLine(rwp, frm);
                        for (int dx = -rad; dx <= rad; dx++) {
                            int clp = qMin((int)width() - 1, qMax(0, col + dx));
                            for (unsigned int chn = 0; chn < colors(); chn++) {
                                toBuffer[col * colors() + chn] = qMin(toBuffer[col * colors() + chn], fmBuffer[clp * colors() + chn]);
                            }
                        }
                    }
                }
            }
        }
    } else if (depth() == sizeof(unsigned short)) {
        for (unsigned int frm = 0; frm < frames(); frm++) {
            for (int row = 0; row < (int)height(); row++) {
                unsigned short *toBuffer = (unsigned short *)object.scanLine(row, frm);
                for (int col = 0; col < (int)width(); col++) {
                    for (int dy = -rad; dy <= rad; dy++) {
                        int rwp = qMin((int)height() - 1, qMax(0, row + dy));
                        unsigned short *fmBuffer = (unsigned short *)constScanLine(rwp, frm);
                        for (int dx = -rad; dx <= rad; dx++) {
                            int clp = qMin((int)width() - 1, qMax(0, col + dx));
                            for (unsigned int chn = 0; chn < colors(); chn++) {
                                toBuffer[col * colors() + chn] = qMin(toBuffer[col * colors() + chn], fmBuffer[clp * colors() + chn]);
                            }
                        }
                    }
                }
            }
        }
    } else if (depth() == sizeof(float)) {
        for (unsigned int frm = 0; frm < frames(); frm++) {
            for (int row = 0; row < (int)height(); row++) {
                float *toBuffer = (float *)object.scanLine(row, frm);
                for (int col = 0; col < (int)width(); col++) {
                    for (int dy = -rad; dy <= rad; dy++) {
                        int rwp = qMin((int)height() - 1, qMax(0, row + dy));
                        float *fmBuffer = (float *)constScanLine(rwp, frm);
                        for (int dx = -rad; dx <= rad; dx++) {
                            int clp = qMin((int)width() - 1, qMax(0, col + dx));
                            for (unsigned int chn = 0; chn < colors(); chn++) {
                                toBuffer[col * colors() + chn] = qMin(toBuffer[col * colors() + chn], fmBuffer[clp * colors() + chn]);
                            }
                        }
                    }
                }
            }
        }
    }
    return (object);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<LAUMemoryObject> LAUMemoryObject::splitStackedFrames(const LAUMemoryObject &stackedVideo, int numFrames)
{
    QVector<double> jetr = stackedVideo.jetr();
    QList<LAUMemoryObject> frameList;

    // Validate input parameters
    if (!stackedVideo.isValid() || numFrames <= 0) {
        return frameList;
    }

    // Calculate the height of each individual frame
    unsigned int totalHeight = stackedVideo.height();
    unsigned int frameHeight = totalHeight / numFrames;

    // Validate that the height divides evenly
    if (totalHeight % numFrames != 0) {
        qDebug() << "Warning: Total height" << totalHeight << "does not divide evenly by" << numFrames << "frames";
        qDebug() << "Frame height will be" << frameHeight << "with" << (totalHeight % numFrames) << "pixels remaining";
    }

    // Extract each frame
    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        // Calculate the starting row for this frame
        unsigned int startRow = frameIndex * frameHeight;

        // For the last frame, make sure we don't exceed the total height
        // and capture any remaining pixels
        unsigned int actualFrameHeight = frameHeight;
        if (frameIndex == numFrames - 1) {
            actualFrameHeight = totalHeight - startRow;
        }

        // Create a new memory object for this frame
        LAUMemoryObject frame(stackedVideo.width(), actualFrameHeight,
                              stackedVideo.colors(), stackedVideo.depth(),
                              stackedVideo.frames());

        // Copy metadata from the original object
        frame.setXML(stackedVideo.xml());
        frame.setRFID(stackedVideo.rfid());
        frame.setTransform(stackedVideo.transform());
        frame.setProjection(stackedVideo.projection());
        frame.setAnchor(stackedVideo.anchor());
        frame.setElapsed(stackedVideo.elapsed());

        // SEE IF THERE IS MORE THAN ONE JETR
        // VECTOR EMBEDDED INTO THE JETR VECTOR FIELD
        if (jetr.length() == 37){
            frame.setJetr(jetr);
        } else {
            QVector<double> subJetr(37, NAN);
            for (int n = 0; n < 37; n++){
                subJetr[n] = jetr[(frameIndex * 37 + n) % jetr.length()];
            }
            frame.setJetr(subJetr);
        }

        // Copy pixel data row by row
        for (unsigned int row = 0; row < actualFrameHeight; row++) {
            unsigned int sourceRow = startRow + row;
            if (sourceRow < totalHeight) {
                memcpy(frame.scanLine(row),
                       stackedVideo.constScanLine(sourceRow),
                       stackedVideo.step());
            }
        }

        frameList.append(frame);
    }

    return frameList;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUMemoryObject::extractFrame(int frameIndex, int totalFrames) const
{
    // Use the static method to split and return the requested frame
    QList<LAUMemoryObject> frames = splitStackedFrames(*this, totalFrames);

    if (frameIndex >= 0 && frameIndex < frames.count()) {
        return frames.at(frameIndex);
    }

    // Return invalid object if index is out of range
    return LAUMemoryObject();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<LAUMemoryObject> LAUMemoryObject::loadStackedVideo(QString filename, int directory, int numFrames)
{
    QList<LAUMemoryObject> frameList;

    // Validate input parameters
    if (numFrames <= 0) {
        qDebug() << "Error: Number of frames must be greater than 0";
        return frameList;
    }

    // Load the stacked video from the specified directory
    LAUMemoryObject stackedVideo(filename, directory);
    if (!stackedVideo.isValid()) {
        qDebug() << "Error: Could not load stacked video from:" << filename << "directory:" << directory;
        return frameList;
    }

    // Split the stacked video into individual frames
    frameList = splitStackedFrames(stackedVideo, numFrames);

    qDebug() << "Successfully split stacked video into" << frameList.count() << "frames from directory" << directory;

    return frameList;
}
