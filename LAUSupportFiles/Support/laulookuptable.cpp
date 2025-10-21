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

#include "laulookuptable.h"
#include "lauconstants.h"

#ifndef HEADLESS
#include <QFileDialog>
#include <QInputDialog>
#include <QProgressDialog>
#include <QThreadPool>
#endif

#include <locale.h>
#include <stdint.h>
#include <math.h>

int LAULookUpTableData::instanceCounter = 0;

using namespace libtiff;

// Structure to hold parameters for concurrent row processing
struct RowProcessingParams {
    int row;
    unsigned int width;
    unsigned int height;
    QMatrix3x3 intParameters;
    QVector<double> rdlParameters;
    QVector<double> tngParameters;
    double sclFactor;
    LAUMemoryObject* idealWorldCoordinates;
    float* buffer;
    float* xMinPtr;
    float* xMaxPtr;
    float* yMinPtr;
    float* yMaxPtr;
    double zMin;
    double zMax;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QPointF getDistortedCoordinates(QVector3D point, QMatrix3x3 intParameters, QVector<double> rdlParameters, QVector<double> tngParameters)
{
    // CREATE A QPOINTF TO RETURN TO THE USER
    QPointF distortedCoodinates;

    // NORMALIZE THE INCOMING WORLD COORDINATE BY Z
    point = point / point.z();

    // CALCULATE THE INTERMEDIATE DISTORTION PARAMETER R AND GAMMA
    double r = point.x()*point.x() + point.y()*point.y();
    double g = (1.0 + rdlParameters[0]*r + rdlParameters[1]*r*r + rdlParameters[2]*r*r*r) / (1.0 + rdlParameters[3]*r + rdlParameters[4]*r*r + rdlParameters[5]*r*r*r);

    // CALCULATE THE NORMALIZED WORLD COORDINATES AFTER RADIAL DISTORTION
    double x = point.x()*g + 2.0*tngParameters[0]*point.x()*point.y() + tngParameters[1]*(r + 2.0*point.x()*point.x());
    double y = point.y()*g + 2.0*tngParameters[1]*point.x()*point.y() + tngParameters[0]*(r + 2.0*point.y()*point.y());

    // NOW PROJECT DISTORTED WORLD COORDINATE TO CAMERA COORDINATE USING PIN HOLE CAMERA MODEL
    distortedCoodinates.setX(intParameters(0, 0)*x + intParameters(0, 2));
    distortedCoodinates.setY(intParameters(1, 1)*y + intParameters(1, 2));

    return(distortedCoodinates);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
double distanceBetweenTwoPoints(QPointF pointA, QPointF pointB)
{
    QPointF pointC = pointA - pointB;

    return(sqrt(pointC.x()*pointC.x() + pointC.y()*pointC.y()));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void processRow(RowProcessingParams& params) {
    int row = params.row;
    unsigned int width = params.width;
    float* buffer = params.buffer + (row * width * 12); // 12 channels per pixel

    // Local min/max values for this row
    float localXMin = +1e10;
    float localXMax = -1e10;
    float localYMin = +1e10;
    float localYMax = -1e10;

    double errorOpt = 1e6;
    double xw = 0.0, yw = 0.0, zw = 0.0;

    for (int col = 0; col < (int)width; col++) {
        double xi = (double) col;
        double yi = (double) row;

        // If this is the first column in a row, then set the position
        if (errorOpt > 1.0) {
            xw = ((double*)params.idealWorldCoordinates->scanLine(row))[3*col + 0];
            yw = ((double*)params.idealWorldCoordinates->scanLine(row))[3*col + 1];
            zw = ((double*)params.idealWorldCoordinates->scanLine(row))[3*col + 2];
        }

        for (int k = 0; k < 4; k++) {
            double dlt = 1.0;
            switch (k) {
            case 0: dlt = 1.0; break;
            case 1: dlt = 0.5; break;
            case 2: dlt = 0.25; break;
            case 3: dlt = 0.125; break;
            }

            while (1) {
                QPointF cO = getDistortedCoordinates(QVector3D(xw+0.0, yw+0.0, zw), params.intParameters, params.rdlParameters, params.tngParameters);
                QPointF cA = getDistortedCoordinates(QVector3D(xw-dlt, yw+0.0, zw), params.intParameters, params.rdlParameters, params.tngParameters);
                QPointF cB = getDistortedCoordinates(QVector3D(xw+dlt, yw+0.0, zw), params.intParameters, params.rdlParameters, params.tngParameters);
                QPointF cC = getDistortedCoordinates(QVector3D(xw+0.0, yw-dlt, zw), params.intParameters, params.rdlParameters, params.tngParameters);
                QPointF cD = getDistortedCoordinates(QVector3D(xw+0.0, yw+dlt, zw), params.intParameters, params.rdlParameters, params.tngParameters);

                double eO = distanceBetweenTwoPoints(cO, QPointF(xi, yi));
                double eA = distanceBetweenTwoPoints(cA, QPointF(xi, yi));
                double eB = distanceBetweenTwoPoints(cB, QPointF(xi, yi));
                double eC = distanceBetweenTwoPoints(cC, QPointF(xi, yi));
                double eD = distanceBetweenTwoPoints(cD, QPointF(xi, yi));

                errorOpt = qMin(eO, qMin(eA, qMin(eB, qMin(eC, eD))));
                if (eA == errorOpt) {
                    xw = xw - dlt;
                } else if (eB == errorOpt) {
                    xw = xw + dlt;
                } else if (eC == errorOpt) {
                    yw = yw - dlt;
                } else if (eD == errorOpt) {
                    yw = yw + dlt;
                } else {
                    break;
                }
            }
        }

        ((double*)params.idealWorldCoordinates->scanLine(row))[3*col + 0] = xw;
        ((double*)params.idealWorldCoordinates->scanLine(row))[3*col + 1] = yw;
        ((double*)params.idealWorldCoordinates->scanLine(row))[3*col + 2] = zw;

        int index = col * 12;

        // Define the Z to XY linear coefficients
        if (errorOpt < 0.1) {
            float xVal = (float)(xw / zw * params.zMin);
            float yVal = (float)(yw / zw * params.zMin);

            localXMin = qMin(localXMin, xVal);
            localXMax = qMax(localXMax, xVal);
            localYMin = qMin(localYMin, yVal);
            localYMax = qMax(localYMax, yVal);

            buffer[index++] = -xw / zw;  // A
            buffer[index++] = 0.0f;      // B
            buffer[index++] = +yw / zw;  // C
            buffer[index++] = 0.0f;      // D
        } else {
            buffer[index++] = NAN;  // A
            buffer[index++] = NAN;  // B
            buffer[index++] = NAN;  // C
            buffer[index++] = NAN;  // D
        }

        // Define the coefficients for a fourth order polynomial
        buffer[index++] = 0.0f;                          // E
        buffer[index++] = 0.0f;                          // F
        buffer[index++] = 0.0f;                          // G
        buffer[index++] = -65535.0f * params.sclFactor;  // H
        buffer[index++] = 0.0f;                          // I
        buffer[index++] = NAN;
        buffer[index++] = NAN;
        buffer[index++] = NAN;
    }

    // Update global min/max values atomically
    if (localXMin < 1e9) {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        *(params.xMinPtr) = qMin(*(params.xMinPtr), localXMin);
        *(params.xMaxPtr) = qMax(*(params.xMaxPtr), localXMax);
        *(params.yMinPtr) = qMin(*(params.yMinPtr), localYMin);
        *(params.yMaxPtr) = qMax(*(params.yMaxPtr), localYMax);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTableData::LAULookUpTableData()
{
    filename = QString();
    xmlString = QString();
    makeString = QString();
    modelString = QString();
    serialString = QString();
    softwareString = QString();
    style = StyleUndefined;

    scaleFactor = 0.25f;

    xMin = 0.0f;
    xMax = 0.0f;
    yMin = 0.0f;
    yMax = 0.0f;
    zMin = 0.0f;
    zMax = 0.0f;
    pMin = 0.0f;
    pMax = 1.0f;

    intrinsics = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    boundingBox = { NAN, NAN, NAN, NAN, NAN, NAN };

    numRows = 0;
    numCols = 0;
    numChns = 0;
    numSmps = 0;

    buffer = nullptr;
    phaseCorrectionBuffer = nullptr;
    transformMatrix = nullptr;
    projectionMatrix = nullptr;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTableData::LAULookUpTableData(const LAULookUpTableData &other) : LAULookUpTableData()
{
    qDebug() << QString("Performing deep copy on %1").arg(filename);

    scaleFactor = other.scaleFactor;

    xMin = other.xMin;
    xMax = other.xMax;
    yMin = other.yMin;
    yMax = other.yMax;
    zMin = other.zMin;
    zMax = other.zMax;
    pMin = other.pMin;
    pMax = other.pMax;

    numRows = other.numRows;
    numCols = other.numCols;
    numChns = other.numChns;

    style = other.style;
    xmlString = other.xmlString;
    makeString = other.makeString;
    filename = other.filename;
    serialString = other.serialString;
    modelString = other.modelString;
    softwareString = other.softwareString;
    intrinsics = other.intrinsics;
    boundingBox = other.boundingBox;

    allocateBuffer();
    memcpy(buffer, other.buffer, numSmps * sizeof(float));
    memcpy(phaseCorrectionBuffer, other.phaseCorrectionBuffer, LENGTHPHASECORRECTIONTABLE*sizeof(float));
    memcpy((void *)transformMatrix->constData(), (void *)other.transformMatrix->constData(), sizeof(QMatrix4x4));
    memcpy((void *)projectionMatrix->constData(), (void *)other.projectionMatrix->constData(), sizeof(QMatrix4x4));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTableData::~LAULookUpTableData()
{
    qDebug() << QString("LAULookUpTableData::~LAULookUpTableData() %1").arg(--instanceCounter);
    if (buffer != nullptr) {
        _mm_free(buffer);
        _mm_free(phaseCorrectionBuffer);
        delete transformMatrix;
        delete projectionMatrix;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAULookUpTableData::allocateBuffer()
{
    qDebug() << QString("LAULookUpTableData::allocateBuffer() %1").arg(instanceCounter++) << "Size: " << numRows << " x " << numCols;

    // ALLOCATE SPACE FOR HOLDING PIXEL DATA BASED ON NUMBER OF CHANNELS AND BYTES PER PIXEL
    numSmps  = (unsigned long long)numRows;
    numSmps *= (unsigned long long)numCols;
    numSmps *= (unsigned long long)numChns;

    if (numSmps) {
        buffer = _mm_malloc(numSmps * sizeof(float), 16);
        if (buffer == nullptr) {
            qDebug() << QString("LAULookUpTableData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
            qDebug() << QString("LAULookUpTableData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
            qDebug() << QString("LAULookUpTableData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
        } else {
            phaseCorrectionBuffer = _mm_malloc(LENGTHPHASECORRECTIONTABLE * sizeof(float), 16);
            transformMatrix = new QMatrix4x4();
            projectionMatrix = new QMatrix4x4();
        }
    } else {
        buffer = nullptr;
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable::LAULookUpTable(unsigned int cols, unsigned int rows, LAUVideoPlaybackDevice device, float hFov, float vFov, float zMin, float zMax, float pMin, float pMax)
{
    data = new LAULookUpTableData();

    if (device == DeviceProsilicaGRY) {
        return;
    }

    data->numRows = rows;
    data->numCols = cols;
    data->numChns = 12;
    data->allocateBuffer();

    // SET THE PHASE CORRECTION TABLE TO THE DEFAULT LINEAR MAPPING
    if (data->phaseCorrectionBuffer){
        for (int n = 0; n < LENGTHPHASECORRECTIONTABLE; n++){
            ((float*)(data->phaseCorrectionBuffer))[n] = (float)n / (float)(LENGTHPHASECORRECTIONTABLE-1);
        }
    }

    // THE DEFAULT LOOK UP TABLE IS A FOURTH ORDER POLYNOMIAL
    data->style = StyleFourthOrderPoly;

    data->xMin = -1.2f;
    data->xMax = 1.2f;
    data->yMin = -1.2f;
    data->yMax = 1.2f;
    data->zMin = -qMax(qAbs(zMax), qAbs(zMin));
    data->zMax = -qMin(qAbs(zMax), qAbs(zMin));
    data->pMin = pMin;
    data->pMax = pMax;

    data->verticalFieldOfView = vFov;
    data->horizontalFieldOfView = hFov;

    if (rows * cols > 0) {
        // CREATE BUFFER TO HOLD ABCD AND EFGH TEXTURES
        float *buffer = (float *)data->buffer;

        // INITIALIZE ABCDEFGH COEFFICIENTS
        if (device == DeviceProsilicaIOS || device == DeviceProsilicaLCG || device == DeviceProsilicaDPR || device == DeviceProsilicaAST) {
            // SET THE Z LIMITS AND CALCULATE THE FIELD OF VIEW
            data->pMin = 0.0f;
            data->pMax = 1.0f;
            data->zMin = -110.0f;
            data->zMax = -90.0f;
            data->yMin = -(float)(data->numRows / 2);
            data->yMax = -data->yMin;
            data->xMin = -(float)(data->numCols / 2);
            data->xMax = -data->xMin;

            float   phiA = atan(data->yMin / data->zMin);
            float   phiB = atan(data->yMax / data->zMin);
            float thetaA = atan(data->xMin / data->zMin);
            float thetaB = atan(data->xMax / data->zMin);

            data->horizontalFieldOfView = fabs(thetaA) + fabs(thetaB);
            data->verticalFieldOfView = fabs(phiA) + fabs(phiB);

            float lftEdge = -100.0f * tan(data->horizontalFieldOfView / 2.0f);
            float rghEdge = -lftEdge;
            float topEdge =  100.0f * tan(data->horizontalFieldOfView / 2.0f);
            float btmEdge = -topEdge;

            int index = 0;
            for (unsigned int row = 0; row < data->numRows; row++) {
                for (unsigned int col = 0; col < data->numCols; col++) {
                    float lambdaX = (float)col / (float)(data->numCols - 1);
                    float lambdaY = (float)row / (float)(data->numRows - 1);

                    // DEFINE THE Z TO XY LINEAR COEFFICIENTS
                    buffer[index++] =   0.0f; // A
                    buffer[index++] = lftEdge * (1.0f - lambdaX) + rghEdge * lambdaX; // B
                    buffer[index++] =   0.0f; // C
                    buffer[index++] = btmEdge * (1.0f - lambdaY) + topEdge * lambdaY; // D

                    // DEFINE THE COEFFICIENTS FOR A FOURTH ORDER POLYNOMIAL
                    buffer[index++] =    0.0f;   // E
                    buffer[index++] =    0.0f;   // F
                    buffer[index++] =    0.0f;   // G
                    buffer[index++] =    0.0f;   // H
                    buffer[index++] = -100.0f;   // I
                    buffer[index++] =     NAN;   // F
                    buffer[index++] =     NAN;   // G
                    buffer[index++] =     NAN;   // H
                }
            }
        } else if (device == DeviceKinect || device == DeviceLucid || device == DeviceOrbbec || device == DeviceVZense) {
            // SET THE Z LIMITS AND CALCULATE THE FIELD OF VIEW
            data->pMin = 0.0f;
            data->pMax = 1.0f;
            data->yMin = tan(data->verticalFieldOfView / 2.0f) * data->zMin;
            data->yMax = -data->yMin;
            data->xMin = tan(data->horizontalFieldOfView / 2.0f) * data->zMin;
            data->xMax = -data->xMin;

            int index = 0;
            for (unsigned int row = 0; row < data->numRows; row++) {
                for (unsigned int col = 0; col < data->numCols; col++) {
                    // DEFINE THE Z TO XY LINEAR COEFFICIENTS
                    buffer[index++] = -tan((((float)col + 0.5f) / (float)(data->numCols) - 0.5f) * data->horizontalFieldOfView); // A
                    buffer[index++] =  0.0f; // B
                    buffer[index++] =  tan((((float)row + 0.5f) / (float)(data->numRows) - 0.5f) * data->verticalFieldOfView); // C
                    buffer[index++] =  0.0f; // D

                    // DEFINE THE COEFFICIENTS FOR A FOURTH ORDER POLYNOMIAL
                    buffer[index++] =      0.0f; // E
                    buffer[index++] =      0.0f; // F
                    buffer[index++] =      0.0f; // G
#ifdef AZUREKINECT
                    buffer[index++] = -65535.0f; // H
                    buffer[index++] =      0.0f; // I
#else
                    buffer[index++] = -65130.7f; // H
                    buffer[index++] =    -20.0f; // I
#endif
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
                }
            }
            setMakeString(QString("Microsoft"));
#ifdef AZUREKINECT
            setModelString(QString("Azure Kinect"));
#else
            setModelString(QString("Kinect V2"));
#endif
        } else if (device == DeviceOrbbec) {
            // SET THE Z LIMITS AND CALCULATE THE FIELD OF VIEW
            data->pMin = 0.0f;
            data->pMax = 1.0f;
            data->yMin = tan(data->verticalFieldOfView / 2.0f) * data->zMin;
            data->yMax = -data->yMin;
            data->xMin = tan(data->horizontalFieldOfView / 2.0f) * data->zMin;
            data->xMax = -data->xMin;

            int index = 0;
            for (unsigned int row = 0; row < data->numRows; row++) {
                for (unsigned int col = 0; col < data->numCols; col++) {
                    // DEFINE THE Z TO XY LINEAR COEFFICIENTS
                    buffer[index++] = -tan((((float)col + 0.5f) / (float)(data->numCols) - 0.5f) * data->horizontalFieldOfView); // A
                    buffer[index++] =  0.0f; // B
                    buffer[index++] =  tan((((float)row + 0.5f) / (float)(data->numRows) - 0.5f) * data->verticalFieldOfView); // C
                    buffer[index++] =  0.0f; // D

                    // DEFINE THE COEFFICIENTS FOR A FOURTH ORDER POLYNOMIAL
                    buffer[index++] =      0.0f; // E
                    buffer[index++] =      0.0f; // F
                    buffer[index++] =      0.0f; // G
                    buffer[index++] = -65535.0f; // H
                    buffer[index++] =      0.0f; // I
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
                }
            }
            setMakeString(QString("Orbbec"));
            setModelString(QString("Femto Mega I"));
        } else if (device == DeviceVidu) {
            // SET THE Z LIMITS AND CALCULATE THE FIELD OF VIEW
            data->pMin = 0.0f;
            data->pMax = 1.0f;
            data->yMin = tan(data->verticalFieldOfView / 2.0f) * data->zMin;
            data->yMax = -data->yMin;
            data->xMin = tan(data->horizontalFieldOfView / 2.0f) * data->zMin;
            data->xMax = -data->xMin;

            int index = 0;
            for (unsigned int row = 0; row < data->numRows; row++) {
                for (unsigned int col = 0; col < data->numCols; col++) {
                    // DEFINE THE Z TO XY LINEAR COEFFICIENTS
                    buffer[index++] = -tan((((float)col + 0.5f) / (float)(data->numCols) - 0.5f) * data->horizontalFieldOfView); // A
                    buffer[index++] =  0.0f; // B
                    buffer[index++] =  tan((((float)row + 0.5f) / (float)(data->numRows) - 0.5f) * data->verticalFieldOfView); // C
                    buffer[index++] =  0.0f; // D

                    // DEFINE THE COEFFICIENTS FOR A FOURTH ORDER POLYNOMIAL
                    buffer[index++] =      0.0f; // E
                    buffer[index++] =      0.0f; // F
                    buffer[index++] =      0.0f; // G
                    buffer[index++] = -65535.0f; // H
                    buffer[index++] =      0.0f; // I
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
                }
            }
            setMakeString(QString("Vidu"));
            setModelString(QString("Okulo"));
        } else if (device == DevicePrimeSense) {
            // SET THE Z LIMITS AND CALCULATE THE FIELD OF VIEW
            data->pMin = 0.0f;
            data->pMax = 1.0f;
            data->yMin = tan(data->verticalFieldOfView / 2.0f) * data->zMin;
            data->yMax = -data->yMin;
            data->xMin = tan(data->horizontalFieldOfView / 2.0f) * data->zMin;
            data->xMax = -data->xMin;

            int index = 0;
            for (unsigned int row = 0; row < data->numRows; row++) {
                for (unsigned int col = 0; col < data->numCols; col++) {
                    // DEFINE THE Z TO XY LINEAR COEFFICIENTS
                    buffer[index++] = -tan((((float)col + 0.5f) / (float)(data->numCols) - 0.5f) * data->horizontalFieldOfView); // A
                    buffer[index++] =  0.0f; // B
                    buffer[index++] = tan((((float)row + 0.5f) / (float)(data->numRows) - 0.5f) * data->verticalFieldOfView); // C
                    buffer[index++] = 0.0f; // D
#ifdef STRUCTURECORE
                    // DEFINE THE COEFFICIENTS FOR A FOURTH ORDER POLYNOMIAL
                    buffer[index++] =      0.0f; // E
                    buffer[index++] =      0.0f; // F
                    buffer[index++] =      0.0f; // G
                    buffer[index++] = -65535.0f; // H
                    buffer[index++] =      0.0f; // I
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
#else
                    // DEFINE THE COEFFICIENTS FOR A FOURTH ORDER POLYNOMIAL
                    buffer[index++] =      0.0f; // E
                    buffer[index++] =      0.0f; // F
                    buffer[index++] =      0.0f; // G
                    buffer[index++] =  -6185.7f; // H
                    buffer[index++] =    -62.0f; // I
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
#endif
                }
            }
        } else if (device == DeviceRealSense) {
            // SET THE Z LIMITS AND CALCULATE THE FIELD OF VIEW
            data->pMin = 0.0f;
            data->pMax = 1.0f;
            data->yMin = tan(data->verticalFieldOfView / 2.0f) * data->zMin;
            data->yMax = -data->yMin;
            data->xMin = tan(data->horizontalFieldOfView / 2.0f) * data->zMin;
            data->xMax = -data->xMin;

            int index = 0;
            for (unsigned int row = 0; row < data->numRows; row++) {
                for (unsigned int col = 0; col < data->numCols; col++) {
                    // DEFINE THE Z TO XY LINEAR COEFFICIENTS
                    buffer[index++] = -tan((((float)col + 0.5f) / (float)(data->numCols) - 0.5f) * data->horizontalFieldOfView); // A
                    buffer[index++] =    0.0f; // B
                    buffer[index++] =  tan((((float)row + 0.5f) / (float)(data->numRows) - 0.5f) * data->verticalFieldOfView); // C
                    buffer[index++] =    0.0f; // D

                    // DEFINE THE COEFFICIENTS FOR A FOURTH ORDER POLYNOMIAL
                    buffer[index++] =      0.0f; // E
                    buffer[index++] =      0.0f; // F
                    buffer[index++] =      0.0f; // G
                    buffer[index++] = -65535.0f; // H
                    buffer[index++] =      0.0f; // I
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
                    buffer[index++] =       NAN;
                }
            }
        }

        // CREATE A DUMMY LOOK UP TABLE SUCH THAT X AND Y ARE EQUAL TO NORMALIZED CAMERA COORDINATES REGARDLESS OF Z
        if (hFov < 0.0f && vFov < 0.0f){
            int index = 0;
            for (unsigned int row = 0; row < data->numRows; row++) {
                for (unsigned int col = 0; col < data->numCols; col++) {
                    // DEFINE THE Z TO XY LINEAR COEFFICIENTS
                    buffer[index++] =  0.0f;
                    buffer[index++] =  2.0f * ((float)col/(float)(data->numCols - 1) - 0.5f);
                    buffer[index++] =  0.0f;
                    buffer[index++] = -2.0f * ((float)row/(float)(data->numRows - 1) - 0.5f);
                    index += 8;
                }
            }
            data->xMin = -1.0f;
            data->xMax = +1.0f;
            data->yMin = -1.0f;
            data->yMax = +1.0f;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable LAULookUpTable::generateTableFromJETR(unsigned int cols, unsigned int rows, QVector<double> justEnoughToReconstruct, QWidget *widget, bool *completed)
{
    // EXTRACT ALL THE INFORMATION WE WILL NEED FROM THE USER SUPPLIED JETR VECTOR
    QMatrix3x3 intParameters;
    intParameters(0, 0) = justEnoughToReconstruct[0];
    intParameters(0, 1) = 0.0f;
    intParameters(0, 2) = justEnoughToReconstruct[1];

    intParameters(1, 0) = 0.0f;
    intParameters(1, 1) = justEnoughToReconstruct[2];
    intParameters(1, 2) = justEnoughToReconstruct[3];

    intParameters(2, 0) = 0.0f;
    intParameters(2, 1) = 0.0f;
    intParameters(2, 2) = 1.0f;

    QVector<double> rdlParameters(6);
    for (int n = 0; n < 6; n++){
        rdlParameters[n] = justEnoughToReconstruct[4 + n];
    }

    QVector<double> tngParameters(2);
    tngParameters[0] = justEnoughToReconstruct[10];
    tngParameters[1] = justEnoughToReconstruct[11];

    double sclFactor = justEnoughToReconstruct[34];
    double zMin = justEnoughToReconstruct[35];
    double zMax = justEnoughToReconstruct[36];

    // PASS EVERYTHING ON TO THE LOOK UP TABLE CONSTRUCTOR AND RETURN TO THE USER
    LAULookUpTable table(cols, rows, intParameters, rdlParameters, tngParameters, sclFactor, zMin, zMax, widget, completed);

    // SET THE TRANSFORM MATRIX (this is what gets stored in XML and returned by jetr())
    QMatrix4x4 transformMatrix;
    int index = 12;
    for (int row = 0; row < 4; row++){
        for (int col = 0; col < 4; col++){
            transformMatrix(row,col) = justEnoughToReconstruct[index++];
        }
    }
    table.setTransform(transformMatrix);

    // SET THE BOUNDING BOX IN THE LOOK UP TABLE
    table.setBoundingBox(LookUpTableBoundingBox{ justEnoughToReconstruct[28], justEnoughToReconstruct[29], justEnoughToReconstruct[30], justEnoughToReconstruct[31], justEnoughToReconstruct[32], justEnoughToReconstruct[33] });

    return(table);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable LAULookUpTable::generateTableFromJETR(unsigned int cols, unsigned int rows, QVector<double> justEnoughToReconstruct, const QString &make, const QString &model, QWidget *widget, bool *completed)
{
    // CHECK IF THIS IS AN ORBBEC FEMTO CAMERA THAT NEEDS SPECIAL HANDLING
    if (make.toLower() == "orbbec" && model.contains("Femto", Qt::CaseInsensitive)) {
        // FOR ORBBEC FEMTO CAMERAS, GENERATE LUT AT 640x576, ROTATE 180, THEN CROP TO 640x480
        const unsigned int nativeCols = LAU_CAMERA_DEFAULT_WIDTH;
        const unsigned int nativeRows = 576;

        // GENERATE LUT AT NATIVE RESOLUTION USING THE ORIGINAL METHOD
        LAULookUpTable nativeLut = generateTableFromJETR(nativeCols, nativeRows, justEnoughToReconstruct, widget, completed);

        // SET MAKE AND MODEL STRINGS
        nativeLut.setMakeString(make);
        nativeLut.setModelString(model);

        // FLIP THE LOOK UP TABLE SINCE WE ARE GOING TO FLIP THE RAW VIDEO
        nativeLut.rotate180InPlace();

        // CROP THE TABLE IF THE CAMERA DIMENSIONS ARE LARGER THAN THE OBJECT DIMENSIONS
        if (nativeCols > cols || nativeRows > rows) {
            // CROP THE IMAGE IF THERE IS DISPARITY BETWEEN SENSOR AND DEPTH DIMENSIONS
            unsigned int lft = (nativeCols - cols) / 2;
            unsigned int top = (nativeRows - rows) / 2;
            return nativeLut.crop(lft, top, cols, rows);
        }

        return nativeLut;
    }

    // FOR ALL OTHER CAMERAS, USE THE STANDARD METHOD
    LAULookUpTable table = generateTableFromJETR(cols, rows, justEnoughToReconstruct, widget, completed);
    table.setMakeString(make);
    table.setModelString(model);
    return(table);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable LAULookUpTable::generateTableFromJETR(unsigned int cols, unsigned int rows, QVector<double> justEnoughToReconstruct, const QString &make, const QString &model, const QDate &folderDate, QWidget *widget, bool *completed)
{
    // CHECK IF THIS IS AN ORBBEC FEMTO CAMERA THAT NEEDS SPECIAL HANDLING
    if (make.toLower() == "orbbec" && model.contains("Femto", Qt::CaseInsensitive)) {
        // FOR ORBBEC FEMTO CAMERAS, GENERATE LUT AT 640x576, POTENTIALLY ROTATE 180, THEN CROP TO 640x480
        const unsigned int nativeCols = LAU_CAMERA_DEFAULT_WIDTH;
        const unsigned int nativeRows = 576;

        // GENERATE LUT AT NATIVE RESOLUTION USING THE ORIGINAL METHOD
        LAULookUpTable nativeLut = generateTableFromJETR(nativeCols, nativeRows, justEnoughToReconstruct, widget, completed);

        // SET MAKE AND MODEL STRINGS
        nativeLut.setMakeString(make);
        nativeLut.setModelString(model);

        // CHECK IF WE SHOULD ROTATE BASED ON THE FOLDER DATE
        // CAMERA MOUNTING CHANGED ON SEPTEMBER 5TH, 2025 - NO ROTATION NEEDED FOR SEP 6TH AND LATER
        // DEFAULT TO NO ROTATION IF DATE IS INVALID (ASSUME NEWER DATA)
        QDate mountingChangeDate(2025, 9, 6);
        bool shouldRotate = folderDate.isValid() && folderDate < mountingChangeDate;

        if (shouldRotate) {
            // FLIP THE LOOK UP TABLE SINCE WE ARE GOING TO FLIP THE RAW VIDEO
            nativeLut.rotate180InPlace();
        }

        // CROP THE TABLE IF THE CAMERA DIMENSIONS ARE LARGER THAN THE OBJECT DIMENSIONS
        if (nativeCols > cols || nativeRows > rows) {
            // CROP THE IMAGE IF THERE IS DISPARITY BETWEEN SENSOR AND DEPTH DIMENSIONS
            unsigned int lft = (nativeCols - cols) / 2;
            unsigned int top = (nativeRows - rows) / 2;
            return nativeLut.crop(lft, top, cols, rows);
        }

        return nativeLut;
    }

    // FOR ALL OTHER CAMERAS, USE THE STANDARD METHOD
    LAULookUpTable table = generateTableFromJETR(cols, rows, justEnoughToReconstruct, widget, completed);
    table.setMakeString(make);
    table.setModelString(model);
    return(table);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QDate LAULookUpTable::parseFolderDate(const QString &folderName)
{
    // Parse folder names in format "Folder########" where ######## is YYYYMMDD
    if (folderName.startsWith("Folder", Qt::CaseInsensitive) && folderName.length() >= 14) {
        QString dateStr = folderName.right(8); // Extract last 8 characters (YYYYMMDD)
        return QDate::fromString(dateStr, "yyyyMMdd");
    }

    // Try to parse dates in other common formats like "########" (YYYYMMDD)
    if (folderName.length() == 8) {
        bool ok;
        folderName.toInt(&ok); // Check if it's all digits
        if (ok) {
            return QDate::fromString(folderName, "yyyyMMdd");
        }
    }

    return QDate(); // Invalid date if parsing fails
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable::LAULookUpTable(unsigned int cols, unsigned int rows, QMatrix3x3 intParameters, QVector<double> rdlParameters, QVector<double> tngParameters, double sclFactor, double zMin, double zMax, QWidget *widget, bool *completed)
{
    // Initialize completed flag to false (will be set to true if generation completes)
    if (completed) {
        *completed = false;
    }

    // MAKE SURE Z LIMITS ARE NEGATIVE
    if (zMin > 0) {
        zMin = -zMin;
    }

    if (zMax > 0) {
        zMax = -zMax;
    }

    // MAKE SURE THAT ZMIN IS FARTHER AWAY FROM THE ORIGIN THAN ZMAX
    if (zMin > zMax) {
        double val = zMin;
        zMin = zMax;
        zMax = val;
    }

    data = new LAULookUpTableData();
    data->numRows = rows;
    data->numCols = cols;
    data->numChns = 12;
    data->allocateBuffer();

    // SAVE THE SCALE FACTOR
    data->scaleFactor = sclFactor;

    // SET THE Z LIMITS FROM THE USER SUPPLIED VALUES
    data->xMin = +1e10;
    data->xMax = -1e10;
    data->yMin = +1e10;
    data->yMax = -1e10;
    data->zMin = zMin;
    data->zMax = zMax;

    // COPY OVER THE INTRINSICS
    data->intrinsics.fx = intParameters(0, 0);
    data->intrinsics.cx = intParameters(0, 2);
    data->intrinsics.fy = intParameters(1, 1);
    data->intrinsics.cy = intParameters(1, 2);
    data->intrinsics.k1 = rdlParameters[0];
    data->intrinsics.k2 = rdlParameters[1];
    data->intrinsics.k3 = rdlParameters[2];
    data->intrinsics.k4 = rdlParameters[3];
    data->intrinsics.k5 = rdlParameters[4];
    data->intrinsics.k6 = rdlParameters[5];
    data->intrinsics.p1 = tngParameters[0];
    data->intrinsics.p2 = tngParameters[1];

    // SET THE PHASE CORRECTION TABLE TO THE DEFAULT LINEAR MAPPING
    if (data->phaseCorrectionBuffer){
        for (int n = 0; n < LENGTHPHASECORRECTIONTABLE; n++){
            ((float*)(data->phaseCorrectionBuffer))[n] = (float)n / (float)(LENGTHPHASECORRECTIONTABLE-1);
        }
    }

    float *buffer = (float *)(data->buffer);

#ifndef DONTCOMPILE
    // LET'S CREATE A BUFFER TO HOLD THE IDEAL WORLD COORDINATES THAT MAP TO PIXELS OF THE IMAGE SENSOR
    LAUMemoryObject idealWorldCoordinates(width(), height(), 3, sizeof(double));

    // WE NEED TO FIND THE RANGE OF ROW AND COLUMN COORDINATES CAUSED BY DISTORTION
    for (int row = 0; row < (int)height(); row++) {
        for (int col = 0; col < (int)width(); col++) {
            ((double*)idealWorldCoordinates.scanLine(row))[3*col + 0] = 1000.0 * ((double)col - intParameters(0, 2)) / intParameters(0, 0);
            ((double*)idealWorldCoordinates.scanLine(row))[3*col + 1] = 1000.0 * ((double)row - intParameters(1, 2)) / intParameters(1, 1);
            ((double*)idealWorldCoordinates.scanLine(row))[3*col + 2] = 1000.0;
        }
    }
    //idealWorldCoordinates.save(QString("C:/Users/Public/Pictures/idealPinHole.tif"));

    // Prepare parameters for concurrent processing
    QList<RowProcessingParams> rowParams;
    for (int row = 0; row < (int)height(); row++) {
        RowProcessingParams params;
        params.row = row;
        params.width = width();
        params.height = height();
        params.intParameters = intParameters;
        params.rdlParameters = rdlParameters;
        params.tngParameters = tngParameters;
        params.sclFactor = sclFactor;
        params.idealWorldCoordinates = &idealWorldCoordinates;
        params.buffer = buffer;
        params.xMinPtr = &data->xMin;
        params.xMaxPtr = &data->xMax;
        params.yMinPtr = &data->yMin;
        params.yMaxPtr = &data->yMax;
        params.zMin = zMin;
        params.zMax = zMax;
        rowParams.append(params);
    }

    // Process rows concurrently
#ifndef HEADLESS
    QFutureWatcher<void> watcher;
    QProgressDialog *concurrentProgressDialog = nullptr;

    // Only create progress dialog if widget parent is provided
    if (widget) {
        concurrentProgressDialog = new QProgressDialog(QString("Building look up table (concurrent)..."), QString("Abort"), 0, height(), widget, Qt::Sheet);
        concurrentProgressDialog->setModal(Qt::WindowModal);
        concurrentProgressDialog->show();

        QObject::connect(&watcher, &QFutureWatcher<void>::progressRangeChanged, concurrentProgressDialog, &QProgressDialog::setRange);
        QObject::connect(&watcher, &QFutureWatcher<void>::progressValueChanged, concurrentProgressDialog, &QProgressDialog::setValue);
        QObject::connect(concurrentProgressDialog, &QProgressDialog::canceled, &watcher, &QFutureWatcher<void>::cancel);
    } else {
        // Print console progress when no UI is available
        qDebug() << "Building lookup table" << cols << "x" << rows << "(concurrent)...";
    }

    // Use half the available CPU cores to reduce memory bandwidth contention
    QThreadPool *pool = QThreadPool::globalInstance();
    int originalMaxThreadCount = pool->maxThreadCount();
    int halfCoreCount = qMax(1, QThread::idealThreadCount() / 2);
    pool->setMaxThreadCount(halfCoreCount);
    qDebug() << "LUT generation using" << halfCoreCount << "threads (half of" << QThread::idealThreadCount() << "cores)";

    QFuture<void> future = QtConcurrent::map(rowParams, processRow);
    watcher.setFuture(future);

    while (!future.isFinished()) {
        // Always process events to keep application responsive
        qApp->processEvents();
        if (widget && concurrentProgressDialog && concurrentProgressDialog->wasCanceled()) {
            future.cancel();
            break;
        }
        QThread::msleep(100);  // Reduced from 10ms to 100ms to minimize overhead
    }

    future.waitForFinished();

    // Restore original thread count
    pool->setMaxThreadCount(originalMaxThreadCount);

    // Check if generation was cancelled or completed
    bool wasCancelled = (widget && concurrentProgressDialog && concurrentProgressDialog->wasCanceled());

    if (concurrentProgressDialog) {
        // Only update progress value if not cancelled (avoids visual flicker)
        if (!wasCancelled) {
            concurrentProgressDialog->setValue(height());
        }
        delete concurrentProgressDialog;
    } else if (!widget) {
        // Print completion message when no UI is available
        qDebug() << "Lookup table generation completed.";
    }

    // Set completed flag if generation finished successfully (not cancelled)
    if (completed) {
        *completed = !wasCancelled;
    }
#else
    QtConcurrent::blockingMap(rowParams, processRow);
#endif
    //idealWorldCoordinates.save(QString("C:/Users/Public/Pictures/idealDistorted.tif"));
#else
    QVector<double> k(3), p(2);
    k[0] = dstParameters[0];
    k[1] = dstParameters[1];
    p[0] = dstParameters[2];
    p[1] = dstParameters[3];
    k[2] = dstParameters[4];

    for (unsigned int row = 0; row < height(); row++) {
        double R = (double)row / (double)(height() - 1);
        double Rc = (R - (intParameters(1, 2) / (double)(height() - 1)));
        for (unsigned int col = 0; col < width(); col++) {
            double C  = (double)col / (double)(width() - 1);
            double Cc = (C - (intParameters(0, 2) / (double)(width() - 1)));

            double D = Rc * Rc + Cc * Cc;

            double Cu = (C - Cc * (k[0] * D + k[1] * D * D + k[2] * D * D * D) - (p[0] * (D + 2 * (Cc * Cc)) + p[1] * Cc * Rc)) * width();
            double Ru = (R - Rc * (k[0] * D + k[1] * D * D + k[2] * D * D * D) - (p[1] * (D + 2 * (Cc * Cc)) + p[0] * Cc * Rc)) * height();

            // DEFINE THE Z TO XY LINEAR COEFFICIENTS
            buffer[index++] = (intParameters(0, 2) - Cu) / intParameters(0, 0);  // A
            buffer[index++] = 0.0f;                                              // B
            buffer[index++] = (Ru - intParameters(1, 2)) / intParameters(1, 1);  // C
            buffer[index++] = 0.0f;                                              // D

            // DEFINE THE COEFFICIENTS FOR A FOURTH ORDER POLYNOMIAL
            buffer[index++] =                    0.0f; // E
            buffer[index++] =                    0.0f; // F
            buffer[index++] =                    0.0f; // G
            buffer[index++] = -65535.0f * scaleFactor; // H
            buffer[index++] =                    0.0f; // I
            buffer[index++] =                     NAN;
            buffer[index++] =                     NAN;
            buffer[index++] =                     NAN;
        }
    }
#endif

#ifdef DONTCOMPILE
    // LET'S SEE WHAT THIS CAMERA IS LOOKING AT
    float *centerPixel = (float*)this->constScanLine(height()/2) + (12 * width()/2);

    float xOff = centerPixel[0] * (zMin + zMax)/2.0;
    float yOff = centerPixel[2] * (zMin + zMax)/2.0;

    // UPDATE THE X AND Y OFFSETS SO THAT THE SCAN IS CENTERED AT THE ORIGIN
    for (unsigned int row = 0; row < height(); row++){
        float *buffer = (float*)this->constScanLine(row);
        for (unsigned int col = 0; col < width(); col++){
            buffer[12*col + 1] = -xOff;
            buffer[12*col + 3] = -yOff;
        }
    }

    data->xMax -= xOff;
    data->xMin -= xOff;
    data->yMax -= yOff;
    data->yMin -= yOff;
#endif

    // THE DEFAULT LOOK UP TABLE IS A FOURTH ORDER POLYNOMIAL
    data->style = StyleFourthOrderPoly;

    // EXPAND THE LIMITS SO THAT ENTIRE FIELD OF VIEW FITS INSIDE SYMMETRICALLY
    data->xMax = qMax(data->xMax, -data->xMin);
    data->xMin = -data->xMax;
    data->yMax = qMax(data->yMax, -data->yMin);
    data->yMin = -data->yMax;

    // DERIVE THE FIELDS OF VIEW
    if (qAbs(data->zMin) > qAbs(data->zMax)) {
        float   phiA = atan(data->yMin / data->zMin);
        float   phiB = atan(data->yMax / data->zMin);
        float thetaA = atan(data->xMin / data->zMin);
        float thetaB = atan(data->xMax / data->zMin);

        data->verticalFieldOfView = fabs(phiA) + fabs(phiB);
        data->horizontalFieldOfView = fabs(thetaA) + fabs(thetaB);
    } else {
        float   phiA = atan(data->yMin / data->zMax);
        float   phiB = atan(data->yMax / data->zMax);
        float thetaA = atan(data->xMin / data->zMax);
        float thetaB = atan(data->xMax / data->zMax);

        data->verticalFieldOfView = fabs(phiA) + fabs(phiB);
        data->horizontalFieldOfView = fabs(thetaA) + fabs(thetaB);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable::LAULookUpTable(unsigned int cols, unsigned int rows, unsigned int chns, LAULookUpTableStyle stl, float hFov, float vFov, float zMin, float zMax, float pMin, float pMax)
{
    // CREATE AND ALLOCATE THE SHARED MEMORY OBJECT
    data = new LAULookUpTableData();
    data->numRows = rows;
    data->numCols = cols;
    data->numChns = chns;
    data->allocateBuffer();

    // SET THE PHASE CORRECTION TABLE TO THE DEFAULT LINEAR MAPPING
    if (data->phaseCorrectionBuffer){
        for (int n = 0; n < LENGTHPHASECORRECTIONTABLE; n++){
            ((float*)(data->phaseCorrectionBuffer))[n] = (float)n / (float)(LENGTHPHASECORRECTIONTABLE-1);
        }
    }

    // THE DEFAULT LOOK UP TABLE IS A FOURTH ORDER POLYNOMIAL
    data->style = stl;

    data->zMin = -qMax(qAbs(zMax), qAbs(zMin));
    data->zMax = -qMin(qAbs(zMax), qAbs(zMin));
    data->pMin = pMin;
    data->pMax = pMax;

    data->xMin = -qAbs(data->zMin) * tan(hFov / 2.0);
    data->xMax =  qAbs(data->zMin) * tan(hFov / 2.0);
    data->yMin = -qAbs(data->zMin) * tan(vFov / 2.0);
    data->yMax =  qAbs(data->zMin) * tan(vFov / 2.0);

    data->verticalFieldOfView = vFov;
    data->horizontalFieldOfView = hFov;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable::LAULookUpTable(unsigned int cols, unsigned int rows, LAULookUpTableStyle stl, float hFov, float vFov, float zMin, float zMax, float pMin, float pMax)
{
    data = new LAULookUpTableData();
    data->numRows = rows;
    data->numCols = cols;
    data->style = stl;

    switch (stl) {
        case StyleLinear:
            data->numChns = 8;
            break;
        case StyleFourthOrderPoly:
            data->numChns = 12;
            break;
        case StyleFourthOrderPolyAugmentedReality:
            data->numChns = 16;
            break;
        case StyleFourthOrderPolyWithPhaseUnwrap:
            data->numChns = 16;
            break;
        case StyleXYZPLookUpTable:
            data->numChns = 4 * 15;
            break;
        case StyleXYZWRCPQLookUpTable:
            data->numChns = 8 * 15;
            break;
        case StyleUndefined:
            data->numChns = 0;
            break;
    }
    data->allocateBuffer();

    // SET THE PHASE CORRECTION TABLE TO THE DEFAULT LINEAR MAPPING
    if (data->phaseCorrectionBuffer){
        for (int n = 0; n < LENGTHPHASECORRECTIONTABLE; n++){
            ((float*)(data->phaseCorrectionBuffer))[n] = (float)n / (float)(LENGTHPHASECORRECTIONTABLE-1);
        }
    }

    // THE DEFAULT LOOK UP TABLE IS A FOURTH ORDER POLYNOMIAL
    data->style = stl;

    data->zMin = -qMax(qAbs(zMax), qAbs(zMin));
    data->zMax = -qMin(qAbs(zMax), qAbs(zMin));
    data->pMin = pMin;
    data->pMax = pMax;

    data->xMin = -qAbs(data->zMin) * tan(hFov / 2.0);
    data->xMax =  qAbs(data->zMin) * tan(hFov / 2.0);
    data->yMin = -qAbs(data->zMin) * tan(vFov / 2.0);
    data->yMax =  qAbs(data->zMin) * tan(vFov / 2.0);

    data->verticalFieldOfView = vFov;
    data->horizontalFieldOfView = hFov;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable::LAULookUpTable(QString filename, int directory)
{
    // CREATE OUR DATA OBJECT TO HOLD THE SCAN
    data = new LAULookUpTableData();

#ifndef HEADLESS
    // GET A FILE TO OPEN FROM THE USER IF NOT ALREADY PROVIDED ONE
    if (filename.isNull()) {
        QSettings settings;
        QString directoryString = settings.value(QString("LAULookUpTable::LastUsedDirectory"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directoryString) == false) {
            directoryString = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }

        if (directory == -1){
            filename = QFileDialog::getOpenFileName(nullptr, QString("Load look-up table from disk (*.lut,*.lutx)"), directoryString, QString("*.lut;*.lutx"));
        } else {
            filename = QFileDialog::getOpenFileName(nullptr, QString("Load look-up table from disk (*.lut)"), directoryString, QString("*.lut"));
        }

        if (!filename.isNull()) {
            settings.setValue(QString("LAULookUpTable::LastUsedDirectory"), QFileInfo(filename).absolutePath());
        } else {
            return;
        }

        if (filename.toLower().endsWith(".lutx")){
            int numLuts = LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(filename);
            bool ok = false;
            directory = QInputDialog::getInt(nullptr, QString("Look Up Table X Loader"), QString("Which Look Up Table do you want to load?"), 1, 1, numLuts, 1, &ok);
            if (ok){
                directory = directory - 1;
            } else {
                return;
            }
        }
    }
#else
    if (filename.isNull()) {
        return;
    }
#endif
    // IF WE HAVE A VALID TIFF FILE, LOAD FROM DISK
    // OTHERWISE TRY TO CONNECT TO SCANNER
    if (QFile::exists(filename)) {
        // SET THE FILENAME
        setFilename(filename);

        // OPEN INPUT TIFF FILE FROM DISK
        TIFF *inTiff = TIFFOpen(filename.toLocal8Bit(), "r");
        if (inTiff) {
            if (directory > -1){
                TIFFSetDirectory(inTiff, directory);
            }
            load(inTiff, directory);
            TIFFClose(inTiff);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<LAULookUpTable> LAULookUpTable::LAULookUpTableX(QString filename)
{
    QList<LAULookUpTable> tables;

#ifndef HEADLESS
    // GET A FILE TO OPEN FROM THE USER IF NOT ALREADY PROVIDED ONE
    if (filename.isNull()) {
        QSettings settings;
        QString directory = settings.value(QString("LAULookUpTable::LastUsedDirectory"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        filename = QFileDialog::getOpenFileName(nullptr, QString("Load look-up tables from disk (*.lutx)"), directory, QString("*.lutx"));
        if (!filename.isNull()) {
            settings.setValue(QString("LAULookUpTable::LastUsedDirectory"), QFileInfo(filename).absolutePath());
        } else {
            return(tables);
        }
    }
#else
    if (filename.isNull()) {
        return(tables);
    }
#endif

    int numTables = LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(filename);
    for (int n = 0; n < numTables; n++){
        tables << LAULookUpTable(filename, n);
    }
    return(tables);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable::LAULookUpTable(TIFF *currentTiffDirectory)
{
    data = new LAULookUpTableData();
    load(currentTiffDirectory);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable::~LAULookUpTable()
{
    qDebug() << "LAULookUpTable::~LAULookUpTable()";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAULookUpTable::save(QString filename) const
{
#ifndef HEADLESS
    if (filename.isNull()) {
        QSettings settings;
        QString directory = settings.value(QString("LAULookUpTable::LastUsedDirectory"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        filename = QFileDialog::getSaveFileName(nullptr, QString("Save look-up table to disk (*.lut)"), directory, QString("*.lut"));
        if (!filename.isNull()) {
            settings.setValue(QString("LAULookUpTable::LastUsedDirectory"), QFileInfo(filename).absolutePath());
            if (!filename.toLower().endsWith(QString(".lut"))) {
                filename = QString("%1.lut").arg(filename);
            }
        } else {
            return (false);
        }
    }
#else
    if (filename.isNull()) {
        return (false);
    }
#endif

    // OPEN TIFF FILE FOR SAVING THE IMAGE
    TIFF *outputTiff = TIFFOpen(filename.toLocal8Bit(), "w8");
    if (!outputTiff) {
        return (false);
    }

    // WRITE IMAGE TO CURRENT DIRECTORY
    save(outputTiff);

    // SEE IF THERE IS A VALID PHASE CORRECTION TABLE THAT WE NEED TO WRITE TO DISK
    if (data->phaseCorrectionBuffer){
        for (int n = 0; n < LENGTHPHASECORRECTIONTABLE; n++){
            float lambda = (float)n / (float)(LENGTHPHASECORRECTIONTABLE - 1);
            if (((float*)(data->phaseCorrectionBuffer))[n] != lambda){
                // CREATE A NEW TIFF DIRECTORY TO HOLD THE PHASE CORRECTION TABLE
                TIFFCreateDirectory(outputTiff);

                // CREATE A NEW DIRECTORY AND SAVE THE PHASE CORRECTION TABLE TO DISK
                savePhaseCorrectionTable(outputTiff);

                // BREAK FROM THIS LOOK SINCE THE PHASE CORRECTION TABLE HAS BEEN SAVED TO DISK
                break;
            }
        }
    }

    // CLOSE TIFF FILE
    TIFFClose(outputTiff);

    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAULookUpTable::save(TIFF *currentTiffDirectory) const
{
    // CREATE THE XML DATA PACKET USING QT'S XML STREAM OBJECTS
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);

    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("lookUpTable");

    // WRITE THE COLOR SPACE AND RANGE LIMITS FOR EACH DIMENSION
    writer.writeTextElement("minimumvalues", QString("%1,%2,%3,%4").arg(data->xMin).arg(data->yMin).arg(data->zMin).arg(0.0f));
    writer.writeTextElement("maximumvalues", QString("%1,%2,%3,%4").arg(data->xMax).arg(data->yMax).arg(data->zMax).arg(1.0f));

    // WRITE THE CAMERA FIELD OF VIEW AND CENTER OF MASS
    writer.writeTextElement("fieldofview", QString("%1,%2").arg(fov().x()).arg(fov().y()));

    // WRITE THE TRANSFORM MATRIX OUT TO THE XML FIELD
    QMatrix4x4 mat = transform();
    if (mat.isIdentity() == false){
        writer.writeTextElement("transform", QString("A = [ %1, %2, %3, %4; %5, %6, %7, %8; %9, %10, %11, %12; %13, %14, %15, %16 ];").arg(mat(0, 0)).arg(mat(0, 1)).arg(mat(0, 2)).arg(mat(0, 3)).arg(mat(1, 0)).arg(mat(1, 1)).arg(mat(1, 2)).arg(mat(1, 3)).arg(mat(2, 0)).arg(mat(2, 1)).arg(mat(2, 2)).arg(mat(2, 3)).arg(mat(3, 0)).arg(mat(3, 1)).arg(mat(3, 2)).arg(mat(3, 3)));
    }

    // WRITE THE PROJECTION MATRIX OUT TO THE XML FIELD
    mat = projection();
    if (mat.isIdentity() == false){
        writer.writeTextElement("projection", QString("B = [ %1, %2, %3, %4; %5, %6, %7, %8; %9, %10, %11, %12; %13, %14, %15, %16 ];").arg(mat(0, 0)).arg(mat(0, 1)).arg(mat(0, 2)).arg(mat(0, 3)).arg(mat(1, 0)).arg(mat(1, 1)).arg(mat(1, 2)).arg(mat(1, 3)).arg(mat(2, 0)).arg(mat(2, 1)).arg(mat(2, 2)).arg(mat(2, 3)).arg(mat(3, 0)).arg(mat(3, 1)).arg(mat(3, 2)).arg(mat(3, 3)));
    }

    // WRITE THE CAMERA SCALE FACTOR
    writer.writeTextElement("scaleFactor", QString("%1").arg(scaleFactor(), 0, 'f', 5));

    // WRITE THE CAMERA INTRINSICS
    writer.writeTextElement("intrinsics", QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12").arg(data->intrinsics.fx, 0, 'f', 5).arg(data->intrinsics.cx, 0, 'f', 5).arg(data->intrinsics.fy, 0, 'f', 5).arg(data->intrinsics.cy, 0, 'f', 5).arg(data->intrinsics.k1, 0, 'f', 5).arg(data->intrinsics.k2, 0, 'f', 5).arg(data->intrinsics.k3, 0, 'f', 5).arg(data->intrinsics.k4, 0, 'f', 5).arg(data->intrinsics.k5, 0, 'f', 5).arg(data->intrinsics.k6, 0, 'f', 5).arg(data->intrinsics.p1, 0, 'f', 5).arg(data->intrinsics.p2, 0, 'f', 5));
    writer.writeTextElement("boundingBox", QString("%1,%2,%3,%4,%5,%6").arg(data->boundingBox.xMin, 0, 'f', 5).arg(data->boundingBox.xMax, 0, 'f', 5).arg(data->boundingBox.yMin, 0, 'f', 5).arg(data->boundingBox.yMax, 0, 'f', 5).arg(data->boundingBox.zMin, 0, 'f', 5).arg(data->boundingBox.zMax, 0, 'f', 5));

    // CLOSE OUT THE XML BUFFER
    writer.writeEndElement();
    writer.writeEndDocument();
    buffer.close();

    // EXPORT THE XML BUFFER TO THE XMLPACKET FIELD OF THE TIFF IMAGE
    QByteArray xmlByteArray = buffer.buffer();
    TIFFSetField(currentTiffDirectory, TIFFTAG_XMLPACKET, xmlByteArray.length(), xmlByteArray.data());

    // SAVE CUSTOM FILESTRINGS FROM THE DOCUMENT NAME TIFFTAG
    TIFFSetField(currentTiffDirectory, TIFFTAG_DOCUMENTNAME, filename().toLocal8Bit().data());
    TIFFSetField(currentTiffDirectory, TIFFTAG_SOFTWARE, softwareString().toLocal8Bit().data());
    TIFFSetField(currentTiffDirectory, TIFFTAG_MODEL, modelString().toLocal8Bit().data());
    TIFFSetField(currentTiffDirectory, TIFFTAG_MAKE, makeString().toLocal8Bit().data());

    if (style() != StyleXYZWRCPQLookUpTable) {
        // WRITE FORMAT PARAMETERS TO CURRENT TIFF DIRECTORY
        TIFFSetField(currentTiffDirectory, TIFFTAG_IMAGEWIDTH, (unsigned long)width());
        TIFFSetField(currentTiffDirectory, TIFFTAG_IMAGELENGTH, (unsigned long)height());
        TIFFSetField(currentTiffDirectory, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
        TIFFSetField(currentTiffDirectory, TIFFTAG_XRESOLUTION, 72.0);
        TIFFSetField(currentTiffDirectory, TIFFTAG_YRESOLUTION, 72.0);
        TIFFSetField(currentTiffDirectory, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(currentTiffDirectory, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(currentTiffDirectory, TIFFTAG_SAMPLESPERPIXEL, (unsigned short)colors());
        TIFFSetField(currentTiffDirectory, TIFFTAG_BITSPERSAMPLE, (unsigned short)(8 * sizeof(float)));
        TIFFSetField(currentTiffDirectory, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
#ifndef _TTY_WIN_
        TIFFSetField(currentTiffDirectory, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
        TIFFSetField(currentTiffDirectory, TIFFTAG_PREDICTOR, 2);
        TIFFSetField(currentTiffDirectory, TIFFTAG_ROWSPERSTRIP, 1);
#endif
        // SET THE EXTRA SAMPLES TAG IF THERE ARE MORE THAN 1 COLOR IN THE TABLE
        if (colors() != 1){
            unsigned short *smples = new unsigned short [colors()];
            for (unsigned int n = 0; n < colors(); n++) {
                smples[n] = EXTRASAMPLE_UNSPECIFIED;
            }
            TIFFSetField(currentTiffDirectory, TIFFTAG_EXTRASAMPLES, colors() - 1, smples);
            delete [] smples;
        }

        // SEE IF WE HAVE TO TELL THE TIFF READER THAT WE ARE STORING
        // PIXELS IN 32-BIT FLOATING POINT FORMAT
        TIFFSetField(currentTiffDirectory, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);

        // MAKE TEMPORARY BUFFER TO HOLD CURRENT ROW BECAUSE COMPRESSION DESTROYS
        // WHATS EVER INSIDE THE BUFFER
        unsigned char *tempBuffer = (unsigned char *)malloc(step());
        for (unsigned int row = 0; row < height(); row++) {
            memcpy(tempBuffer, constScanLine(row), step());
            TIFFWriteScanline(currentTiffDirectory, tempBuffer, row, 0);
        }
        free(tempBuffer);
        return (true);
    } else {
        unsigned int bytesPerRow = width() * sizeof(float) * 8;
        unsigned int bytesPerFrm = height() * bytesPerRow;
        for (unsigned int dir = 0; dir < colors() / 8; dir++) {
            // WRITE FORMAT PARAMETERS TO CURRENT TIFF DIRECTORY
            TIFFSetField(currentTiffDirectory, TIFFTAG_IMAGEWIDTH, (unsigned long)width());
            TIFFSetField(currentTiffDirectory, TIFFTAG_IMAGELENGTH, (unsigned long)height());
            TIFFSetField(currentTiffDirectory, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
            TIFFSetField(currentTiffDirectory, TIFFTAG_XRESOLUTION, 72.0);
            TIFFSetField(currentTiffDirectory, TIFFTAG_YRESOLUTION, 72.0);
            TIFFSetField(currentTiffDirectory, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
            TIFFSetField(currentTiffDirectory, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            TIFFSetField(currentTiffDirectory, TIFFTAG_SAMPLESPERPIXEL, (unsigned short)8);
            TIFFSetField(currentTiffDirectory, TIFFTAG_BITSPERSAMPLE, (unsigned short)(8 * sizeof(float)));
            TIFFSetField(currentTiffDirectory, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
#ifndef _TTY_WIN_
            TIFFSetField(currentTiffDirectory, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
            TIFFSetField(currentTiffDirectory, TIFFTAG_PREDICTOR, 2);
            TIFFSetField(currentTiffDirectory, TIFFTAG_ROWSPERSTRIP, 1);
#endif
            // SET THE EXTRA SAMPLES TAG IF THERE ARE MORE THAN 1 COLOR IN THE TABLE
            if (colors() != 1){
                unsigned short *smples = new unsigned short [colors()];
                for (unsigned int n = 0; n < colors(); n++) {
                    smples[n] = EXTRASAMPLE_UNSPECIFIED;
                }
                TIFFSetField(currentTiffDirectory, TIFFTAG_EXTRASAMPLES, colors() - 1, smples);
                delete [] smples;
            }

            // SEE IF WE HAVE TO TELL THE TIFF READER THAT WE ARE STORING
            // PIXELS IN 32-BIT FLOATING POINT FORMAT
            TIFFSetField(currentTiffDirectory, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);

            // SET THE Z LIMITS AND FLIP THEIR SIGNS IF NECESSARY
            float rMin = qMin(qAbs(data->zMin), qAbs(data->zMax));
            float rMax = qMax(qAbs(data->zMin), qAbs(data->zMax));

            // SAVE THE XYZ DEPTH RANGE
            TIFFSetField(currentTiffDirectory, TIFFTAG_MINSAMPLEVALUE, static_cast<unsigned short>(rMin));
            TIFFSetField(currentTiffDirectory, TIFFTAG_MAXSAMPLEVALUE, static_cast<unsigned short>(rMax));

            // SAVE THE RAW DEPTH VIDEO RANGE LIMITS
            TIFFSetField(currentTiffDirectory, TIFFTAG_SMINSAMPLEVALUE, static_cast<double>(data->pMin));
            TIFFSetField(currentTiffDirectory, TIFFTAG_SMAXSAMPLEVALUE, static_cast<double>(data->pMax));

            // MAKE TEMPORARY BUFFER TO HOLD CURRENT ROW BECAUSE COMPRESSION DESTROYS
            // WHATS EVER INSIDE THE BUFFER
            unsigned char *fmBuffer = constScanLine(0) + bytesPerFrm * dir;
            unsigned char *toBuffer = (unsigned char *)malloc(step());
            for (unsigned int row = 0; row < height(); row++) {
                memcpy(toBuffer, fmBuffer + bytesPerRow * row, bytesPerRow);
                TIFFWriteScanline(currentTiffDirectory, toBuffer, row, 0);
            }
            free(toBuffer);

            // CREATE A NEW DIRECTORY IF WE NEED IT
            TIFFSetDirectory(currentTiffDirectory, dir);
            TIFFRewriteDirectory(currentTiffDirectory);
        }
        return (true);
    }
    return (false);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAULookUpTable::savePhaseCorrectionTable(TIFF *currentTiffDirectory) const
{
    // SAVE CUSTOM FILESTRINGS FROM THE DOCUMENT NAME TIFFTAG
    TIFFSetField(currentTiffDirectory, TIFFTAG_DOCUMENTNAME, filename().toLocal8Bit().data());
    TIFFSetField(currentTiffDirectory, TIFFTAG_SOFTWARE, softwareString().toLocal8Bit().data());
    TIFFSetField(currentTiffDirectory, TIFFTAG_MODEL, modelString().toLocal8Bit().data());
    TIFFSetField(currentTiffDirectory, TIFFTAG_MAKE, makeString().toLocal8Bit().data());

    TIFFSetField(currentTiffDirectory, TIFFTAG_IMAGEWIDTH, (unsigned long)LENGTHPHASECORRECTIONTABLE);
    TIFFSetField(currentTiffDirectory, TIFFTAG_IMAGELENGTH, (unsigned long)1);
    TIFFSetField(currentTiffDirectory, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    TIFFSetField(currentTiffDirectory, TIFFTAG_XRESOLUTION, 72.0);
    TIFFSetField(currentTiffDirectory, TIFFTAG_YRESOLUTION, 72.0);
    TIFFSetField(currentTiffDirectory, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(currentTiffDirectory, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(currentTiffDirectory, TIFFTAG_SAMPLESPERPIXEL, (unsigned short)1);
    TIFFSetField(currentTiffDirectory, TIFFTAG_BITSPERSAMPLE, (unsigned short)(8 * sizeof(float)));
    TIFFSetField(currentTiffDirectory, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
#ifndef _TTY_WIN_
    TIFFSetField(currentTiffDirectory, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    TIFFSetField(currentTiffDirectory, TIFFTAG_PREDICTOR, 2);
    TIFFSetField(currentTiffDirectory, TIFFTAG_ROWSPERSTRIP, 1);
#endif
    // SEE IF WE HAVE TO TELL THE TIFF READER THAT WE ARE STORING
    // PIXELS IN 32-BIT FLOATING POINT FORMAT
    TIFFSetField(currentTiffDirectory, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);

    // MAKE TEMPORARY BUFFER TO HOLD CURRENT ROW BECAUSE COMPRESSION DESTROYS
    // WHATS EVER INSIDE THE BUFFER
    unsigned char *tempBuffer = (unsigned char *)malloc(LENGTHPHASECORRECTIONTABLE * sizeof(float));
    memcpy(tempBuffer, constPhaseCorrectionTable(), LENGTHPHASECORRECTIONTABLE * sizeof(float));
    TIFFWriteScanline(currentTiffDirectory, tempBuffer, 0, 0);
    free(tempBuffer);

    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAULookUpTable::load(TIFF *inTiff, int directory)
{
    // LOAD INPUT TIFF FILE PARAMETERS IMPORTANT TO RESAMPLING THE IMAGE
    unsigned long uLongVariable;
    unsigned short uShortVariable;

    // SEE HOW MANY DIRECTORIES ARE IN THE TIFF FILE
    unsigned short directories = (directory == -1) ? TIFFNumberOfDirectories(inTiff) : 1;

    // CREATE A FLAG TO STORE IF THIS TIFF FILE HAS A PHASE CORRECTION TABLE
    bool foundPhaseCorrectionTable = false;

    // SEARCH DIRECTORIES TO SEE IF THERE IS A PHASE CORRECTION TABLE AT THE END
    unsigned int currentDirectory = TIFFCurrentDirectory(inTiff);
    if (directories > 1){
        unsigned long rowCount = 0, colCount = 0;
        TIFFSetDirectory(inTiff, directories-1);
        TIFFGetField(inTiff, TIFFTAG_IMAGEWIDTH, &colCount);
        TIFFGetField(inTiff, TIFFTAG_IMAGELENGTH, &rowCount);

        // SEE IF THE DIMENSIONS ARE CONSISTENT WITH A PHASE CORRECITON TABLE
        if ((rowCount == 1) && (colCount == LENGTHPHASECORRECTIONTABLE)){
            unsigned short bitCount = 0, smpCount = 0;
            TIFFGetField(inTiff, TIFFTAG_SAMPLESPERPIXEL, &smpCount);
            TIFFGetField(inTiff, TIFFTAG_BITSPERSAMPLE, &bitCount);

            // SEE IF THE SAMPLES PER PIXEL AND BITCOUNT ARE CONSISTENT WITH A PHASE CORRECITON TABLE
            if ((smpCount == 1) && (bitCount == 32)){
                // SET THE FOUND PHASE CORRECTION FLAG TO TRUE
                foundPhaseCorrectionTable = true;

                // ADJUST THE NUMBER OF DIRECTORIES
                directories = directories - 1;
            }
        }
    }
    TIFFSetDirectory(inTiff, currentDirectory);

    // GET THE HEIGHT AND WIDTH OF INPUT IMAGE IN PIXELS
    TIFFGetField(inTiff, TIFFTAG_IMAGEWIDTH, &uLongVariable);
    data->numCols = static_cast<unsigned int>(uLongVariable);
    TIFFGetField(inTiff, TIFFTAG_IMAGELENGTH, &uLongVariable);
    data->numRows = static_cast<unsigned int>(uLongVariable);
    TIFFGetField(inTiff, TIFFTAG_SAMPLESPERPIXEL, &uShortVariable);
    if (directories == 1) {
        if (uShortVariable == 8) {
            data->style = StyleLinear;
            data->numChns = uShortVariable;
        } else if (uShortVariable == 12) {
            data->style = StyleFourthOrderPoly;
            data->numChns = uShortVariable;
        } else if (uShortVariable == 16) {
            data->style = StyleFourthOrderPolyAugmentedReality;
            data->numChns = uShortVariable;
        } else {
            return (false);
        }
    } else if (directories == 2) {
        if (uShortVariable == 12) {
            data->style = StyleFourthOrderPolyWithPhaseUnwrap;
            data->numChns = uShortVariable + 1;
        } else {
            return (false);
        }
    } else if (directories == 3) {
        if (uShortVariable == 4) {
            data->style = StyleActiveStereoVisionPoly;
            data->numChns = 20;
        } else {
            return (false);
        }
    } else {
        if (uShortVariable == 4) {
            data->style = StyleXYZPLookUpTable;
            data->numChns = uShortVariable * directories;
        } else if (uShortVariable == 8) {
            data->style = StyleXYZWRCPQLookUpTable;
            data->numChns = uShortVariable * directories;
        } else {
            return (false);
        }
    }

    TIFFGetField(inTiff, TIFFTAG_BITSPERSAMPLE, &uShortVariable);
    if (uShortVariable != 32) {
        return (false);
    }

    TIFFGetField(inTiff, TIFFTAG_PHOTOMETRIC, &uShortVariable);
    if (uShortVariable != PHOTOMETRIC_MINISBLACK && uShortVariable != PHOTOMETRIC_SEPARATED) {
        return (false);
    }

    TIFFGetField(inTiff, TIFFTAG_SAMPLEFORMAT, &uShortVariable);
    if (uShortVariable != SAMPLEFORMAT_IEEEFP) {
        return (false);
    }

    TIFFGetField(inTiff, TIFFTAG_PLANARCONFIG, &uShortVariable);
    if (uShortVariable != PLANARCONFIG_CONTIG) {
        return (false);
    }

    // GET THE Z LIMITS AND FLIP THEIR SIGNS IF NECESSARY
    TIFFGetField(inTiff, TIFFTAG_MINSAMPLEVALUE, &uShortVariable);
    data->zMin = (float)uShortVariable;

    TIFFGetField(inTiff, TIFFTAG_MAXSAMPLEVALUE, &uShortVariable);
    data->zMax = (float)uShortVariable;

    double doubleVariable = 0.0;
    TIFFGetField(inTiff, TIFFTAG_SMINSAMPLEVALUE, &doubleVariable);
    if (doubleVariable != 0.0) {
        data->zMin = (float)doubleVariable;
        data->pMin = (float)doubleVariable;
    }

    TIFFGetField(inTiff, TIFFTAG_SMAXSAMPLEVALUE, &doubleVariable);
    if (doubleVariable != 0.0) {
        data->zMax = (float)doubleVariable;
        data->pMax = (float)doubleVariable;
    }

    if (data->zMin > data->zMax) {
        data->zMin *= -1.0f;
        data->zMax *= -1.0f;
    }

    int dataLength;
    char *dataString;
    bool dataPresent = TIFFGetField(inTiff, TIFFTAG_XMLPACKET, &dataLength, &dataString);
    if (dataPresent) {
        data->xmlString = QString(QByteArray(dataString));
    }

    dataPresent = TIFFGetField(inTiff, TIFFTAG_MODEL, &dataString);
    if (dataPresent) {
        data->modelString = QString(QByteArray(dataString));
    }

    dataPresent = TIFFGetField(inTiff, TIFFTAG_SOFTWARE, &dataString);
    if (dataPresent) {
        data->softwareString = QString(QByteArray(dataString));
    }

    dataPresent = TIFFGetField(inTiff, TIFFTAG_MAKE, &dataString);
    if (dataPresent) {
        data->makeString = QString(QByteArray(dataString));
    }

    // ALLOCATE SPACE TO HOLD IMAGE DATA
    data->allocateBuffer();

    if (directories == 1) {
        // READ DATA AS CHUNKY FORMAT
        for (unsigned int row = 0; row < height(); row++) {
            unsigned char *buffer = scanLine(row);
            TIFFReadScanline(inTiff, buffer, static_cast<uint32_t>(row));
        }
    } else if (directories == 2) {
        // CALCULATE MEMORY STEP SIZES ASSUMING 12 FLOATS PER PIXEL
        unsigned int bytesPerRow = (data->numCols * 12 * sizeof(float));
        unsigned int bytesPerFrame = (data->numRows * bytesPerRow);

        unsigned char *buffer = (unsigned char *)scanLine(0);
        for (unsigned int row = 0; row < height(); row++) {
            TIFFReadScanline(inTiff, (buffer + row * bytesPerRow), static_cast<uint32_t>(row));
        }

        // MOVE THE POINTER TO THE NEXT AVAILABLE ADDRESS
        buffer += bytesPerFrame;

        // AT THIS POINT, WE HAVE JUST ENOUGH ROOM FOR 1 FLOAT PER PIXEL
        // SO LET'S GO TO THE NEXT DIRECTORY TO LOAD THIS UNWRAPPING MASK
        LAUMemoryObject unwrapMaskObject(inTiff, 1);
        if (unwrapMaskObject.isValid()) {
            memcpy(buffer, unwrapMaskObject.constPointer(), unwrapMaskObject.length());
        }
    } else if (directories == 3) {
        // LOAD THE THREE DIRECTORIES FROM THE TIFF FILE INTO THEIR OWN OBJECTS
        LAUMemoryObject rsmObject(inTiff, 0);
        LAUMemoryObject crmObject(inTiff, 1);
        LAUMemoryObject lutObject(inTiff, 2);

        // MERGE THE MEMORY OBJECTS INTO A SINGLE LOOK UP TABLE
        memcpy(constScanLine(0), rsmObject.constPointer(), rsmObject.length());
        memcpy(constScanLine(0) + rsmObject.length(), crmObject.constPointer(), crmObject.length());
        memcpy(constScanLine(0) + rsmObject.length() + crmObject.length(), lutObject.constPointer(), lutObject.length());

        // RESET THE DIRECTORY TO THE FIRST DIRECTORY IN THIS LOOK UP TABLE FILE
        TIFFSetDirectory(inTiff, 0);
    } else {
        // CALCULATE MEMORY STEP SIZES
        unsigned int bytesPerRow = (data->numCols * sizeof(float));
        if (data->style == StyleXYZPLookUpTable) {
            bytesPerRow *= 4;
        } else if (data->style == StyleXYZWRCPQLookUpTable) {
            bytesPerRow *= 8;
        }
        unsigned int bytesPerFrame = (data->numRows * bytesPerRow);

#define USEMULTITHREADLOOKUPTABLELOADING
#ifdef USEMULTITHREADLOOKUPTABLELOADING
        // READ EACH DIRECTORY TO FORM A PLANAR IMAGE OF VEC4S
        QList<QThread *> loaders;
        for (unsigned short dir = 0; dir < directories; dir++) {
            // CREATE A LOADER OBJECT FOR THE CURRENT DIRECTORY
            QThread *thread = new LAULookUpTableLoader(filename(), dir, (unsigned char *)constScanLine(0) + bytesPerFrame * dir);
            thread->start();
            loaders << thread;
        }

        // NOW WAIT FOR ALL THREADS TO COMPLETE AND THEN DELETE THEM ONE BY ONE
        while (loaders.count() > 0) {
            QThread *thread = loaders.takeFirst();
            while (thread->isRunning()) {
                qApp->processEvents();
            }
            delete thread;
        }
#else
        // READ EACH DIRECTORY TO FORM A PLANAR IMAGE OF VEC4S
        unsigned char *buffer = (unsigned char *)scanLine(0);
        for (unsigned short dir = 0; dir < directories; dir++) {
            TIFFSetDirectory(inTiff, dir);
            for (unsigned int row = 0; row < height(); row++) {
                TIFFReadScanline(inTiff, (buffer + row * bytesPerRow), (int)row);
            }
            // MOVE THE POINTER TO THE NEXT AVAILABLE ADDRESS
            buffer += bytesPerFrame;
        }
#endif
    }

    // CREATE VARIABLES TO READ THE XML HEADER
    QMatrix4x4 transform;
    QMatrix4x4 projection;

    // LOAD THE XML FIELD OF THE TIFF FILE, IF PROVIDED
    if (data->xmlString.isEmpty() == false) {
        QXmlStreamReader reader(data->xmlString);
        while (!reader.atEnd()) {
            if (reader.readNext()) {
                QString name = reader.name().toString();
                if (name == "minimumvalues") {
                    QString rangeString = reader.readElementText();
                    QStringList floatList = rangeString.split(",");
                    if (floatList.count() >= 3) {
                        data->xMin = floatList.at(0).toFloat();
                        data->yMin = floatList.at(1).toFloat();
                        data->zMin = floatList.at(2).toFloat();
                    }
                } else if (name == "maximumvalues") {
                    QString rangeString = reader.readElementText();
                    QStringList floatList = rangeString.split(",");
                    if (floatList.count() >= 3) {
                        data->xMax = floatList.at(0).toFloat();
                        data->yMax = floatList.at(1).toFloat();
                        data->zMax = floatList.at(2).toFloat();
                    }
                } else if (name == "transform") {
                    QString matrixString = reader.readElementText();
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

                    setConstTransform(transform);
                } else if (name == "projection") {
                    QString matrixString = reader.readElementText();
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

                    setConstProjection(projection);
                } else if (name == "scaleFactor") {
                    QString rangeString = reader.readElementText();
                    QStringList floatList = rangeString.split(",");
                    if (floatList.count() >= 1) {
                        data->scaleFactor = floatList.at(0).toDouble();
                    }
                } else if (name == "intrinsics") {
                    QString rangeString = reader.readElementText();
                    QStringList floatList = rangeString.split(",");
                    if (floatList.count() >= 9) {
                        data->intrinsics.fx = floatList.at(0).toDouble();
                        data->intrinsics.cx = floatList.at(1).toDouble();
                        data->intrinsics.fy = floatList.at(2).toDouble();
                        data->intrinsics.cy = floatList.at(3).toDouble();
                        data->intrinsics.k1 = floatList.at(4).toDouble();
                        data->intrinsics.k2 = floatList.at(5).toDouble();
                        data->intrinsics.k3 = floatList.at(6).toDouble();
                        data->intrinsics.k4 = floatList.at(7).toDouble();
                        data->intrinsics.k5 = floatList.at(8).toDouble();
                    }
                    if (floatList.count() >= 10){
                        data->intrinsics.k6 = floatList.at(9).toDouble();
                    }
                    if (floatList.count() >= 11){
                        data->intrinsics.p1 = floatList.at(10).toDouble();
                    }
                    if (floatList.count() >= 12){
                        data->intrinsics.p2 = floatList.at(11).toDouble();
                    }
                } else if (name == "boundingBox"){
                    QString rangeString = reader.readElementText();
                    QStringList floatList = rangeString.split(",");
                    if (floatList.count() >= 6) {
                        data->boundingBox.xMin = floatList.at(0).toDouble();
                        data->boundingBox.xMax = floatList.at(1).toDouble();
                        data->boundingBox.yMin = floatList.at(2).toDouble();
                        data->boundingBox.yMax = floatList.at(3).toDouble();
                        data->boundingBox.zMin = floatList.at(4).toDouble();
                        data->boundingBox.zMax = floatList.at(5).toDouble();
                    }
                }
            }
        }
        reader.clear();
    }

    // SEE IF THIS TIFF FILE HAS A PHASE CORRECTION TABLE AND IF SO, THEN
    // READ IT IN OTHERWISE USE THE DEFAULT LINEAR CORRECTION TABLE
    if (foundPhaseCorrectionTable){
        // MOVE TO THE DIRECTORY THAT STORES THE PHASE CORRECTION TABLE
        TIFFSetDirectory(inTiff, directories);

        // READ IN THE ROW OF FLOATING POINT VALUES INTO OUR PHASE CORRECTION TABLE
        TIFFReadScanline(inTiff, data->phaseCorrectionBuffer, static_cast<uint32_t>(0));
    } else {
        // SET THE PHASE CORRECTION TABLE TO THE DEFAULT LINEAR MAPPING
        for (int n = 0; n < LENGTHPHASECORRECTIONTABLE; n++){
            ((float*)(data->phaseCorrectionBuffer))[n] = (float)n / (float)(LENGTHPHASECORRECTIONTABLE-1);
        }
    }

    // SET THE LIMITS OF THE LUT
    updateLimits();

    // IF WE MADE IT THIS FAR, EVERYTHING IS GOOD AND RETURN TRUE
    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector<double> LAULookUpTable::jetr() const
{
    QVector<double> vector(37, NAN);

    // COPY OVER THE INTRINSICS
    vector[ 0] = data->intrinsics.fx;
    vector[ 1] = data->intrinsics.cx;
    vector[ 2] = data->intrinsics.fy;
    vector[ 3] = data->intrinsics.cy;
    vector[ 4] = data->intrinsics.k1;
    vector[ 5] = data->intrinsics.k2;
    vector[ 6] = data->intrinsics.k3;
    vector[ 7] = data->intrinsics.k4;
    vector[ 8] = data->intrinsics.k5;
    vector[ 9] = data->intrinsics.k6;
    vector[10] = data->intrinsics.p1;
    vector[11] = data->intrinsics.p2;

    // COPY OVER THE TRANSFORM MATRIX (user-defined alignment transforms)
    // NOTE: QMatrix4x4::data() returns column-major order, but JETR expects row-major
    const float* matrixData = data->transformMatrix->constData();
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int jetrIndex = 12 + (row * 4 + col);     // JETR uses row-major storage
            int matrixIndex = col * 4 + row;          // QMatrix4x4 uses column-major storage
            vector[jetrIndex] = matrixData[matrixIndex];
        }
    }

    // COPY OVER THE BOUNDING BOX
    vector[28] = data->boundingBox.xMin;
    vector[29] = data->boundingBox.xMax;
    vector[30] = data->boundingBox.yMin;
    vector[31] = data->boundingBox.yMax;
    vector[32] = data->boundingBox.zMin;
    vector[33] = data->boundingBox.zMax;

    // COPY OVER THE SCALE FACTOR AND THE RANGE LIMITS
    vector[34] = data->scaleFactor;
    vector[35] = data->zMin;
    vector[36] = data->zMax;

    // RETURN JUST ENOUGH INFORMATION TO RECONSTRUCT A POINT CLOUD FROM RAW DATA TO USER
    return(vector);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAULookUpTable::createRangeMasks(float xmn, float xmx, float ymn, float ymx, float zmn, float zmx) const
{
    // MAKE SURE WE CAN HANDLE THE CURRENT LOOK UP TABLE STYLE
    if (style() != StyleFourthOrderPoly){
        return(LAUMemoryObject());
    }

    // CREATE A MEMORY OBJECT TO HOLD OUR OUTPUT RANGE LIMITS AS MATRICES
    LAUMemoryObject rangeLimits(width(), height(), 1, sizeof(unsigned short), 2);

    // ITERATE THROUGH EVERY ROW AND COLUMN OF THE LOOK UP TABLE
    for (unsigned int row = 0; row < height(); row++){
        float *lut = (float*)constScanLine(row);
        for (unsigned int col = 0; col < width(); col++){
            // FIND THE ORIGIN POINT FOR THE CAMERA PROJECTION MATRIX
            QVector4D pa;
            pa.setW(1.0f);
            pa.setZ(0.0f * lut[7] + lut[8]);
            pa.setX(pa.z() * lut[0] + lut[1]);
            pa.setY(pa.z() * lut[2] + lut[3]);
            pa = transform() * pa;

            // FIND THE LINE OF SIGHT FOR THE CAMERA PROJECTION MATRIX
            QVector4D pb;
            pb.setW(1.0f);
            pb.setZ(1.0f * lut[7] + lut[8]);
            pb.setX(pb.z() * lut[0] + lut[1]);
            pb.setY(pb.z() * lut[2] + lut[3]);
            pb = (transform() * pb) - pa;

            // FIND THE LAMBDA COORDINATE THAT INTERSECTS THE PLANES FORMING THE BOUNDING BOX
            float xa = (xmn - pa.x())/pb.x();
            float xb = (xmx - pa.x())/pb.x();

            float ya = (ymn - pa.y())/pb.y();
            float yb = (ymx - pa.y())/pb.y();

            float za = (zmn - pa.z())/pb.z();
            float zb = (zmx - pa.z())/pb.z();

            float dmn = qMax(qMin(xa,xb), qMax(qMin(ya,yb), qMin(za,zb)));
            float dmx = qMin(qMax(xa,xb), qMin(qMax(ya,yb), qMax(za,zb)));

            // FIND THE RANGE LIMITS IN THE UNSIGNED SHORT COORDINATE FROM 0 TO 65535
            *((unsigned short*)rangeLimits.constPixel(col, row, 0)) = (unsigned short)qRound(qMax(0.0f, qMin(dmn, 1.0f)) * 65535.0f);
            *((unsigned short*)rangeLimits.constPixel(col, row, 1)) = (unsigned short)qRound(qMax(0.0f, qMin(dmx, 1.0f)) * 65535.0f);

            // MOVE THE LOOK UP TABLE POINT TO THE NEXT PIXEL
            lut = lut + colors();
        }
    }

    // RETURN OUR RANGE LIMIT MEMORY OBJECT TO THE USER
    return(rangeLimits);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector4D LAULookUpTable::focalPlaneArrayLimits() const
{
    // CREATE A VECTOR TO RETURN LIMITS
    QVector4D fpaLimits(1e6, -1e6, 1e6, -1e6);

    // GET THE LOCAL INTRINSICS
    LookUpTableIntrinsics intParameters = intrinsics();

    for (unsigned int row = 0; row < height(); row++) {
        double R = (double)row / (double)(height() - 1);
        double Rc = (R - (intParameters.cy / (double)(height() - 1)));
        for (unsigned int col = 0; col < width(); col++) {
            double C  = (double)col / (double)(width() - 1);
            double Cc = (C - (intParameters.cx / (double)(width() - 1)));

            double D = Rc * Rc + Cc * Cc;

            double Cu = (C - Cc * (intParameters.k1 * D + intParameters.k2 * D * D + intParameters.k3 * D * D * D) - (intParameters.p1 * (D + 2 * (Cc * Cc)) + intParameters.p2 * Cc * Rc)) * width();
            double Ru = (R - Rc * (intParameters.k1 * D + intParameters.k2 * D * D + intParameters.k3 * D * D * D) - (intParameters.p2 * (D + 2 * (Cc * Cc)) + intParameters.p1 * Cc * Rc)) * height();

            fpaLimits.setX((float)qMin((double)fpaLimits.x(), Cu));
            fpaLimits.setY((float)qMax((double)fpaLimits.y(), Cu));
            fpaLimits.setZ((float)qMin((double)fpaLimits.z(), Ru));
            fpaLimits.setW((float)qMax((double)fpaLimits.w(), Ru));
        }
    }
    return(fpaLimits);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QPointF LAULookUpTable::cameraCoordinate(QVector3D point) const
{
    QMatrix4x4 intParameters;
    intParameters(0, 0) = data->intrinsics.fx;
    intParameters(0, 1) = 0.0f;
    intParameters(0, 2) = data->intrinsics.cx;
    intParameters(0, 3) = 0.0f;

    intParameters(1, 0) = 0.0f;
    intParameters(1, 1) = data->intrinsics.fy;
    intParameters(1, 2) = data->intrinsics.cy;
    intParameters(1, 3) = 0.0f;

    intParameters(2, 0) = 0.0f;
    intParameters(2, 1) = 0.0f;
    intParameters(2, 2) = 1.0f;
    intParameters(2, 3) = 0.0f;

    intParameters(3, 0) = 0.0f;
    intParameters(3, 1) = 0.0f;
    intParameters(3, 2) = 0.0f;
    intParameters(3, 3) = 0.0f;

    // GET THE WORLD COORDINATE INSIDE THE CAMERA'S INTRINSIC SPACE
    QVector4D pointA = transform() * QVector4D(point.x(), point.y(), point.z(), 1.0);
    pointA /= pointA.w();

    // GET THE ROW AND COLUMN CAMERA COORDINATE NOT TAKING INTO ACCOUNT DISTORTION
    QVector4D pointB = intParameters * pointA;
    pointB /= pointB.z();
    pointB.setX(pointB.x() / (width() - 1));
    pointB.setY(pointB.y() / (height() - 1));

    // TWEAK THE CAMERA COORDINATES USING THE DISTORTION PARAMETERS
    double r = pointB.x() * pointB.x() + pointB.y() * pointB.y();
    double x = pointB.x() * (1.0 + data->intrinsics.k1 * r + data->intrinsics.k2 * r * r + data->intrinsics.k3 * r * r * r) + 2.0 * data->intrinsics.p1 * pointB.x() * pointB.y() + data->intrinsics.p2 * (r + 2 * pointB.x() * pointB.x());
    double y = pointB.y() * (1.0 + data->intrinsics.k1 * r + data->intrinsics.k2 * r * r + data->intrinsics.k3 * r * r * r) + data->intrinsics.p1 * (r + 2 * pointB.y() * pointB.y()) + 2.0 * data->intrinsics.p2 * pointB.x() * pointB.y();

    x = (x + 0.5) * width();
    y = (y + 0.5) * height();

    // RETURN THE COLUMN AND ROW COORDINATE INSIDE A 2D POINT OBJECT
    return (QPointF(x, y));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable LAULookUpTable::convertToStyle(LAULookUpTableStyle stl) const
{
    LAULookUpTable table(width(), height(), stl, fov().x(), fov().y(), zLimits().x(), zLimits().y(), pLimits().x(), pLimits().y());
    table.setProjection(projection());
    table.setTransform(transform());
    table.setFilename(filename());
    table.setXmlString(xmlString());
    table.setMakeString(makeString());
    table.setModelString(modelString());
    table.setSoftwareString(softwareString());
    table.setBoundingBox(boundingBox());
    table.setIntrinsics(intrinsics());

    if (style() == StyleFourthOrderPoly && stl == StyleLinear) {
        for (unsigned int row = 0; row < height(); row++) {
            float *toBuffer = (float *)table.constScanLine(row);
            float *fmBuffer = (float *)constScanLine(row);
            for (unsigned int col = 0; col < height(); col++) {
                toBuffer[8 * col + 0] = fmBuffer[12 * col + 0];
                toBuffer[8 * col + 1] = fmBuffer[12 * col + 1];
                toBuffer[8 * col + 2] = fmBuffer[12 * col + 2];
                toBuffer[8 * col + 3] = fmBuffer[12 * col + 3];
                toBuffer[8 * col + 4] = fmBuffer[12 * col + 7];
                toBuffer[8 * col + 5] = fmBuffer[12 * col + 8];
                toBuffer[8 * col + 6] = 1.0f;
                toBuffer[8 * col + 7] = 0.0f;
            }
        }
    }
    return (table);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAULookUpTable::combineLookUpTablesFromDisk(QStringList filenames)
{
#ifndef HEADLESS
    // GET FILES TO OPEN FROM THE USER IF NOT ALREADY PROVIDED
    if (filenames.isEmpty()) {
        QSettings settings;
        QString directory = settings.value(QString("LAULookUpTable::LastUsedDirectory"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        filenames = QFileDialog::getOpenFileNames(nullptr, QString("Load look-up table from disk (*.lut)"), directory, QString("*.lut"));
        if (!filenames.isEmpty()) {
            settings.setValue(QString("LAULookUpTable::LastUsedDirectory"), QFileInfo(filenames.constFirst()).absolutePath());
        } else {
            return;
        }
    }
#else
    if (filenames.isEmpty()) {
        return;
    }
#endif

    QList<LAULookUpTable> tables;
    for (int n = 0; n < filenames.count(); n++) {
        tables << LAULookUpTable(filenames.at(n));
    }

    //LAULookUpTable table = combineLookUpTables(tables);
    //table.save(QString());
    saveLookUpTables(tables);
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable LAULookUpTable::combineLookUpTables(QList<LAULookUpTable> tables)
{
    // MAKE SURE ALL TABLES ARE THE SAME SIZE AND FORMAT
    for (int n = 1; n < tables.count(); n++) {
        if (tables.constFirst().width() != tables.at(n).width()) {
            return (LAULookUpTable());
        }
        if (tables.constFirst().height() != tables.at(n).height()) {
            return (LAULookUpTable());
        }
        if (tables.constFirst().style() != tables.at(n).style()) {
            return (LAULookUpTable());
        }
    }

    // CREATE A TABLE TO HOLD THE MERGED TABLES
    LAULookUpTable table = LAULookUpTable(tables.constFirst().width(), tables.count() * tables.constFirst().height(), tables.constFirst().colors(), tables.constFirst().style());

    // ITERATE THROUGH EACH TABLE COPYING OVER INTO THE NEW TABLE
    unsigned char *toBuffer = table.constScanLine(0);
    for (int n = 0; n < tables.count(); n++) {
        unsigned char *fmBuffer = tables.at(n).constScanLine(0);
        memcpy(toBuffer, fmBuffer, tables.at(n).length());
        toBuffer += tables.at(n).length();
    }

    // RETURN THE MERGED TABLE TO THE USER
    return (table);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAULookUpTable::saveLookUpTables(QList<LAULookUpTable> tables, QString filename)
{
#ifndef HEADLESS
    if (filename.isNull()) {
        QSettings settings;
        QString directory = settings.value(QString("LAULookUpTable::LastUsedDirectory"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        filename = QFileDialog::getSaveFileName(nullptr, QString("Save compound look-up table to disk (*.lutx)"), directory, QString("*.lutx"));
        if (!filename.isNull()) {
            settings.setValue(QString("LAULookUpTable::LastUsedDirectory"), QFileInfo(filename).absolutePath());
            if (!filename.toLower().endsWith(QString(".lutx"))) {
                filename = QString("%1.lutx").arg(filename);
            }
        } else {
            return (false);
        }
    }
#else
    if (filename.isNull()) {
        return (false);
    }
#endif

    // OPEN TIFF FILE FOR SAVING THE IMAGE
    TIFF *outputTiff = TIFFOpen(filename.toLocal8Bit(), "w8");
    if (!outputTiff) {
        return (false);
    }

    // WRITE IMAGE TO CURRENT DIRECTORY
    for (int n = 0; n < tables.count(); n++) {
        TIFFSetDirectory(outputTiff, (unsigned short)n);
        tables.at(n).save(outputTiff);
        TIFFRewriteDirectory(outputTiff);
    }

    // CLOSE TIFF FILE
    TIFFClose(outputTiff);

    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAULookUpTable::replace(const LAULookUpTable &other)
{
    data = other.data;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAULookUpTable::rotate180InPlace()
{
    if (style() == StyleFourthOrderPoly){
        // FIRST WE ARE GOING TO SWAP THE TOP ROW WITH THE BOTTOM
        float *bufferT = (float*)malloc(step());
        for (unsigned int row = 0; row < height()/2; row++){
            // GRAB POINTERS TO THE FIRST ADDRESS IN THE ROWS ABOUT TO BE FLIPPED
            float *bufferA = (float*)this->constScanLine(row);
            float *bufferB = (float*)this->constScanLine(height() - 1 - row);

            // COPY THE TOP OF TWO ROWS INTO A TEMPORARY BUFFER
            memcpy(bufferT, bufferA, step());

            // COPY THE TOP HALF ROW INTO THE CURRENT ROW, FLIPPING IT LEFT TO RIGHT
            // AND THEN COPY THE TEMPORARY BUFFER INTO THE BOTTOM ROW, FLIPPING IT LEFT TO RIGHT
            for (unsigned int col = 0; col < width(); col++){
                for (unsigned int chn = 0; chn < colors(); chn++){
                    bufferA[colors() * (width() - 1 - col) + chn] = bufferB[colors() * col + chn];
                    bufferB[colors() * col + chn] = bufferT[colors() * (width() - 1 - col) + chn];
                }
            }
        }
        free(bufferT);
        return(true);
    } else {
        return(false);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable LAULookUpTable::crop(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    if ((y + h) > height()) {
        h = height() - y;
    }

    if ((x + w) > width()) {
        w = width() - x;
    }

    if (style() == StyleFourthOrderPoly){
        // CREATE A NEW TABLE TO COPY OUR PARAMETERS INTO
        LAULookUpTable cropTable = LAULookUpTable(w, h, StyleFourthOrderPoly, fov().x(), fov().y(), zLimits().x(), zLimits().y(), pLimits().x(), pLimits().y());

        cropTable.setFilename(filename());
        cropTable.setXmlString(xmlString());
        cropTable.setMakeString(makeString());
        cropTable.setModelString(modelString());
        cropTable.setSoftwareString(softwareString());
        cropTable.setTransform(transform());
        cropTable.setProjection(projection());
        cropTable.setIntrinsics(intrinsics());
        cropTable.setBoundingBox(boundingBox());

        if (constPhaseCorrectionTable()){
            memcpy(cropTable.constPhaseCorrectionTable(), constPhaseCorrectionTable(), LENGTHPHASECORRECTIONTABLE * sizeof(float));
        }

        for (unsigned int row = 0; row < h; row++){
            float *toBuffer = (float*)cropTable.constScanLine(row);
            float *fmBuffer = (float*)constScanLine(row + y);
            for (unsigned int col = 0; col < w; col++){
                memcpy(&toBuffer[12 * col], &fmBuffer[12 * (col + x)], 12 * sizeof(float));
            }
        }
        cropTable.updateLimits();

        // RETURN THE NEW LOOK UP TABLE TO THE USER
        return(cropTable);
    } else {
        return(LAULookUpTable());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAULookUpTable::updateLimits()
{
    if (data->numChns == 12 || data->numChns == 13 || data->numChns == 16 || data->numChns == 20) {
        int offset = (data->numChns == 16) ? 16 : 12;

        // USE THE Z LIMITS TO FIND THE X AND Y LIMITS FROM THE ABCD COEFFICIENTS
        int index = 0;

        // INITIALIZE THE X AND Y LIMITS
        data->xMin = 1e8;
        data->yMin = 1e8;
        data->xMax = -1e8;
        data->yMax = -1e8;

        // GET THE POINTER TO THE FIRST BYTE IN THE FOURTH ORDER POLYNOMIAL TABLE
        float *buffer = (float *)data->buffer;

        // MOVE THE BUFFER POINTER IF THIS IS AN ACTIVE STEREO VISION TABLE
        if (data->numChns == 20){
            buffer += (data->numRows * data->numCols * 8);
        }

        // ITERATE THROUGH EACH AND EVERY PIXEL
        for (unsigned int row = 0; row < data->numRows; row++) {
            for (unsigned int col = 0; col < data->numCols; col++) {
                // ONLY SEARCH THE X-COORDINATE RANGE IN THE MIDDLE BAND OF ROWS
                if ((row > (data->numRows / 2 - 3)) && (row < (data->numRows / 2 + 3))){
                    // GET THE X RANGE FROM THE ZMIN VALUE
                    float x = buffer[index + 0] * data->zMin + buffer[index + 1];
                    data->xMin = qMin(data->xMin, x);
                    data->xMax = qMax(data->xMax, x);

                    // GET THE X RANGE FROM THE ZMAX VALUE
                    x = buffer[index + 0] * data->zMax + buffer[index + 1];
                    data->xMin = qMin(data->xMin, x);
                    data->xMax = qMax(data->xMax, x);
                }

                // ONLY SEARCH THE Y-COORDINATE RANGE IN THE MIDDLE BAND OF COLUMNS
                if ((col > (data->numCols / 2 - 3)) && (col < (data->numCols / 2 + 3))){
                    // GET THE Y RANGE FROM THE ZMIN VALUE
                    float y = buffer[index + 2] * data->zMin + buffer[index + 3];
                    data->yMin = qMin(data->yMin, y);
                    data->yMax = qMax(data->yMax, y);

                    // GET THE Y RANGE FROM THE ZMAX VALUE
                    y = buffer[index + 2] * data->zMax + buffer[index + 3];
                    data->yMin = qMin(data->yMin, y);
                    data->yMax = qMax(data->yMax, y);
                }

                // INCREMENT THE INDEX TO THE NEXT PIXEL
                index += offset;
            }
        }
    } else {
        __m128 minVec = _mm_set1_ps(1e6f);
        __m128 maxVec = _mm_set1_ps(-1e6f);

        float *buffer = (float *)this->constScanLine(0);
        if (data->style == StyleXYZWRCPQLookUpTable) {
            for (unsigned int ind = 0; ind < 2 * width()*height(); ind++) {
                __m128 pixVec = _mm_load_ps(&buffer[8 * ind]);
                __m128 mskVec = _mm_cmpeq_ps(pixVec, pixVec);
                if (_mm_test_all_ones(_mm_castps_si128(mskVec))) {
                    minVec = _mm_min_ps(minVec, pixVec);
                    maxVec = _mm_max_ps(maxVec, pixVec);
                }
            }
        } else {
            for (unsigned int ind = 0; ind < width()*height(); ind++) {
                __m128 pixVec = _mm_load_ps(&buffer[4 * ind]);
                __m128 mskVec = _mm_cmpeq_ps(pixVec, pixVec);
                if (_mm_test_all_ones(_mm_castps_si128(mskVec))) {
                    minVec = _mm_min_ps(minVec, pixVec);
                    maxVec = _mm_max_ps(maxVec, pixVec);
                }
            }
        }

        *(int *)&(data->xMin) = _mm_extract_ps(minVec, 0);
        *(int *)&(data->yMin) = _mm_extract_ps(minVec, 1);
        *(int *)&(data->zMin) = _mm_extract_ps(minVec, 2);

        *(int *)&(data->xMax) = _mm_extract_ps(maxVec, 0);
        *(int *)&(data->yMax) = _mm_extract_ps(maxVec, 1);
        *(int *)&(data->zMax) = _mm_extract_ps(maxVec, 2);
    }

    // EXPAND THE LIMITS SO THAT ENTIRE FIELD OF VIEW FITS INSIDE SYMMETRICALLY
    data->xMax = qMax(data->xMax, -data->xMin);
    data->xMin = -data->xMax;
    data->yMax = qMax(data->yMax, -data->yMin);
    data->yMin = -data->yMax;

    // DERIVE THE FIELDS OF VIEW
    if (qAbs(data->zMin) > qAbs(data->zMax)) {
        float   phiA = atan(data->yMin / data->zMin);
        float   phiB = atan(data->yMax / data->zMin);
        float thetaA = atan(data->xMin / data->zMin);
        float thetaB = atan(data->xMax / data->zMin);

        data->verticalFieldOfView = fabs(phiA) + fabs(phiB);
        data->horizontalFieldOfView = fabs(thetaA) + fabs(thetaB);
    } else {
        float   phiA = atan(data->yMin / data->zMax);
        float   phiB = atan(data->yMax / data->zMax);
        float thetaA = atan(data->xMin / data->zMax);
        float thetaB = atan(data->xMax / data->zMax);

        data->verticalFieldOfView = fabs(phiA) + fabs(phiB);
        data->horizontalFieldOfView = fabs(thetaA) + fabs(thetaB);
    }

    qDebug() << data->xMin << data->xMax << data->yMin << data->yMax << data->zMin << data->zMax;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTableLoader::LAULookUpTableLoader(QString filename, unsigned short drc, unsigned char *bffr, QObject *parent) : QThread(parent), tiff(NULL), buffer(bffr), step(0), rows(0), directory(drc)
{
    // OPEN INPUT TIFF FILE FROM DISK
    tiff = TIFFOpen(filename.toLocal8Bit(), "r");
    if (tiff) {
        // SET THE DIRECTORY
        TIFFSetDirectory(tiff, directory);

        unsigned int cols = 0;
        unsigned short chns = 0;
        unsigned short bits = 0;

        // GET THE NUMBER OF ROWS, COLUMNS, AND BITS PER PIXEL
        TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &rows);
        TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &cols);
        TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &chns);
        TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bits);

        // CALCULATE THE NUMBER OF BYTES PER IMAGE ROW
        step = cols * chns * bits / 8;
    } else {
        qDebug() << "TIFFLoader Error:" << filename << directory;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTableLoader::~LAULookUpTableLoader()
{
    if (tiff) {
        TIFFClose(tiff);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAULookUpTableLoader::run()
{
    if (buffer && tiff) {
        // ITERATE THROUGH EACH ROW READING INTO THE SUPPLIED MEMORY BUFFER
        for (unsigned int row = 0; row < rows; row++) {
            TIFFReadScanline(tiff, (buffer + row * step), (int)row);
        }
    }
}
