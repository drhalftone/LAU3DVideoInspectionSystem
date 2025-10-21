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

#include "lauplyobject.h"

#include <QSettings>
#include <QLineEdit>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QApplication>
#include <QProgressDialog>

int LAUPlyObjectData::instanceCounter = 0;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUPlyObjectData::LAUPlyObjectData()
{
    numVrts = 0;
    numInds = 0;
    numChns = 0;

    numVerticeBytesTotal = 0;
    numIndiceBytesTotal = 0;

    vertices = NULL;
    indices = NULL;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUPlyObjectData::LAUPlyObjectData(unsigned int vrts, unsigned int inds, unsigned int chns)
{
    numVrts = vrts;
    numInds = inds;
    numChns = chns;

    numVerticeBytesTotal = 0;
    numIndiceBytesTotal = 0;

    vertices = NULL;
    indices = NULL;

    allocateBuffer();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUPlyObjectData::LAUPlyObjectData(const LAUPlyObjectData &other)
{
    // COPY OVER THE SIZE PARAMETERS FROM THE SUPPLIED OBJECT
    numVrts = other.numVrts;
    numInds = other.numInds;
    numChns = other.numChns;

    // SET THESE VARIABLES TO ZERO AND LET THEM BE MODIFIED IN THE ALLOCATION METHOD
    vertices = NULL;
    indices = NULL;

    numVerticeBytesTotal = 0;
    numIndiceBytesTotal = 0;

    allocateBuffer();

    if (vertices) {
        memcpy(vertices, other.vertices, numVerticeBytesTotal);
    }
    if (indices) {
        memcpy(indices, other.indices, numIndiceBytesTotal);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUPlyObjectData::~LAUPlyObjectData()
{
    if (vertices) {
        qDebug() << QString("LAUPlyObjectData::~LAUPlyObjectData() %1").arg(--instanceCounter) << numVrts << numInds << numChns << numVerticeBytesTotal << numIndiceBytesTotal;
    }
    if (vertices) {
        _mm_free(vertices);
    }
    if (indices) {
        _mm_free(indices);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUPlyObjectData::allocateBuffer()
{
    // ALLOCATE SPACE FOR HOLDING PIXEL DATA BASED ON NUMBER OF CHANNELS AND BYTES PER PIXEL
    numVerticeBytesTotal  = (unsigned long long)numVrts;
    numVerticeBytesTotal *= (unsigned long long)numChns;
    numVerticeBytesTotal *= (unsigned long long)sizeof(float);

    numIndiceBytesTotal  = (unsigned long long)numInds;
    numIndiceBytesTotal *= (unsigned long long)sizeof(unsigned int);

    if (numVerticeBytesTotal > 0) {
        qDebug() << QString("LAUPlyObjectData::allocateBuffer() %1").arg(instanceCounter++) << numVrts << numInds << numChns << numVerticeBytesTotal << numIndiceBytesTotal;

        vertices = (float *)_mm_malloc(numVerticeBytesTotal + 128, 16);
        if (vertices == NULL) {
            qDebug() << QString("LAUPlyObjectData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
            qDebug() << QString("LAUPlyObjectData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
            qDebug() << QString("LAUPlyObjectData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
        }
    }

    if (numIndiceBytesTotal > 0) {
        indices = (unsigned int *)_mm_malloc(numIndiceBytesTotal + 128, 16);
        if (indices == NULL) {
            qDebug() << QString("LAUPlyObjectData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
            qDebug() << QString("LAUPlyObjectData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
            qDebug() << QString("LAUPlyObjectData::allocateBuffer() MAJOR ERROR DID NOT ALLOCATE SPACE!!!");
        }
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUPlyObject::save(QString filename)
{
    if (filename.isEmpty()) {
        QSettings settings;
        QString directory = settings.value(QString("LAUScan::lastUsedDirectory"), QString()).toString();
        filename = QFileDialog::getSaveFileName(0, QString("Save PLY file to disk..."), directory, QString("*.ply"));
        if (!filename.isNull()) {
            settings.setValue(QString("LAUScan::lastSaveDirectory"), QFileInfo(filename).absolutePath());
        } else {
            return (false);
        }
    }
    return (true);

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        ;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUPlyObject::updateLimits()
{
    __m128 minVec = _mm_set1_ps(1e9f);
    __m128 maxVec = _mm_set1_ps(-1e9f);
    __m128 menVec = _mm_set1_ps(0.0f);

    int pixelCount = 0;
    for (unsigned int ind = 0; ind < vertices(); ind++) {
        __m128 pixVec = _mm_load_ps(constVertex() + 8 * ind);
        __m128 mskVec = _mm_cmpeq_ps(pixVec, pixVec);
        if (_mm_test_all_ones(_mm_castps_si128(mskVec))) {
            pixelCount++;
            minVec = _mm_min_ps(minVec, pixVec);
            maxVec = _mm_max_ps(maxVec, pixVec);
            menVec = _mm_add_ps(menVec, pixVec);
        }
    }

    *(int *)&xMin = _mm_extract_ps(minVec, 0);
    *(int *)&xMax = _mm_extract_ps(maxVec, 0);
    *(int *)&yMin = _mm_extract_ps(minVec, 1);
    *(int *)&yMax = _mm_extract_ps(maxVec, 1);
    *(int *)&zMin = _mm_extract_ps(minVec, 2);
    *(int *)&zMax = _mm_extract_ps(maxVec, 2);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUPlyObject::LAUPlyObject(QString filename) : data(new LAUPlyObjectData()), xMin(0.0f), xMax(0.0f), yMin(0.0f), yMax(0.0f), zMin(0.0f), zMax(0.0f)
{
    if (filename.isEmpty()) {
        QSettings settings;
        QString directory = settings.value(QString("LAUScan::lastUsedDirectory"), QString()).toString();
        filename = QFileDialog::getOpenFileName(0, QString("Load PLY file from disk..."), directory, QString("*.ply"));
        if (!filename.isNull()) {
            settings.setValue(QString("LAUScan::lastSaveDirectory"), QFileInfo(filename).absolutePath());
        } else {
            return;
        }
    }

    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        int numChns = 0;
        while (file.atEnd() == false) {
            QString string = QString(file.readLine());
            if (string.contains(QString("element vertex"))) {
                bool okay;
                data->numVrts = string.split(" ").last().toInt(&okay);
                if (okay == false) {
                    data->numVrts = 0;
                    break;
                }
            } else if (string.contains(QString("element face"))) {
                bool okay;
                data->numInds = 3 * string.split(" ").last().toInt(&okay);
                if (okay == false) {
                    data->numInds = 0;
                    break;
                }
            } else if (string.contains(QString("property list"))) {
                ;
            } else if (string.contains(QString("property "))) {
                numChns++;
            } else if (string.contains(QString("end_header"))) {
                break;
            }
        }

        int maxLineCount = data->numVrts + data->numInds / 3;
        int lineCount = 0;
        QProgressDialog dialog(QString("Loading PLY file..."), QString(), 0, maxLineCount, NULL, Qt::Sheet);
        dialog.show();

        // ALLOCATE SPACE FOR THE VERTICES AND READ THE VERTICES IN FROM DISK
        data->numChns = 8;
        data->allocateBuffer();

        if (numChns == 8) {
            unsigned int index = 0;
            while (index < data->numVrts) {
                QStringList strings = QString(file.readLine()).simplified().split(" ");
                if (strings.count() == (int)numChns) {
                    data->vertices[8 * index + 0] = strings.at(0).toFloat();
                    data->vertices[8 * index + 1] = strings.at(1).toFloat();
                    data->vertices[8 * index + 2] = strings.at(2).toFloat();
                    data->vertices[8 * index + 3] = strings.at(3).toFloat();
                    data->vertices[8 * index + 4] = strings.at(4).toFloat() / 255.0f;
                    data->vertices[8 * index + 5] = strings.at(5).toFloat() / 255.0f;
                    data->vertices[8 * index + 6] = strings.at(6).toFloat() / 255.0f;
                    data->vertices[8 * index + 7] = strings.at(7).toFloat() / 255.0f;

                    index++;
                }
                lineCount++;
                if (lineCount % 100000 == 0) {
                    dialog.setValue(lineCount);
                    qApp->processEvents();
                }
            }
        } else if (numChns == 6) {
            unsigned int index = 0;
            while (index < data->numVrts) {
                QStringList strings = QString(file.readLine()).simplified().split(" ");
                if (strings.count() == (int)numChns) {
                    data->vertices[8 * index + 0] = strings.at(0).toFloat();
                    data->vertices[8 * index + 1] = strings.at(1).toFloat();
                    data->vertices[8 * index + 2] = strings.at(2).toFloat();
                    data->vertices[8 * index + 3] = 1.0f;
                    data->vertices[8 * index + 4] = strings.at(3).toFloat() / 255.0f;
                    data->vertices[8 * index + 5] = strings.at(4).toFloat() / 255.0f;
                    data->vertices[8 * index + 6] = strings.at(5).toFloat() / 255.0f;
                    data->vertices[8 * index + 7] = 1.0f;

                    index++;
                }
                lineCount++;
                if (lineCount % 100000 == 0) {
                    dialog.setValue(lineCount);
                    qApp->processEvents();
                }
            }
        } else if (numChns == 7) {
            unsigned int index = 0;
            while (index < data->numVrts) {
                QStringList strings = QString(file.readLine()).simplified().split(" ");
                if (strings.count() == (int)numChns) {
                    data->vertices[8 * index + 0] = strings.at(0).toFloat();
                    data->vertices[8 * index + 1] = strings.at(1).toFloat();
                    data->vertices[8 * index + 2] = strings.at(2).toFloat();
                    data->vertices[8 * index + 3] = 1.0f;
                    data->vertices[8 * index + 4] = strings.at(3).toFloat() / 255.0f;
                    data->vertices[8 * index + 5] = strings.at(4).toFloat() / 255.0f;
                    data->vertices[8 * index + 6] = strings.at(5).toFloat() / 255.0f;
                    data->vertices[8 * index + 7] = strings.at(6).toFloat() / 255.0f;

                    index++;
                }
                lineCount++;
                if (lineCount % 100000 == 0) {
                    dialog.setValue(lineCount);
                    qApp->processEvents();
                }
            }
        } else if (numChns == 3) {
            unsigned int index = 0;
            while (index < data->numVrts) {
                QStringList strings = QString(file.readLine()).simplified().split(" ");
                if (strings.count() == (int)numChns) {
                    data->vertices[8 * index + 0] = strings.at(0).toFloat();
                    data->vertices[8 * index + 1] = strings.at(1).toFloat();
                    data->vertices[8 * index + 2] = strings.at(2).toFloat();
                    data->vertices[8 * index + 3] = 1.0f;
                    data->vertices[8 * index + 4] = 0.0f;
                    data->vertices[8 * index + 5] = 0.0f;
                    data->vertices[8 * index + 6] = 0.0f;
                    data->vertices[8 * index + 7] = 1.0f;

                    index++;
                }
                lineCount++;
                if (lineCount % 100000 == 0) {
                    dialog.setValue(lineCount);
                    qApp->processEvents();
                }
            }
        } else if (numChns == 4) {
            unsigned int index = 0;
            while (index < data->numVrts) {
                QStringList strings = QString(file.readLine()).simplified().split(" ");
                if (strings.count() == (int)numChns) {
                    data->vertices[8 * index + 0] = strings.at(0).toFloat();
                    data->vertices[8 * index + 1] = strings.at(1).toFloat();
                    data->vertices[8 * index + 2] = strings.at(2).toFloat();
                    data->vertices[8 * index + 3] = 1.0f;
                    data->vertices[8 * index + 4] = strings.at(3).toFloat() / 255.0f;
                    data->vertices[8 * index + 5] = data->vertices[8 * index + 4];
                    data->vertices[8 * index + 6] = data->vertices[8 * index + 4];
                    data->vertices[8 * index + 7] = 1.0f;

                    index++;
                }
                lineCount++;
                if (lineCount % 100000 == 0) {
                    dialog.setValue(lineCount);
                    qApp->processEvents();
                }
            }
        }

        unsigned int index = 0;
        while (index < data->numInds) {
            QStringList strings = QString(file.readLine()).simplified().split(" ");
            if (strings.count() == 4) {
                data->indices[index++] = strings.at(1).toInt();
                data->indices[index++] = strings.at(2).toInt();
                data->indices[index++] = strings.at(3).toInt();
            }
            lineCount++;
            if (lineCount % 100000 == 0) {
                dialog.setValue(lineCount);
                qApp->processEvents();
            }
        }
        qDebug() << file.pos() << file.size();

        // CLOSE THE FILE
        file.close();

        // UPDATE THE RANGE LIMITS
        updateLimits();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUPlyObject::LAUPlyObject(QList<LAUScan> scans, bool flag) : data(new LAUPlyObjectData()), xMin(0.0f), xMax(0.0f), yMin(0.0f), yMax(0.0f), zMin(0.0f), zMax(0.0f)
{
    // scans must contain at least one
    if (scans.size() < 2) {
        qWarning() << "two small scans... " << scans.size();
        return;
    }

    // ply save : folder & header
    QString plyfheader;
    if (plyfheader.isEmpty()) {
        QSettings settings;
        QString directory = settings.value(QString("LAUScan::lastUsedDirectory"), QString()).toString();

        QString plySaveFolderPath = QFileDialog::getExistingDirectory(0, QString("Select Foler to Save PLYs.."), directory);
        qWarning() << "plySaveFolderPath" << plySaveFolderPath;

        QString plySaveFileHeader = QInputDialog::getText(0, QString("Header for PLYs"), QString("Type Header"), QLineEdit::Normal);

        plyfheader = plySaveFolderPath + QString("/") + plySaveFileHeader;
    }

#ifdef ENABLEPOINTCLOUDLIBRARY
    /*
     * convert to skwlibPCLcontainer..
     */
    skw::skwlibPCLcontainer<PointT> skwContainer;
    {
        qWarning() << "[convert LAU scans into PCL scans]...";
#ifdef DEBUG_SWKPCL
        qWarning() << "\t Size of lauscanList : " << scans.size();
#endif
        // duration : begin
        m_begin_chrono = std::chrono::high_resolution_clock::now();

        // convert
        int idx = 0;
        CloudT tmpCloud;
        while (idx < scans.size()) {
            // GRAB A COPY OF THE CURRENT SCAN
            LAUScan scan = scans.at(idx);

            tmpCloud = convert_LAUScan_to_PointXYZRGBA(scan);
            skwContainer.addCloud(tmpCloud);

            idx++;
        }

        // save as ply
        skwContainer.saveAllClouds(plyfheader.toStdString(), true);

        // duration : end
        m_end_chrono = std::chrono::high_resolution_clock::now();
        unsigned duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_end_chrono - m_begin_chrono).count();
        m_duration_loadclouds = (double)duration / 1000.;
        qWarning() << "\t[Duration] convert LAU->PCL : " << m_duration_loadclouds << " (sec)";

#ifdef DEBUG_SWKPCL
        m_viewer.showMultipleClouds(skwContainer.getCloudsVectorInContainer(), "input clouds", true, false);
#endif
    }

    /*
     * Settings for Dental
     */
    // Basics
    m_params.numscans = skwContainer.getNumberOfClouds();
    m_params.nominal_resolution = 0.03;
    m_params.factor_normal_estimation_radius = 4.;

    // ICP with Downsampled
    m_params.factor_downsampling = 5.;

    m_params.kfpcs_factor_downsampling = 20.;
    m_params.kfpcs_rule_max_translation = 100.;
    m_params.kfpcs_rule_max_rotation_angle = 50.;
    m_params.kfpcs_rule_factor_max_corr = 20.;
    m_params.kfpcs_rule_ratio_corr_size = 1.05;
    m_params.kfpcs_rule_ratio_success_score = 0.95;
    m_params.kfpcs_ratio_overlap = 0.85;
    m_params.kfpcs_delta = 0.05;
    m_params.kfpcs_abort_score = 0.;

    m_params.preicp_max_iteration = 20;
    m_params.preicp_factor_translation_threshold = 0.1;
    m_params.preicp_mse_threshold_relative = 0.001;
    m_params.preicp_factor_mse_threshold_absolute = 0.1;
    m_params.preicp_similar_transform_max_iteration = 2;
    m_params.preicp_factor_median_rejector = 2.;

    m_params.mainicp_max_iteration = 100;
    m_params.mainicp_factor_translation_threshold = 0.1;
    m_params.mainicp_mse_threshold_relative = 1e-4;
    m_params.mainicp_factor_mse_threshold_absolute = 0.1;
    m_params.mainicp_similar_transform_max_iteration = 3;

    // ICP with Overlaps
    m_params.overlapicp_basic_min_size_of_roa = 200;

    m_params.overlapicp_factor_roa_max_distance = 3.;
    m_params.overlapicp_factor_roa_expand = 2.;

    m_params.overlapicp_max_iteration = 50;
    m_params.overlapicp_factor_threshold_translation = 0.5;
    m_params.overlapicp_threshold_rotation_angle_degree = 1.;
    m_params.overlapicp_threshold_mse_relative = 0.001;
    m_params.overlapicp_similar_transform_max_iteration_relativeFitness = 3;
    m_params.overlapicp_similar_transform_max_iteration_distance_rotation = 3;

    // Merge
    m_params.merge_factor_distance4OverlapSeed = 10.;
    m_params.merge_factor_ratio4IsoptRemoval = 1.;
    m_params.merge_numItr4isolatedPtRemoval = 1;


    /*
     * Normal Computation
     */
    pcl::NormalEstimationOMP<PointT, PointT> normest;
    {
        qWarning() << "[computing Normals of all scans]...";

        // duration : begin
        m_begin_chrono = std::chrono::high_resolution_clock::now();

        CloudT::Ptr tmpCloudPtr(new CloudT);
        double normrad = m_params.factor_normal_estimation_radius * m_params.nominal_resolution;
        int numthread = omp_get_max_threads();

        for (int i = 0; i < m_params.numscans; i++) {
            *tmpCloudPtr = skwContainer.getOneCloudInContainer(i);

            pcl::search::KdTree<PointT>::Ptr kdtreeptr(new pcl::search::KdTree<PointT>());

            normest.setSearchMethod(kdtreeptr);

            normest.setNumberOfThreads(numthread);
            normest.setRadiusSearch(normrad);
            normest.setViewPoint(0, 0, 0);

            normest.setInputCloud(tmpCloudPtr);
            normest.compute(*tmpCloudPtr);

            skwContainer.updateCloud(i, tmpCloudPtr);
#ifdef DEBUG_SWKPCL
            //            m_viewer.showOneCloud_Normal(tmpCloudPtr,"normal:"+std::to_string(i)+"/"+std::to_string(m_params.numscans-1),true,false,50,0.7);
#endif
        }

        // duration : end
        m_end_chrono = std::chrono::high_resolution_clock::now();
        unsigned duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_end_chrono - m_begin_chrono).count();
        m_duration_normalestimation = (double)duration / 1000.;
        qWarning() << "\t Duration : " << m_duration_normalestimation << " (sec)";

    }

    /*
     * Align :
     *          ICPwithDown [ KFPCS + PreICP (Uniform + Rejector) + MainICP ]
     *          ICPwithOverlap [OVERLAPING_ONLY, NEAREST, POINT2PLANE]
     */
    skw::skwlibPCLcontainer<PointT> skwAlignedContainer;
    std::vector<Eigen::Matrix4f> matrix4fVector;
    {
        qWarning() << "[Pairwise, Sequential Alignment]...";

        // duration : begin
        m_begin_chrono = std::chrono::high_resolution_clock::now();

        // initialization
        CloudT::Ptr targetCloudPtr(new CloudT);
        CloudT::Ptr sourceCloudPtr(new CloudT);
        CloudT::Ptr aligndCloudPtr(new CloudT);

        skwAlignedContainer.resizeContainer(m_params.numscans);
        {
            skwAlignedContainer.updateCloud(0, skwContainer.getOneCloudInContainer(0));
        }
        matrix4fVector.resize(m_params.numscans);
        {
            matrix4fVector[0] = Eigen::Matrix4f::Identity();
        }

        // align each pair
        for (int idx = 1; idx < m_params.numscans; idx++) {
            qWarning() << "-- Pair (Target/Source/Total#) : " << idx - 1 << " / " << idx << " / " << m_params.numscans - 1;

            *targetCloudPtr = skwContainer.getOneCloudInContainer(idx - 1);
            *sourceCloudPtr = skwContainer.getOneCloudInContainer(idx);

            matrix4fVector[idx] = align_TwoClouds(*targetCloudPtr, *sourceCloudPtr);

#ifdef DEBUG_SWKPCL
            std::string title = "result (" + std::to_string(idx - 1) + "/" + std::to_string(idx) + "/" + std::to_string(m_params.numscans - 1) + ")";
            m_viewer.showTargetAlignedWithTransform(targetCloudPtr, sourceCloudPtr, matrix4fVector[idx], title, true, false);
#endif

        }

        // accumulate Transform & update skwAlignedContainer
        Eigen::Matrix4f accuTmat = Eigen::Matrix4f::Identity();
        for (int idx = 1; idx < m_params.numscans; idx++) {
            accuTmat = matrix4fVector[idx] * accuTmat;
            pcl::transformPointCloudWithNormals<PointT>(skwContainer.getOneCloudInContainer(idx), *aligndCloudPtr, accuTmat);
            skwAlignedContainer.updateCloud(idx, *aligndCloudPtr);
        }

        // duration : end
        m_end_chrono = std::chrono::high_resolution_clock::now();
        unsigned duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_end_chrono - m_begin_chrono).count();
        m_duration_align = (double)duration / 1000.;
        qWarning() << "\t Duration : " << m_duration_align << " (sec)";

#ifdef DEBUG_SWKPCL
        m_viewer.showMultipleClouds(skwContainer.getCloudsVectorInContainer(), "all input Clouds", true, false);
        m_viewer.showMultipleClouds(skwAlignedContainer.getCloudsVectorInContainer(), "all aligned Clouds", true, false);
#endif
    }


    /*
     * Merge all
     */
    skw::merge::skwlibMergeNClouds<PointT> merging;
    CloudT::Ptr allMergedPtr(new CloudT);
    {
        qWarning() << "[Merging aligned clouds]...";

        // duration : begin
        m_begin_chrono = std::chrono::high_resolution_clock::now();

        // initialization
        skw::merge::skwlibMergeNClouds<PointT>::PARAMETERS pm;
        {
            pm.nominal_resolution = m_params.nominal_resolution;
            pm.factor_distance4OverlapSeed = m_params.merge_factor_distance4OverlapSeed;
            pm.factor_ratio4IsoptRemoval = m_params.merge_factor_ratio4IsoptRemoval;
            pm.numItr4isolatedPtRemoval = m_params.merge_numItr4isolatedPtRemoval;
            pm.numThreads = omp_get_max_threads();
        }

        // set values
        merging.setInputCloudsVec(skwAlignedContainer.getCloudsVectorInContainer());
        merging.setParameters(pm);

#ifdef DEBUG_SWKPCL
        double down_merge = 3. * m_params.nominal_resolution;
        merging.doMerging_simpleUniformSampling(down_merge);
#else
        //        merging.doMergingAllOMP();
        double down_merge = 3. * m_params.nominal_resolution;
        merging.doMerging_simpleUniformSampling(down_merge);
#endif

        // get result
        *allMergedPtr = merging.getFinalMergedCloud();

        // duration : end
        m_end_chrono = std::chrono::high_resolution_clock::now();
        unsigned duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_end_chrono - m_begin_chrono).count();
        m_duration_merge = (double)duration / 1000.;
        qWarning() << "\t Duration : " << m_duration_merge << " (sec)";
    }


    /*
     * Save merged
     */
    pcl::io::savePLYFileBinary(plyfheader.toStdString() + "_mergedFinal.ply", *allMergedPtr);

    m_viewer.showOneCloud(allMergedPtr, "Merged Result", true, true);
#endif
}

#ifdef ENABLEPOINTCLOUDLIBRARY
CloudT LAUPlyObject::convert_LAUScan_to_PointXYZRGBA(LAUScan scan)
{
    float *toBuffer = (float *)_mm_malloc(BIG_ENOUGH_NUMBER, 16);
    int sizeBuffer = 0;

    // All the transforms are Identity -> Needless.
    // APPLY THE SCANS INTERNAL TRANSFORM TO GET WORLD COORDINATES
    //    bool flag_tansform;
    //    if( flag_transform == true )
    //    {
    //        qWarning() << "scan transform \n" << scan.transform();
    //        if (scan.transform() != QMatrix4x4()){
    //            scan = scan.transformScan(scan.transform());
    //        }
    //    }

    // CONVERT THE SCAN TO XYZWRGBA FORMAT, IF NOT ALREADY IN THAT FORMAT
    if (scan.color() != ColorXYZWRGBA) {
        scan = scan.convertToColor(ColorXYZWRGBA);
    }


    // modified with Dr. Lau's comments on May 28
    for (unsigned row = 0; row < scan.height(); row++) {
        float *inBuffer = (float *)scan.constScanLine(row);

        for (unsigned col = 0; col < scan.width(); col++) {
            // GRAB COPY OF CURRENT PIXEL'S XYZW COORDINATES
            __m128 vecIn = _mm_load_ps(inBuffer + 8 * col);

            // TEST TO SEE IF ANY ELEMENTS IN THE INCOMING VECTOR ARE NANS
            // AND IF NOT THEN COPY THE VALID PIXEL INTO THE OUTPUT BUFFER
            if (_mm_test_all_ones(_mm_castps_si128(_mm_cmpeq_ps(vecIn, vecIn)))) {
                _mm_store_ps(toBuffer + 8 * sizeBuffer + 0, vecIn);
                _mm_store_ps(toBuffer + 8 * sizeBuffer + 4, _mm_load_ps(inBuffer + 8 * col + 4));

                sizeBuffer++;
            }
        }
    }

    // convert toBuffer to PCL data
    pcl::PointCloud<pcl::PointXYZRGBA> cloudGCtmp;
    cloudGCtmp.resize(sizeBuffer);
    memcpy(&cloudGCtmp.points[0], toBuffer, sizeBuffer * sizeof(pcl::PointXYZRGBA));
    {
        // Correct RGB mismatch
        int aryIdx;
        for (int i = 0; i < sizeBuffer; i++) {
            aryIdx = i * 8 + 4;

            float grey = toBuffer[aryIdx] * 255;
            if (grey < 0.) {
                grey = 0.;
            }
            if (grey > 255.) {
                grey = 255;
            }

            cloudGCtmp.points[i].r = cloudGCtmp.points[i].g = cloudGCtmp.points[i].b = grey;
        }
    }

    _mm_free(toBuffer);

    CloudT cloudDefault;
    pcl::copyPointCloud<pcl::PointXYZRGBA, PointT>(cloudGCtmp, cloudDefault);

    // return
    return cloudDefault;
}

Eigen::Matrix4f LAUPlyObject::align_TwoClouds(const CloudT tgtcld, const CloudT srcdld)
{
    // initialization
    Eigen::Matrix4f accumulatedTmatrix = Eigen::Matrix4f::Identity();
    CloudT::Ptr tgtcldptr(new CloudT);
    CloudT::Ptr srccldptr(new CloudT);

    // set inputs
    *tgtcldptr = tgtcld;
    *srccldptr = srcdld;
#ifdef DEBUG_SWKPCL
    m_viewer.showTwoCloud(tgtcldptr, srccldptr, "[align_TwoClouds] target vs source", true, false);
#endif

    // align with downsampled
    skw::align::skwlibAlignWithDownICP<PointT> icpDown;
    {
        QString title = "KFPCS + preICP (Uniform + Rejector) + MainICP";
        qWarning() << "\t" << title;

        icpDown.setTarget(tgtcldptr);
        icpDown.setSource(srccldptr);

        icpDown.setNominalResolution(m_params.nominal_resolution);
        icpDown.setDownsamplingRadiusFactor(m_params.factor_downsampling);

        icpDown.setNormalComputation(false);

        icpDown.setAlign_runInitialTransformRefinement(true);
        icpDown.setParameters_KFPCS(m_params.kfpcs_factor_downsampling,
                                    m_params.kfpcs_rule_max_translation,
                                    m_params.kfpcs_rule_max_rotation_angle,
                                    m_params.kfpcs_rule_factor_max_corr,
                                    m_params.kfpcs_rule_ratio_corr_size,
                                    m_params.kfpcs_rule_ratio_success_score,
                                    m_params.kfpcs_ratio_overlap,
                                    m_params.kfpcs_delta,
                                    m_params.kfpcs_abort_score);

        icpDown.setAlign_runPreICP(true,
                                   skw::align::skwlibAlignWithDownICP<PointT>::PREICP_INPUT::UNIFORM_SAMPLED,
                                   skw::align::skwlibAlignWithDownICP<PointT>::PREICP_COR_REJECTOR::ON_REJECTION);
        icpDown.setParameters_PreICP(m_params.preicp_max_iteration,
                                     m_params.preicp_factor_translation_threshold,
                                     m_params.preicp_mse_threshold_relative,
                                     m_params.preicp_factor_mse_threshold_absolute,
                                     m_params.preicp_similar_transform_max_iteration,
                                     m_params.preicp_factor_median_rejector);

        icpDown.setAlign_runMainICP(true);
        icpDown.setParameters_MainICP(m_params.mainicp_max_iteration,
                                      m_params.mainicp_factor_translation_threshold,
                                      m_params.mainicp_mse_threshold_relative,
                                      m_params.mainicp_factor_mse_threshold_absolute,
                                      m_params.mainicp_similar_transform_max_iteration);
        icpDown.execute();

        accumulatedTmatrix = icpDown.getFinalTransformationMatrix() * accumulatedTmatrix;

#ifdef DEBUG_SWKPCL
        qWarning() << icpDown.showICPstatus(title);
        m_viewer.showTargetAlignedWithTransform(tgtcldptr, srccldptr, accumulatedTmatrix, "[align_TwoClouds]" + title.toStdString(), true, false);
#endif
    }


    // update source
    pcl::transformPointCloud(*srccldptr, *srccldptr, icpDown.getFinalTransformationMatrix());
#ifdef DEBUG_SWKPCL
    m_viewer.showTwoCloud(tgtcldptr, srccldptr, "[align_TwoClouds] fineicp inputs: target vs source-updated", true, false);
#endif

    // align with overlaps
    skw::align::skwlibAlignWithOverlaps<PointT> icpOverlap;
    {
        QString title = "OVERLAPING_ONLY, NEAREST, POINT2PLANE";
        qWarning() << "\t" << title;

        icpOverlap.setNominalResolution(m_params.nominal_resolution);
        icpOverlap.setMinThresholdSize4RegionOfAlign(m_params.overlapicp_basic_min_size_of_roa);
        icpOverlap.setNormalComputation(false);

        icpOverlap.setRegionOfAlignType(
            skw::align::skwlibAlignWithOverlaps<PointT>::REGION_OF_ALIGN::OVERLAPING_ONLY,
            m_params.overlapicp_factor_roa_max_distance,
            m_params.overlapicp_factor_roa_expand);

        icpOverlap.setTransformEstimationType(
            skw::align::skwlibAlignWithOverlaps<PointT>::TRANSFORM_ESTIMATION_TYPES::POINT2PLANE);
        icpOverlap.setParameters_skwICP(
            m_params.overlapicp_max_iteration,
            m_params.overlapicp_factor_threshold_translation,
            m_params.overlapicp_threshold_rotation_angle_degree,
            m_params.overlapicp_threshold_mse_relative,
            m_params.overlapicp_similar_transform_max_iteration_relativeFitness,
            m_params.overlapicp_similar_transform_max_iteration_distance_rotation);

        icpOverlap.setTarget(tgtcldptr);
        icpOverlap.setSource(srccldptr);

        icpOverlap.execute();

        accumulatedTmatrix = icpOverlap.getFinalTransformationMatrix() * accumulatedTmatrix;
#ifdef DEBUG_SWKPCL
        qWarning() << icpOverlap.showICPstatus(title);
        m_viewer.showTargetAlignedWithTransform(tgtcldptr, srccldptr, icpOverlap.getFinalTransformationMatrix(), "[align_TwoClouds]" + title.toStdString(), true, false);
#endif
    }
    // RETURN TRANSFORM MATRIX
    return accumulatedTmatrix;
}
#endif
