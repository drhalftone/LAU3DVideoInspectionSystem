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

#ifndef LAUITERATIVECLOSESTPOINTOBJECT_H
#define LAUITERATIVECLOSESTPOINTOBJECT_H

#include <QList>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QVector3D>
#include <QFile>
#include <QMatrix4x4>

// PCL includes
#ifdef ENABLEPOINTMATCHER
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/registration/icp.h>
#include <pcl/filters/filter.h>
#include <pcl/filters/random_sample.h>
#include <pcl/filters/voxel_grid.h>
#include <Eigen/Dense>
#endif

#include "lauproximityglfilter.h"

class LAUIterativeClosestPointObject : public QObject
{
    Q_OBJECT

public:
    explicit LAUIterativeClosestPointObject(QObject *parent = nullptr);
    ~LAUIterativeClosestPointObject();

    static unsigned int icpBusyCounterA;
    static unsigned int icpBusyCounterB;

    void setFmScan(LAUScan scan);
    void setToScan(LAUScan scan);
    void alignPointClouds();

public slots:
    void onAlignPointLists(QList<QVector3D> fromList, QList<QVector3D> toList);
    void onAlignPointClouds(QList<QVector3D> fromList, QList<QVector3D> toList);

signals:
    void emitTransform(QMatrix4x4 transform);

private:
    // PCL objects
#ifdef ENABLEPOINTMATCHER
    pcl::IterativeClosestPoint<pcl::PointXYZ, pcl::PointXYZ> icp;
    pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud;
    pcl::PointCloud<pcl::PointXYZ>::Ptr targetCloud;
    pcl::PointCloud<pcl::PointXYZ>::Ptr alignedCloud;

    // Filters
    pcl::VoxelGrid<pcl::PointXYZ> voxelFilter;
    pcl::RandomSample<pcl::PointXYZ> randomSampler;
#endif
    QMatrix4x4 transform;
    int numFrSamples, numToSamples;
    float *fmMatrixBuffer, *toMatrixBuffer;
    LAUProximityGLFilter *proximityGLFilter;

    // Helper functions
    void setupICP();
#ifdef ENABLEPOINTMATCHER
    pcl::PointCloud<pcl::PointXYZ>::Ptr createPointCloudFromBuffer(float* buffer, int numPoints);
#endif
    QMatrix4x4 eigenToQMatrix(const Eigen::Matrix4f& eigenMatrix);
    Eigen::Matrix4f qMatrixToEigen(const QMatrix4x4& qMatrix);
    pcl::PointCloud<pcl::PointXYZ>::Ptr qListToPointCloud(const QList<QVector3D>& pointList);
};

class LAUIterativeClosestPoint
{
public:
    LAUIterativeClosestPoint(QList<QVector3D> fromList, QList<QVector3D> toList);

    QMatrix4x4 transform() const { return(optTransform); }
    QList<int> pairings() const { return(mapping); }
    LAUScan error() const { return(LAUScan()); }

    static float alignPoints(QList<QVector3D> fromList, QList<QVector3D> toList, QMatrix4x4 *transform);
    static bool isValid(QVector3D point) { return(!qIsNaN(point.x() * point.y() * point.z())); }
    static QList<int> permutations(int N, int n);

private:
    static QImage permutationsImage;
    QMatrix4x4 optTransform;
    QList<int> mapping;
};

#endif // LAUITERATIVECLOSESTPOINTOBJECT_H
