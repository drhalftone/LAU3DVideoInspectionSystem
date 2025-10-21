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

#include "lauiterativeclosestpointobject.h"
#include <iostream>
#include <fstream>
#include <QDebug>

QImage LAUIterativeClosestPoint::permutationsImage = QImage();
unsigned int LAUIterativeClosestPointObject::icpBusyCounterA = 0;
unsigned int LAUIterativeClosestPointObject::icpBusyCounterB = 0;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUIterativeClosestPointObject::LAUIterativeClosestPointObject(QObject *parent)
    : QObject(parent), numFrSamples(0), numToSamples(0), fmMatrixBuffer(nullptr), toMatrixBuffer(nullptr)
{
    // Initialize PCL point clouds
    sourceCloud.reset(new pcl::PointCloud<pcl::PointXYZ>);
    targetCloud.reset(new pcl::PointCloud<pcl::PointXYZ>);
    alignedCloud.reset(new pcl::PointCloud<pcl::PointXYZ>);

    setupICP();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUIterativeClosestPointObject::~LAUIterativeClosestPointObject()
{
    if (fmMatrixBuffer) {
        _mm_free(fmMatrixBuffer);
    }
    if (toMatrixBuffer) {
        _mm_free(toMatrixBuffer);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUIterativeClosestPointObject::setupICP()
{
    // Configure ICP parameters (equivalent to libpointmatcher configuration)
    icp.setMaximumIterations(150);  // CounterTransformationChecker maxIterationCount
    icp.setTransformationEpsilon(0.001);  // DifferentialTransformationChecker minDiffTransErr
    icp.setEuclideanFitnessEpsilon(0.001);  // DifferentialTransformationChecker minDiffRotErr
    icp.setMaxCorrespondenceDistance(1.0);  // Similar to MinDistDataPointsFilter
    icp.setRANSACOutlierRejectionThreshold(0.1);  // TrimmedDistOutlierFilter ratio equivalent

    // Configure voxel grid filter (optional preprocessing)
    voxelFilter.setLeafSize(0.01f, 0.01f, 0.01f);

    // Configure random sampler (equivalent to RandomSamplingDataPointsFilter prob=0.15)
    randomSampler.setSample(static_cast<unsigned int>(1000)); // Will be adjusted based on input size
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
pcl::PointCloud<pcl::PointXYZ>::Ptr LAUIterativeClosestPointObject::createPointCloudFromBuffer(float* buffer, int numPoints)
{
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    cloud->width = numPoints;
    cloud->height = 1;
    cloud->is_dense = false;
    cloud->points.resize(cloud->width * cloud->height);

    for (int i = 0; i < numPoints; ++i) {
        cloud->points[i].x = buffer[i * 4 + 0];
        cloud->points[i].y = buffer[i * 4 + 1];
        cloud->points[i].z = buffer[i * 4 + 2];
        // buffer[i * 4 + 3] is the pad value, ignored
    }

    // Remove NaN points
    std::vector<int> indices;
    pcl::removeNaNFromPointCloud(*cloud, *cloud, indices);

    return cloud;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
pcl::PointCloud<pcl::PointXYZ>::Ptr LAUIterativeClosestPointObject::qListToPointCloud(const QList<QVector3D>& pointList)
{
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);

    // Filter out invalid points
    for (const QVector3D& point : pointList) {
        if (LAUIterativeClosestPoint::isValid(point)) {
            pcl::PointXYZ pclPoint;
            pclPoint.x = point.x();
            pclPoint.y = point.y();
            pclPoint.z = point.z();
            cloud->points.push_back(pclPoint);
        }
    }

    cloud->width = cloud->points.size();
    cloud->height = 1;
    cloud->is_dense = true;

    return cloud;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QMatrix4x4 LAUIterativeClosestPointObject::eigenToQMatrix(const Eigen::Matrix4f& eigenMatrix)
{
    QMatrix4x4 qMatrix;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            qMatrix(row, col) = eigenMatrix(row, col);
        }
    }
    return qMatrix;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
Eigen::Matrix4f LAUIterativeClosestPointObject::qMatrixToEigen(const QMatrix4x4& qMatrix)
{
    Eigen::Matrix4f eigenMatrix;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            eigenMatrix(row, col) = qMatrix(row, col);
        }
    }
    return eigenMatrix;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUIterativeClosestPointObject::onAlignPointLists(QList<QVector3D> fromList, QList<QVector3D> toList)
{
    QMatrix4x4 transform;
    if (fromList.count() > 2 && toList.count() > 2) {
        // For small point lists, use the original SVD-based alignment
        transform = LAUIterativeClosestPoint(fromList, toList).transform();
    }
    emit emitTransform(transform);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUIterativeClosestPointObject::onAlignPointClouds(QList<QVector3D> fromList, QList<QVector3D> toList)
{
    // Use SVD-based initial alignment for fiducials
    transform = LAUIterativeClosestPoint(fromList, toList).transform();

    // For large point clouds, use PCL ICP
    if (numFrSamples > 1000 && numToSamples > 1000) {
        alignPointClouds();
    }

    emit emitTransform(transform);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUIterativeClosestPointObject::alignPointClouds()
{
    if (!fmMatrixBuffer || !toMatrixBuffer || numFrSamples == 0 || numToSamples == 0) {
        return;
    }

    // Create point clouds from buffers
    sourceCloud = createPointCloudFromBuffer(fmMatrixBuffer, numFrSamples);
    targetCloud = createPointCloudFromBuffer(toMatrixBuffer, numToSamples);

    if (sourceCloud->empty() || targetCloud->empty()) {
        return;
    }

    // Apply random sampling if clouds are large (equivalent to RandomSamplingDataPointsFilter)
    pcl::PointCloud<pcl::PointXYZ>::Ptr sourceSampled(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr targetSampled(new pcl::PointCloud<pcl::PointXYZ>);

    if (sourceCloud->size() > 10000) {
        randomSampler.setSample(static_cast<unsigned int>(sourceCloud->size() * 0.15));
        randomSampler.setInputCloud(sourceCloud);
        randomSampler.filter(*sourceSampled);
    } else {
        *sourceSampled = *sourceCloud;
    }

    if (targetCloud->size() > 10000) {
        randomSampler.setSample(static_cast<unsigned int>(targetCloud->size() * 0.15));
        randomSampler.setInputCloud(targetCloud);
        randomSampler.filter(*targetSampled);
    } else {
        *targetSampled = *targetCloud;
    }

    // Set initial transformation from SVD-based alignment
    Eigen::Matrix4f initialTransform = qMatrixToEigen(transform);

    // Configure ICP
    icp.setInputSource(sourceSampled);
    icp.setInputTarget(targetSampled);

    // Perform alignment
    icp.align(*alignedCloud, initialTransform);

    if (icp.hasConverged()) {
        // Get final transformation
        Eigen::Matrix4f finalTransform = icp.getFinalTransformation();
        transform = eigenToQMatrix(finalTransform);

        qDebug() << "ICP converged with fitness score:" << icp.getFitnessScore();
    } else {
        qDebug() << "ICP did not converge";
    }

    // Debug output (equivalent to original debug statements)
    qDebug() << transform(0, 0) << transform(0, 1) << transform(0, 2) << transform(0, 3);
    qDebug() << transform(1, 0) << transform(1, 1) << transform(1, 2) << transform(1, 3);
    qDebug() << transform(2, 0) << transform(2, 1) << transform(2, 2) << transform(2, 3);
    qDebug() << transform(3, 0) << transform(3, 1) << transform(3, 2) << transform(3, 3);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUIterativeClosestPointObject::setFmScan(LAUScan scan)
{
    // Release any previous memory buffer
    if (fmMatrixBuffer) {
        _mm_free(fmMatrixBuffer);
    }

    // Get the maximum number of pixels in the incoming scan
    numFrSamples = scan.width() * scan.height();

    // Allocate space to hold this maximum number of points
    fmMatrixBuffer = (float *)_mm_malloc(4 * (numFrSamples + 16) * sizeof(float), 16);

    // Extract valid points
    numFrSamples = scan.extractXYZWVertices(fmMatrixBuffer, 4);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUIterativeClosestPointObject::setToScan(LAUScan scan)
{
    // Release any previous memory buffer
    if (toMatrixBuffer) {
        _mm_free(toMatrixBuffer);
    }

    // Get the maximum number of pixels in the incoming scan
    numToSamples = scan.width() * scan.height();

    // Allocate space to hold this maximum number of points
    toMatrixBuffer = (float *)_mm_malloc(4 * (numToSamples + 16) * sizeof(float), 16);

    // Extract valid points
    numToSamples = scan.extractXYZWVertices(toMatrixBuffer, 4);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUIterativeClosestPoint::LAUIterativeClosestPoint(QList<QVector3D> fromList, QList<QVector3D> toList)
{
    if (qMin(fromList.count(), toList.count()) > 2) {
        // Calculate the number of permutations of the from list
        int numPermutationsFromList = 1;
        for (int n = 1; n < fromList.count(); n++) {
            numPermutationsFromList *= (n + 1);
        }

        float optError = 1e6;
        for (int a = 0; a < toList.count(); a++) {
            for (int b = a + 1; b < toList.count(); b++) {
                for (int c = b + 1; c < toList.count(); c++) {
                    // Make a copy of three points from the to-list
                    QList<QVector3D> toListB;
                    toListB << toList.at(a) << toList.at(b) << toList.at(c);

                    // Iterate through all possible permutations of from-points
                    for (int d = 0; d < numPermutationsFromList; d++) {
                        // Get the current permutation of the from list
                        QList<int> fromPermutations = permutations(3, d);

                        // Make a copy of the first numPoints of the from list
                        QList<QVector3D> fromListB;
                        fromListB << fromList.at(fromPermutations.at(0));
                        fromListB << fromList.at(fromPermutations.at(1));
                        fromListB << fromList.at(fromPermutations.at(2));

                        // Align current point permutation
                        QMatrix4x4 T;
                        float error = alignPoints(fromListB, toListB, &T);
                        if (error < optError) {
                            mapping.clear();
                            mapping << a << b << c;
                            optError = error;
                            optTransform = T;
                        }
                    }
                }
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<int> LAUIterativeClosestPoint::permutations(int N, int n)
{
    if (permutationsImage.isNull()) {
        QImage image(":/Images/permutationsTable.tif");
        if (image.format() != QImage::Format_RGB888) {
            image = image.convertToFormat(QImage::Format_RGB888);
        }
        permutationsImage = QImage(image.width(), image.height(), QImage::Format_Indexed8);
        for (int r = 0; r < image.height(); r++) {
            unsigned char *inScanLine = (unsigned char *)image.constScanLine(r);
            unsigned char *otScanLine = (unsigned char *)permutationsImage.constScanLine(r);
            for (int c = 0; c < image.width(); c++) {
                otScanLine[c] = inScanLine[3 * c + 1];
            }
        }
    }

    // Grab the last N elements of the nth row of the permutations table
    QList<int> list;
    unsigned char *otScanLine = (unsigned char *)permutationsImage.constScanLine(n);
    for (int c = 0; c < N && c < permutationsImage.width(); c++) {
        list << otScanLine[permutationsImage.width() - 1 - c];
    }
    return (list);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUIterativeClosestPoint::alignPoints(QList<QVector3D> fromList, QList<QVector3D> toList, QMatrix4x4 *transform)
{
    float error = 1e6;

    // Clean point lists of any invalid points
    QList<QVector3D> pList;
    QList<QVector3D> qList;
    for (int n = 0; n < fromList.count() && n < toList.count(); n++) {
        if (isValid(fromList.at(n)) && isValid(toList.at(n))) {
            pList << fromList.at(n);
            qList << toList.at(n);
        }
    }
    int numPoints = pList.count();

    if (numPoints > 2) {
        // Ensure both lists have the same number of points
        while (pList.count() > numPoints) {
            pList.takeLast();
        }
        while (qList.count() > numPoints) {
            qList.takeLast();
        }

        // Calculate point cloud centroids
        QVector3D pBar, qBar;
        for (int n = 0; n < numPoints; n++) {
            pBar = pBar + pList.at(n);
            qBar = qBar + qList.at(n);
        }
        pBar = pBar / (float)numPoints;
        qBar = qBar / (float)numPoints;

        // Subtract away the centroids from each list
        for (int n = 0; n < numPoints; n++) {
            pList.replace(n, pList.at(n) - pBar);
            qList.replace(n, qList.at(n) - qBar);
        }

        // Create N matrix from point lists using Eigen
        Eigen::Matrix3d N = Eigen::Matrix3d::Zero();
        for (int n = 0; n < numPoints; n++) {
            QVector3D p = pList.at(n);
            QVector3D q = qList.at(n);

            N(0, 0) += p.x() * q.x();
            N(0, 1) += p.x() * q.y();
            N(0, 2) += p.x() * q.z();
            N(1, 0) += p.y() * q.x();
            N(1, 1) += p.y() * q.y();
            N(1, 2) += p.y() * q.z();
            N(2, 0) += p.z() * q.x();
            N(2, 1) += p.z() * q.y();
            N(2, 2) += p.z() * q.z();
        }

        Eigen::JacobiSVD<Eigen::Matrix3d> svd(N, Eigen::ComputeFullU | Eigen::ComputeFullV);
        Eigen::Matrix3d K = Eigen::Matrix3d::Identity();
        K(2, 2) = (svd.matrixU() * svd.matrixV().transpose()).determinant();

        Eigen::Matrix3d R = svd.matrixV() * K * svd.matrixU().transpose();

        Eigen::Vector3d qVec(qBar.x(), qBar.y(), qBar.z());
        Eigen::Vector3d pVec(pBar.x(), pBar.y(), pBar.z());
        Eigen::Vector3d T = qVec - R * pVec;

        *transform = QMatrix4x4(R(0, 0), R(0, 1), R(0, 2), T(0),
                                R(1, 0), R(1, 1), R(1, 2), T(1),
                                R(2, 0), R(2, 1), R(2, 2), T(2),
                                0.0000, 0.0000, 0.0000, 1.0000);

        // Calculate alignment error
        error = 0.0f;
        for (int n = 0; n < numPoints; n++) {
            QVector3D p = pList.at(n) + pBar;
            QVector3D q = qList.at(n) + qBar;

            float x = R(0, 0) * p.x() + R(0, 1) * p.y() + R(0, 2) * p.z() + T(0) - q.x();
            float y = R(1, 0) * p.x() + R(1, 1) * p.y() + R(1, 2) * p.z() + T(1) - q.y();
            float z = R(2, 0) * p.x() + R(2, 1) * p.y() + R(2, 2) * p.z() + T(2) - q.z();

            error += sqrt(x * x + y * y + z * z);
        }
    }
    return (error);
}
