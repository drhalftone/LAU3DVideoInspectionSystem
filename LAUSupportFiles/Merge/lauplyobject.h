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

#ifndef LAUPLYOBJECT_H
#define LAUPLYOBJECT_H

#include <QDebug>
#include <QSharedData>
#include <QSharedDataPointer>

#include "lauscan.h"

#ifdef ENABLEPOINTCLOUDLIBRARY
#include <chrono>
#include <pcl/features/normal_3d_omp.h>
#include <skwlibpclcontainer.h>
#include <skwlibpclviewer.h>
#include <skwlibalignwithdownicp.h>
#include <skwlibalignwithoverlaps.h>
#include <skwlibmergenclouds.h>
#define BIG_ENOUGH_NUMBER 10328064 // 656 * 492 * 8 * 4(sizeof(float)
typedef pcl::PointXYZRGBNormal PointT;
typedef pcl::PointCloud<PointT> CloudT;
//#define DEBUG_SWKPCL
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPlyObjectData : public QSharedData
{
public:
    LAUPlyObjectData();
    LAUPlyObjectData(const LAUPlyObjectData &other);
    LAUPlyObjectData(unsigned int vrts, unsigned int inds, unsigned int chns = 1);

    ~LAUPlyObjectData();

    static int instanceCounter;

    float *vertices;
    unsigned int *indices;
    unsigned int numVrts;
    unsigned int numInds;
    unsigned int numChns;
    unsigned long long numVerticeBytesTotal;
    unsigned long long numIndiceBytesTotal;

    void allocateBuffer();
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPlyObject
{
public:
    LAUPlyObject() : data(new LAUPlyObjectData()), xMin(0.0f), xMax(0.0f), yMin(0.0f), yMax(0.0f), zMin(0.0f), zMax(0.0f) { ; }
    LAUPlyObject(unsigned int vrts, unsigned int inds, unsigned int chns = 1) : xMin(0.0f), xMax(0.0f), yMin(0.0f), yMax(0.0f), zMin(0.0f), zMax(0.0f)
    {
        data = new LAUPlyObjectData(vrts, inds, chns);
        updateLimits();
    }
    LAUPlyObject(const LAUPlyObject &other) : data(other.data), xMin(other.xMin), xMax(other.xMax), yMin(other.yMin), yMax(other.yMax), zMin(other.zMin), zMax(other.zMax) { ; }


    LAUPlyObject &operator = (const LAUPlyObject &other)
    {
        if (this != &other) {
            data = other.data;
            xMin = other.xMin;
            xMax = other.xMax;
            yMin = other.yMin;
            yMax = other.yMax;
            zMin = other.zMin;
            zMax = other.zMax;
        }
        return (*this);
    }

    LAUPlyObject(QString filename);
    LAUPlyObject(QList<LAUScan> scans, bool flag = false);

public:
    bool save(QString filename = QString());

    ~LAUPlyObject() { ; }

    bool isNull() const
    {
        return (data->vertices == NULL);
    }
    bool isValid() const
    {
        return (data->vertices != NULL);
    }

    unsigned long long verticeLength() const
    {
        return (data->numVerticeBytesTotal);
    }
    unsigned long long indiceLength() const
    {
        return (data->numIndiceBytesTotal);
    }

    unsigned int vertices() const
    {
        return (data->numVrts);
    }
    unsigned int indices() const
    {
        return (data->numInds);
    }
    unsigned int channels() const
    {
        return (data->numChns);
    }

    float *vertex()
    {
        return (data->vertices);
    }
    float *constVertex() const
    {
        return (data->vertices);
    }
    unsigned int *index()
    {
        return (data->indices);
    }
    unsigned int *constIndex() const
    {
        return (data->indices);
    }

    void updateLimits();

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

private:
    QSharedDataPointer<LAUPlyObjectData> data;
    float xMin, xMax, yMin, yMax, zMin, zMax;

#ifdef ENABLEPOINTCLOUDLIBRARY
    CloudT convert_LAUScan_to_PointXYZRGBA(LAUScan scan);

    struct PARAMETERS {
        // BASIC
        unsigned numscans;
        double nominal_resolution;
        double factor_normal_estimation_radius;

        // ICP WITH DOWNSAMPLED
        float factor_downsampling;

        float kfpcs_factor_downsampling;
        float kfpcs_rule_max_translation;
        float kfpcs_rule_max_rotation_angle;
        float kfpcs_rule_factor_max_corr;
        float kfpcs_rule_ratio_corr_size;
        float kfpcs_rule_ratio_success_score;
        float kfpcs_ratio_overlap;
        float kfpcs_delta;
        float kfpcs_abort_score;

        int     preicp_max_iteration;
        double  preicp_factor_translation_threshold;
        double  preicp_mse_threshold_relative;
        double  preicp_factor_mse_threshold_absolute;
        int     preicp_similar_transform_max_iteration;
        double  preicp_factor_median_rejector;

        int     mainicp_max_iteration;
        double  mainicp_factor_translation_threshold;
        double  mainicp_mse_threshold_relative;
        double  mainicp_factor_mse_threshold_absolute;
        int     mainicp_similar_transform_max_iteration;

        // ICP WITH OVERLAP
        int overlapicp_basic_min_size_of_roa;

        double overlapicp_factor_roa_max_distance;
        double overlapicp_factor_roa_expand;

        int     overlapicp_max_iteration;
        double  overlapicp_factor_threshold_translation;
        double  overlapicp_threshold_rotation_angle_degree;
        double  overlapicp_threshold_mse_relative;
        int     overlapicp_similar_transform_max_iteration_relativeFitness;
        int     overlapicp_similar_transform_max_iteration_distance_rotation;

        // MERGING
        float   merge_factor_distance4OverlapSeed;
        float   merge_factor_ratio4IsoptRemoval;
        int     merge_numItr4isolatedPtRemoval;
    } m_params;

    Eigen::Matrix4f align_TwoClouds(const CloudT tgtcld, const CloudT srcdld);

    std::chrono::time_point<std::chrono::high_resolution_clock> m_begin_chrono;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_end_chrono;
    float m_duration_loadclouds;
    float m_duration_normalestimation;
    float m_duration_align;
    float m_duration_merge;

    skw::visualizer::skwlibPCLviewer<PointT> m_viewer;
#endif
};
#endif // LAUPLYOBJECT_H
