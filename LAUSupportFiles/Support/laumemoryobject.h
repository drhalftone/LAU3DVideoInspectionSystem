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

#ifndef LAUMEMORYOBJECT_H
#define LAUMEMORYOBJECT_H

#ifndef HEADLESS
#include <QImage>
#include <QMatrix4x4>
#else
// Minimal Qt type replacements for headless mode (console applications)
// These provide just enough interface for laumemoryobject to compile
// LAUEncodeObjectIDFilter doesn't actually use these methods at runtime

class QMatrix4x4 {
public:
    QMatrix4x4() { for (int i = 0; i < 16; i++) m[i] = 0.0f; }
    float* data() { return m; }
    const float* constData() const { return m; }
private:
    float m[16];
};

class QPoint {
public:
    QPoint() : xp(0), yp(0) {}
    QPoint(int x, int y) : xp(x), yp(y) {}
    int x() const { return xp; }
    int y() const { return yp; }
    void setX(int x) { xp = x; }
    void setY(int y) { yp = y; }
private:
    int xp, yp;
};

class QPointF {
public:
    QPointF() : xp(0.0), yp(0.0) {}
    QPointF(double x, double y) : xp(x), yp(y) {}
    double x() const { return xp; }
    double y() const { return yp; }
private:
    double xp, yp;
};

class QSize {
public:
    QSize() : wd(0), ht(0) {}
    QSize(int width, int height) : wd(width), ht(height) {}
    int width() const { return wd; }
    int height() const { return ht; }
private:
    int wd, ht;
};

class QRect {
public:
    QRect() : x1(0), y1(0), x2(0), y2(0) {}
    QRect(int left, int top, int width, int height) : x1(left), y1(top), x2(left+width-1), y2(top+height-1) {}
    int left() const { return x1; }
    int top() const { return y1; }
    int right() const { return x2; }
    int bottom() const { return y2; }
    int width() const { return x2 - x1 + 1; }
    int height() const { return y2 - y1 + 1; }
private:
    int x1, y1, x2, y2;
};

class QImage {
public:
    QImage() : w(0), h(0) {}
    QImage(int width, int height, int format) : w(width), h(height) { (void)format; }
    int width() const { return w; }
    int height() const { return h; }
private:
    int w, h;
};
#endif

#include <QThread>
#include <QSharedData>
#include <QSharedDataPointer>
#include <QDateTime>
#include <cmath>

namespace libtiff
{
#include "tiffio.h"
}

#ifndef Q_PROCESSOR_ARM
#include "emmintrin.h"
#include "smmintrin.h"
#include "tmmintrin.h"
#include "xmmintrin.h"
#else
#include "sse2neon.h"
#endif

namespace LAU3DVideoParameters
{
    enum LAUVideoPlaybackState  { StateLiveVideo, StateVideoPlayback };
    enum LAUVideoPlaybackDevice { DeviceUndefined, DeviceKinect, DevicePrimeSense, DeviceProsilicaLCG, DeviceProsilicaDPR, DeviceProsilicaIOS, Device2DCamera, DeviceProsilicaPST, DeviceProsilicaAST, DeviceProsilicaGRY, DeviceProsilicaRGB, DeviceProsilicaTOF, DeviceXimea, DeviceIDS, DeviceRealSense, DeviceLucid, DeviceOrbbec, DeviceVZense, DeviceVidu, DeviceSeek, DeviceDemo };
    enum LAUVideoPlaybackColor  { ColorUndefined, ColorGray, ColorRGB, ColorRGBA, ColorXYZ, ColorXYZW, ColorXYZG, ColorXYZRGB, ColorXYZWRGBA };
    enum LAUVideoPatternSequence { SequenceNone, SequenceCustom, SequenceUnitFrequency, SequenceTwoFrequency, SequenceThreeFrequency, SequenceDualFrequency, SequenceCalibration, SequenceMultipath, SequenceTiming };
    enum LAUVideoProjector { ProjectorLC4500, ProjectorLC3000, ProjectorTI2010, ProjectorML500, ProjectoML750ST, ProjectorUnknown };
    enum LAUVideoProjectorSynchronizationMode { ModeSlave, ModeMaster, ModeMono, ModeMasterHandshake, ModeHDMIFPGA, ModeFPGA };
    enum LAUVideoPatternSynchronizationScheme { SchemeFlashingSequence, SchemePatternBit, SchemeNone };

    int colors(LAUVideoPlaybackColor clr);
    bool isMachineVision(LAUVideoPlaybackDevice dvc);
}

using namespace LAU3DVideoParameters;

#define MINNUMBEROFFRAMESAVAILABLE        40
#define MAXNUMBEROFFRAMESAVAILABLE        100
#define LAUMEMORYOBJECTINVALIDELAPSEDTIME 0xFFFFFFFF

void myTIFFWarningHandler(const char *stringA, const char *stringB, va_list args);
void myTIFFErrorHandler(const char *stringA, const char *stringB, va_list args);

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUMemoryObjectData : public QSharedData
{
public:
    LAUMemoryObjectData();
    LAUMemoryObjectData(const LAUMemoryObjectData &other);
    LAUMemoryObjectData(unsigned int cols, unsigned int rows, unsigned int chns = 1, unsigned int byts = 1, unsigned int frms = 1);
    LAUMemoryObjectData(unsigned long long bytes);

    ~LAUMemoryObjectData();

    static int instanceCounter;

    unsigned int numRows, numCols, numChns, numFrms, numByts;
    unsigned int stepBytes, frameBytes;
    unsigned long long numBytesTotal;
    void *buffer;

    QString *rfidString;
    QByteArray *xmlByteArray;
    QMatrix4x4 *transformMatrix;
    QMatrix4x4 *projectionMatrix;
    QPoint *anchorPt;
    QVector<double> *jetr;
    unsigned int *elapsedTime;

    void allocateBuffer();
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUMemoryObject
{
public:
    LAUMemoryObject()
    {
        data = new LAUMemoryObjectData();
    }

    LAUMemoryObject(unsigned int cols, unsigned int rows, unsigned int chns = 1, unsigned int byts = 1, unsigned int frms = 1)
    {
        data = new LAUMemoryObjectData(cols, rows, chns, byts, frms);
    }

    LAUMemoryObject(unsigned long long bytes)
    {
        data = new LAUMemoryObjectData(bytes);
    }

    LAUMemoryObject(const LAUMemoryObject &other) : data(other.data) { ; }

    LAUMemoryObject &operator = (const LAUMemoryObject &other)
    {
        if (this != &other) {
            data = other.data;
        }
        return (*this);
    }

    LAUMemoryObject(QImage image);
    LAUMemoryObject(QString filename, int index = -1);
    LAUMemoryObject(libtiff::TIFF *inTiff, int index = -1);

    bool save(QString filename = QString(), QString *savedFilePath = nullptr) const;
    bool save(libtiff::TIFF *otTiff, int index = 0) const;
    bool load(libtiff::TIFF *inTiff, int index = -1);

    // LOAD INTO READS A FILE INTO THE EXISTING BUFFER BUT ALL
    // SIZE PARAMETERS MUST BE SAME OTHERWISE RETURN FALSE
    bool loadInto(libtiff::TIFF *inTiff, int index = -1);
    bool loadInto(QString filename, int index = -1);

    unsigned int nonZeroPixelsCount(int chn = 0) const;
    LAUMemoryObject toFloat();
    LAUMemoryObject rotate();
    LAUMemoryObject getFrame(int frm) const;
    LAUMemoryObject minAreaFilter(int rad) const;
    LAUMemoryObject flipLeftRight() const;
    LAUMemoryObject crop(QRect rect) const;
    LAUMemoryObject peakEnvelope(float dx, float dy, float radius) const;

    QImage toImage(int frame = 0) const;

    void flipUpDownInPlace() const;
    void flipLeftRightInPlace() const;
    void rotate180InPlace() const;
    void rotateFrame180InPlace(int frm) const;
    void peakEnvelopeInPlace(float dx, float dy, float radius);

    // SEE IF THE POINTERS ARE LOOKING AT SAME MEMORY
    bool operator == (const LAUMemoryObject &other) const
    {
        if (this == &other) {
            return (true);
        }
        return (data->buffer == other.data->buffer);
    }

    bool operator  < (const LAUMemoryObject &other) const
    {
        if (this == &other) {
            return (false);
        }
        return (elapsed() < other.elapsed());
    }

    bool operator  > (const LAUMemoryObject &other) const
    {
        if (this == &other) {
            return (false);
        }
        return (elapsed() > other.elapsed());
    }

    bool operator <= (const LAUMemoryObject &other) const
    {
        if (this == &other) {
            return (true);
        }
        return (elapsed() <= other.elapsed());
    }

    bool operator >= (const LAUMemoryObject &other) const
    {
        if (this == &other) {
            return (true);
        }
        return (elapsed() >= other.elapsed());
    }

    ~LAUMemoryObject() { ; }

    inline bool isNull() const
    {
        return (data->buffer == nullptr);
    }

    inline bool isValid() const
    {
        return (data->buffer != nullptr);
    }

    inline unsigned long long length() const
    {
        return (data->numBytesTotal);
    }

    inline QSize size() const
    {
        return (QSize(width(), height()));
    }

    inline unsigned int nugget() const
    {
        return (data->numChns * data->numByts);
    }

    inline unsigned int width() const
    {
        return (data->numCols);
    }

    inline unsigned int height() const
    {
        return (data->numRows);
    }

    inline unsigned int depth() const
    {
        return (data->numByts);
    }

    inline unsigned int colors() const
    {
        return (data->numChns);
    }

    inline unsigned int frames() const
    {
        return (data->numFrms);
    }

    inline unsigned int step() const
    {
        return (data->stepBytes);
    }

    inline unsigned long long block() const
    {
        return (data->frameBytes);
    }

    inline unsigned char *pixel(unsigned int col, unsigned int row, unsigned int frm = 0)
    {
        return (&(scanLine(row, frm)[static_cast<unsigned long long>(col * colors() * depth())]));
    }

    inline unsigned char *constPixel(unsigned int col, unsigned int row, unsigned int frm = 0) const
    {
        return (&(constScanLine(row, frm)[static_cast<unsigned long long>(col * colors() * depth())]));
    }

    // GET POINT TO PIXEL WITH WRAP AROUND
    inline unsigned char *pixelWW(int col, int row, int frm = 0)
    {
        while (row < 0) {
            row += static_cast<int>(height());
        }
        while (col < 0) {
            col += static_cast<int>(width());
        }
        while (frm < 0) {
            frm += static_cast<int>(frames());
        }
        return (&(scanLine(static_cast<unsigned int>(row) % height(), static_cast<unsigned int>(frm) % frames())[static_cast<unsigned long long>(static_cast<unsigned int>(col) % width() * colors() * depth())]));
    }

    // GET POINT TO PIXEL WITH WRAP AROUND
    inline unsigned char *constPixelWW(int col, int row, int frm = 0) const
    {
        while (row < 0) {
            row += static_cast<int>(height());
        }
        while (col < 0) {
            col += static_cast<int>(width());
        }
        while (frm < 0) {
            frm += static_cast<int>(frames());
        }
        return (&(constScanLine(static_cast<unsigned int>(row) % height(), static_cast<unsigned int>(frm) % frames())[static_cast<unsigned long long>(static_cast<unsigned int>(col) % width() * colors() * depth())]));
    }

    inline void triggerDeepCopy()
    {
        unsigned char *data = pointer();
        Q_UNUSED(data);
    }

    inline unsigned char *pointer()
    {
        return (scanLine(0));
    }

    inline unsigned char *constPointer() const
    {
        return (constScanLine(0));
    }

    inline unsigned char *scanLine(unsigned int row, unsigned int frame = 0)
    {
        return (&(((unsigned char *)(data->buffer))[frame * block() + row * step()]));
    }

    inline unsigned char *constScanLine(unsigned int row, unsigned int frame = 0) const
    {
        return (&(((unsigned char *)(data->buffer))[frame * block() + row * step()]));
    }

    inline unsigned char *frame(unsigned int frm = 0)
    {
        return (scanLine(0, frm));
    }

    inline unsigned char *constFrame(unsigned int frm = 0) const
    {
        return (constScanLine(0, frm));
    }

    inline QByteArray xml() const
    {
        if (data->xmlByteArray) {
            return (*(data->xmlByteArray));
        } else {
            return (QByteArray("XML String wasn't allocated!"));
        }
    }

    inline void setXML(QByteArray string)
    {
        if (data->xmlByteArray) {
            data->xmlByteArray->clear();
            data->xmlByteArray->append(string);
        }
    }

    inline void setConstXML(QByteArray string) const
    {
        if (data->xmlByteArray) {
            data->xmlByteArray->clear();
            data->xmlByteArray->append(string);
        }
    }

    inline QString rfid() const
    {
        if (data->rfidString) {
            return (*(data->rfidString));
        } else {
            return (QString("RFID String wasn't allocated!"));
        }
    }

    inline void setRFID(QString string)
    {
        if (data->rfidString) {
            data->rfidString->clear();
            data->rfidString->append(string);
        }
    }

    inline void setConstRFID(QString string) const
    {
        if (data->rfidString) {
            data->rfidString->clear();
            data->rfidString->append(string);
        }
    }

    inline QMatrix4x4 transform() const
    {
        if (data->transformMatrix) {
            return (*(data->transformMatrix));
        } else {
            return (QMatrix4x4());
        }
    }

    inline void setTransform(QMatrix4x4 mat)
    {
        if (data->transformMatrix) {
            memcpy((void *)(data->transformMatrix->data()), (void *)mat.constData(), sizeof(QMatrix4x4));
        }
    }

    inline void setConstTransform(QMatrix4x4 mat) const
    {
        if (data->transformMatrix) {
            memcpy((void *)(data->transformMatrix->data()), (void *)mat.constData(), sizeof(QMatrix4x4));
        }
    }

    inline QMatrix4x4 projection() const
    {
        if (data->projectionMatrix) {
            return (*(data->projectionMatrix));
        } else {
            return (QMatrix4x4());
        }
    }

    inline void setProjection(QMatrix4x4 mat)
    {
        if (data->projectionMatrix) {
            memcpy((void *)(data->projectionMatrix->data()), (void *)mat.constData(), sizeof(QMatrix4x4));
        }
    }

    inline void setConstProjection(QMatrix4x4 mat) const
    {
        if (data->projectionMatrix) {
            memcpy((void *)(data->projectionMatrix->data()), (void *)mat.constData(), sizeof(QMatrix4x4));
        }
    }

    inline QVector<double> jetr() const
    {
        if (data->jetr) {
            return (*data->jetr);
        } else {
            return (QVector<double>(37,NAN));
        }
    }

    inline void setJetr(QVector<double> vector)
    {
        if (data->jetr) {
            *data->jetr = vector;
        }
    }

    inline void setConstJetr(QVector<double> vector) const
    {
        if (data->jetr) {
            *data->jetr = vector;
        }
    }
    
    // Check if the memory object has a valid JETR vector (not all NANs)
    inline bool hasValidJETRVector() const
    {
        QVector<double> jetrVector = jetr();
        if (jetrVector.isEmpty() || ((jetrVector.size() % 37) != 0)) {
            return false;  // Empty or not a multiple of 37
        }
        
        // Check if all values are NAN
        for (const double &value : jetrVector) {
            if (!std::isnan(value)) {
                return true;  // Found at least one non-NAN value
            }
        }
        return false;  // All values are NAN
    }

    inline unsigned int elapsed() const
    {
        if (data->elapsedTime) {
            return (*data->elapsedTime);
        } else {
            return ((unsigned int)(-1));
        }
    }

    inline void setElapsed(unsigned int elps)
    {
        if (data->elapsedTime) {
            *data->elapsedTime = elps;
        }
    }

    inline void setConstElapsed(unsigned int elps) const
    {
        if (data->elapsedTime) {
            *data->elapsedTime = elps;
        }
    }

    inline bool isElapsedValid() const
    {
        return (elapsed() != LAUMEMORYOBJECTINVALIDELAPSEDTIME);
    }

    inline void makeElapsedInvalid()
    {
        if (data->elapsedTime) {
            *data->elapsedTime = LAUMEMORYOBJECTINVALIDELAPSEDTIME;
        }
    }

    inline void constMakeElapsedInvalid() const
    {
        if (data->elapsedTime) {
            *data->elapsedTime = LAUMEMORYOBJECTINVALIDELAPSEDTIME;
        }
    }

    inline QPoint anchor() const
    {
        if (data->anchorPt) {
            return (*data->anchorPt);
        } else {
            return (QPoint());
        }
    }

    inline void setAnchor(QPoint pt)
    {
        if (data->anchorPt) {
            data->anchorPt->setX(pt.x());
            data->anchorPt->setY(pt.y());
        }
    }

    inline void setConstAnchor(QPoint pt) const
    {
        if (data->anchorPt) {
            data->anchorPt->setX(pt.x());
            data->anchorPt->setY(pt.y());
        }
    }

    static int numberOfColors(LAUVideoPlaybackColor color)
    {
        switch (color) {
            case ColorGray:
                return (1);
            case ColorRGB:
            case ColorXYZ:
                return (3);
            case ColorRGBA:
            case ColorXYZG:
                return (4);
            case ColorXYZRGB:
                return (6);
            case ColorXYZWRGBA:
                return (8);
            default:
                return (-1);
        }
    }

    static bool saveObjectsToDisk(QList<LAUMemoryObject> objects, QString filename);

    static QString lastTiffErrorString;
    static QString lastTiffWarningString;

    static QHash<QString, QString> xmlToHash(QByteArray byteArray);

    static int howManyDirectoriesDoesThisTiffFileHave(QString filename);
    static int howManyChannelsDoesThisTiffFileHave(QString filename, int frame = 0);
    static int howManyColumnsDoesThisTiffFileHave(QString filename, int frame = 0);
    static int howManyRowsDoesThisTiffFileHave(QString filename, int frame = 0);
    static QDateTime getTiffDateTime(QString filename, int directory = 0);

    // Split vertically stacked frames into individual frame objects
    static QList<LAUMemoryObject> splitStackedFrames(const LAUMemoryObject &stackedVideo, int numFrames);

    // Extract a specific frame from stacked video
    LAUMemoryObject extractFrame(int frameIndex, int totalFrames) const;

    // Load stacked video from specific directory and split into frames
    static QList<LAUMemoryObject> loadStackedVideo(QString filename, int directory, int numFrames);

protected:
    QSharedDataPointer<LAUMemoryObjectData> data;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUMemoryObjectManager : public QObject
{
    Q_OBJECT

public:
    explicit LAUMemoryObjectManager(unsigned int cols = 0, unsigned int rows = 0, unsigned int chns = 0, unsigned int byts = 0, unsigned int frms = 0, QObject *parent = 0) : QObject(parent), numRows(rows), numCols(cols), numChns(chns), numByts(byts), numFrms(frms) { ; }
    ~LAUMemoryObjectManager();

public slots:
    void onGetFrame();
    void onReleaseFrame(LAUMemoryObject frame);

private:
    unsigned int numRows, numCols, numChns, numByts, numFrms;
    QList<LAUMemoryObject> framesAvailable;

signals:
    void emitFrame(LAUMemoryObject frame);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUModalityObject
{
public:
    LAUModalityObject(LAUMemoryObject dpt = LAUMemoryObject(), LAUMemoryObject clr = LAUMemoryObject(), LAUMemoryObject map = LAUMemoryObject()) : depth(dpt), color(clr), mappi(map) { ; }
    LAUModalityObject(const LAUModalityObject &other) : depth(other.depth), color(other.color), mappi(other.mappi) { ; }
    LAUModalityObject &operator = (const LAUModalityObject &other)
    {
        if (this != &other) {
            depth = other.depth;
            color = other.color;
            mappi = other.mappi;
        }
        return (*this);
    }

    bool isAnyValid()
    {
        return (depth.isValid() || color.isValid() || mappi.isValid());
    }

    LAUMemoryObject depth;
    LAUMemoryObject color;
    LAUMemoryObject mappi;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUMemoryObjectWriter : public QThread
{
    Q_OBJECT

public:
    explicit LAUMemoryObjectWriter(QString flnm, LAUMemoryObject obj, QObject *parent = nullptr);
    ~LAUMemoryObjectWriter();

    bool isNull() const
    {
        return (!isValid());
    }

    bool isValid() const
    {
        return (tiff != nullptr);
    }

protected:
    void run();

private:
    libtiff::TIFF *tiff;
    LAUMemoryObject object;

signals:
    void emitSaveComplete();
};

Q_DECLARE_METATYPE(LAUMemoryObject);

#endif // LAUMEMORYOBJECT_H
