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

#include "lau3dtrackingfilter.h"
#include <fstream>

#define EIGEN_NO_EIGEN2_DEPRECATED_WARNING
#define EIGEN_DEBUG_ASSIGN
#include "Eigen/Eigen"

typedef Eigen::Matrix<float, 4, Eigen::Dynamic> Matrixf4d;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DTrackingFilter::LAU3DTrackingFilter(int cols, int rows, QObject *parent) : QObject(parent), numCols(cols), numRows(rows), maxNumberOfSamples(cols * rows)
{
    // SET THE DOWNSAMPLE FACTOR
    downSampleFactor = 4; //qMax(1, (int)qFloor(sqrt((float)maxNumberOfSamples/5000.0f)));

    // INITIALIZE THE FEATURE LABELS VECTOR
    featureLabels.push_back(Label("x", 1));
    featureLabels.push_back(Label("y", 1));
    featureLabels.push_back(Label("z", 1));
    featureLabels.push_back(Label("pad", 1));

#ifdef DONTCOMPILE
    PointMatcherSupport::Parametrizable::Parameters parameters;
    parameters[std::string("minDist")] = std::string("1.0");
    icp.readingDataPointsFilters.push_back(PointMatcher<float>::get().DataPointsFilterRegistrar.create(std::string("IdentityDataPointsFilter"), parameters));
    icp.referenceDataPointsFilters.push_back(PointMatcher<float>::get().DataPointsFilterRegistrar.create(std::string("IdentityDataPointsFilter"), parameters));
    parameters.clear();

    parameters[std::string("knn")] = std::string("1");
    parameters[std::string("epsilon")] = std::string("3.16");
    icp.matcher = PointMatcher<float>::get().MatcherRegistrar.create(std::string("KDTreeMatcher"), parameters);
    parameters.clear();

    parameters[std::string("ratio")] = std::string("0.60");
    icp.outlierFilters.push_back(PointMatcher<float>::get().OutlierFilterRegistrar.create(std::string("TrimmedDistOutlierFilter"), parameters));
    parameters.clear();

    errorMinimizer = PointMatcher<float>::get().ErrorMinimizerRegistrar.create(std::string("PointToPointErrorMinimizer"));
    icp.errorMinimizer = errorMinimizer;

    parameters[std::string("maxIterationCount")] = std::string("1000");
    icp.transformationCheckers.push_back(PointMatcher<float>::get().TransformationCheckerRegistrar.create(std::string("CounterTransformationChecker"), parameters));
    parameters.clear();

    parameters[std::string("minDiffRotErr")] = std::string("0.00001");
    parameters[std::string("minDiffTransErr")] = std::string("0.0001");
    parameters[std::string("smoothLength")] = std::string("4");
    icp.transformationCheckers.push_back(PointMatcher<float>::get().TransformationCheckerRegistrar.create(std::string("DifferentialTransformationChecker"), parameters));
    parameters.clear();

    icp.transformations.push_back(PointMatcher<float>::get().TransformationRegistrar.create("RigidTransformation"));

    inspector = PointMatcher<float>::get().InspectorRegistrar.create(std::string("PerformanceInspector"), parameters);
    icp.inspector = inspector;
    parameters.clear();

#ifdef Q_OS_WIN
    parameters[std::string("dumpStats")] = std::string("1");
    parameters[std::string("dumpPerfOnExit")] = std::string("1");
    parameters[std::string("infoFileName")] = std::string("C:/Users/Daniel Lau/Documents/filelogger.txt");
    parameters[std::string("warningFileName")] = std::string("C:/Users/Daniel Lau/Documents/warnlogger.txt");
    PointMatcherSupport::setLogger(new PointMatcherSupport::NullLogger());
#else
    parameters[std::string("dumpStats")] = std::string("1");
    parameters[std::string("dumpPerfOnExit")] = std::string("1");
    PointMatcherSupport::setLogger(PointMatcher<float>::get().LoggerRegistrar.create(std::string("FileLogger"), parameters));
    //PointMatcherSupport::setLogger(new PointMatcherSupport::NullLogger());
#endif
#endif
    // ALLOCATE SPACE TO HOLD THE VALID POINTS OF THE INCOMING SCANS
    toMatrixBuffer = (float *)_mm_malloc(maxNumberOfSamples * 4 * sizeof(float), 16);
    fmMatrixBuffer = (float *)_mm_malloc(maxNumberOfSamples * 4 * sizeof(float), 16);

    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DTrackingFilter::~LAU3DTrackingFilter()
{
    if (toMatrixBuffer) {
        _mm_free(toMatrixBuffer);
    }
    if (fmMatrixBuffer) {
        _mm_free(fmMatrixBuffer);
    }

    qDebug() << QString("LAU3DTrackingFilter::~LAU3DTrackingFilter()");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QMatrix4x4 LAU3DTrackingFilter::findTransform(LAUScan scanTo, LAUScan scanFm)
{
    // CREATE A TRANSFORM TO RETURN TO THE USER
    QMatrix4x4 transform;

    // COPY THE INCOMING VALID SCAN POINTS INTO FLOATING POINT BUFFERS
    int numToVertices = scanTo.extractXYZWVertices(toMatrixBuffer, downSampleFactor);
    int numFmVertices = scanFm.extractXYZWVertices(fmMatrixBuffer, downSampleFactor);

    if (qMin(numToVertices, numFmVertices) > 200) {
#ifdef ENABLEPOINTMATCHER
        // CREATE MATRIX MAPS TO WRAP AROUND THE BUFFERS BASED ON THE NUMBER OF VALID POINTS
        Eigen::Map<Matrixf4d, Eigen::Aligned> fmMatrix(fmMatrixBuffer, 4, numFmVertices);
        Eigen::Map<Matrixf4d, Eigen::Aligned> toMatrix(toMatrixBuffer, 4, numToVertices);

        // CALL THE ICP ALGORITHM TO SET THE TRANSFORM MATRIX
        DataPoints pointsFm(fmMatrix, featureLabels);
        DataPoints pointsTo(toMatrix, featureLabels);

        // COMPUTE THE TRANSFORMATION TO EXPRESS DATA IN REF
        PointMatcher<float>::TransformationParameters T = icp(pointsFm, pointsTo);
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                transform(row, col) = T.coeff(row, col);
            }
        }
#endif
    } else {
        qDebug() << "Not enough points.";
    }
    return (transform);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingFilter::onUpdateBuffer(LAUScan scanTo, LAUScan scanFm)
{
    QMatrix4x4 transform = findTransform(scanTo, scanFm);
    scanTo.setConstTransform(transform);
    emit emitBuffer(scanTo);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DTrackingController::LAU3DTrackingController(int cols, int rows, QObject *parent) : QObject(parent), numCols(cols), numRows(rows)
{
    if (isValid()) {
        for (int n = 0; n < TRACKINGNUMBEROFMERGETHREADS; n++) {
            busy[n] = false;
            mergeObjects[n] = new LAU3DTrackingFilter(cols, rows);
            switch (n) {
                case 0:
                    connect(this, SIGNAL(emitBufferA(LAUScan, LAUScan)), mergeObjects[n], SLOT(onUpdateBuffer(LAUScan, LAUScan)));
                    connect(mergeObjects[n], SIGNAL(emitBuffer(LAUScan)), this, SLOT(onReceiveBufferA(LAUScan)));
                    break;
                case 1:
                    connect(this, SIGNAL(emitBufferB(LAUScan, LAUScan)), mergeObjects[n], SLOT(onUpdateBuffer(LAUScan, LAUScan)));
                    connect(mergeObjects[n], SIGNAL(emitBuffer(LAUScan)), this, SLOT(onReceiveBufferB(LAUScan)));
                    break;
                case 2:
                    connect(this, SIGNAL(emitBufferC(LAUScan, LAUScan)), mergeObjects[n], SLOT(onUpdateBuffer(LAUScan, LAUScan)));
                    connect(mergeObjects[n], SIGNAL(emitBuffer(LAUScan)), this, SLOT(onReceiveBufferC(LAUScan)));
                    break;
                case 3:
                    connect(this, SIGNAL(emitBufferD(LAUScan, LAUScan)), mergeObjects[n], SLOT(onUpdateBuffer(LAUScan, LAUScan)));
                    connect(mergeObjects[n], SIGNAL(emitBuffer(LAUScan)), this, SLOT(onReceiveBufferD(LAUScan)));
                    break;
                case 4:
                    connect(this, SIGNAL(emitBufferE(LAUScan, LAUScan)), mergeObjects[n], SLOT(onUpdateBuffer(LAUScan, LAUScan)));
                    connect(mergeObjects[n], SIGNAL(emitBuffer(LAUScan)), this, SLOT(onReceiveBufferE(LAUScan)));
                    break;
                case 5:
                    connect(this, SIGNAL(emitBufferF(LAUScan, LAUScan)), mergeObjects[n], SLOT(onUpdateBuffer(LAUScan, LAUScan)));
                    connect(mergeObjects[n], SIGNAL(emitBuffer(LAUScan)), this, SLOT(onReceiveBufferF(LAUScan)));
                    break;
                case 6:
                    connect(this, SIGNAL(emitBufferG(LAUScan, LAUScan)), mergeObjects[n], SLOT(onUpdateBuffer(LAUScan, LAUScan)));
                    connect(mergeObjects[n], SIGNAL(emitBuffer(LAUScan)), this, SLOT(onReceiveBufferG(LAUScan)));
                    break;
                case 7:
                    connect(this, SIGNAL(emitBufferH(LAUScan, LAUScan)), mergeObjects[n], SLOT(onUpdateBuffer(LAUScan, LAUScan)));
                    connect(mergeObjects[n], SIGNAL(emitBuffer(LAUScan)), this, SLOT(onReceiveBufferH(LAUScan)));
                    break;
            }
            threads[n] = new QThread();
            mergeObjects[n]->moveToThread(threads[n]);
            threads[n]->start();
        }
    } else {
        for (int n = 0; n < TRACKINGNUMBEROFMERGETHREADS; n++) {
            busy[n] = false;
            mergeObjects[n] = NULL;
            threads[n] = NULL;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DTrackingController::~LAU3DTrackingController()
{
    // TELL ALL THE THREADS TO STOP
    for (int n = 0; n < TRACKINGNUMBEROFMERGETHREADS; n++) {
        if (threads[n]) {
            threads[n]->quit();
        }
    }

    // WAIT UNTIL EACH THREAD STOPS, AND THEN DELETE ITS OBJECTS
    for (int n = 0; n < TRACKINGNUMBEROFMERGETHREADS; n++) {
        if (threads[n]) {
            while (threads[n]->isRunning()) {
                qApp->processEvents();
            }
            delete mergeObjects[n];
            delete threads[n];
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingController::onUpdateBuffer(LAUScan scan)
{
    // MAKE SURE THE SCAN IS VALID BEFORE WE SPEND ANY TIME PROCESSING IT
    if (scan.isValid()) {
        // SEE IF THERE IS A NODE FREE TO PROCESS THIS INCOMING SCAN
        for (int n = 0; n < TRACKINGNUMBEROFMERGETHREADS; n++) {
            if (busy[n] == false) {
                // PUT THE INCOMING SCAN INTO OUR IN LIST TO WE KNOW ITS BEING PROCESSED
                inList << scan;

                // SET THE BUSY FLAG TO TRUE FOR THE NODE ABOUT TO RECEIVE THE SCAN
                busy[n] = true;

                // SEND THE SCAN TO THE NEXT AVAILABLE NODE
                switch (n) {
                    case 0:
                        emit emitBufferA(scan, prevScan);
                        break;
                    case 1:
                        emit emitBufferB(scan, prevScan);
                        break;
                    case 2:
                        emit emitBufferC(scan, prevScan);
                        break;
                    case 3:
                        emit emitBufferD(scan, prevScan);
                        break;
                    case 4:
                        emit emitBufferE(scan, prevScan);
                        break;
                    case 5:
                        emit emitBufferF(scan, prevScan);
                        break;
                    case 6:
                        emit emitBufferG(scan, prevScan);
                        break;
                    case 7:
                        emit emitBufferH(scan, prevScan);
                        break;
                }
                prevScan = scan;
                return;
            }
        }
        // IF WE MADE IT THIS FAR, THEN THERE MUST NOT BY A FREE NODE
        // SO WE NEED TO ADD THE INCOMING SCAN TO THE WAITING LIST
        wtList << scan;
    } else {
        emitBuffer(scan);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingController::onReceiveBufferA(LAUScan scan)
{
    // SINCE NODE A IS NOW AVAILABLE, WE SHOULD SEND IT THE NEXT SCAN IN THE WAITING LIST IMMEDIATELY
    if (wtList.count() > 0) {
        LAUScan scan = wtList.takeFirst();
        inList << scan;
        emit emitBufferA(scan, prevScan);
        prevScan = scan;
    } else {
        busy[0] = false;
    }

    // SEARCH INLIST FOR THIS SCAN SO WE CAN REMOVE IT
    for (int n = 0; n < inList.count(); n++) {
        if (inList.at(n).timeStamp() == scan.timeStamp()) {
            inList.takeAt(n);
            break;
        }
    }

    // PLACE SCAN INTO OUTLIST IN CHRONOLOGICAL ORDER
    if (otList.isEmpty() || otList.last().timeStamp() < scan.timeStamp()) {
        otList << scan;
    } else {
        for (int n = 0; n < otList.count(); n++) {
            if (otList.at(n).timeStamp() > scan.timeStamp()) {
                otList.insert(n, scan);
                break;
            }
        }
    }

    // RELEASE BUFFERS
    releaseBuffers();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingController::onReceiveBufferB(LAUScan scan)
{
    // SINCE NODE A IS NOW AVAILABLE, WE SHOULD SEND IT THE NEXT SCAN IN THE WAITING LIST IMMEDIATELY
    if (wtList.count() > 0) {
        LAUScan scan = wtList.takeFirst();
        inList << scan;
        emit emitBufferB(scan, prevScan);
        prevScan = scan;
    } else {
        busy[1] = false;
    }

    // SEARCH INLIST FOR THIS SCAN SO WE CAN REMOVE IT
    for (int n = 0; n < inList.count(); n++) {
        if (inList.at(n).timeStamp() == scan.timeStamp()) {
            inList.takeAt(n);
            break;
        }
    }

    // PLACE SCAN INTO OUTLIST IN CHRONOLOGICAL ORDER
    if (otList.isEmpty() || otList.last().timeStamp() < scan.timeStamp()) {
        otList << scan;
    } else {
        for (int n = 0; n < otList.count(); n++) {
            if (otList.at(n).timeStamp() > scan.timeStamp()) {
                otList.insert(n, scan);
                break;
            }
        }
    }

    // NOW EMIT OUT SCANS THAT PRECEED THE OLDEST SCAN BEING PROCESSED
    releaseBuffers();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingController::onReceiveBufferC(LAUScan scan)
{
    // SINCE NODE A IS NOW AVAILABLE, WE SHOULD SEND IT THE NEXT SCAN IN THE WAITING LIST IMMEDIATELY
    if (wtList.count() > 0) {
        LAUScan scan = wtList.takeFirst();
        inList << scan;
        emit emitBufferC(scan, prevScan);
        prevScan = scan;
    } else {
        busy[2] = false;
    }

    // SEARCH INLIST FOR THIS SCAN SO WE CAN REMOVE IT
    for (int n = 0; n < inList.count(); n++) {
        if (inList.at(n).timeStamp() == scan.timeStamp()) {
            inList.takeAt(n);
            break;
        }
    }

    // PLACE SCAN INTO OUTLIST IN CHRONOLOGICAL ORDER
    if (otList.isEmpty() || otList.last().timeStamp() < scan.timeStamp()) {
        otList << scan;
    } else {
        for (int n = 0; n < otList.count(); n++) {
            if (otList.at(n).timeStamp() > scan.timeStamp()) {
                otList.insert(n, scan);
                break;
            }
        }
    }

    // NOW EMIT OUT SCANS THAT PRECEED THE OLDEST SCAN BEING PROCESSED
    releaseBuffers();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingController::onReceiveBufferD(LAUScan scan)
{
    // SINCE NODE A IS NOW AVAILABLE, WE SHOULD SEND IT THE NEXT SCAN IN THE WAITING LIST IMMEDIATELY
    if (wtList.count() > 0) {
        LAUScan scan = wtList.takeFirst();
        inList << scan;
        emit emitBufferD(scan, prevScan);
        prevScan = scan;
    } else {
        busy[3] = false;
    }

    // SEARCH INLIST FOR THIS SCAN SO WE CAN REMOVE IT
    for (int n = 0; n < inList.count(); n++) {
        if (inList.at(n).timeStamp() == scan.timeStamp()) {
            inList.takeAt(n);
            break;
        }
    }

    // PLACE SCAN INTO OUTLIST IN CHRONOLOGICAL ORDER
    if (otList.isEmpty() || otList.last().timeStamp() < scan.timeStamp()) {
        otList << scan;
    } else {
        for (int n = 0; n < otList.count(); n++) {
            if (otList.at(n).timeStamp() > scan.timeStamp()) {
                otList.insert(n, scan);
                break;
            }
        }
    }

    // NOW EMIT OUT SCANS THAT PRECEED THE OLDEST SCAN BEING PROCESSED
    releaseBuffers();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingController::onReceiveBufferE(LAUScan scan)
{
    // SINCE NODE A IS NOW AVAILABLE, WE SHOULD SEND IT THE NEXT SCAN IN THE WAITING LIST IMMEDIATELY
    if (wtList.count() > 0) {
        LAUScan scan = wtList.takeFirst();
        inList << scan;
        emit emitBufferE(scan, prevScan);
        prevScan = scan;
    } else {
        busy[4] = false;
    }

    // SEARCH INLIST FOR THIS SCAN SO WE CAN REMOVE IT
    for (int n = 0; n < inList.count(); n++) {
        if (inList.at(n).timeStamp() == scan.timeStamp()) {
            inList.takeAt(n);
            break;
        }
    }

    // PLACE SCAN INTO OUTLIST IN CHRONOLOGICAL ORDER
    if (otList.isEmpty() || otList.last().timeStamp() < scan.timeStamp()) {
        otList << scan;
    } else {
        for (int n = 0; n < otList.count(); n++) {
            if (otList.at(n).timeStamp() > scan.timeStamp()) {
                otList.insert(n, scan);
                break;
            }
        }
    }

    // NOW EMIT OUT SCANS THAT PRECEED THE OLDEST SCAN BEING PROCESSED
    releaseBuffers();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingController::onReceiveBufferF(LAUScan scan)
{
    // SINCE NODE A IS NOW AVAILABLE, WE SHOULD SEND IT THE NEXT SCAN IN THE WAITING LIST IMMEDIATELY
    if (wtList.count() > 0) {
        LAUScan scan = wtList.takeFirst();
        inList << scan;
        emit emitBufferF(scan, prevScan);
        prevScan = scan;
    } else {
        busy[5] = false;
    }

    // SEARCH INLIST FOR THIS SCAN SO WE CAN REMOVE IT
    for (int n = 0; n < inList.count(); n++) {
        if (inList.at(n).timeStamp() == scan.timeStamp()) {
            inList.takeAt(n);
            break;
        }
    }

    // PLACE SCAN INTO OUTLIST IN CHRONOLOGICAL ORDER
    if (otList.isEmpty() || otList.last().timeStamp() < scan.timeStamp()) {
        otList << scan;
    } else {
        for (int n = 0; n < otList.count(); n++) {
            if (otList.at(n).timeStamp() > scan.timeStamp()) {
                otList.insert(n, scan);
                break;
            }
        }
    }

    // NOW EMIT OUT SCANS THAT PRECEED THE OLDEST SCAN BEING PROCESSED
    releaseBuffers();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingController::onReceiveBufferG(LAUScan scan)
{
    // SINCE NODE A IS NOW AVAILABLE, WE SHOULD SEND IT THE NEXT SCAN IN THE WAITING LIST IMMEDIATELY
    if (wtList.count() > 0) {
        LAUScan scan = wtList.takeFirst();
        inList << scan;
        emit emitBufferG(scan, prevScan);
        prevScan = scan;
    } else {
        busy[6] = false;
    }

    // SEARCH INLIST FOR THIS SCAN SO WE CAN REMOVE IT
    for (int n = 0; n < inList.count(); n++) {
        if (inList.at(n).timeStamp() == scan.timeStamp()) {
            inList.takeAt(n);
            break;
        }
    }

    // PLACE SCAN INTO OUTLIST IN CHRONOLOGICAL ORDER
    if (otList.isEmpty() || otList.last().timeStamp() < scan.timeStamp()) {
        otList << scan;
    } else {
        for (int n = 0; n < otList.count(); n++) {
            if (otList.at(n).timeStamp() > scan.timeStamp()) {
                otList.insert(n, scan);
                break;
            }
        }
    }

    // NOW EMIT OUT SCANS THAT PRECEED THE OLDEST SCAN BEING PROCESSED
    releaseBuffers();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingController::onReceiveBufferH(LAUScan scan)
{
    // SINCE NODE A IS NOW AVAILABLE, WE SHOULD SEND IT THE NEXT SCAN IN THE WAITING LIST IMMEDIATELY
    if (wtList.count() > 0) {
        LAUScan scan = wtList.takeFirst();
        inList << scan;
        emit emitBufferH(scan, prevScan);
        prevScan = scan;
    } else {
        busy[7] = false;
    }

    // SEARCH INLIST FOR THIS SCAN SO WE CAN REMOVE IT
    for (int n = 0; n < inList.count(); n++) {
        if (inList.at(n).timeStamp() == scan.timeStamp()) {
            inList.takeAt(n);
            break;
        }
    }

    // PLACE SCAN INTO OUTLIST IN CHRONOLOGICAL ORDER
    if (otList.isEmpty() || otList.last().timeStamp() < scan.timeStamp()) {
        otList << scan;
    } else {
        for (int n = 0; n < otList.count(); n++) {
            if (otList.at(n).timeStamp() > scan.timeStamp()) {
                otList.insert(n, scan);
                break;
            }
        }
    }

    // NOW EMIT OUT SCANS THAT PRECEED THE OLDEST SCAN BEING PROCESSED
    releaseBuffers();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DTrackingController::releaseBuffers()
{
    // NOW EMIT OUT SCANS THAT PRECEED THE OLDEST SCAN BEING PROCESSED
    unsigned int earliestElapsed = (unsigned int)(-1);
    for (int n = 0; n < inList.count(); n++) {
        earliestElapsed = qMin(earliestElapsed, inList.at(n).elapsed());
    }
    for (int n = 0; n < wtList.count(); n++) {
        earliestElapsed = qMin(earliestElapsed, wtList.at(n).elapsed());
    }
    while (otList.isEmpty() == false) {
        if (otList.first().elapsed() < earliestElapsed) {
            emitBuffer(otList.takeFirst());
        } else {
            return;
        }
    }
}
