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
#include "lauorbbeccamera.h"
#include "lauconstants.h"
#include <cmath>  // For fabs function
#include <algorithm>  // For std::sort
#include <QMap>  // For filtering profiles by resolution
#include <QSettings>  // For remembering resolution selections

#ifdef ENABLECASCADE
#include "opencv2/imgproc/imgproc.hpp"
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUOrbbecCamera_camerasLessThan(const LAUOrbbecCamera::CameraPacket &s1, const LAUOrbbecCamera::CameraPacket &s2)
{
    return (s1.serialString < s2.serialString);
}

#ifndef Q_OS_MAC
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUOrbbecCamera::processError(ob_error **err)
{
    emit emitError(errorString);
    if (err){
        ob_delete_error(*err);
        *err = NULL;
    }
}
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUOrbbecCamera::LAUOrbbecCamera(LAUVideoPlaybackColor color, QObject *parent) : LAU3DCamera(color, parent), numDepthRows(0), numDepthCols(0), numColorRows(0), numColorCols(0), hasMappingVideo(false), context(NULL), major_version(-1), minor_version(-1), patch_version(-1)
{
    initialize();
    errorString = errorString;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUOrbbecCamera::LAUOrbbecCamera(QObject *parent) : LAU3DCamera(ColorXYZRGB, parent), numDepthRows(0), numDepthCols(0), numColorRows(0), numColorCols(0), hasMappingVideo(false), context(NULL), major_version(-1), minor_version(-1), patch_version(-1)
{
    initialize();
    errorString = errorString;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<LAUVideoPlaybackColor> LAUOrbbecCamera::playbackColors()
{
    QList<LAUVideoPlaybackColor> list;
    list << ColorGray << ColorXYZ << ColorXYZG;
    return (list);
}

#ifdef ORBBEC_USE_RESOLUTION_DIALOG
#ifndef Q_OS_MAC
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<LAUOrbbecCamera::StreamProfileInfo> LAUOrbbecCamera::getAvailableDepthProfiles(ob_sensor *sensor)
{
    QList<StreamProfileInfo> allProfiles;
    StreamProfileInfo baseProfile640x576;
    baseProfile640x576.profile = nullptr;
    ob_error *error = NULL;

    ob_stream_profile_list *profileList = ob_sensor_get_stream_profile_list(sensor, &error);
    if (error) {
        qDebug() << "Error getting depth sensor profile list:" << error->message;
        ob_delete_error(error);
        return allProfiles;
    }

    uint32_t count = ob_stream_profile_list_count(profileList, &error);
    if (error) {
        qDebug() << "Error getting profile count:" << error->message;
        ob_delete_error(error);
        ob_delete_stream_profile_list(profileList, &error);
        return allProfiles;
    }

    // Collect all profiles
    for (uint32_t i = 0; i < count; i++) {
        ob_stream_profile *profile = ob_stream_profile_list_get_profile(profileList, i, &error);
        if (error) {
            qDebug() << "Error getting profile at index" << i << ":" << error->message;
            ob_delete_error(error);
            continue;
        }

        StreamProfileInfo info;
        info.width = ob_video_stream_profile_width(profile, &error);
        info.height = ob_video_stream_profile_height(profile, &error);
        info.fps = ob_video_stream_profile_fps(profile, &error);
        info.format = ob_stream_profile_format(profile, &error);
        info.profile = profile;
        
        QString formatStr;
        switch(info.format) {
            case OB_FORMAT_Y16: formatStr = "Y16"; break;
            case OB_FORMAT_Y8: formatStr = "Y8"; break;
            default: formatStr = QString("Format_%1").arg((int)info.format); break;
        }
        
        info.name = QString("%1x%2 @ %3fps (%4)")
                   .arg(info.width)
                   .arg(info.height)
                   .arg(info.fps)
                   .arg(formatStr);

        allProfiles.append(info);
        
        // Store 640x576 profile with highest fps as base for creating 640x480 pseudo profile
        if (info.width == LAU_ORBBEC_GEMINI2_NATIVE_WIDTH && info.height == LAU_ORBBEC_GEMINI2_NATIVE_HEIGHT) {
            if (baseProfile640x576.profile == nullptr || info.fps > baseProfile640x576.fps) {
                baseProfile640x576 = info;
            }
        }
    }
    
    // Filter to keep only highest fps for each resolution
    QMap<QPair<uint32_t, uint32_t>, StreamProfileInfo> resolutionMap;
    for (const auto &profile : allProfiles) {
        QPair<uint32_t, uint32_t> resolution(profile.width, profile.height);
        if (!resolutionMap.contains(resolution) || profile.fps > resolutionMap[resolution].fps) {
            resolutionMap[resolution] = profile;
        }
    }
    
    // Convert map to list
    QList<StreamProfileInfo> profiles = resolutionMap.values();
    
    // Add pseudo 640x480 profile if 640x576 exists
    if (baseProfile640x576.profile != nullptr) {
        StreamProfileInfo pseudoProfile = baseProfile640x576;
        pseudoProfile.height = LAU_ORBBEC_PSEUDO_HEIGHT;  // Pseudo height
        pseudoProfile.name = QString("640x480 @ %1fps (%2) [Pseudo]")
                           .arg(pseudoProfile.fps)
                           .arg(QString(pseudoProfile.format == OB_FORMAT_Y16 ? "Y16" : 
                                      pseudoProfile.format == OB_FORMAT_Y8 ? "Y8" : 
                                      QString("Format_%1").arg((int)pseudoProfile.format)));
        profiles.append(pseudoProfile);
    }
    
    // Sort profiles by resolution (highest first)
    std::sort(profiles.begin(), profiles.end(), [](const StreamProfileInfo &a, const StreamProfileInfo &b) {
        uint32_t resolutionA = a.width * a.height;
        uint32_t resolutionB = b.width * b.height;
        return resolutionA > resolutionB;
    });

    ob_delete_stream_profile_list(profileList, &error);
    return profiles;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<LAUOrbbecCamera::StreamProfileInfo> LAUOrbbecCamera::getAvailableColorProfiles(ob_sensor *sensor)
{
    QList<StreamProfileInfo> profiles;
    QMap<QPair<uint32_t, uint32_t>, StreamProfileInfo> resolutionMap;
    ob_error *error = NULL;

    ob_stream_profile_list *profileList = ob_sensor_get_stream_profile_list(sensor, &error);
    if (error) {
        qDebug() << "Error getting color sensor profile list:" << error->message;
        ob_delete_error(error);
        return profiles;
    }

    uint32_t count = ob_stream_profile_list_count(profileList, &error);
    if (error) {
        qDebug() << "Error getting profile count:" << error->message;
        ob_delete_error(error);
        ob_delete_stream_profile_list(profileList, &error);
        return profiles;
    }

    for (uint32_t i = 0; i < count; i++) {
        ob_stream_profile *profile = ob_stream_profile_list_get_profile(profileList, i, &error);
        if (error) {
            qDebug() << "Error getting profile at index" << i << ":" << error->message;
            ob_delete_error(error);
            continue;
        }

        StreamProfileInfo info;
        info.width = ob_video_stream_profile_width(profile, &error);
        info.height = ob_video_stream_profile_height(profile, &error);
        info.fps = ob_video_stream_profile_fps(profile, &error);
        info.format = ob_stream_profile_format(profile, &error);
        info.profile = profile;
        
        // Only include RGB format profiles
        if (info.format != OB_FORMAT_RGB) {
            continue;
        }
        
        QString formatStr = "RGB";
        
        info.name = QString("%1x%2 @ %3fps (%4)")
                   .arg(info.width)
                   .arg(info.height)
                   .arg(info.fps)
                   .arg(formatStr);

        QPair<uint32_t, uint32_t> resolution(info.width, info.height);
        
        // Keep only the highest frame rate for each resolution
        if (!resolutionMap.contains(resolution) || info.fps > resolutionMap[resolution].fps) {
            resolutionMap[resolution] = info;
        }
    }

    // Convert map back to list
    for (auto it = resolutionMap.begin(); it != resolutionMap.end(); ++it) {
        profiles.append(it.value());
    }

    ob_delete_stream_profile_list(profileList, &error);
    return profiles;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUOrbbecCamera::StreamProfileInfo LAUOrbbecCamera::selectBestColorProfile(const QList<StreamProfileInfo> &colorProfiles, uint32_t targetFps)
{
    StreamProfileInfo bestProfile;
    bestProfile.profile = nullptr;
    
    uint32_t bestResolution = 0;
    
    for (const auto &profile : colorProfiles) {
        // Only consider profiles with fps >= targetFps
        if (profile.fps >= targetFps) {
            uint32_t resolution = profile.width * profile.height;
            if (resolution > bestResolution) {
                bestResolution = resolution;
                bestProfile = profile;
            }
        }
    }
    
    return bestProfile;
}
#endif // Q_OS_MAC
#endif // ORBBEC_USE_RESOLUTION_DIALOG

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUOrbbecCamera::initialize()
{
    makeString = QString("Orbbec");
    localScaleFactor = 0.25f;
    isConnected = false;
    bitsPerPixel = 8;

#ifndef Q_OS_MAC
    // CREATE ERROR OBJECT
    ob_error *error = NULL;
#else
    bool *error = NULL;
#endif

#ifndef Q_OS_MAC
    // Get OrbbecSDK version.
    major_version = ob_get_major_version();
    minor_version = ob_get_minor_version();
    patch_version = ob_get_patch_version();
#else
    major_version = -1;
    minor_version = -1;
    patch_version = -1;
#endif

#ifndef Q_OS_MAC
    // create context
    context = ob_create_context(&error);
    if (error){
        errorString = QString("Orbbec Error creating context: %1").arg(error->message);
        processError(&error);
        return;
    }

    // GET A LIST OF DEVICES
    ob_device_list *deviceList = ob_query_device_list(context, &error);
    if (error){
        errorString = QString("Orbbec Error getting device list: %1").arg(error->message);
        processError(&error);
        return;
    }

    uint32_t numDevices = ob_device_list_device_count(deviceList, &error);
    if (error){
        errorString = QString("Orbbec Error getting number of devices: %1").arg(error->message);
        processError(&error);
        return;
    } else if (numDevices == 0){
        errorString = QString("No devices found!");
        return;
    }

    for (unsigned int dvc = 0; dvc < numDevices; dvc++){
        CameraPacket camera;
        camera.config = NULL;
        camera.device = NULL;
        camera.pipeline = NULL;
        camera.isConnected = false;
        camera.isPseudoDepthProfile = false;
        camera.isPseudoColorProfile = false;
        camera.numDepthRows = 0;
        camera.numDepthCols = 0;
        camera.numColorRows = 0;
        camera.numColorCols = 0;
        camera.deviceIntrinsics = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

        camera.device = ob_device_list_get_device(deviceList, dvc, &error);
        if (error){
            errorString = QString("Orbbec Error creating USB device: %1").arg(error->message);
            processError(&error);
            return;
        }

        ob_device_info *info = ob_device_get_device_info(camera.device, &error);
        camera.makeString = QString("Orbbec");
        camera.modelString = QString(ob_device_info_name(info, &error));
        camera.modelString.remove(QString("Orbbec "));
        camera.serialString = QString(ob_device_info_serial_number(info, &error));

        // Configure the depth stream
        ob_sensor_list *sensorList = ob_device_get_sensor_list(camera.device, &error);
        if (error){
            errorString = QString("Orbbec Error getting sensor list: %1").arg(error->message);
            processError(&error);
            return;
        }

        ob_sensor *nirSensor = NULL;
        ob_sensor *rgbSensor = NULL;
        ob_sensor *dptSensor = NULL;

        // Automatic selection based on playbackColor
        switch (playbackColor) {
        case ColorGray:
            hasColorVideo = true;
            hasDepthVideo = false;

            nirSensor = ob_sensor_list_get_sensor_by_type(sensorList, OB_SENSOR_IR, &error);
            if (error){
                errorString = QString("Orbbec Error getting NIR sensor: %1").arg(error->message);
                processError(&error);
                return;
            }

            break;
        case ColorRGB:
            hasColorVideo = true;
            hasDepthVideo = false;

            rgbSensor = ob_sensor_list_get_sensor_by_type(sensorList, OB_SENSOR_COLOR, &error);
            if (error){
                errorString = QString("Orbbec Error getting RGB sensor: %1").arg(error->message);
                processError(&error);
                return;
            }

            break;
        case ColorXYZ:
            hasColorVideo = false;
            hasDepthVideo = true;

            dptSensor = ob_sensor_list_get_sensor_by_type(sensorList, OB_SENSOR_DEPTH, &error);
            if (error){
                errorString = QString("Orbbec Error getting depth sensor: %1").arg(error->message);
                processError(&error);
                return;
            }

            break;
        case ColorXYZG:
            hasColorVideo = true;
            hasDepthVideo = true;

            // For Orbbec XYZG mode, only grab depth sensor (copy depth to color to avoid bandwidth issues)
            // Do NOT enable NIR sensor to save bandwidth - Orbbec bandwidth is limited
            dptSensor = ob_sensor_list_get_sensor_by_type(sensorList, OB_SENSOR_DEPTH, &error);
            if (error){
                errorString = QString("Orbbec Error getting depth sensor: %1").arg(error->message);
                processError(&error);
                return;
            }

            break;
        case ColorXYZRGB:
            hasColorVideo = true;
            hasDepthVideo = true;

            rgbSensor = ob_sensor_list_get_sensor_by_type(sensorList, OB_SENSOR_COLOR, &error);
            if (error){
                errorString = QString("Orbbec Error getting RGB sensor: %1").arg(error->message);
                processError(&error);
                return;
            }

            dptSensor = ob_sensor_list_get_sensor_by_type(sensorList, OB_SENSOR_DEPTH, &error);
            if (error){
                errorString = QString("Orbbec Error getting depth sensor: %1").arg(error->message);
                processError(&error);
                return;
            }

            break;
        default:
            return;
        }

        ob_delete_sensor_list(sensorList, &error);
        if (error) {
            errorString = QString("Orbbec Error deleting sensor list: %1").arg(error->message);
            processError(&error);
        }
        sensorList = NULL;

        ob_stream_profile *dptProfile = nullptr;
        ob_stream_profile *rgbProfile = nullptr;
        ob_stream_profile *nirProfile = nullptr;
        qDebug() << "Initial nirProfile:" << (void*)nirProfile << "nirSensor:" << (void*)nirSensor << "rgbSensor:" << (void*)rgbSensor << "dptSensor:" << (void*)dptSensor;

#ifdef ORBBEC_USE_RESOLUTION_DIALOG
        // Use dialog-based profile selection
        StreamProfileInfo selectedDepthProfile;
        selectedDepthProfile.profile = nullptr;
        
        if (dptSensor){
            QList<StreamProfileInfo> depthProfiles = getAvailableDepthProfiles(dptSensor);
            if (depthProfiles.isEmpty()) {
                errorString = QString("No depth profiles found!");
                return;
            }

            // Profiles are already filtered and sorted by getAvailableDepthProfiles

            // Create list of profile names for dialog
            QStringList profileNames;
            for (const auto &profile : depthProfiles) {
                profileNames << profile.name;
            }

            // Remember last selected depth profile
            QSettings settings;
            QString lastDepthProfile = settings.value("OrbbecCamera/LastDepthProfile", QString()).toString();
            int defaultIndex = 0;
            if (!lastDepthProfile.isEmpty()) {
                int index = profileNames.indexOf(lastDepthProfile);
                if (index >= 0) {
                    defaultIndex = index;
                }
            }

            bool ok;
            QString selectedName = QInputDialog::getItem(nullptr, "Depth Resolution Selection",
                                                       "Select depth resolution and frame rate:",
                                                       profileNames, defaultIndex, false, &ok);
            
            if (!ok) {
                errorString = QString("Depth profile selection cancelled");
                return;
            }

            // Find the selected profile
            for (const auto &profile : depthProfiles) {
                if (profile.name == selectedName) {
                    dptProfile = profile.profile;
                    selectedDepthProfile = profile;
                    qDebug() << "Selected depth profile:" << profile.name;

                    // Save the selection for next time
                    settings.setValue("OrbbecCamera/LastDepthProfile", selectedName);
                    break;
                }
            }

            if (!dptProfile) {
                errorString = QString("Could not find selected depth profile");
                return;
            }
            
            // Set bitsPerPixel based on depth format
            if (selectedDepthProfile.format == OB_FORMAT_Y16) {
                bitsPerPixel = qMax(bitsPerPixel, (unsigned short)10);
            } else if (selectedDepthProfile.format == OB_FORMAT_Y8) {
                bitsPerPixel = qMax(bitsPerPixel, (unsigned short)8);
            }

            // Set camera depth dimensions from selected profile
            camera.numDepthCols = selectedDepthProfile.width;
            camera.numDepthRows = selectedDepthProfile.height;
            
            // Handle pseudo 640x480 profile - use 640x576 camera config but report 640x480
            bool isPseudo640x480 = (selectedDepthProfile.width == LAU_CAMERA_DEFAULT_WIDTH && selectedDepthProfile.height == LAU_CAMERA_DEFAULT_HEIGHT && 
                                   selectedDepthProfile.name.contains("[Pseudo]"));
            if (isPseudo640x480) {
                // Configure camera for actual 640x576 capture
                camera.numDepthRows = 576;  // Camera captures 576 rows
                camera.isPseudoDepthProfile = true;  // Set flag for later dimension reporting
                qDebug() << "Using pseudo 640x480 profile - camera captures 640x576, reports 640x480";
            }

            ob_delete_sensor(dptSensor, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting sensor: %1").arg(error->message);
                processError(&error);
            }
            dptSensor = NULL;
        }
#else
        // Original automatic profile selection
        if (dptSensor){
            ob_stream_profile_list *dptProfiles = ob_sensor_get_stream_profile_list(dptSensor, &error);
            if (error){
                errorString = QString("Orbbec Error getting depth stream profile list: %1").arg(error->message);
                processError(&error);
                return;
            }

            uint32_t numProfiles = ob_stream_profile_list_count(dptProfiles, &error);
            if (error){
                errorString = QString("Orbbec Error getting number of depth profiles: %1").arg(error->message);
                processError(&error);
                return;
            } else if (numProfiles == 0){
                errorString = QString("No depth profiles found!");
                emit emitError(errorString);
                return;
            }

            // CREATE THE DEPTH PROFILE ASSUMING THE DEVICE IS A FEMTO MEGA I
#ifdef LUCID
            dptProfile = ob_stream_profile_list_get_video_stream_profile(dptProfiles, LAU_CAMERA_DEFAULT_WIDTH, OB_HEIGHT_ANY, OB_FORMAT_Y16, 15, &error);
#else
            dptProfile = ob_stream_profile_list_get_video_stream_profile(dptProfiles, LAU_CAMERA_DEFAULT_WIDTH, OB_HEIGHT_ANY, OB_FORMAT_Y16, 30, &error);
#endif
            if (error) {
                ob_delete_error(error);
                error = NULL;

                // CREATE THE DEPTH PROFILE ASSUMING THE DEVICE IS AN ASTRA 2
#ifdef LUCID
                dptProfile = ob_stream_profile_list_get_video_stream_profile(dptProfiles, 800, OB_HEIGHT_ANY, OB_FORMAT_Y16, 15, &error);
#else
                dptProfile = ob_stream_profile_list_get_video_stream_profile(dptProfiles, 800, OB_HEIGHT_ANY, OB_FORMAT_Y16, 30, &error);
#endif
                if (error) {
                    errorString = QString("Orbbec Error creating depth profile: %1").arg(error->message);
                    processError(&error);
                    return;
                }
            }

            // Set bitsPerPixel for depth - Y16 format uses 10 bits typically
            if (dptProfile) {
                bitsPerPixel = qMax(bitsPerPixel, (unsigned short)10);
            }

            ob_delete_stream_profile_list(dptProfiles, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting depth profile list: %1").arg(error->message);
                processError(&error);
            }

            ob_delete_sensor(dptSensor, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting sensor: %1").arg(error->message);
                processError(&error);
            }
            dptSensor = NULL;
        }
#endif

#ifdef ORBBEC_USE_RESOLUTION_DIALOG
        // Handle color sensor profile selection
        if (nirSensor || rgbSensor) {
            ob_sensor *colorSensor = nirSensor ? nirSensor : rgbSensor;
            QList<StreamProfileInfo> colorProfiles = nirSensor ? 
                getAvailableDepthProfiles(colorSensor) : // NIR uses same profile structure as depth
                getAvailableColorProfiles(colorSensor);

            if (colorProfiles.isEmpty()) {
                errorString = QString("No color profiles found!");
                return;
            }

            StreamProfileInfo selectedColorProfile;
            selectedColorProfile.profile = nullptr;

            if (hasDepthVideo && selectedDepthProfile.profile) {
                if (nirSensor) {
                    // For NIR, match the depth resolution exactly
                    // Note: selectedDepthProfile dimensions are what user selected (could be 640x480 pseudo)
                    for (const auto &profile : colorProfiles) {
                        if (profile.width == selectedDepthProfile.width && 
                            profile.height == selectedDepthProfile.height &&
                            profile.fps == selectedDepthProfile.fps) {
                            selectedColorProfile = profile;
                            qDebug() << "Matched NIR profile to depth:" << profile.name;
                            break;
                        }
                    }
                    if (!selectedColorProfile.profile) {
                        // If no exact match and we're looking for 640x480, try to find the pseudo profile
                        if (selectedDepthProfile.width == LAU_CAMERA_DEFAULT_WIDTH && selectedDepthProfile.height == LAU_CAMERA_DEFAULT_HEIGHT) {
                            for (const auto &profile : colorProfiles) {
                                if (profile.name.contains("[Pseudo]") && profile.fps == selectedDepthProfile.fps) {
                                    selectedColorProfile = profile;
                                    qDebug() << "Matched NIR pseudo profile to depth:" << profile.name;
                                    break;
                                }
                            }
                        }
                    }
                    if (!selectedColorProfile.profile) {
                        errorString = QString("No NIR profile found matching depth resolution %1x%2 @ %3fps")
                                     .arg(selectedDepthProfile.width)
                                     .arg(selectedDepthProfile.height)
                                     .arg(selectedDepthProfile.fps);
                        return;
                    }
                } else {
                    // Auto-select best color profile based on depth fps
                    selectedColorProfile = selectBestColorProfile(colorProfiles, selectedDepthProfile.fps);
                    if (!selectedColorProfile.profile) {
                        errorString = QString("No color profile found matching depth frame rate");
                        return;
                    }
                    qDebug() << "Auto-selected color profile:" << selectedColorProfile.name 
                             << "for depth fps:" << selectedDepthProfile.fps;
                }
            } else {
                // Manual color profile selection
                // Sort profiles by resolution (largest first)
                std::sort(colorProfiles.begin(), colorProfiles.end(), [](const StreamProfileInfo &a, const StreamProfileInfo &b) {
                    uint32_t resolutionA = a.width * a.height;
                    uint32_t resolutionB = b.width * b.height;
                    return resolutionA > resolutionB;
                });

                QStringList profileNames;
                for (const auto &profile : colorProfiles) {
                    profileNames << profile.name;
                }

                // Remember last selected color/NIR profile
                QSettings settings;
                QString settingsKey = nirSensor ? "OrbbecCamera/LastNIRProfile" : "OrbbecCamera/LastColorProfile";
                QString lastColorProfile = settings.value(settingsKey, QString()).toString();
                int defaultColorIndex = 0;
                if (!lastColorProfile.isEmpty()) {
                    int index = profileNames.indexOf(lastColorProfile);
                    if (index >= 0) {
                        defaultColorIndex = index;
                    }
                }

                bool ok;
                QString dialogTitle = nirSensor ? "NIR Resolution Selection" : "Color Resolution Selection";
                QString dialogPrompt = nirSensor ? "Select NIR resolution and frame rate:" : "Select color resolution and frame rate:";
                QString selectedName = QInputDialog::getItem(nullptr, dialogTitle,
                                                           dialogPrompt,
                                                           profileNames, defaultColorIndex, false, &ok);
                
                if (!ok) {
                    errorString = QString("Color profile selection cancelled");
                    return;
                }

                // Find the selected profile
                for (const auto &profile : colorProfiles) {
                    if (profile.name == selectedName) {
                        selectedColorProfile = profile;
                        qDebug() << "Selected color profile:" << profile.name;

                        // Save the selection for next time
                        settings.setValue(settingsKey, selectedName);
                        break;
                    }
                }

                if (!selectedColorProfile.profile) {
                    errorString = QString("Could not find selected color profile");
                    return;
                }
            }

            if (nirSensor) {
                nirProfile = selectedColorProfile.profile;
                // Set bitsPerPixel based on NIR format
                if (selectedColorProfile.format == OB_FORMAT_Y16) {
                    bitsPerPixel = qMax(bitsPerPixel, (unsigned short)10);
                } else if (selectedColorProfile.format == OB_FORMAT_Y8) {
                    bitsPerPixel = qMax(bitsPerPixel, (unsigned short)8);
                }
                // Set camera color dimensions from selected NIR profile
                camera.numColorCols = selectedColorProfile.width;
                camera.numColorRows = selectedColorProfile.height;
                
                // Handle pseudo 640x480 NIR profile - use 640x576 camera config but report 640x480
                if (selectedColorProfile.width == LAU_CAMERA_DEFAULT_WIDTH && selectedColorProfile.height == LAU_CAMERA_DEFAULT_HEIGHT && 
                    selectedColorProfile.name.contains("[Pseudo]")) {
                    // Configure camera for actual 640x576 capture
                    camera.numColorRows = 576;  // Camera captures 576 rows
                    camera.isPseudoColorProfile = true;  // Set flag for later dimension reporting
                    qDebug() << "Using pseudo 640x480 NIR profile - camera captures 640x576, reports 640x480";
                }
            } else {
                rgbProfile = selectedColorProfile.profile;
                // RGB is typically 8 bits per channel
                bitsPerPixel = qMax(bitsPerPixel, (unsigned short)8);
                // Set camera color dimensions from selected RGB profile
                camera.numColorCols = selectedColorProfile.width;
                camera.numColorRows = selectedColorProfile.height;
            }
            // Store the selected color profile

            ob_delete_sensor(colorSensor, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting color sensor: %1").arg(error->message);
                processError(&error);
            }
            nirSensor = NULL;
            rgbSensor = NULL;
        }
#else
        // Original automatic profile selection
        if (nirSensor){
            ob_stream_profile_list *nirProfiles = ob_sensor_get_stream_profile_list(nirSensor, &error);
            if (error){
                errorString = QString("Orbbec Error getting NIR stream profile list: %1").arg(error->message);
                processError(&error);
                return;
            }

            uint32_t numProfiles = ob_stream_profile_list_count(nirProfiles, &error);
            if (error){
                errorString = QString("Orbbec Error getting number of NIR profiles: %1").arg(error->message);
                processError(&error);
                return;
            } else if (numDevices == 0){
                errorString = QString("No NIR profiles found!");
                return;
            }

            // CREATE THE DEPTH PROFILE ASSUMING THE DEVICE IS A FEMTO MEGA I
            nirProfile = ob_stream_profile_list_get_video_stream_profile(nirProfiles, LAU_CAMERA_DEFAULT_WIDTH, OB_HEIGHT_ANY, OB_FORMAT_Y16, 30, &error);
            if (error) {
                ob_delete_error(error);
                error = NULL;

                // CREATE THE DEPTH PROFILE ASSUMING THE DEVICE IS AN ASTRA 2
                nirProfile = ob_stream_profile_list_get_video_stream_profile(nirProfiles, 800, OB_HEIGHT_ANY, OB_FORMAT_Y8, 30, &error);
                if (error) {
                    errorString = QString("Orbbec Error creating NIR profile: %1").arg(error->message);
                    processError(&error);
                    return;
                } else {
                    bitsPerPixel = qMax(bitsPerPixel, (unsigned short)8);
                }
            } else {
                bitsPerPixel = qMax(bitsPerPixel, (unsigned short)10);
            }

            ob_delete_stream_profile_list(nirProfiles, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting color profile list: %1").arg(error->message);
                processError(&error);
            }

            ob_delete_sensor(nirSensor, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting sensor: %1").arg(error->message);
                processError(&error);
            }
            nirSensor = NULL;
        }

        if (rgbSensor){
            ob_stream_profile_list *rgbProfiles = ob_sensor_get_stream_profile_list(rgbSensor, &error);
            if (error){
                errorString = QString("Orbbec Error getting RGB stream profile list: %1").arg(error->message);
                processError(&error);
                return;
            }

            uint32_t numProfiles = ob_stream_profile_list_count(rgbProfiles, &error);
            if (error){
                errorString = QString("Orbbec Error getting number of RGB profiles: %1").arg(error->message);
                processError(&error);
                return;
            } else if (numDevices == 0){
                errorString = QString("No RGB profiles found!");
                emit emitError(errorString);
                return;
            }

            // CREATE THE RGB PROFILE ASSUMING THE DEVICE IS AN ASTRA 2
#ifdef LUCID
            rgbProfile = ob_stream_profile_list_get_video_stream_profile(rgbProfiles, LAU_CAMERA_DEFAULT_WIDTH, OB_HEIGHT_ANY, OB_FORMAT_RGB, 15, &error);
#else
            rgbProfile = ob_stream_profile_list_get_video_stream_profile(rgbProfiles, LAU_CAMERA_DEFAULT_WIDTH, OB_HEIGHT_ANY, OB_FORMAT_RGB, 30, &error);
#endif
            if (error) {
                ob_delete_error(error);
                error = NULL;

                // CREATE THE RGB PROFILE ASSUMING THE DEVICE IS A FEMTO MEGA I
#ifdef LUCID
                rgbProfile = ob_stream_profile_list_get_video_stream_profile(rgbProfiles, LAU_CAMERA_HD_WIDTH, OB_HEIGHT_ANY, OB_FORMAT_RGB, 15, &error);
#else
                rgbProfile = ob_stream_profile_list_get_video_stream_profile(rgbProfiles, LAU_CAMERA_HD_WIDTH, OB_HEIGHT_ANY, OB_FORMAT_RGB, 30, &error);
#endif
                if (error) {
                    errorString = QString("Orbbec Error creating RGB profile: %1").arg(error->message);
                    processError(&error);
                    return;
                }
            }

            ob_delete_stream_profile_list(rgbProfiles, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting color profile list: %1").arg(error->message);
                processError(&error);
            }

            ob_delete_sensor(rgbSensor, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting sensor: %1").arg(error->message);
                processError(&error);
            }
            rgbSensor = NULL;
        }
#endif

        // Create a pipeline to open the depth stream after connecting the device
        camera.pipeline = ob_create_pipeline_with_device(camera.device, &error);
        if (error){
            errorString = QString("Orbbec Error creating pipeline with device: %1").arg(error->message);
            processError(&error);
            return;
        }

        // Create config to configure the resolution, frame rate, and format of the depth stream
        camera.config = ob_create_config(&error);
        if (error){
            errorString = QString("Orbbec Error creating configuration: %1").arg(error->message);
            processError(&error);
            return;
        }

        if (nirProfile){
            qDebug() << "Enabling NIR stream with nirProfile:" << (void*)nirProfile;
            ob_config_enable_stream_with_stream_profile(camera.config, nirProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error enabling NIR stream with NIR profile: %1").arg(error->message);
                processError(&error);
                return;
            }
        }

        if (rgbProfile){
            ob_config_enable_stream_with_stream_profile(camera.config, rgbProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error enabling color stream with color profile: %1").arg(error->message);
                processError(&error);
                return;
            }
        }

        if (dptProfile){
            ob_config_set_align_mode(camera.config, ALIGN_DISABLE, &error);
            if (error) {
                errorString = QString("Orbbec Error setting alignment mode: %1").arg(error->message);
                processError(&error);
                return;
            }

            // Check if alignment is actually disabled after setting it
            bool isAlignmentEnabled = ob_device_get_bool_property(camera.device, OB_PROP_DEPTH_ALIGN_HARDWARE_BOOL, &error);
            if (error) {
                errorString = QString("Orbbec Error checking alignment status: %1").arg(error->message);
                processError(&error);
                return;
            } else {
                qDebug() << "Alignment status after disabling: " << (isAlignmentEnabled ? "ENABLED" : "DISABLED") << " for " << camera.modelString << " " << camera.serialString;
            }

            ob_config_enable_stream_with_stream_profile(camera.config, dptProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error enabling depth stream with depth profile: %1").arg(error->message);
                processError(&error);
                return;
            }

            // Get dimensions from the depth profile
            uint32_t profileWidth = ob_video_stream_profile_width(dptProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error getting depth profile width: %1").arg(error->message);
                processError(&error);
                return;
            }

            uint32_t profileHeight = ob_video_stream_profile_height(dptProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error getting depth profile height: %1").arg(error->message);
                processError(&error);
                return;
            }

            qDebug() << "Depth profile resolution:" << profileWidth << "x" << profileHeight;

            // Get raw device calibration parameters instead of pipeline parameters
            ob_camera_param_list *paramList = ob_device_get_calibration_camera_param_list(camera.device, &error);
            if (error) {
                errorString = QString("Orbbec Error getting device calibration parameters: %1").arg(error->message);
                processError(&error);
                return;
            }

            // Get the number of camera parameters
            uint32_t paramCount = ob_camera_param_list_count(paramList, &error);
            if (error) {
                errorString = QString("Orbbec Error getting parameter count: %1").arg(error->message);
                processError(&error);
                return;
            }

            // Find the depth camera parameters matching the profile resolution
            bool foundDepthParams = false;
            for (uint32_t i = 0; i < paramCount; i++) {
                ob_camera_param depthParam = ob_camera_param_list_get_param(paramList, i, &error);
                if (error) {
                    errorString = QString("Orbbec Error getting parameter at index %1: %2").arg(i).arg(error->message);
                    processError(&error);
                    continue;
                }

                // Check if this matches our depth profile resolution
                if (depthParam.depthIntrinsic.width == profileWidth && 
                    depthParam.depthIntrinsic.height == profileHeight) {
                    // EXTRACT THE CAMERA LENS INTRINSICS FOR DEPTH SENSOR
                    camera.deviceIntrinsics.cx = depthParam.depthIntrinsic.cx;
                    camera.deviceIntrinsics.cy = depthParam.depthIntrinsic.cy;
                    camera.deviceIntrinsics.fx = depthParam.depthIntrinsic.fx;
                    camera.deviceIntrinsics.fy = depthParam.depthIntrinsic.fy;

                    // EXTRACT THE RAW CAMERA LENS DISTORTION PARAMETERS FOR DEPTH SENSOR
                    camera.deviceIntrinsics.k1 = depthParam.depthDistortion.k1;
                    camera.deviceIntrinsics.k2 = depthParam.depthDistortion.k2;
                    camera.deviceIntrinsics.k3 = depthParam.depthDistortion.k3;
                    camera.deviceIntrinsics.k4 = depthParam.depthDistortion.k4;
                    camera.deviceIntrinsics.k5 = depthParam.depthDistortion.k5;
                    camera.deviceIntrinsics.k6 = depthParam.depthDistortion.k6;
                    camera.deviceIntrinsics.p1 = depthParam.depthDistortion.p1;
                    camera.deviceIntrinsics.p2 = depthParam.depthDistortion.p2;

                    qDebug() << "Using raw device distortion parameters for " << camera.modelString << " " << camera.serialString;
                    qDebug() << "Matched resolution:" << depthParam.depthIntrinsic.width << "x" << depthParam.depthIntrinsic.height;
                    qDebug() << "k1:" << camera.deviceIntrinsics.k1 << "k2:" << camera.deviceIntrinsics.k2 << "k3:" << camera.deviceIntrinsics.k3;
                    qDebug() << "k4:" << camera.deviceIntrinsics.k4 << "k5:" << camera.deviceIntrinsics.k5 << "k6:" << camera.deviceIntrinsics.k6;
                    foundDepthParams = true;
                    break; // Found exact match, no need to continue
                }
            }

            // Clean up the parameter list
            ob_delete_camera_param_list(paramList, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting camera parameter list: %1").arg(error->message);
                processError(&error);
            }

            if (!foundDepthParams) {
                errorString = QString("Could not find depth camera parameters matching profile resolution %1x%2").arg(profileWidth).arg(profileHeight);
                return;
            }
        }

        // GET THE DIMENSIONS OF OUR COLOR BUFFER MEMORY OBJECT
        if (modelString.contains("Femto")){
            if (nirProfile){
                ob_device_set_int_property(camera.device, OB_PROP_SWITCH_IR_MODE_INT, 0, &error);
                if (error) {
                    errorString = QString("Orbbec Error setting IR property to active: %1").arg(error->message);
                    processError(&error);
                    return;
                }
            }
        }

        if (dptProfile){
            camera.numDepthCols = ob_video_stream_profile_width(dptProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error getting depth video width: %1").arg(error->message);
                processError(&error);
                return;
            }

            camera.numDepthRows = ob_video_stream_profile_height(dptProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error getting depth video height: %1").arg(error->message);
                processError(&error);
                return;
            }

#ifndef ORBBEC_USE_RESOLUTION_DIALOG
            // When resolution dialog is not available, automatically use pseudo 640x480
            // if camera reports 640x576 (e.g., Femto Mega i)
            if (camera.numDepthCols == LAU_ORBBEC_GEMINI2_NATIVE_WIDTH && camera.numDepthRows == LAU_ORBBEC_GEMINI2_NATIVE_HEIGHT) {
                camera.isPseudoDepthProfile = true;
                qDebug() << "Auto-enabling pseudo 640x480 depth profile - camera captures 640x576, reports 640x480";
            }
#endif

        }

        if (nirProfile){
            camera.numColorCols = ob_video_stream_profile_width(nirProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error getting NIR video width: %1").arg(error->message);
                processError(&error);
                return;
            }

            camera.numColorRows = ob_video_stream_profile_height(nirProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error getting NIR video height: %1").arg(error->message);
                processError(&error);
                return;
            }

#ifndef ORBBEC_USE_RESOLUTION_DIALOG
            // When resolution dialog is not available, automatically use pseudo 640x480
            // if camera reports 640x576 (e.g., Femto Mega i)
            if (camera.numColorCols == LAU_ORBBEC_GEMINI2_NATIVE_WIDTH && camera.numColorRows == LAU_ORBBEC_GEMINI2_NATIVE_HEIGHT) {
                camera.isPseudoColorProfile = true;
                qDebug() << "Auto-enabling pseudo 640x480 NIR profile - camera captures 640x576, reports 640x480";
            }
#endif
        } else if (rgbProfile){
            camera.numColorCols = ob_video_stream_profile_width(rgbProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error getting RGB video width: %1").arg(error->message);
                processError(&error);
                return;
            }

            camera.numColorRows = ob_video_stream_profile_height(rgbProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error getting RGB video height: %1").arg(error->message);
                processError(&error);
                return;
            }
        } else {
            // Neither nirProfile nor rgbProfile exists (e.g., #ifndef LUCID ColorXYZG case)
            // Use depth dimensions for color buffer since we copy depth to color
            camera.numColorCols = camera.numDepthCols;
            camera.numColorRows = camera.numDepthRows;
        }

        // Start the pipeline with config
        ob_pipeline_start_with_config(camera.pipeline, camera.config, &error);
        if (error) {
            errorString = QString("Orbbec Error starting pipeline with configuration: %1").arg(error->message);
            processError(&error);
            return;
        }

        // IF WE MAKE IT THIS FAR, THEN WE MUST BE CONNECTED TO THE CAMERA
        isConnected = true;
        camera.isConnected = true;
        cameras.prepend(camera);

        if (dptProfile){
            ob_delete_stream_profile(dptProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting depth profile: %1").arg(error->message);
                processError(&error);
            }
            dptProfile = NULL;
        }

        if (rgbProfile){
            ob_delete_stream_profile(rgbProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting color profile: %1").arg(error->message);
                processError(&error);
            }
            rgbProfile = NULL;
        }

        if (nirProfile){
            ob_delete_stream_profile(nirProfile, &error);
            if (error) {
                errorString = QString("Orbbec Error deleting NIR profile: %1").arg(error->message);
                processError(&error);
            }
            nirProfile = NULL;
        }
    }

    // Set class member dimensions from the first camera
    if (cameras.count() > 0) {
        if (hasColor()){
            numColorCols = cameras.at(0).numColorCols;
            numColorRows = cameras.at(0).numColorRows;
            
            // Override dimensions if using pseudo 640x480 NIR profile
            // Only apply the dimension override if the pseudo profile was explicitly selected
            if (cameras.at(0).isPseudoColorProfile) {
                // Report 640x480 dimensions even though camera captures 640x576
                numColorRows = LAU_ORBBEC_PSEUDO_HEIGHT;
                qDebug() << "Setting global NIR dimensions to 640x480 for pseudo profile";
            }
        }

        if (hasDepth()){
            numDepthCols = cameras.at(0).numDepthCols;
            numDepthRows = cameras.at(0).numDepthRows;
            
            // Override dimensions if using pseudo 640x480 depth profile
            // Only apply the dimension override if the pseudo profile was explicitly selected
            if (cameras.at(0).isPseudoDepthProfile) {
                // Report 640x480 dimensions even though camera captures 640x576
                numDepthRows = LAU_ORBBEC_PSEUDO_HEIGHT;
                qDebug() << "Setting global depth dimensions to 640x480 for pseudo profile";
            }
        }
    }

    if (hasDepth()){
        zMinDistance = static_cast<unsigned short>(33.0);
        zMaxDistance = static_cast<unsigned short>(8400.0);
    } else {
        zMinDistance = static_cast<unsigned short>(0);
        zMaxDistance = static_cast<unsigned short>((1 << bitsPerPixel) - 1);
    }

    ob_delete_device_list(deviceList, &error);
    if (error) {
        errorString = QString("Orbbec Error deleting device list: %1").arg(error->message);
        processError(&error);
    }
    deviceList = NULL;
#else
    // macOS: No Orbbec SDK initialization needed
    ;
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUOrbbecCamera::colorMemoryObject() const
{
    if (hasColorVideo) {
        if (playbackColor == ColorGray || playbackColor == ColorXYZG) {
            // Always use unsigned short (16-bit) for ColorGray/ColorXYZG to match Lucid camera format
            // This ensures compatibility in multi-sensor setups where Lucid cameras use 16-bit NIR
            return (LAUMemoryObject(numColorCols, numColorRows, 1, sizeof(unsigned short), cameras.count()));
        } else if (playbackColor == ColorRGB || playbackColor == ColorXYZRGB) {
            return (LAUMemoryObject(numColorCols, numColorRows, 3, sizeof(unsigned char), cameras.count()));
        }
    }
    return (LAUMemoryObject());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUOrbbecCamera::depthMemoryObject() const
{
    if (hasDepthVideo) {
        return (LAUMemoryObject(numDepthCols, numDepthRows, 1, sizeof(unsigned short), cameras.count()));
    }
    return (LAUMemoryObject());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUOrbbecCamera::mappiMemoryObject() const
{
    return (LAUMemoryObject());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUOrbbecCamera::~LAUOrbbecCamera()
{
#ifndef Q_OS_MAC
    // CREATE ERROR OBJECT
    ob_error *error = NULL;

    while (cameras.count() > 0){
        CameraPacket camera = cameras.takeFirst();

        if (camera.isConnected){
            if (camera.pipeline){
                ob_pipeline_stop(camera.pipeline, &error);
                if (error){
                    errorString = QString("Orbbec Error stopping pipeline: %1").arg(error->message);
                    processError(&error);
                    return;
                }
            }
        }

        ob_delete_config(camera.config, &error);
        if (error) {
            errorString = QString("Orbbec Error deleting config: %1").arg(error->message);
            processError(&error);
        }

        // stop the pipeline
        ob_pipeline_stop(camera.pipeline, &error);
        if (error) {
            errorString = QString("Orbbec Error stopping pipeline: %1").arg(error->message);
            processError(&error);
        }

        ob_delete_device(camera.device, &error);
        if (error) {
            errorString = QString("Orbbec Error deleting device: %1").arg(error->message);
            processError(&error);
        }
    }

    if (context){
        ob_delete_context(context, &error);
        if (error) {
            errorString = QString("Orbbec Error deleting context: %1").arg(error->message);
            processError(&error);
        }
        context = NULL;
    }
#else
    ;
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUOrbbecCamera::onUpdateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    depth.constMakeElapsedInvalid();
    color.constMakeElapsedInvalid();
    mapping.constMakeElapsedInvalid();

    // KEEP TRACK OF CONSECUTIVE BAD FRAMES TO SEE IF WE LOST OUR CONNECTION
    static int badFrameCounter = 0;

#ifndef Q_OS_MAC
    // ITERATE THROUGH ALL CONNECTED CAMERAS
    for (int cam = 0; cam < cameras.count(); cam++) {
        CameraPacket camera = cameras.at(cam);

        // RESET ERROR STRING
        errorString = QString();

        for (unsigned int frame = 0; frame < frameReplicateCount; frame++) {
            // CREATE AN ERROR OBJECT
            ob_error *error = NULL;

            // Wait for up to 1000ms for a frameset in blocking mode.
            ob_frame *frameset = NULL;
            for (int tries = 0; tries < 5; tries++){
                frameset = ob_pipeline_wait_for_frameset(camera.pipeline, 1000, &error);
                if (error) {
                    errorString = QString("Orbbec Error waiting for frameset: %1").arg(error->message);
                    processError(&error);
                    break;
                }
                if (frameset) break;
            }

            // SEE IF WE GRABBED A NEW FRAME SET
            if (frameset == NULL){
                errorString = QString("NO VALID FRAMESET FROM ORBBEC CAMERA");
                processError(NULL);
                break;
            }

            if (hasDepth() && depth.isValid()){
                // GET THE DEPTH FRAME HANDLE
                ob_frame *dptFrame = ob_frameset_depth_frame(frameset, &error);

                // CHECK FOR ERRORS AND THEN PASS PIXELS TO DEPTH BUFFER
                if (error) {
                    errorString = QString("Orbbec Error getting depth from frameset: %1").arg(error->message);
                    processError(&error);
                    break;
                } else if (dptFrame) {
                    // GET THE INTERNAL FRAME COUNTER
                    uint32_t index = ob_frame_index(dptFrame, &error);
                    Q_UNUSED(index);
                    if (error) {
                        errorString = QString("Orbbec Error getting index: %1").arg(error->message);
                        processError(&error);
                        break;
                    }

                    ob_format format = ob_frame_format(dptFrame, &error);
                    if (error) {
                        errorString = QString("Orbbec Error getting format: %1").arg(error->message);
                        processError(&error);
                        break;
                    }

                    if (format == OB_FORMAT_Y16) {
                        uint32_t width = ob_video_frame_width(dptFrame, &error);
                        if (width == numDepthCols){
                            uint32_t height = ob_video_frame_height(dptFrame, &error);
                            if (height == numDepthRows){
                                uint16_t *data = (uint16_t *)ob_frame_data(dptFrame, &error);
                                memcpy(depth.constFrame(cam + startingIndex), data, qMin((int)(width*height*sizeof(short)), (int)depth.block()));
                            } else if (height > numDepthRows){
                                uint16_t *data = (uint16_t *)ob_frame_data(dptFrame, &error) + (height - numDepthRows)/2 * width;
                                memcpy(depth.constFrame(cam + startingIndex), data, qMin((int)(width*height*sizeof(short)), (int)depth.block()));
                            }
                        }
                    }

                    // GET THE DEPTH SCALE AND SEE IF WE NEED TO SHIFT PIXELS SO THAT SCALE IS 0.25 mm
                    float scale = ob_depth_frame_get_value_scale(dptFrame, &error);
                    if (error) {
                        errorString = QString("Orbbec Error getting format: %1").arg(error->message);
                        processError(&error);
                        break;
                    } else if (fabs(scale / 0.25 - 4.0) < 0.001) {
                        // LETS SHIFT THE PIXELS LEFT IN ORDER TO MULTIPLY BY FOUR
                        unsigned short *buffer = (unsigned short*)depth.constFrame(cam + startingIndex);
                        int numPixels = (int)(depth.height() * depth.width());
                        for (int c = 0; c < numPixels; c += 8) {
                            _mm_store_si128((__m128i *)((unsigned short *)(buffer + c)), _mm_slli_epi16(_mm_load_si128((__m128i *)(buffer + c)), 2));
                        }
                    }

                    // DELETE THE DEPTH FRAME
                    ob_delete_frame(dptFrame, &error);
                    if (error) {
                        errorString = QString("Orbbec Error deleting depth frame: %1").arg(error->message);
                        processError(&error);
                        break;
                    }
                } else {
                    errorString = "FAILED TO GRAB A VALID DEPTH FRAME";
                    processError(NULL);
                }

                // IF WE MADE IT THIS FAR WITHOUT ERROR, WE SHOULD SET THE ELAPSED VALUE OF THE DEPTH AND COLOR OBJECTS
                depth.setConstElapsed(elapsed());
            }

            // GRAB COLOR VIDEO IF IT IS ENABLED
            if (hasColor() && color.isValid()){
#ifndef LUCID
                if (playbackColor == ColorXYZG){
                    // For XYZG mode, copy depth data to color buffer (no NIR stream to save bandwidth)
                    if (depth.isValid() && depth.isElapsedValid()){
                        memcpy(color.constFrame(cam + startingIndex), depth.constFrame(cam + startingIndex), qMin(color.block(), depth.block()));
                        color.setConstElapsed(depth.constElapsed());
                    }
                } else
#endif
                if (playbackColor == ColorGray){
                    // GET THE NIR FRAME HANDLE
                    ob_frame *nirFrame = ob_frameset_ir_frame(frameset, &error);

                    // CHECK FOR ERRORS AND THEN PASS PIXELS TO DEPTH BUFFER
                    if (error) {
                        errorString = QString("Orbbec Error getting IR from frameset: %1").arg(error->message);
                        processError(&error);
                        break;
                    } else if (nirFrame) {
                        // GET THE INTERNAL FRAME COUNTER
                        uint32_t index = ob_frame_index(nirFrame, &error);
                        Q_UNUSED(index);
                        if (error) {
                            errorString = QString("Orbbec Error getting index: %1").arg(error->message);
                            processError(&error);
                            break;
                        }

                        // GET THE FORMAT OF THE CURRENT DEPTH FRAMGE
                        ob_format format = ob_frame_format(nirFrame, &error);
                        if (error) {
                            errorString = QString("Orbbec Error getting format: %1").arg(error->message);
                            processError(&error);
                            break;
                        }

                        if (format == OB_FORMAT_Y8) {
                            uint32_t width = ob_video_frame_width(nirFrame, &error);
                            if (width == numColorCols){
                                uint32_t height = ob_video_frame_height(nirFrame, &error);
                                uint8_t *data8 = (uint8_t *)ob_frame_data(nirFrame, &error);
                                unsigned short *data16 = (unsigned short *)color.constFrame(cam + startingIndex);

                                if (height == numColorRows){
                                    // Convert 8-bit NIR data to 16-bit by scaling (multiply by 256 to use full 16-bit range)
                                    for (uint32_t i = 0; i < width * height; i++){
                                        data16[i] = (unsigned short)data8[i] << 8;
                                    }
                                } else if (height > numColorRows){
                                    // Handle cropping for oversized frames (640x576 -> 640x480)
                                    data8 += (height - numColorRows)/2 * width;
                                    for (uint32_t i = 0; i < width * numColorRows; i++){
                                        data16[i] = (unsigned short)data8[i] << 8;
                                    }
                                }
                            }
                        } else if (format == OB_FORMAT_Y16) {
                            uint32_t width = ob_video_frame_width(nirFrame, &error);
                            if (width == numColorCols){
                                uint32_t height = ob_video_frame_height(nirFrame, &error);
                                if (height == numColorRows){
                                    uint16_t *data = (uint16_t *)ob_frame_data(nirFrame, &error);
                                    memcpy(color.constFrame(cam + startingIndex), data, qMin((int)(width*height*sizeof(unsigned short)), (int)color.block()));
                                } else if (height > numColorRows){
                                    uint16_t *data = (uint16_t *)ob_frame_data(nirFrame, &error) + (height - numColorRows)/2 * width;
                                    memcpy(color.constFrame(cam + startingIndex), data, qMin((int)(width*height*sizeof(unsigned short)), (int)color.block()));
                                }
                            }
                        }

                        // DELETE THE NIR FRAME
                        ob_delete_frame(nirFrame, &error);
                        if (error) {
                            errorString = QString("Orbbec Error deleting nir frame: %1").arg(error->message);
                            processError(&error);
                            break;
                        }
                    } else {
                        errorString = "FAILED TO GRAB A VALID DEPTH FRAME";
                        processError(NULL);
                    }
                } else if (color.colors() == 3){
                    // GET THE RGB FRAME HANDLE
                    ob_frame *rgbFrame = ob_frameset_color_frame(frameset, &error);

                    // CHECK FOR ERRORS AND THEN PASS PIXELS TO DEPTH BUFFER
                    if (error) {
                        errorString = QString("Orbbec Error getting color from frameset: %1").arg(error->message);
                        processError(&error);
                        break;
                    } else if (rgbFrame) {
                        // GET THE INTERNAL FRAME COUNTER
                        uint32_t index = ob_frame_index(rgbFrame, &error);
                        Q_UNUSED(index);
                        if (error) {
                            errorString = QString("Orbbec Error getting index: %1").arg(error->message);
                            processError(&error);
                            break;
                        }

                        // GET THE FORMAT OF THE CURRENT DEPTH FRAMGE
                        ob_format format = ob_frame_format(rgbFrame, &error);
                        if (error) {
                            errorString = QString("Orbbec Error getting format: %1").arg(error->message);
                            processError(&error);
                            break;
                        }

                        if (format == OB_FORMAT_RGB) {
                            uint32_t width = ob_video_frame_width(rgbFrame, &error);
                            if (width == numColorCols){
                                uint32_t height = ob_video_frame_height(rgbFrame, &error);
                                if (height == numColorRows){
                                    uint16_t *data = (uint16_t *)ob_frame_data(rgbFrame, &error);
                                    memcpy(color.constFrame(cam + startingIndex), data, qMin((int)(width*height*3*sizeof(char)), (int)color.block()));
                                } else if (height > numColorRows){
                                    uint16_t *data = (uint16_t *)ob_frame_data(rgbFrame, &error) + (height - numColorRows)/2 * width;
                                    memcpy(color.constFrame(cam + startingIndex), data, qMin((int)(width*height*3*sizeof(char)), (int)color.block()));
                                }
                            }
                        }

                        // DELETE THE RGB FRAME
                        ob_delete_frame(rgbFrame, &error);
                        if (error) {
                            errorString = QString("Orbbec Error deleting RGB frame: %1").arg(error->message);
                            processError(&error);
                            break;
                        }
                    } else {
                        errorString = "FAILED TO GRAB A VALID COLOR FRAME";
                        processError(NULL);
                    }
                }

                // IF WE MADE IT THIS FAR WITHOUT ERROR, WE SHOULD SET THE ELAPSED VALUE OF THE DEPTH AND COLOR OBJECTS
                color.setConstElapsed(elapsed());
            }

            if (frameset != NULL){
                ob_delete_frame(frameset, &error);
                if (error) {
                    errorString = QString("Orbbec Error deleting frameset: %1").arg(error->message);
                    processError(&error);
                    break;
                }
            }
        }
    }

    if (depth.isValid()){
        if (depth.isElapsedValid() == false){
            badFrameCounter++;
            if (badFrameCounter > 5){
                qApp->exit(100);
            }
        }
    }
#endif
    // SEND THE USER BUFFER TO THE NEXT STAGE
    emit emitBuffer(depth, color, mapping);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable LAUOrbbecCamera::lut(int chn, QWidget *widget) const
{
    CameraPacket camera = cameras.at(chn);
    QMatrix3x3 intParameters;
    intParameters(0, 0) = camera.deviceIntrinsics.fx; // Cust::CalibFocalLengthX
    intParameters(0, 1) = 0.0f;
    intParameters(0, 2) = camera.deviceIntrinsics.cx; // Cust::CalibOpticalCenterX

    intParameters(1, 0) = 0.0f;
    intParameters(1, 1) = camera.deviceIntrinsics.fy; // Cust::CalibFocalLengthY
    intParameters(1, 2) = camera.deviceIntrinsics.cy; // Cust::CalibOpticalCenterY

    intParameters(2, 0) = 0.0f;
    intParameters(2, 1) = 0.0f;
    intParameters(2, 2) = 1.0f;

    QVector<double> rdlParameters(6);
    rdlParameters[0] = camera.deviceIntrinsics.k1; // * 2.0f;
    rdlParameters[1] = camera.deviceIntrinsics.k2;
    rdlParameters[2] = camera.deviceIntrinsics.k3; // * 10.0f;
    rdlParameters[3] = camera.deviceIntrinsics.k4; // * 2.0f;
    rdlParameters[4] = camera.deviceIntrinsics.k5;
    rdlParameters[5] = camera.deviceIntrinsics.k6; // * 10.0f;

    QVector<double> tngParameters(2);
    tngParameters[0] = camera.deviceIntrinsics.p1;
    tngParameters[1] = camera.deviceIntrinsics.p2;

    // All cameras should use 0.25 scale factor for cascade classifier consistency
    LAULookUpTable lookUpTable(camera.numDepthCols, camera.numDepthRows, intParameters, rdlParameters, tngParameters, 0.25, zMinDistance, zMaxDistance, widget);
    lookUpTable.setIntrinsics(camera.deviceIntrinsics);
    lookUpTable.setMakeString(makeString);
    lookUpTable.setModelString(modelString);

    // FLIP THE LOOK UP TABLE SINCE WE ARE GOING TO FLIP THE RAW VIDEO
    if (camera.modelString.contains("Femto")){
        //lookUpTable.rotate180InPlace();
    }

    // CROP THE TABLE IF THE CAMERA DIMENSIONS ARE LARGER THAN THE OBJECT DIMENSIONS
    if (camera.numDepthCols > numDepthCols || camera.numDepthRows > numDepthRows){
        // CROP THE IMAGE IF THERE IS DISPARITY BETWEEN SENSOR AND DEPTH DIMENSIONS
        unsigned int lft = (camera.numDepthCols - numDepthCols)/2;
        unsigned int top = (camera.numDepthRows - numDepthRows)/2;
        lookUpTable = lookUpTable.crop(lft, top, numDepthCols, numDepthRows);
    }

    return (lookUpTable);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector<double> LAUOrbbecCamera::jetr(int chn) const
{
    QVector<double> vector(37, NAN);
#ifndef Q_OS_MAC
    CameraPacket packet = cameras.at(chn);

    // COPY OVER THE INTRINSICS
    vector[ 0] = packet.deviceIntrinsics.fx;
    vector[ 1] = packet.deviceIntrinsics.cx;
    vector[ 2] = packet.deviceIntrinsics.fy;
    vector[ 3] = packet.deviceIntrinsics.cy;
    vector[ 4] = packet.deviceIntrinsics.k1;
    vector[ 5] = packet.deviceIntrinsics.k2;
    vector[ 6] = packet.deviceIntrinsics.k3;
    vector[ 7] = packet.deviceIntrinsics.k4;
    vector[ 8] = packet.deviceIntrinsics.k5;
    vector[ 9] = packet.deviceIntrinsics.k6;
    vector[10] = packet.deviceIntrinsics.p1;
    vector[11] = packet.deviceIntrinsics.p2;

    // COPY OVER THE PROJECTION MATRIX
    QMatrix4x4 projectionMatrix;
    for (int n = 0; n < 16; n++){
        vector[12 + n] = projectionMatrix.data()[n];
    }

    // COPY OVER THE BOUNDING BOX
    vector[28] = -std::numeric_limits<float>::infinity();
    vector[29] = +std::numeric_limits<float>::infinity();
    vector[30] = -std::numeric_limits<float>::infinity();
    vector[31] = +std::numeric_limits<float>::infinity();
    vector[32] = -std::numeric_limits<float>::infinity();
    vector[33] = +std::numeric_limits<float>::infinity();

    // COPY OVER THE SCALE FACTOR AND THE RANGE LIMITS
    vector[34] = localScaleFactor;
    vector[35] = zMinDistance;
    vector[36] = zMaxDistance;
#endif
    // RETURN JUST ENOUGH INFORMATION TO RECONSTRUCT A POINT CLOUD FROM RAW DATA TO USER
    return(vector);
}
