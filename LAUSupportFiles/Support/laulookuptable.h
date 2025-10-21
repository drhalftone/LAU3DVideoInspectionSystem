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

#ifndef LAULOOKUPTABLE_H
#define LAULOOKUPTABLE_H

#ifndef HEADLESS
#include <QtGui>
#endif
#include <QtCore>
#include <QThread>
#include <QtConcurrent>
#include <QDate>

#include "laumemoryobject.h"

#ifndef PI
#define PI 3.14159265359
#endif

#define LENGTHPHASECORRECTIONTABLE  4096

using namespace LAU3DVideoParameters;

namespace libtiff
{
#include "tiffio.h"
}

enum LAULookUpTableStyle  { StyleLinear, StyleFourthOrderPoly, StyleFourthOrderPolyAugmentedReality, StyleFourthOrderPolyWithPhaseUnwrap, StyleXYZPLookUpTable, StyleXYZWRCPQLookUpTable, StyleActiveStereoVisionPoly, StyleUndefined };

typedef struct {
    double fx;
    double cx;
    double fy;
    double cy;
    double k1;
    double k2;
    double k3;
    double k4;
    double k5;
    double k6;
    double p1;
    double p2;
} LookUpTableIntrinsics;

typedef struct {
    double xMin;
    double xMax;
    double yMin;
    double yMax;
    double zMin;
    double zMax;
} LookUpTableBoundingBox;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAULookUpTableData : public QSharedData
{
public:
    LAULookUpTableData();
    LAULookUpTableData(const LAULookUpTableData &other);
    ~LAULookUpTableData();

    QString filename;

    QString xmlString;
    QString makeString;
    QString modelString;
    QString serialString;
    QString softwareString;

    void *buffer;
    void *phaseCorrectionBuffer;

    QMatrix4x4 *transformMatrix;
    QMatrix4x4 *projectionMatrix;
    LAULookUpTableStyle style;
    float scaleFactor;
    float xMin, xMax, yMin, yMax, zMin, zMax, pMin, pMax;
    float horizontalFieldOfView;
    float verticalFieldOfView;
    unsigned int numRows, numCols, numChns;
    unsigned long long numSmps;
    LookUpTableIntrinsics intrinsics;
    LookUpTableBoundingBox boundingBox;

    static int instanceCounter;

    void allocateBuffer();
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAULookUpTable
{
public:
    explicit LAULookUpTable(unsigned int cols = 0, unsigned int rows = 0, LAUVideoPlaybackDevice device = DeviceProsilicaLCG, float hFov = 0.0f, float vFov = 0.0f, float zMin = 0.0f, float zMax = 0.0f, float pMin = 0.0f, float pMax = 1.0f);
    explicit LAULookUpTable(unsigned int cols, unsigned int rows, LAULookUpTableStyle stl, float hFov = 0.0f, float vFov = 0.0f, float zMin = 0.0f, float zMax = 0.0f, float pMin = 0.0f, float pMax = 1.0f);
    explicit LAULookUpTable(unsigned int cols, unsigned int rows, unsigned int chns, LAULookUpTableStyle stl, float hFov = 0.0f, float vFov = 0.0f, float zMin = 0.0f, float zMax = 0.0f, float pMin = 0.0f, float pMax = 1.0f);
    explicit LAULookUpTable(const QString filename, int directory = -1);
    ~LAULookUpTable();

    LAULookUpTable(unsigned int cols, unsigned int rows, QMatrix3x3 intParameters, QVector<double> rdlParameters, QVector<double> tngParameters, double sclFactor = 0.1, double zMin = 500.0, double zMax = 8000.0, QWidget *widget = nullptr, bool *completed = nullptr);
    LAULookUpTable(libtiff::TIFF *currentTiffDirectory);
    LAULookUpTable(const LAULookUpTable &other) : data(other.data) { ; }
    LAULookUpTable &operator = (const LAULookUpTable &other)
    {
        if (this != &other) {
            data = other.data;
        }
        return (*this);
    }

    static LAULookUpTable generateTableFromJETR(unsigned int cols, unsigned int rows, QVector<double> justEnoughToReconstructVector, QWidget *widget = nullptr, bool *completed = nullptr);
    static LAULookUpTable generateTableFromJETR(unsigned int cols, unsigned int rows, QVector<double> justEnoughToReconstructVector, const QString &make, const QString &model, QWidget *widget = nullptr, bool *completed = nullptr);
    static LAULookUpTable generateTableFromJETR(unsigned int cols, unsigned int rows, QVector<double> justEnoughToReconstructVector, const QString &make, const QString &model, const QDate &folderDate, QWidget *widget = nullptr, bool *completed = nullptr);
    static QDate parseFolderDate(const QString &folderName);
    static void combineLookUpTablesFromDisk(QStringList filenames = QStringList());
    static LAULookUpTable combineLookUpTables(QList<LAULookUpTable> tables);
    static bool saveLookUpTables(QList<LAULookUpTable> tables, QString filename = QString());
    static QList<LAULookUpTable> LAULookUpTableX(QString filename);

    LAULookUpTable convertToStyle(LAULookUpTableStyle stl) const;
    LAUMemoryObject createRangeMasks(float xmn, float xmx, float ymn, float ymx, float zmn, float zmx) const;

    void replace(const LAULookUpTable &other);
    bool rotate180InPlace();

    bool load(libtiff::TIFF *inTiff, int directory = -1);
    bool save(QString filename = QString()) const;
    bool save(libtiff::TIFF *currentTiffDirectory) const;
    bool savePhaseCorrectionTable(libtiff::TIFF *inTiff) const;

    bool isNull() const
    {
        return (data->buffer == nullptr);
    }

    bool isValid() const
    {
        return (!isNull());
    }

    unsigned int length() const
    {
        return (height() * step());
    }

    unsigned int width() const
    {
        return (data->numCols);
    }

    unsigned int height() const
    {
        return (data->numRows);
    }

    unsigned int colors() const
    {
        return (data->numChns);
    }

    unsigned int step() const
    {
        return (data->numCols * data->numChns * sizeof(float));
    }

    LAULookUpTableStyle style() const
    {
        return (data->style);
    }

    QPoint size() const
    {
        return (QPoint((int)width(), (int)height()));
    }

    unsigned char *scanLine(unsigned int row)
    {
        return (&(((unsigned char *)(data->buffer))[row * step()]));
    }

    unsigned char *constScanLine(unsigned int row) const
    {
        return (&(((unsigned char *)(data->buffer))[row * step()]));
    }

    unsigned char *phaseCorrectionTable()
    {
        return ((unsigned char *)(data->phaseCorrectionBuffer));
    }

    unsigned char *constPhaseCorrectionTable() const
    {
        return ((unsigned char *)(data->phaseCorrectionBuffer));
    }

    void setFilename(const QString string)
    {
        data->filename = string;
    }

    QString filename() const
    {
        return (data->filename);
    }

    void setXmlString(const QString string)
    {
        data->xmlString = string;
    }

    QString xmlString() const
    {
        return (data->xmlString);
    }

    void setMakeString(const QString string)
    {
        data->makeString = string;
    }

    QString makeString() const
    {
        return (data->makeString);
    }

    void setModelString(const QString string)
    {
        data->modelString = string;
    }

    QString modelString() const
    {
        return (data->modelString);
    }

    void setSerialString(const QString string)
    {
        data->serialString = string;
    }

    QString serialString() const
    {
        return (data->serialString);
    }

    void setSoftwareString(const QString string)
    {
        data->softwareString = string;
    }

    QString softwareString() const
    {
        return (data->softwareString);
    }

    QPointF xLimits() const
    {
        return (QPointF(data->xMin, data->xMax));
    }

    QPointF yLimits() const
    {
        return (QPointF(data->yMin, data->yMax));
    }

    QPointF zLimits() const
    {
        return (QPointF(data->zMin, data->zMax));
    }

    QPointF pLimits() const
    {
        return (QPointF(data->pMin, data->pMax));
    }

    void setZLimits(QPointF point)
    {
        data->zMin = point.x();
        data->zMax = point.y();
        updateLimits();
    }

    float scaleFactor() const
    {
        return(data->scaleFactor);
    }

    QPointF fov() const
    {
        return (QPointF(data->horizontalFieldOfView, data->verticalFieldOfView));
    }

    LAULookUpTable crop(unsigned int x, unsigned int y, unsigned int w, unsigned int h);

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

    inline void setIntrinsics(LookUpTableIntrinsics intrscs)
    {
        data->intrinsics = intrscs;
    }

    inline LookUpTableIntrinsics intrinsics() const
    {
        return (data->intrinsics);
    }

    inline void setBoundingBox(LookUpTableBoundingBox bndngbx)
    {
        data->boundingBox = bndngbx;
    }

    inline LookUpTableBoundingBox boundingBox() const
    {
        return (data->boundingBox);
    }

    QVector4D focalPlaneArrayLimits() const;
    QPointF cameraCoordinate(QVector3D point) const; // THIS FUNCTION TAKES THE WORLD COORDINATE PROVIDED BY USER AND MAPS IT TO A CAMERA PIXEL
    QVector<double> jetr() const;                    // THIS FUNCTION RETURNS JUST ENOUGH INFORMATION TO RECONSTRUCT A RAW DEPTH IMAGE

private:
    QSharedDataPointer<LAULookUpTableData> data;
    void updateLimits();
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAULookUpTableLoader : public QThread
{
    Q_OBJECT

public:
    explicit LAULookUpTableLoader(QString flnm, unsigned short drc, unsigned char *bffr, QObject *parent = NULL);
    ~LAULookUpTableLoader();

protected:
    void run();

private:
    libtiff::TIFF *tiff;
    unsigned char *buffer;
    unsigned int step, rows;
    unsigned short directory;
};

#endif // LAULOOKUPTABLE_H
