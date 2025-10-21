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

#ifndef LAU3DTRACKINGFILTER_H
#define LAU3DTRACKINGFILTER_H

#include <QFile>
#include <QtCore>
#include <QObject>
#include <QThread>
#include <QtGlobal>
#include <QMatrix4x4>
#include <QFileDialog>
#include <QApplication>

#include "lauscan.h"

#define MAXTRACKINGNUMBEROFMERGETHREADS   8
#ifdef ENABLEPOINTMATCHER
#define TRACKINGNUMBEROFMERGETHREADS   8
#else
#define TRACKINGNUMBEROFMERGETHREADS   1
#endif

#ifdef ENABLEPOINTMATCHER
#include "PointMatcher.h"
typedef PointMatcher<float>::Vector Vector; //!< alias
typedef PointMatcher<float>::Matrix Matrix; //!< alias
typedef PointMatcher<float>::DataPoints DataPoints; //!< alias
typedef PointMatcher<float>::TransformationParameters TransformationParameters; //!< alias
typedef PointMatcher<float>::Matrix Parameters; //!< alias
typedef PointMatcher<float>::DataPoints::Label Label; //!< alias
typedef PointMatcher<float>::DataPoints::Labels Labels; //!< alias
typedef PointMatcher<float>::ICP ICP;
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DTrackingFilter : public QObject
{
    Q_OBJECT

public:
    explicit LAU3DTrackingFilter(int cols, int rows, QObject *parent = 0);
    ~LAU3DTrackingFilter();

    QMatrix4x4 findTransform(LAUScan scanTo, LAUScan scanFm);

public slots:
    void onUpdateBuffer(LAUScan scanTo, LAUScan scanFm);

private:
#ifdef ENABLEPOINTMATCHER
    ICP icp;
    Labels featureLabels;
    PointMatcher<float>::Matches matches;
    PointMatcher<float>::ErrorMinimizer::ErrorElements elements;
    std::shared_ptr<PointMatcher<float>::Inspector> inspector;
    std::shared_ptr<PointMatcher<float>::ErrorMinimizer> errorMinimizer;
#endif
    int numCols, numRows, maxNumberOfSamples, downSampleFactor;
    float *fmMatrixBuffer, *toMatrixBuffer;

signals:
    void emitBuffer(LAUScan scan);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DTrackingController : public QObject
{
    Q_OBJECT

public:
    explicit LAU3DTrackingController(int cols, int rows, QObject *parent = 0);
    ~LAU3DTrackingController();

    bool isValid() const
    {
        return (numRows * numCols > 0);
    }
    bool isNull() const
    {
        return (!isValid());
    }

public slots:
    void onUpdateBuffer(LAUScan scan);

private:
    int numCols, numRows;
    LAUScan prevScan;
    QThread *threads[MAXTRACKINGNUMBEROFMERGETHREADS];
    LAU3DTrackingFilter *mergeObjects[MAXTRACKINGNUMBEROFMERGETHREADS];
    bool busy[MAXTRACKINGNUMBEROFMERGETHREADS];
    QList<LAUScan> inList;
    QList<LAUScan> otList;
    QList<LAUScan> wtList;

    void releaseBuffers();

private slots:
    void onReceiveBufferA(LAUScan scan);
    void onReceiveBufferB(LAUScan scan);
    void onReceiveBufferC(LAUScan scan);
    void onReceiveBufferD(LAUScan scan);
    void onReceiveBufferE(LAUScan scan);
    void onReceiveBufferF(LAUScan scan);
    void onReceiveBufferG(LAUScan scan);
    void onReceiveBufferH(LAUScan scan);

signals:
    void emitBuffer(LAUScan scan);

    void emitBufferA(LAUScan scanTo, LAUScan scanFm);
    void emitBufferB(LAUScan scanTo, LAUScan scanFm);
    void emitBufferC(LAUScan scanTo, LAUScan scanFm);
    void emitBufferD(LAUScan scanTo, LAUScan scanFm);
    void emitBufferE(LAUScan scanTo, LAUScan scanFm);
    void emitBufferF(LAUScan scanTo, LAUScan scanFm);
    void emitBufferG(LAUScan scanTo, LAUScan scanFm);
    void emitBufferH(LAUScan scanTo, LAUScan scanFm);
};

#endif // LAU3DTRACKINGFILTER_H
