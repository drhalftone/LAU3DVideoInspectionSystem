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

#ifndef LAUSCAN_H
#define LAUSCAN_H

#include <QTime>

#include "laumemoryobject.h"

class LAULookUpTable;

using namespace LAU3DVideoParameters;

class LAUScan : public LAUMemoryObject
{
public:
    explicit LAUScan(unsigned int cols = 0, unsigned int rows = 0, LAUVideoPlaybackColor clr = ColorUndefined);
    explicit LAUScan(QString fileName, int index = -1);
    ~LAUScan() { ; }

    LAUScan(libtiff::TIFF *inTiff);
    LAUScan(const LAUMemoryObject &other, LAUVideoPlaybackColor clr);
    LAUScan(const LAUScan &other);
    LAUScan &operator = (const LAUScan &other);
    LAUScan &operator + (const LAUScan &other);

    void updateLimits();

    bool save(QString filename = QString());
    bool save(libtiff::TIFF *otTiff, int index = 0);
    bool load(libtiff::TIFF *inTiff, int index = -1);
    bool loadInto(libtiff::TIFF *inTiff, int index = -1);
    bool loadInto(QString filename, int index = -1);
    bool saveAsUint8(QString filename = QString());

    QVector<float> pixel(int col, int row) const;
    QVector<float> pixel(QPoint point) const;
    QVector3D centroid() const
    {
        return (com);
    }

    void setFilename(const QString string)
    {
        fileString = string;
        if (parentString.isEmpty()){
            parentString = fileString;
        }
    }

    QString filename() const
    {
        return (fileString);
    }

    void setMake(const QString string)
    {
        makeString = string;
    }

    QString make() const
    {
        return (makeString);
    }

    void setModel(const QString string)
    {
        modelString = string;
    }

    QString model() const
    {
        return (modelString);
    }

    void setSoftware(const QString string)
    {
        softwareString = string;
    }

    QString serial() const
    {
        return (serialString);
    }

    void setSerial(const QString string)
    {
        serialString = string;
    }

    QString software() const
    {
        return (softwareString);
    }

    float minX() const
    {
        return (xMin);
    }

    float maxX() const
    {
        return (xMax);
    }

    float minY() const
    {
        return (yMin);
    }

    float maxY() const
    {
        return (yMax);
    }

    float minZ() const
    {
        return (zMin);
    }

    float maxZ() const
    {
        return (zMax);
    }

    QPointF zLimits() const
    {
        return (QPointF(zMin, zMax));
    }

    void setZLimits(QPointF point)
    {
        zMin = point.x();
        zMax = point.y();
    }

    void setZLimits(float zmn, float zmx)
    {
        zMin = zmn;
        zMax = zmx;
    }

    QPointF fieldOfView() const
    {
        return (fov);
    }

    void setFov(QPointF fv)
    {
        fov = fv;
    }

    QTime timeStamp() const
    {
        return (time);
    }

    void setTimeStamp(QTime tm)
    {
        time = tm;
    }

    QString parentName() const
    {
        return (parentString);
    }

    void setParentName(QString string)
    {
        parentString = string;
    }

    LAUScan rotate();
    LAUScan resize(unsigned int cols, unsigned int rows);
    LAUScan crop(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
    LAUScan flipLeftRight();
    LAUScan extractChannel(unsigned int channel) const;
    LAUScan transformScan(QMatrix4x4 mat);

    LAUMemoryObject channelsToFrames() const;

    void transformScanInPlace(QMatrix4x4 mat);

    QVector<float> const boundingBox();
    QMatrix4x4 const lookAt();
    QVector3D const center();

    QList<QImage> nearestNeighborMap();

    LAUVideoPlaybackColor color() const
    {
        return (playbackColor);
    }
    LAUScan convertToColor(LAUVideoPlaybackColor col) const;
    LAUScan maskChannel(unsigned int chn, float threshold) const;
    int extractXYZWVertices(float *toBuffer, int downSampleFactor = 1) const;
    int pointCount() const;

    QImage preview(QSize size, Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio);
#ifndef HEADLESS
#ifndef EXCLUDE_LAUSCANINSPECTOR
    bool approveImage(bool *doNotShowAgainCheckBoxEnabled = nullptr, QWidget *parent = nullptr) const;
    int inspectImage() const;
#else
    bool approveImage(bool *doNotShowAgainCheckBoxEnabled = nullptr, QWidget *parent = nullptr) const
    {
        Q_UNUSED(doNotShowAgainCheckBoxEnabled);
        Q_UNUSED(parent);
        return (true);
    }

    int inspectImage()
    {
        return (0);
    }
#endif
#endif
    static LAUScan fromRawDepth(const LAUMemoryObject &object, const LAULookUpTable &table);
    static LAUScan mergeScans(QList<LAUScan> scans);
    static LAUScan loadFromSKW(QString filename);
    static LAUScan loadFromCSV(QString filename);
    static LAUScan loadFromTIFF(QString filename);
    static float fitCircle(float *points, int length, int step, int offset, float *weights);

    static LAUVideoPlaybackColor whatColorIsThisTiffFile(QString filename, int frame = 0);

private:
    QTime time;
    QString fileString;
    QString makeString;
    QString modelString;
    QString serialString;
    QString softwareString;
    QString parentString;
    LAUVideoPlaybackColor playbackColor;

    float xMin, xMax, yMin, yMax, zMin, zMax;
    QVector3D com;
    QPointF fov;
};
#endif // LAUSCAN_H
