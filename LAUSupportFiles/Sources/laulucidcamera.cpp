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

#include "laulucidcamera.h"
#include "laucameraclassifierdialog.h"
#include <cmath>  // For fabs function

bool LAULucidCamera_camerasLessThan(const LAULucidCamera::CameraPacket &s1, const LAULucidCamera::CameraPacket &s2)
{
#ifdef RAW_NIR_VIDEO
    // For RAW_NIR_VIDEO mode, always sort by serial number for consistent ordering
    // This ensures cameras appear in the same order every time for debugging
    return (s1.serialString < s2.serialString);
#else
    // For normal mode, sort by user-defined name if available
    // If both cameras have user-defined names set, sort by user-defined name
    // This ensures cameras are ordered by their physical location labels (e.g., "SIDE", "TOP")
    if (!s1.userDefinedName.isEmpty() && !s2.userDefinedName.isEmpty()) {
        return (s1.userDefinedName < s2.userDefinedName);
    }

    // If only one has a user-defined name, prioritize it (cameras with names come first)
    if (!s1.userDefinedName.isEmpty()) {
        return true;  // s1 comes before s2
    }
    if (!s2.userDefinedName.isEmpty()) {
        return false; // s2 comes before s1
    }

    // If neither has a user-defined name, fall back to sorting by serial number
    return (s1.serialString < s2.serialString);
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULucidCamera::LAULucidCamera(QString range, LAUVideoPlaybackColor color, QObject *parent) : LAU3DCamera(color, parent), rangeModeString(range), numDepthRows(0), numDepthCols(0), numColorRows(0), numColorCols(0), hasMappingVideo(false)
#if !defined(Q_OS_MAC)
    , hSystem(NULL)
#endif
{
    initialize();
    qDebug() << errorString;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULucidCamera::LAULucidCamera(LAUVideoPlaybackColor color, QObject *parent) : LAU3DCamera(color, parent), rangeModeString(QString(LUCIDRANGEMODESTRING)), numDepthRows(0), numDepthCols(0), numColorRows(0), numColorCols(0), hasMappingVideo(false)
#if !defined(Q_OS_MAC)
    , hSystem(NULL)
#endif
{
    initialize();
    qDebug() << errorString;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULucidCamera::LAULucidCamera(QObject *parent) : LAU3DCamera(ColorXYZRGB, parent), rangeModeString(QString(LUCIDRANGEMODESTRING)), numDepthRows(0), numDepthCols(0), numColorRows(0), numColorCols(0), hasMappingVideo(false)
#if !defined(Q_OS_MAC)
    , hSystem(NULL)
#endif
{
    initialize();
    qDebug() << errorString;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<LAUVideoPlaybackColor> LAULucidCamera::playbackColors()
{
    QList<LAUVideoPlaybackColor> list;
    list << ColorGray << ColorXYZ << ColorXYZG;
    return (list);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAULucidCamera::initialize()
{
    readVideoFromDiskFlag = false;
    makeString = QString("Lucid");
    modelString = QString("Helios 2");
    serialString = QString("");
    localScaleFactor = 0.25f;
    isConnected = false;

    switch (playbackColor) {
        case ColorGray:
            hasColorVideo = true;
            hasDepthVideo = false;
            break;
        case ColorRGB:
            hasColorVideo = true;
            hasDepthVideo = false;
            break;
        case ColorXYZ:
            hasColorVideo = false;
            hasDepthVideo = true;
            break;
        case ColorXYZG:
            hasColorVideo = true;
            hasDepthVideo = true;
            break;
        case ColorXYZRGB:
            hasColorVideo = true;
            hasDepthVideo = true;
            break;
        default:
            return;
    }

#ifdef Q_OS_WIN
    // CREATE INTEGER TO READ VALUES FROM CAMERAS
    int64_t pDeviceInt = -1;

    hTLSystemNodeMap = NULL;
    size_t pBufLen = 64;
    char pDeviceNameBuf[64];
    char pDeviceParameterBuf[256];
    bool8_t pDeviceBool = false;
    AC_ERROR err = AC_ERR_SUCCESS;

    // prepare example
    err = acOpenSystem(&hSystem);
    if (err != AC_ERR_SUCCESS) {
        errorString = QString("Lucid Error opening system: %1").arg(errorMessages(err));
        return;
    }

    err = acSystemUpdateDevices(hSystem, 100);
    if (err != AC_ERR_SUCCESS) {
        errorString = QString("Lucid Error updating devices: %1").arg(errorMessages(err));
        return;
    }

    size_t numDevices = 0;
    err = acSystemGetNumDevices(hSystem, &numDevices);
    if (err != AC_ERR_SUCCESS) {
        errorString = QString("Lucid Error enumerating devices: %1").arg(errorMessages(err));
        return;
    }

    if (numDevices == 0) {
        errorString = QString("No Lucid devices found!");
        return;
    }

    for (unsigned int n = 0; n < numDevices; n++) {
        CameraPacket packet = {NULL, NULL, NULL, false, LUCIDDEPTHSENSORHEIGHT, LUCIDDEPTHSENSORWIDTH, LUCIDDEPTHSENSORHEIGHT, LUCIDDEPTHSENSORWIDTH, 0.25, makeString, modelString };

        err = acSystemCreateDevice(hSystem, (size_t)n, &packet.hDevice);
        if (err != AC_ERR_SUCCESS) {
            continue;
        }

        err = acDeviceGetNodeMap(packet.hDevice, &packet.hNodeMap);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting device node map: %1").arg(errorMessages(err));
            return;
        }

        // Reset buffer length before reading serial number
        pBufLen = 64;
        err = acNodeMapGetStringValue(packet.hNodeMap, "DeviceSerialNumber", pDeviceNameBuf, &pBufLen);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting device name: %1").arg(errorMessages(err));
            return;
        } else {
            packet.serialString = QString(pDeviceNameBuf);
        }

        // Get user-defined name from camera hardware
        // This should have been programmed by main.cpp from systemConfig.ini at startup
        pBufLen = LUCIDMAXBUF;
        err = acNodeMapGetStringValue(packet.hNodeMap, "DeviceUserID", pDeviceNameBuf, &pBufLen);
        if (err != AC_ERR_SUCCESS || strlen(pDeviceNameBuf) == 0) {
            // No user-defined name in hardware, use serial number
            packet.userDefinedName = packet.serialString;
            qDebug() << "Lucid camera" << packet.serialString << "no user-defined name in hardware, using serial number";
        } else {
            packet.userDefinedName = QString(pDeviceNameBuf);
            qDebug() << "Lucid camera" << packet.serialString << "user-defined name from hardware:" << packet.userDefinedName;
        }

        // Validate serial number format - Lucid cameras have 9-digit numeric serial numbers
        // Skip devices with invalid serial numbers (e.g., Orbbec cameras that appear in device list)
        if (packet.serialString.length() != 9) {
            qDebug() << "Skipping device with invalid Lucid serial number format:" << packet.serialString << "(expected 9 digits)";
            acSystemDestroyDevice(hSystem, packet.hDevice);
            continue;
        }

        // Check that all characters are digits
        bool allDigits = true;
        for (int i = 0; i < packet.serialString.length(); i++) {
            if (!packet.serialString.at(i).isDigit()) {
                allDigits = false;
                break;
            }
        }

        if (!allDigits) {
            qDebug() << "Skipping device with non-numeric serial number:" << packet.serialString << "(expected all digits)";
            acSystemDestroyDevice(hSystem, packet.hDevice);
            continue;
        }

        // GET CAMERA INTRINSICS
        err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibFocalLengthX", &packet.deviceIntrinsics.fx);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting focal length X: %1").arg(errorMessages(err));
            return;
        }

        err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibFocalLengthY", &packet.deviceIntrinsics.fy);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting focal length X: %1").arg(errorMessages(err));
            return;
        }

        err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibOpticalCenterX", &packet.deviceIntrinsics.cx);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting focal length X: %1").arg(errorMessages(err));
            return;
        }

        err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibOpticalCenterY", &packet.deviceIntrinsics.cy);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting focal length X: %1").arg(errorMessages(err));
            return;
        }

        // set Cust::CalibLensDistortionValueSelector 0
        // Note: Distortion coefficients may not be accessible if camera lacks calibration data
        // In that case, we'll use zero values (no distortion correction)
        bool distortionAvailable = true;
        for (int n = 0; n < 8; n++) {
            switch (n) {
                case 0:
                    err = acNodeMapSetStringValue(packet.hNodeMap, "Cust::CalibLensDistortionValueSelector", "Value0");
                    break;
                case 1:
                    err = acNodeMapSetStringValue(packet.hNodeMap, "Cust::CalibLensDistortionValueSelector", "Value1");
                    break;
                case 2:
                    err = acNodeMapSetStringValue(packet.hNodeMap, "Cust::CalibLensDistortionValueSelector", "Value2");
                    break;
                case 3:
                    err = acNodeMapSetStringValue(packet.hNodeMap, "Cust::CalibLensDistortionValueSelector", "Value3");
                    break;
                case 4:
                    err = acNodeMapSetStringValue(packet.hNodeMap, "Cust::CalibLensDistortionValueSelector", "Value4");
                    break;
                case 5:
                    err = acNodeMapSetStringValue(packet.hNodeMap, "Cust::CalibLensDistortionValueSelector", "Value5");
                    break;
                case 6:
                    err = acNodeMapSetStringValue(packet.hNodeMap, "Cust::CalibLensDistortionValueSelector", "Value6");
                    break;
                case 7:
                    err = acNodeMapSetStringValue(packet.hNodeMap, "Cust::CalibLensDistortionValueSelector", "Value7");
                    break;
            }
            if (err != AC_ERR_SUCCESS) {
                // If we can't access distortion values on first coefficient, camera likely has no calibration data
                // Set all distortion coefficients to zero and continue
                if (n == 0) {
                    qWarning() << "Lucid camera distortion coefficients not accessible (camera may lack calibration data). Using zero distortion.";
                    packet.deviceIntrinsics.k1 = 0.0;
                    packet.deviceIntrinsics.k2 = 0.0;
                    packet.deviceIntrinsics.p1 = 0.0;
                    packet.deviceIntrinsics.p2 = 0.0;
                    packet.deviceIntrinsics.k3 = 0.0;
                    packet.deviceIntrinsics.k4 = 0.0;
                    packet.deviceIntrinsics.k5 = 0.0;
                    packet.deviceIntrinsics.k6 = 0.0;
                    distortionAvailable = false;
                    break;
                }
                if (n < 5){
                    errorString = QString("Lucid Error setting lens distortion value selector %1: %2").arg(n).arg(errorMessages(err));
                    return;
                }
            }
            switch (n) {
                case 0:
                    err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibLensDistortionValue", &packet.deviceIntrinsics.k1);
                    break;
                case 1:
                    err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibLensDistortionValue", &packet.deviceIntrinsics.k2);
                    break;
                case 2:
                    err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibLensDistortionValue", &packet.deviceIntrinsics.p1);
                    break;
                case 3:
                    err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibLensDistortionValue", &packet.deviceIntrinsics.p2);
                    break;
                case 4:
                    err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibLensDistortionValue", &packet.deviceIntrinsics.k3);
                    break;
                case 5:
                    err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibLensDistortionValue", &packet.deviceIntrinsics.k4);
                    break;
                case 6:
                    err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibLensDistortionValue", &packet.deviceIntrinsics.k5);
                    break;
                case 7:
                    err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::CalibLensDistortionValue", &packet.deviceIntrinsics.k6);
                    break;
            }
            if (err != AC_ERR_SUCCESS) {
                if (n < 5){
                    errorString = QString("Lucid Error getting lens distortion value %1: %2").arg(n).arg(errorMessages(err));
                    return;
                } else {
                    packet.deviceIntrinsics.k4 = 0.0;
                    packet.deviceIntrinsics.k5 = 0.0;
                    packet.deviceIntrinsics.k6 = 0.0;
                }
            }
        }

        if (hasDepthVideo){
            pBufLen = 256;
            err = acNodeMapGetStringValue(packet.hNodeMap, "PixelFormat", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting pixel format: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("Coord3D_C16")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "PixelFormat", "Coord3D_C16");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting pixel format: %1").arg(errorMessages(err));
                    return;
                }
            }

            // 6 MODES: (1) 1250 MM, (2) 3000 MM, (3) 4000 MM, (4) 5000 MM, (5) 6000 MM, (6) 8300 MM
            pBufLen = 256;
            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "Scan3dOperatingMode", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting operating mode: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("Distance5000mmMultiFreq")) {
                if (SetNodeValue(packet.hNodeMap, "Scan3dOperatingMode", "Distance5000mmMultiFreq") == false) {
                    errorString = QString("Lucid Error setting operating mode: %1").arg(errorMessages(err));
                    return;
                }
            }

            pBufLen = 256;
            err = acNodeMapGetStringValue(packet.hNodeMap, "Scan3dCoordinateSelector", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting scan 3d coordinate selector: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("CoordinateC")) {
                err = acNodeMapSetStringValue(packet.hNodeMap, "Scan3dCoordinateSelector", "CoordinateC");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting scan 3d coordinate selector: %1").arg(errorMessages(err));
                    return;
                }
            }

            // GET THE Z COORDINATE SCALE IN ORDER TO CONVERT Z VALUES TO MM
            err = acNodeMapGetFloatValue(packet.hNodeMap, "Scan3dCoordinateScale", &(packet.scaleFactor));
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting depth scale: %1").arg(errorMessages(err));
                return;
            }
       } else {
            pBufLen = 256;
            err = acNodeMapGetStringValue(packet.hNodeMap, "PixelFormat", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting pixel format: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("Mono16")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "PixelFormat", "Mono16");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting pixel format: %1").arg(errorMessages(err));
                    return;
                }
            }
        }
        cameras << packet;
    }

    // SORT THE CAMERAS
    sortCameras();

#ifdef ORBBEC
    int64_t throughput = 125000000 / (cameras.count() + 1);
#else
    int64_t throughput = 125000000 / cameras.count();
#endif
    Q_UNUSED(throughput);

    // ITERATE THROUGH SORTED CAMERA LIST
    for (int n = 0; n < cameras.length(); n++) {
        // GRAB CURRENT CAMERA
        CameraPacket packet = cameras.at(n);

        pBufLen = 256;
        err = acNodeMapGetEnumerationValue(packet.hNodeMap, "DeviceLinkThroughputLimitMode", pDeviceParameterBuf, &pBufLen);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting DeviceLink throughput limit mode: %1").arg(errorMessages(err));
            return;
        } else if (QString(pDeviceParameterBuf) != QString("Off")) {
            err = acNodeMapSetEnumerationValue(packet.hNodeMap, "DeviceLinkThroughputLimitMode", "Off");
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting DeviceLink throughput limit mode: %1").arg(errorMessages(err));
                return;
            }
        }

        // // get latch
        // int64_t deviceLinkThroughput = 0;

        // err = acNodeMapGetIntegerValue(packet.hNodeMap, "DeviceLinkThroughputLimit", &deviceLinkThroughput);
        // if (err != AC_ERR_SUCCESS) {
        //     errorString = QString("Lucid Error getting DeviceLink throughput limit: %1").arg(errorMessages(err));
        //     return;
        // } else if (deviceLinkThroughput != throughput) {
        //     err = acNodeMapSetIntegerValue(packet.hNodeMap, "DeviceLinkThroughputLimit", throughput);
        //     if (err != AC_ERR_SUCCESS) {
        //         errorString = QString("Lucid Error setting DeviceLink throughput limit: %1").arg(errorMessages(err));
        //         return;
        //     }
        // }

#if defined(LUCID_USEPTPCOMMANDS)
        err = acNodeMapGetBooleanValue(packet.hNodeMap, "PtpEnable", &pDeviceBool);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting PTP enable: %1").arg(errorMessages(err));
            return;
        } else if (pDeviceBool == false) {
            err = acNodeMapSetBooleanValue(packet.hNodeMap, "PtpEnable", true);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting PTP enable to true: %1").arg(errorMessages(err));
                return;
            }

            pBufLen = 256;
            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "TriggerSelector", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting trigger selector: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("FrameStart")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "TriggerSelector", "FrameStart");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting trigger selector: %1").arg(errorMessages(err));
                    return;
                }
            }

            pBufLen = 256;
            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "TriggerSource", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting trigger source: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("Action0")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "TriggerSource", "Action0");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting trigger source to Action0: %1").arg(errorMessages(err));
                    return;
                }
            }

            pBufLen = 256;
            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "TriggerMode", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting trigger mode: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("On")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "TriggerMode", "On");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting trigger mode to On: %1").arg(errorMessages(err));
                    return;
                }
            }

            pBufLen = 256;
            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "ActionUnconditionalMode", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting action unconditional mode to on: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("On")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ActionUnconditionalMode", "On");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting action unconditional mode to on: %1").arg(errorMessages(err));
                    return;
                }

                err = acNodeMapSetIntegerValue(packet.hNodeMap, "ActionSelector", 0);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting action selector to 0: %1").arg(errorMessages(err));
                    return;
                }

                err = acNodeMapSetIntegerValue(packet.hNodeMap, "ActionDeviceKey", 1);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting action device key to 1: %1").arg(errorMessages(err));
                    return;
                }

                err = acNodeMapSetIntegerValue(packet.hNodeMap, "ActionGroupKey", 1);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting action group key to 1: %1").arg(errorMessages(err));
                    return;
                }

                err = acNodeMapSetIntegerValue(packet.hNodeMap, "ActionGroupMask", 1);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting action group mask to 1: %1").arg(errorMessages(err));
                    return;
                }
            }
        }

        if (n == 0) {
            err = acNodeMapGetBooleanValue(packet.hNodeMap, "PtpSlaveOnly", &pDeviceBool);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting PTP enable: %1").arg(errorMessages(err));
                return;
            } else {
                if (pDeviceBool != false) {
                    err = acNodeMapSetBooleanValue(packet.hNodeMap, "PtpSlaveOnly", false);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting PTP enable to false: %1").arg(errorMessages(err));
                        return;
                    }
                    while (pDeviceBool != false) {
                        err = acNodeMapGetBooleanValue(packet.hNodeMap, "PtpSlaveOnly", &pDeviceBool);
                        if (err != AC_ERR_SUCCESS) {
                            errorString = QString("Lucid Error getting PTP enable: %1").arg(errorMessages(err));
                            return;
                        }
                    }
                }
            }
        } else {
            err = acNodeMapGetBooleanValue(packet.hNodeMap, "PtpSlaveOnly", &pDeviceBool);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting PTP enable: %1").arg(errorMessages(err));
                return;
            } else {
                if (pDeviceBool != true) {
                    err = acNodeMapSetBooleanValue(packet.hNodeMap, "PtpSlaveOnly", true);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting PTP enable to false: %1").arg(errorMessages(err));
                        return;
                    }

                    while (pDeviceBool != true) {
                        err = acNodeMapGetBooleanValue(packet.hNodeMap, "PtpSlaveOnly", &pDeviceBool);
                        if (err != AC_ERR_SUCCESS) {
                            errorString = QString("Lucid Error getting PTP enable: %1").arg(errorMessages(err));
                            return;
                        }
                    }
                }
            }
        }

        pBufLen = 256;
        err = acNodeMapGetEnumerationValue(packet.hNodeMap, "AcquisitionStartMode", pDeviceParameterBuf, &pBufLen);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting acquisition start mode: %1").arg(errorMessages(err));
            return;
        } else if (QString(pDeviceParameterBuf) != QString("PTPSync")) {
            err = acNodeMapSetEnumerationValue(packet.hNodeMap, "AcquisitionStartMode", "PTPSync");
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting acquisition start mode: %1").arg(errorMessages(err));
                return;
            }
        }

        double fValue = 0.0;
        err = acNodeMapGetFloatValue(packet.hNodeMap, "Cust::PTPSyncFrameRate", &fValue);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting PTP sync frame rate: %1").arg(errorMessages(err));
            return;
        } else if (fValue != 25.0f) {
            err = acNodeMapSetFloatValue(packet.hNodeMap, "Cust::PTPSyncFrameRate", 25.0f);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting PTP sync frame rate: %1").arg(errorMessages(err));
                return;
            }
        }
#elif defined(LUCID_SYNCBYGPIO)
        err = acNodeMapGetBooleanValue(packet.hNodeMap, "PtpEnable", &pDeviceBool);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting PTP enable: %1").arg(errorMessages(err));
            return;
        } else if (pDeviceBool == true) {
            err = acNodeMapSetBooleanValue(packet.hNodeMap, "PtpEnable", false);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting PTP enable to false: %1").arg(errorMessages(err));
                return;
            }
        }

        // SET COMMUNICATION CHANNEL
        if (n == 0) {
            pBufLen = 256;
            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "LineSelector", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting line selector: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("Line1")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "LineSelector", "Line1");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting line selector to Line1: %1").arg(errorMessages(err));
                    return;
                }
            }

            pBufLen = 256;
            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "LineSource", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting line source: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("ExposureActive")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "LineSource", "ExposureActive");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting line source to ExposureActive: %1").arg(errorMessages(err));
                    return;
                }
            }

            pBufLen = 256;
            err = acNodeMapGetStringValue(packet.hNodeMap, "LineSelector", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting line selector: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("Line4")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "LineSelector", "Line4");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting line selector to Line4: %1").arg(errorMessages(err));
                    return;
                }
            }

            err = acNodeMapGetBooleanValue(packet.hNodeMap, "VoltageExternalEnable", &pDeviceBool);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting voltage external enable: %1").arg(errorMessages(err));
                return;
            } else if (pDeviceBool == false) {
                err = acNodeMapSetBooleanValue(packet.hNodeMap, "VoltageExternalEnable", true);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting voltage external enable to true: %1").arg(errorMessages(err));
                    return;
                }
            }

            err = acNodeMapGetBooleanValue(packet.hNodeMap, "AcquisitionFrameRateEnable", &pDeviceBool);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting acquisition frame rate enable: %1").arg(errorMessages(err));
                return;
            } else if (pDeviceBool == false) {
                err = acNodeMapSetBooleanValue(packet.hNodeMap, "AcquisitionFrameRateEnable", true);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting acquisition frame rate enable to true: %1").arg(errorMessages(err));
                    return;
                }
            }

            // GET THE Z COORDINATE SCALE IN ORDER TO CONVERT Z VALUES TO MM
            double fValue = 0.0;
            err = acNodeMapGetFloatValue(packet.hNodeMap, "AcquisitionFrameRate", &fValue);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting acquisition frame rate: %1").arg(errorMessages(err));
                return;
            } else if (fValue < 29.0f) {
                err = acNodeMapSetFloatValue(packet.hNodeMap, "AcquisitionFrameRate", 29.0f);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting acquisition frame rate: %1").arg(errorMessages(err));
                    return;
                }
            }

            pBufLen = 256;
            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "TriggerMode", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting trigger mode: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("Off")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "TriggerMode", "Off");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting trigger mode to Off: %1").arg(errorMessages(err));
                    return;
                }
            }
        } else {
            pBufLen = 256;
            err = acNodeMapGetStringValue(packet.hNodeMap, "TriggerSelector", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting trigger selector: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("FrameStart")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "TriggerSelector", "FrameStart");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting trigger selector: %1").arg(errorMessages(err));
                    return;
                }
            }

            pBufLen = 256;
            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "TriggerSource", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting trigger source: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("Line0")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "TriggerSource", "Line0");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting trigger source to Line0: %1").arg(errorMessages(err));
                    return;
                }
            }

            pBufLen = 256;
            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "TriggerMode", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting trigger mode: %1").arg(errorMessages(err));
                return;
            } else if (QString(pDeviceParameterBuf) != QString("On")) {
                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "TriggerMode", "On");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting trigger mode to On: %1").arg(errorMessages(err));
                    return;
                }
            }
        }
#else
        err = acNodeMapGetBooleanValue(packet.hNodeMap, "PtpEnable", &pDeviceBool);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting PTP enable: %1").arg(errorMessages(err));
            return;
        } else if (pDeviceBool == true) {
            err = acNodeMapSetBooleanValue(packet.hNodeMap, "PtpEnable", false);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting PTP enable to false: %1").arg(errorMessages(err));
                return;
            }
        }

        pBufLen = 256;
        err = acNodeMapGetEnumerationValue(packet.hNodeMap, "TriggerMode", pDeviceParameterBuf, &pBufLen);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting trigger mode: %1").arg(errorMessages(err));
            return;
        } else if (QString(pDeviceParameterBuf) != QString("Off")) {
            err = acNodeMapSetEnumerationValue(packet.hNodeMap, "TriggerMode", "Off");
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting trigger mode off: %1").arg(errorMessages(err));
                return;
            }
        }

        pBufLen = 256;
        err = acNodeMapGetEnumerationValue(packet.hNodeMap, "AcquisitionMode", pDeviceParameterBuf, &pBufLen);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting trigger mode: %1").arg(errorMessages(err));
            return;
        } else if (QString(pDeviceParameterBuf) != QString("Continuous")) {
            err = acNodeMapSetEnumerationValue(packet.hNodeMap, "AcquisitionMode", "Continuous");
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting trigger mode off: %1").arg(errorMessages(err));
                return;
            }
        }

        err = acNodeMapGetBooleanValue(packet.hNodeMap, "AcquisitionFrameRateEnable", &pDeviceBool);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting acquisition frame rate enable: %1").arg(errorMessages(err));
            return;
        } else if (pDeviceBool == false) {
            err = acNodeMapSetBooleanValue(packet.hNodeMap, "AcquisitionFrameRateEnable", true);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting acquisition frame rate enable to true: %1").arg(errorMessages(err));
                return;
            }
        }

        double fValue = 0.0;
        err = acNodeMapGetFloatValue(packet.hNodeMap, "AcquisitionFrameRate", &fValue);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting acquisition frame rate: %1").arg(errorMessages(err));
            return;
        } else if (fValue != 29.2f) {
            err = acNodeMapSetFloatValue(packet.hNodeMap, "AcquisitionFrameRate", 29.2f);
            if (err != AC_ERR_SUCCESS) {
                err = acNodeMapSetFloatValue(packet.hNodeMap, "AcquisitionFrameRate", 10.0f);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting acquisition frame rate: %1").arg(errorMessages(err));
                    return;
                }
            }
        }
#endif

        if (hasDepth()){
            // SET COMMUNICATION CHANNEL
            if (n == 0){
                if (packet.serialString == QString("210300864")){
                    // SKIP SETTING THE COMMUNICATION CHANNEL BECAUSE IT DOESN'T HAVE THIS FEATURE
                } else {
                    err = acNodeMapGetIntegerValue(packet.hNodeMap, "Scan3dCommunicationChannel", &pDeviceInt);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error getting communication channel: %1").arg(errorMessages(err));
                        return;
                    } else if (pDeviceInt != 0) {
                        err = acNodeMapSetIntegerValue(packet.hNodeMap, "Scan3dCommunicationChannel", 0);
                        if (err != AC_ERR_SUCCESS) {
                            errorString = QString("Lucid Error setting communication channel: %1").arg(errorMessages(err));
                            return;
                        }
                    }
                }

                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ConversionGain", "Low");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting conversion gain: %1").arg(errorMessages(err));
                    return;
                }

                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ExposureTimeSelector", "Exp1000Us"); //250Us");
                if (err != AC_ERR_SUCCESS) {
                    err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ExposureTimeSelector", "Exp750Us"); //250Us");
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting exposure time selector: %1").arg(errorMessages(err));
                        return;
                    }
                }

#ifdef LUCID_SYNCBYGPIO
                double fValue = 0.0;
                err = acNodeMapGetFloatValue(packet.hNodeMap, "TriggerDelay", &fValue);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error getting trigger delay: %1").arg(errorMessages(err));
                    return;
                } else if (fValue != 0.0f) {
                    err = acNodeMapSetFloatValue(packet.hNodeMap, "TriggerDelay", 0.0f);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting acquisition frame rate: %1").arg(errorMessages(err));
                        return;
                    }
                }
#endif
            } else if (n == 1){ //packet.serialString == QString("221300900")) {
                err = acNodeMapGetIntegerValue(packet.hNodeMap, "Scan3dCommunicationChannel", &pDeviceInt);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error getting communication channel: %1").arg(errorMessages(err));
                    return;
                } else if (pDeviceInt != 1) {
                    err = acNodeMapSetIntegerValue(packet.hNodeMap, "Scan3dCommunicationChannel", 1);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting communication channel: %1").arg(errorMessages(err));
                        return;
                    }
                }

                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ConversionGain", "Low");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting conversion gain: %1").arg(errorMessages(err));
                    return;
                }

                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ExposureTimeSelector", "Exp1000Us"); //250Us");
                if (err != AC_ERR_SUCCESS) {
                    err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ExposureTimeSelector", "Exp750Us"); //250Us");
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting exposure time selector: %1").arg(errorMessages(err));
                        return;
                    }
                }

#ifdef LUCID_SYNCBYGPIO
                double fValue = 0.0;
                err = acNodeMapGetFloatValue(packet.hNodeMap, "TriggerDelay", &fValue);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error getting trigger delay: %1").arg(errorMessages(err));
                    return;
                } else if (fValue != 0.0f) {
                    err = acNodeMapSetFloatValue(packet.hNodeMap, "TriggerDelay", 0.0f);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting acquisition frame rate: %1").arg(errorMessages(err));
                        return;
                    }
                }
#endif
            } else if (n == 2){ //packet.serialString == QString("221300912")) {
                err = acNodeMapGetIntegerValue(packet.hNodeMap, "Scan3dCommunicationChannel", &pDeviceInt);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error getting communication channel: %1").arg(errorMessages(err));
                    return;
                } else if (pDeviceInt != 2) {
                    err = acNodeMapSetIntegerValue(packet.hNodeMap, "Scan3dCommunicationChannel", 2);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting communication channel: %1").arg(errorMessages(err));
                        return;
                    }
                }

                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ConversionGain", "High");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting conversion gain: %1").arg(errorMessages(err));
                    return;
                }

                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ExposureTimeSelector", "Exp1000Us"); //250Us");
                if (err != AC_ERR_SUCCESS) {
                    err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ExposureTimeSelector", "Exp750Us"); //250Us");
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting exposure time selector: %1").arg(errorMessages(err));
                        return;
                    }
                }

#ifdef LUCID_SYNCBYGPIO
                double fValue = 0.0;
                err = acNodeMapGetFloatValue(packet.hNodeMap, "TriggerDelay", &fValue);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error getting trigger delay: %1").arg(errorMessages(err));
                    return;
                } else if (fValue != 0.0f) {
                    err = acNodeMapSetFloatValue(packet.hNodeMap, "TriggerDelay", 0.0f);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting acquisition frame rate: %1").arg(errorMessages(err));
                        return;
                    }
                }
#endif
            } else if (n == 3){ //packet.serialString == QString("221300954")) {
                err = acNodeMapGetIntegerValue(packet.hNodeMap, "Scan3dCommunicationChannel", &pDeviceInt);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error getting communication channel: %1").arg(errorMessages(err));
                    return;
                } else if (pDeviceInt != 3) {
                    err = acNodeMapSetIntegerValue(packet.hNodeMap, "Scan3dCommunicationChannel", 3);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting communication channel: %1").arg(errorMessages(err));
                        return;
                    }
                }

                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ConversionGain", "Low");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error setting conversion gain: %1").arg(errorMessages(err));
                    return;
                }

                err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ExposureTimeSelector", "Exp1000Us"); //250Us");
                if (err != AC_ERR_SUCCESS) {
                    err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ExposureTimeSelector", "Exp750Us"); //250Us");
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting exposure time selector: %1").arg(errorMessages(err));
                        return;
                    }
                }

#ifdef LUCID_SYNCBYGPIO
                double fValue = 0.0;
                err = acNodeMapGetFloatValue(packet.hNodeMap, "TriggerDelay", &fValue);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error getting trigger delay: %1").arg(errorMessages(err));
                    return;
                } else if (fValue != 0.0f) {
                    err = acNodeMapSetFloatValue(packet.hNodeMap, "TriggerDelay", 0.0f);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error setting acquisition frame rate: %1").arg(errorMessages(err));
                        return;
                    }
                }
#endif
            }

            // 6 MODES: (1) 1250 MM, (2) 3000 MM, (3) 4000 MM, (4) 5000 MM, (5) 6000 MM, (6) 8300 MM
            pBufLen = 256;
            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "Scan3dOperatingMode", pDeviceParameterBuf, &pBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting operating mode: %1").arg(errorMessages(err));
                return;
            } else {
                if (rangeModeString.contains("1250")) {
                    if (QString(pDeviceParameterBuf) != QString("Distance1250mmSingleFreq")) {
                        if (SetNodeValue(packet.hNodeMap, "Scan3dOperatingMode", "Distance1250mmSingleFreq") == false) {
                            errorString = QString("Lucid Error setting operating mode: %1").arg(errorMessages(err));
                            return;
                        }
                    }
                } else if (rangeModeString.contains("3000")) {
                    if (QString(pDeviceParameterBuf) != QString("Distance3000mmSingleFreq")) {
                        if (SetNodeValue(packet.hNodeMap, "Scan3dOperatingMode", "Distance3000mmSingleFreq") == false) {
                            errorString = QString("Lucid Error setting operating mode: %1").arg(errorMessages(err));
                            return;
                        }
                    }
                } else if (rangeModeString.contains("4000")) {
                    if (QString(pDeviceParameterBuf) != QString("Distance4000mmSingleFreq")) {
                        if (SetNodeValue(packet.hNodeMap, "Scan3dOperatingMode", "Distance4000mmSingleFreq") == false) {
                            errorString = QString("Lucid Error setting operating mode: %1").arg(errorMessages(err));
                            return;
                        }
                    }
                } else if (rangeModeString.contains("5000")) {
                    if (QString(pDeviceParameterBuf) != QString("Distance5000mmMultiFreq")) {
                        if (SetNodeValue(packet.hNodeMap, "Scan3dOperatingMode", "Distance5000mmMultiFreq") == false) {
                            errorString = QString("Lucid Error setting operating mode: %1").arg(errorMessages(err));
                            return;
                        }
                    }
                } else if (rangeModeString.contains("6000")) {
                    if (QString(pDeviceParameterBuf) != QString("Distance6000mmSingleFreq")) {
                        if (SetNodeValue(packet.hNodeMap, "Scan3dOperatingMode", "Distance6000mmSingleFreq") == false) {
                            errorString = QString("Lucid Error setting operating mode: %1").arg(errorMessages(err));
                            return;
                        }
                    }
                } else if (rangeModeString.contains("8300")) {
                    if (QString(pDeviceParameterBuf) != QString("Distance8300mmMultiFreq")) {
                        if (SetNodeValue(packet.hNodeMap, "Scan3dOperatingMode", "Distance8300mmMultiFreq") == false) {
                            errorString = QString("Lucid Error setting operating mode: %1").arg(errorMessages(err));
                            return;
                        }
                    }
                }
            }
        }

        // err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ExposureTimeSelector", "Exp1000Us"); //250Us");
        // if (err != AC_ERR_SUCCESS) {
        //     err = acNodeMapSetEnumerationValue(packet.hNodeMap, "ExposureTimeSelector", "Exp750Us"); //250Us");
        //     if (err != AC_ERR_SUCCESS) {
        //         errorString = QString("Lucid Error setting exposure time selector: %1").arg(errorMessages(err));
        //         return;
        //     }
        // }

        err = acDeviceGetTLStreamNodeMap(packet.hDevice, &packet.hTLStreamNodeMap);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting device stream node map: %1").arg(errorMessages(err));
            return;
        }

        // ENABLE STREAM AUTO NEGOTIATE PACKET SIZE
        pDeviceBool = false;
        err = acNodeMapGetBooleanValue(packet.hTLStreamNodeMap, "StreamAutoNegotiatePacketSize", &pDeviceBool);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting auto-negotiate packet size enable: %1").arg(errorMessages(err));
            return;
        } else if (pDeviceBool != true) {
            err = acNodeMapSetBooleanValue(packet.hTLStreamNodeMap, "StreamAutoNegotiatePacketSize", true);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting auto-negotiate packet size enabled: %1").arg(errorMessages(err));
                return;
            }
        }

        // ENABLE STREAM PACKET RESEND
        pDeviceBool = false;
        err = acNodeMapGetBooleanValue(packet.hTLStreamNodeMap, "StreamPacketResendEnable", &pDeviceBool);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting stream packet resend enable: %1").arg(errorMessages(err));
            return;
        } else if (pDeviceBool != true) {
            err = acNodeMapSetBooleanValue(packet.hTLStreamNodeMap, "StreamPacketResendEnable", true);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting stream packet resend enable: %1").arg(errorMessages(err));
                return;
            }
        }

        // SET BUFFER HANDLING MODE
        pBufLen = 256;
        err = acNodeMapGetEnumerationValue(packet.hTLStreamNodeMap, "StreamBufferHandlingMode", pDeviceParameterBuf, &pBufLen);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting action unconditional mode to on: %1").arg(errorMessages(err));
            return;
        } else if (QString(pDeviceParameterBuf) != QString("NewestOnly")) {
            if (SetNodeValue(packet.hTLStreamNodeMap, "StreamBufferHandlingMode", "NewestOnly") == false) {
                errorString = QString("Lucid Error setting stream buffer handling mode: %1").arg(errorMessages(err));
                return;
            }
        }

        packet.isConnected = true;
        cameras.replace(n, packet);
    }

    // PTP synchronization and stream starting now happens in onThreadStart()
    // which is called when the camera is moved to its worker thread

    isConnected = true;

    bitsPerPixel = 12;
    zMinDistance = static_cast<unsigned short>(qRound(33.0));
    zMaxDistance = static_cast<unsigned short>(qRound(8400.0));
    numDepthCols = LUCIDDEPTHSENSORWIDTH;
    numDepthRows = LUCIDDEPTHSENSORHEIGHT;
    numColorCols = LUCIDCOLORSENSORWIDTH;
    numColorRows = LUCIDCOLORSENSORHEIGHT;
    horizontalFieldOfView = LUCIDDEPTHSENSORHFOV;
    verticalFieldOfView = LUCIDDEPTHSENSORVFOV;
    if (hasDepth() == false){
        zMaxDistance = (1 << bitsPerPixel) - 1;
    }
#else
    errorString = QString("No devices found!");
#endif

    // RESET THE INTERNAL TIMER
    restart();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAULucidCamera::onThreadStart()
{
#ifndef Q_OS_MAC
    AC_ERROR err;

#ifdef LUCID_USEPTPCOMMANDS
    qInfo() << "Configuring" << cameras.count() << "Lucid camera(s) for PTP synchronization...";
    qInfo() << "This may take 30-60 seconds. Please wait...";

    for (int n = 0; n < cameras.count(); n++) {
        CameraPacket packet = cameras.at(n);

#ifdef LUCID_USERCONTROLLEDTRANSFER
        char pDeviceParameterBuf[256];
        size_t pBufLen = 256;
        err = acNodeMapGetEnumerationValue(packet.hTLStreamNodeMap, "TransferControlMode", pDeviceParameterBuf, &pBufLen);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting transfer control mode: %1").arg(errorMessages(err));
            emit emitError(errorString);
            return;
        } else if (QString(pDeviceParameterBuf) != QString("UserControlled")) {
            if (SetNodeValue(packet.hTLStreamNodeMap, "TransferControlMode", "UserControlled") == false) {
                errorString = QString("Lucid Error setting  transfer control mode: %1").arg(errorMessages(err));
                emit emitError(errorString);
                return;
            }
        }

        err = acNodeMapGetEnumerationValue(packet.hTLStreamNodeMap, "TransferOperationMode", pDeviceParameterBuf, &pBufLen);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting transfer control mode: %1").arg(errorMessages(err));
            emit emitError(errorString);
            return;
        } else if (QString(pDeviceParameterBuf) != QString("TransferStop")) {
            if (SetNodeValue(packet.hTLStreamNodeMap, "TransferOperationMode", "TransferStop") == false) {
                errorString = QString("Lucid Error setting  transfer operation mode: %1").arg(errorMessages(err));
                emit emitError(errorString);
                return;
            }
        }

        err = acNodeMapExecute(packet.hNodeMap, "TransferStop");
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error executing transfer stop: %1").arg(errorMessages(err));
            emit emitError(errorString);
        }
#else
        // CALCULATE TIME TO SEND AN IMAGE
        int64_t packetDelay = 80000; //(int64_t)(9014.0 * 1000000000.0 / (double)throughput * 1.10);

        // SET TRANSMISSION DELAYS SO ALL FRAMES DON'T TRY TO ARRIVE AT THE SAME TIME
        long long ptpStreamChannelPacketDelay = 0;
        err = acNodeMapGetIntegerValue(packet.hNodeMap, "GevSCPD", &ptpStreamChannelPacketDelay);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting PTP stream channel packet delay: %1").arg(errorMessages(err));
            emit emitError(errorString);
            return;
        } else if (ptpStreamChannelPacketDelay != packetDelay * (cameras.count() - 1)) {
            err = acNodeMapSetIntegerValue(packet.hNodeMap, "GevSCPD", packetDelay * (cameras.count() - 1));
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting PTP stream channel packet delay: %1").arg(errorMessages(err));
                emit emitError(errorString);
                return;
            }
        }

        long long ptpStreamChannelFrameTransmissionDelay = 0;
        err = acNodeMapGetIntegerValue(packet.hNodeMap, "GevSCFTD", &ptpStreamChannelFrameTransmissionDelay);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting PTP stream channel frame transmission delay: %1").arg(errorMessages(err));
            emit emitError(errorString);
            return;
        } else if (ptpStreamChannelFrameTransmissionDelay != (packetDelay * n)) {
            err = acNodeMapSetIntegerValue(packet.hNodeMap, "GevSCFTD", packetDelay * n);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting PTP stream channel frame transmission delay: %1").arg(errorMessages(err));
                emit emitError(errorString);
                return;
            }
        }
#endif
    }

    // PREPARE SYSTEM
    long long pDeviceInt;
    err = acSystemGetTLSystemNodeMap(hSystem, &hTLSystemNodeMap);
    if (err != AC_ERR_SUCCESS) {
        errorString = QString("Lucid Error getting TLSystem Node: %1").arg(errorMessages(err));
        emit emitError(errorString);
        return;
    }

    err = acNodeMapGetIntegerValue(hTLSystemNodeMap, "ActionCommandDeviceKey", &pDeviceInt);
    if (err != AC_ERR_SUCCESS) {
        errorString = QString("Lucid Error getting action command device key: %1").arg(errorMessages(err));
        emit emitError(errorString);
        return;
    } else if (pDeviceInt != 1) {
        err = acNodeMapSetIntegerValue(hTLSystemNodeMap, "ActionCommandDeviceKey", 1);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting TLSystem Node: %1").arg(errorMessages(err));
            emit emitError(errorString);
            return;
        }

        err = acNodeMapSetIntegerValue(hTLSystemNodeMap, "ActionCommandGroupKey", 1);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error setting Action Command Group Key: %1").arg(errorMessages(err));
            emit emitError(errorString);
            return;
        }

        err = acNodeMapSetIntegerValue(hTLSystemNodeMap, "ActionCommandGroupMask", 1);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error setting Action Command Group Mask: %1").arg(errorMessages(err));
            emit emitError(errorString);
            return;
        }

        err = acNodeMapSetIntegerValue(hTLSystemNodeMap, "ActionCommandTargetIP", 0xFFFFFFFF);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error setting Action Command Target IP: %1").arg(errorMessages(err));
            emit emitError(errorString);
            return;
        }
    }

    bool masterFound = false;
    bool restartSyncCheck = false;
    int syncCheckIterations = 0;
    do {
        masterFound = false;
        restartSyncCheck = false;

        QThread::msleep(100);

        syncCheckIterations++;
        if (syncCheckIterations % 20 == 0) {
            qInfo() << "Looking for master camera (iteration" << syncCheckIterations << ")";
        }

        // check devices
        for (int n = 0; n < cameras.count(); n++) {
            CameraPacket packet = cameras.at(n);

            // get PTP status
            char ptpStatusBuf[LUCIDMAXBUF];
            size_t ptpStatusBufLen = LUCIDMAXBUF;

            err = acNodeMapGetEnumerationValue(packet.hNodeMap, "PtpStatus", ptpStatusBuf, &ptpStatusBufLen);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error getting PTP Status: %1").arg(errorMessages(err));
                emit emitError(errorString);
                return;
            }

            if (QString(ptpStatusBuf) == QString("Master")) {
                if (masterFound) {
                    // Multiple masters -- ptp negotiation is not complete
                    restartSyncCheck = true;
                    break;
                }
                masterFound = true;
            } else if (QString(ptpStatusBuf) != QString("Slave")) {
                // Uncalibrated state -- ptp negotiation is not complete
                restartSyncCheck = true;
                // Only print PTP status every 20th iteration to reduce spam
                if (syncCheckIterations % 20 == 0) {
                    qInfo() << "PTP Status:" << QString(ptpStatusBuf);
                }
                break;
            }
        }
    } while (restartSyncCheck || !masterFound);

    qInfo() << "PTP synchronization complete! Starting camera streams...";
#endif

    // START STREAM
    for (int n = 0; n < cameras.count(); n++) {
        err = acDeviceStartStream(cameras.at(n).hDevice);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error starting stream: %1").arg(errorMessages(err));
            emit emitError(errorString);
            return;
        }
    }
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAULucidCamera::sortCameras()
{
#ifndef DONTCOMPILE
    std::sort(cameras.begin(), cameras.end(), LAULucidCamera_camerasLessThan);
#else
    // SEE IF THERE IS A HASH TABLE THAT WAS ASSIGNED BY THE USER
    QHash<QString, QString> hash = LAUCameraClassifierDialog::getCameraAssignments();

    // IF THE HASH TABLE IS EMPTY, THEN JUST SORT IN ALPHABETICAL/NUMERICAL ORDER
    if (hash.isEmpty()){
        // Sort cameras by serial number (alphabetical/numerical order)
        std::sort(cameras.begin(), cameras.end(), LAULucidCamera_camerasLessThan);
    } else {
        // USE A BUBBLE SORT TO SORT THE CAMERAS FROM TOP TO TAIL
        for (int n = 0; n < cameras.length(); n++){
            for (int m = n+1; m < cameras.length(); m++){
                QString nString = hash[cameras.at(n).serialString];
                QString mString = hash[cameras.at(m).serialString];

                // SEE IF WE SHOULD PLACE CAMERA M IN FRONT OF CAMERA N
                if (mString < nString){
                    cameras.swapItemsAt(m,n);
                }
            }
        }
    }
#endif
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAULucidCamera::enableReadVideoFromDisk(bool state)
{
    readVideoFromDiskFlag = state;
    if (readVideoFromDiskFlag) {
        zMinDistance = static_cast<unsigned short>(qRound(33.0));
        zMaxDistance = static_cast<unsigned short>(qRound(8300.0));
        numDepthCols = LUCIDDEPTHSENSORWIDTH;
        numDepthRows = LUCIDDEPTHSENSORHEIGHT;
        numColorCols = LUCIDCOLORSENSORWIDTH;
        numColorRows = LUCIDCOLORSENSORHEIGHT;
        horizontalFieldOfView = LUCIDDEPTHSENSORHFOV;
        verticalFieldOfView = LUCIDDEPTHSENSORVFOV;
        bitsPerPixel = 12;

        isConnected = true;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAULucidCamera::colorMemoryObject() const
{
    if (hasColorVideo) {
        if (readVideoFromDiskFlag) {
            if (playbackColor == ColorGray || playbackColor == ColorXYZG) {
                return (LAUMemoryObject(numDepthCols, numDepthRows, 1, sizeof(unsigned short), 1));
            } else if (playbackColor == ColorRGB || playbackColor == ColorXYZRGB) {
                return (LAUMemoryObject(numDepthCols, numDepthRows, 3, sizeof(unsigned char), 1));
            }
        } else {
            if (playbackColor == ColorGray || playbackColor == ColorXYZG) {
                return (LAUMemoryObject(numDepthCols, numDepthRows, 1, sizeof(unsigned short), cameras.count()));
            } else if (playbackColor == ColorRGB || playbackColor == ColorXYZRGB) {
                return (LAUMemoryObject(numDepthCols, numDepthRows, 3, sizeof(unsigned char), cameras.count()));
            }
        }
    }
    return (LAUMemoryObject());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAULucidCamera::depthMemoryObject() const
{
    if (hasDepthVideo) {
        if (readVideoFromDiskFlag) {
            return (LAUMemoryObject(numDepthCols, numDepthRows, 1, sizeof(unsigned short), 1));
        } else {
            return (LAUMemoryObject(numDepthCols, numDepthRows, 1, sizeof(unsigned short), cameras.count()));
        }
    }
    return (LAUMemoryObject());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAULucidCamera::mappiMemoryObject() const
{
    return (LAUMemoryObject());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULucidCamera::~LAULucidCamera()
{
#if !defined(Q_OS_MAC)
    while (cameras.count()) {
        CameraPacket packet = cameras.takeFirst();

        if (packet.isConnected) {
            AC_ERROR err = acDeviceStopStream(packet.hDevice);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error stopping stream: %1").arg(errorMessages(err));
            }

            err = acNodeMapSetEnumerationValue(packet.hNodeMap, "TriggerMode", "Off");
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error setting trigger mode: %1").arg(errorMessages(err));
            }
        }

        if (packet.hDevice) {
            AC_ERROR err = acSystemDestroyDevice(hSystem, packet.hDevice);
            if (err != AC_ERR_SUCCESS) {
                errorString = QString("Lucid Error destroying device: %1").arg(errorMessages(err));
            }
        }
    }

    if (hSystem) {
        AC_ERROR err = acCloseSystem(hSystem);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error closing system: %1").arg(errorMessages(err));
        }
    }
#endif
    qDebug() << QString("LAULucidCamera::~LAULucidCamera()");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAULucidCamera::onUpdateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping)
{
    depth.constMakeElapsedInvalid();
    color.constMakeElapsedInvalid();
    mapping.constMakeElapsedInvalid();

    // KEEP TRACK OF CONSECUTIVE BAD FRAMES TO SEE IF WE LOST OUR CONNECTION
    static int badFrameCounter = 0;
    static int badTotalCounter = 0;

    // SEE IF WE HAVE ACCESS TO LIVE VIDEO OR FROM DISK
    if (readVideoFromDiskFlag) {
        // SAVE THE MEMORY OBJECTS FOR LATER
        frameObjects << LAUModalityObject(depth, color, mapping);
        onUpdateBuffer(QString(), -1);
        return;
#if !defined(Q_OS_MAC)
    } else if (cameras.count() > 0) {
        // LETS ASSUME THAT THERE WAS AN ERROR GRABBING THE NEXT FRAME
        badFrameCounter++;

#ifdef LUCID_USEPTPCOMMANDS
        AC_ERROR err;
        CameraPacket packet = cameras.first();

        // execute latch
        err = acNodeMapExecute(packet.hNodeMap, "PtpDataSetLatch");
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error executing PTP data set latch: %1, %2").arg(errorMessages(err)).arg(packet.serialString);
            emit emitError(errorString);
        }

        // get latch
        int64_t ptpDataSetLatchValue = 0;

        err = acNodeMapGetIntegerValue(packet.hNodeMap, "PtpDataSetLatchValue", &ptpDataSetLatchValue);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting PTP data set latch value: %1").arg(errorMessages(err));
            emit emitError(errorString);
        }

        err = acNodeMapSetIntegerValue(hTLSystemNodeMap, "ActionCommandTargetIP", 0xFFFFFFFF);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error setting Action Command Target IP: %1").arg(errorMessages(err));
            return;
        }

        // set execute time to future time
        err = acNodeMapSetIntegerValue(hTLSystemNodeMap, "ActionCommandExecuteTime", ptpDataSetLatchValue + (int64_t)LUCIDDELTATIME);
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error getting PTP data set latch value: %1").arg(errorMessages(err));
            emit emitError(errorString);
        }

        err = acNodeMapExecute(hTLSystemNodeMap, "ActionCommandFireCommand");
        if (err != AC_ERR_SUCCESS) {
            errorString = QString("Lucid Error firing command: %1").arg(errorMessages(err));
        } else {
            for (int cam = 0; cam < cameras.count(); cam++) {
                CameraPacket packet = cameras.at(cam);
                acBuffer hBuffer = NULL;

                // RESET ERROR STRING
                errorString = QString();

#ifdef LUCID_USERCONTROLLEDTRANSFER
                // execute latch
                err = acNodeMapExecute(packet.hNodeMap, "TransferStart");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error executing transfer start: %1").arg(errorMessages(err));
                    emit emitError(errorString);
                }
#endif
                // Initiate image transfer from current camera
                err = acDeviceGetBuffer(packet.hDevice, 3000, &hBuffer);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error getting buffer: %1").arg(errorMessages(err));
                    emit emitError(errorString);
                } else {
                    // IF WE MADE IT HERE THEN WE GRABBED A FRAME SUCCESSFULLY
                    badFrameCounter = 0;

                    // NOW WE NEED TO COPY THE INCOMING FRAME TO OUR MEMORY OBJECTS
                    if (depth.isValid()) {
                        unsigned char *buffer = NULL;
                        err = acImageGetData(hBuffer, &buffer);
                        if (err != AC_ERR_SUCCESS) {
                            errorString = QString("Lucid Error getting image buffer: %1").arg(errorMessages(err));
                            emit emitError(errorString);
                        } else {
                            depth.setConstElapsed(elapsed());
                            memcpy(depth.constFrame(cam + startingIndex), buffer, depth.block());

                            // If color buffer is also valid, copy depth data to color as well
                            if (color.isValid()) {
                                color.setConstElapsed(elapsed());
                                memcpy(color.constFrame(cam + startingIndex), buffer, qMin(color.block(), depth.block()));
                            }
                        }
                    } else if (color.isValid()) {
                        unsigned char *buffer = NULL;
                        err = acImageGetData(hBuffer, &buffer);
                        if (err != AC_ERR_SUCCESS) {
                            errorString = QString("Lucid Error getting image buffer: %1").arg(errorMessages(err));
                            emit emitError(errorString);
                        } else {
                            color.setConstElapsed(elapsed());
                            memcpy(color.constFrame(cam + startingIndex), buffer, color.block());
                        }
                    }

                    // Requeue image buffer
                    err = acDeviceRequeueBuffer(packet.hDevice, hBuffer);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error requeing image buffer: %1").arg(errorMessages(err));
                        emit emitError(errorString);
                    }
                }
#ifdef LUCID_USERCONTROLLEDTRANSFER
                // execute latch
                err = acNodeMapExecute(packet.hNodeMap, "TransferStop");
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error executing transfer stop: %1").arg(errorMessages(err));
                    emit emitError(errorString);
                }
#endif
            }
        }
#else
        for (int cam = 0; cam < cameras.count(); cam++) {
            CameraPacket packet = cameras.at(cam);

            // RESET ERROR STRING
            errorString = QString();

            for (unsigned int frame = 0; frame < frameReplicateCount; frame++) {
                // Get image
                acBuffer hBuffer = NULL;
                AC_ERROR err = acDeviceGetBuffer(packet.hDevice, 2000, &hBuffer);
                if (err != AC_ERR_SUCCESS) {
                    errorString = QString("Lucid Error getting buffer: %1").arg(errorMessages(err));
                } else {
                    // IF WE MADE IT HERE THEN WE GRABBED A FRAME SUCCESSFULLY
                    badFrameCounter = 0;

                    // get width
                    size_t width = 0;
                    err = acImageGetWidth(hBuffer, &width);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error getting image width: %1").arg(errorMessages(err));
                    }

                    // get height
                    size_t height = 0;
                    err = acImageGetHeight(hBuffer, &height);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error getting image height: %1").arg(errorMessages(err));
                    }

                    if (depth.isValid()) {
                        if (depth.width() == (unsigned int)width && depth.height() == (unsigned int)height) {
                            unsigned char *buffer = NULL;
                            err = acImageGetData(hBuffer, &buffer);
                            if (err != AC_ERR_SUCCESS) {
                                errorString = QString("Lucid Error getting image buffer: %1").arg(errorMessages(err));
                            } else {
                                depth.setConstElapsed(elapsed());
                                memcpy(depth.constFrame(cam + startingIndex), buffer, depth.block());

                                // SEE IF WE HAVE TO MULTIPLY BUFFER BY FOUR (SCALE FACTOR IS EQUAL TO 1)
                                // Apply bit shift scaling when native scale is 4x larger than target (0.25)
                                if (fabs(packet.scaleFactor / 0.25 - 4.0) < 0.001){
                                    // LETS SHIFT THE PIXELS LEFT IN ORDER TO MULTIPLY BY FOUR
                                    unsigned short *buffer = (unsigned short*)depth.constFrame(cam + startingIndex);
                                    int numPixels = (int)(depth.height() * depth.width());
                                    for (int c = 0; c < numPixels; c += 8) {
                                        _mm_store_si128((__m128i *)((unsigned short *)(buffer + c)), _mm_slli_epi16(_mm_load_si128((__m128i *)(buffer + c)), 2));
                                    }
                                }

                                // If color buffer is also valid, copy depth data to color as well
                                if (color.isValid()) {
                                    if (color.width() == depth.width() && color.height() == depth.height()) {
                                        color.setConstElapsed(elapsed());
                                        memcpy(color.constFrame(cam + startingIndex), depth.constFrame(cam + startingIndex), qMin(color.block(), depth.block()));
                                    }
                                }
                            }

                            // get timestamp
                            //uint64_t timestamp = 0;
                            //err = acImageGetTimestamp(hBuffer, &timestamp);
                            //if (err != AC_ERR_SUCCESS) {
                            //    errorString = QString("Lucid Error getting time stamp: %1").arg(errorMessages(err));
                            //}
                            //depth.setConstElapsed((unsigned int)timestamp);
                        } else {
                            errorString = QString("Error, incoming buffer is not the same size as depth buffer (%1 x %2)").arg(width).arg(height);
                        }
                    } else if (color.isValid()) {
                        if (color.width() == (unsigned int)width && color.height() == (unsigned int)height) {
                            unsigned char *buffer = NULL;
                            err = acImageGetData(hBuffer, &buffer);
                            if (err != AC_ERR_SUCCESS) {
                                errorString = QString("Lucid Error getting image buffer: %1").arg(errorMessages(err));
                            } else {
                                color.setConstElapsed(elapsed());
                                memcpy(color.constFrame(cam + startingIndex), buffer, color.block());
                            }

                            // get timestamp
                            //uint64_t timestamp = 0;
                            //err = acImageGetTimestamp(hBuffer, &timestamp);
                            //if (err != AC_ERR_SUCCESS) {
                            //    errorString = QString("Lucid Error getting time stamp: %1").arg(errorMessages(err));
                            //}
                            //color.setConstElapsed((unsigned int)timestamp);
                        } else {
                            errorString = QString("Error, incoming buffer is not the same size as color buffer (%1 x %2)").arg(width).arg(height);
                        }
                    }

                    // Requeue image buffer
                    err = acDeviceRequeueBuffer(packet.hDevice, hBuffer);
                    if (err != AC_ERR_SUCCESS) {
                        errorString = QString("Lucid Error requeing image buffer: %1").arg(errorMessages(err));
                    }
                }
                if (errorString.isEmpty() == false) {
                    emit emitError(errorString);
                }
            }
        }
#endif
        //depth.save("C:/Users/Public/depth.tif");
#endif
    }

    // SEE IF WE HAD ANY BAD FRAMES
    if (badFrameCounter > 0){
        badTotalCounter++;
        if (badTotalCounter > 20){
            qApp->exit(100);
        }
    }

    // SEE IF WE GRABBED FIVE BAD FRAMES IN A ROW AND IF SO WE NEED TO QUIT THE PROGRAM
    if (badFrameCounter > 5){
        qApp->exit(100);
    } else if (depth.isValid() && startingIndex == 0) {
        //depth.rotateFrame180InPlace(0);
    }

    // SEND THE USER BUFFER TO THE NEXT STAGE
    emit emitBuffer(depth, color, mapping);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAULucidCamera::onUpdateBuffer(QString filename, int frame)
{
    if (filename.isEmpty() == false) {
        FramePacket packet = {filename, frame};
        framePackets << packet;
    }

    if (framePackets.count() > 0 && frameObjects.count() > 0) {
        // GRAB THE NEXT AVAILABLE FRAME PACKET
        FramePacket packet = framePackets.takeFirst();

        // CHECK TO SEE IF WE NEED TO UPDATE THE GREENSCREEN FROM THE FIRST DIRECTORY IN A NEW FILE
        static QString lastFileString = QString();
        if (lastFileString != packet.filename) {
            emit emitBackgroundTexture(LAUMemoryObject(packet.filename, 0));
        }
        lastFileString = packet.filename;

        if (hasDepth()){
            // LOAD THE NEXT AVAILABLE FRAME FROM DISK
            if (depthBuffer.isNull()) {
                depthBuffer = LAUMemoryObject(packet.filename, packet.frame);
            } else {
                if (depthBuffer.loadInto(packet.filename, packet.frame) == false) {
                    depthBuffer = LAUMemoryObject(packet.filename, packet.frame);
                }
            }

            if (hasColor()){
                // LOAD THE NEXT AVAILABLE FRAME FROM DISK
                if (colorBuffer.isNull()) {
                    colorBuffer = LAUMemoryObject(packet.filename, packet.frame + 1);
                } else {
                    if (colorBuffer.loadInto(packet.filename, packet.frame + 1) == false) {
                        colorBuffer = LAUMemoryObject(packet.filename, packet.frame + 1);
                    }
                }
            }
        } else if (hasColor()){
            // LOAD THE NEXT AVAILABLE FRAME FROM DISK
            if (colorBuffer.isNull()) {
                colorBuffer = LAUMemoryObject(packet.filename, packet.frame);
            } else {
                if (colorBuffer.loadInto(packet.filename, packet.frame) == false) {
                    colorBuffer = LAUMemoryObject(packet.filename, packet.frame);
                }
            }
        }

        // GRAB NEXT AVAILABLE FRAME BUFFERS
        LAUModalityObject object = frameObjects.takeFirst();
        object.depth.constMakeElapsedInvalid();
        object.color.constMakeElapsedInvalid();

        // COPY OVER THE INCOMING FRAME TO THE DEPTH BUFFER
        if (depthBuffer.isValid()) {
            if (object.depth.isValid()) {
                object.depth.setConstElapsed(elapsed());
                memcpy(object.depth.constPointer(), depthBuffer.constPointer(), qMin(object.depth.length(), depthBuffer.length()));
            } else {
                object.depth.constMakeElapsedInvalid();
            }
        }

        if (colorBuffer.isValid()) {
            if (object.color.isValid()) {
                if (object.color.length() == colorBuffer.length()) {
                    object.color.setConstElapsed(elapsed());
                    memcpy(object.color.constPointer(), colorBuffer.constPointer(), colorBuffer.length());
                } else {
                    object.color.setConstElapsed(elapsed());
                    memset(object.color.constPointer(), 0x00, object.color.length());
                }
            } else {
                object.color.constMakeElapsedInvalid();
            }
            //object.color.rotateFrame180InPlace(0);
        } else {
            if (object.color.isValid()) {
                object.color.setConstElapsed(elapsed());
                memset(object.color.constPointer(), 0x00, object.color.length());
            } else {
                object.color.constMakeElapsedInvalid();
            }
        }

        // SEND THE USER BUFFER TO THE NEXT STAGE
        emit emitBuffer(object.depth, object.color, object.mappi);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable LAULucidCamera::lut(int chn, QWidget *widget) const
{
    CameraPacket packet = cameras.at(chn);
    QMatrix3x3 intParameters;
    intParameters(0, 0) = packet.deviceIntrinsics.fx; // Cust::CalibFocalLengthX
    intParameters(0, 1) = 0.0f;
    intParameters(0, 2) = packet.deviceIntrinsics.cx; // Cust::CalibOpticalCenterX

    intParameters(1, 0) = 0.0f;
    intParameters(1, 1) = packet.deviceIntrinsics.fy; // Cust::CalibFocalLengthY
    intParameters(1, 2) = packet.deviceIntrinsics.cy; // Cust::CalibOpticalCenterY

    intParameters(2, 0) = 0.0f;
    intParameters(2, 1) = 0.0f;
    intParameters(2, 2) = 1.0f;

    QVector<double> rdlParameters(6);
    rdlParameters[0] = packet.deviceIntrinsics.k1; // * 2.0f;
    rdlParameters[1] = packet.deviceIntrinsics.k2;
    rdlParameters[2] = packet.deviceIntrinsics.k3; // * 10.0f;
    rdlParameters[3] = packet.deviceIntrinsics.k4; // * 2.0f;
    rdlParameters[4] = packet.deviceIntrinsics.k5;
    rdlParameters[5] = packet.deviceIntrinsics.k6; // * 10.0f;

    QVector<double> tngParameters(2);
    tngParameters[0] = packet.deviceIntrinsics.p1;
    tngParameters[1] = packet.deviceIntrinsics.p2;

    // All cameras should use 0.25 scale factor for cascade classifier consistency
    LAULookUpTable lookUpTable(numDepthCols, numDepthRows, intParameters, rdlParameters, tngParameters, 0.25, zMinDistance, zMaxDistance, widget);
    lookUpTable.setIntrinsics(packet.deviceIntrinsics);
    lookUpTable.setMakeString(QString("Lucid"));
    lookUpTable.setModelString(QString("Helios 2"));

    return (lookUpTable);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector<double> LAULucidCamera::jetr(int chn) const
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

#if !defined(Q_OS_MAC)
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAULucidCamera::GetNodeValue(acNodeMap hMap, const char *nodeName)
{
    // get node
    acNode hNode = NULL;
    AC_ACCESS_MODE accessMode = 0;

    AC_ERROR err = acNodeMapGetNodeAndAccessMode(hMap, nodeName, &hNode, &accessMode);
    if (err != AC_ERR_SUCCESS) {
        errorString = QString("Lucid Error getting node (%1) access mode: %2").arg(QString(nodeName)).arg(errorMessages(err));
        return (QString());
    }

    if (accessMode != AC_ACCESS_MODE_RO && accessMode != AC_ACCESS_MODE_RW) {
        errorString = QString("Lucid Error getting node (%1) value: node is not readable.").arg(QString(nodeName));
        return (QString());
    }

    char pValue[256];
    size_t pLen = 256;
    err = acValueToString(hNode, pValue, &pLen);
    if (err != AC_ERR_SUCCESS) {
        errorString = QString("Lucid Error mapping node (%1) value to string: %2").arg(QString(nodeName)).arg(errorMessages(err));
        return (QString());
    }
    return (QString(pValue));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAULucidCamera::SetNodeValue(acNodeMap hMap, const char *nodeName, const char *pValue)
{
    // get node
    acNode hNode = NULL;
    AC_ACCESS_MODE accessMode = 0;

    AC_ERROR err = acNodeMapGetNodeAndAccessMode(hMap, nodeName, &hNode, &accessMode);
    if (err != AC_ERR_SUCCESS) {
        errorString = QString("Lucid Error getting node access mode: %1").arg(errorMessages(err));
        return (false);
    }

    if (accessMode != AC_ACCESS_MODE_WO && accessMode != AC_ACCESS_MODE_RW) {
        errorString = QString("Lucid Error setting node value: node is not writable.");
        return (false);
    }

    err = acValueFromString(hNode, pValue);
    if (err != AC_ERR_SUCCESS) {
        errorString = QString("Lucid Error setting %1 to %2: %3").arg(QString(nodeName)).arg(QString(pValue)).arg(errorMessages(err));
        return (false);
    }
    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAULucidCamera::errorMessages(AC_ERROR err)
{
    switch (err) {
        case AC_ERR_SUCCESS:
            return (QString("Success, no error"));
        case AC_ERR_ERROR:
            return (QString("Generic error"));
        case AC_ERR_NOT_INITIALIZED:
            return (QString("Arena SDK not initialized"));
        case AC_ERR_NOT_IMPLEMENTED:
            return (QString("Function not implemented"));
        case AC_ERR_RESOURCE_IN_USE:
            return (QString("Resource already in use"));
        case AC_ERR_ACCESS_DENIED:
            return (QString("Incorrect access"));
        case AC_ERR_INVALID_HANDLE:
            return (QString("Null/incorrect handle"));
        case AC_ERR_INVALID_ID:
            return (QString("Incorrect ID"));
        case AC_ERR_NO_DATA:
            return (QString("No data available"));
        case AC_ERR_INVALID_PARAMETER:
            return (QString("Null/incorrect parameter"));
        case AC_ERR_IO:
            return (QString("Input/output error"));
        case AC_ERR_TIMEOUT:
            return (QString("Timed out"));
        case AC_ERR_ABORT:
            return (QString("Function aborted"));
        case AC_ERR_INVALID_BUFFER:
            return (QString("Invalid buffer"));
        case AC_ERR_NOT_AVAILABLE:
            return (QString("Function not available"));
        case AC_ERR_INVALID_ADDRESS:
            return (QString("Invalid register address"));
        case AC_ERR_BUFFER_TOO_SMALL:
            return (QString("Buffer too small"));
        case AC_ERR_INVALID_INDEX:
            return (QString("Invalid index"));
        case AC_ERR_PARSING_CHUNK_DATA:
            return (QString("Error parsing chunk data"));
        case AC_ERR_INVALID_VALUE:
            return (QString("Invalid value"));
        case AC_ERR_RESOURCE_EXHAUSTED:
            return (QString("Resource cannot perform more actions"));
        case AC_ERR_OUT_OF_MEMORY:
            return (QString("Not enough memory"));
        case AC_ERR_BUSY:
            return (QString("Busy on anothe process"));
        case AC_ERR_CUSTOM:
            return (QString("Custom or unknown error"));
        default:
            return (QString("Unknown error."));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAULucidCamera::onSetCameraUserDefinedName(int channel, QString name)
{
#if !defined(Q_OS_MAC)
    // Validate channel index
    if (channel < 0 || channel >= cameras.count()) {
        qWarning() << "Invalid camera channel:" << channel << "(valid range: 0 -" << cameras.count()-1 << ")";
        return false;
    }

    // Get the camera packet for this channel
    CameraPacket &packet = cameras[channel];

    if (!packet.isConnected) {
        qWarning() << "Camera" << channel << "is not connected";
        return false;
    }

    // Set the user-defined name using the camera's node map
    AC_ERROR err = acNodeMapSetStringValue(packet.hNodeMap, "DeviceUserID", name.toLatin1().constData());
    if (err != AC_ERR_SUCCESS) {
        qWarning() << "Failed to set user-defined name for camera" << channel << ":" << errorMessages(err);
        return false;
    }

    // Update the local cached name
    packet.userDefinedName = name;

    qDebug() << "Successfully set camera" << channel << "(S/N:" << packet.serialString << ") to name:" << name;
    return true;
#else
    Q_UNUSED(channel);
    Q_UNUSED(name);
    qWarning() << "Camera naming not supported on macOS";
    return false;
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAULucidCamera::onGetCameraUserDefinedName(int channel)
{
#if !defined(Q_OS_MAC)
    // Validate channel index
    if (channel < 0 || channel >= cameras.count()) {
        qWarning() << "Invalid camera channel:" << channel << "(valid range: 0 -" << cameras.count()-1 << ")";
        return QString();
    }

    // Get the camera packet for this channel
    CameraPacket &packet = cameras[channel];

    if (!packet.isConnected) {
        qWarning() << "Camera" << channel << "is not connected";
        return QString();
    }

    // Read the user-defined name from the camera's node map
    char pDeviceNameBuf[LUCIDMAXBUF];
    size_t pBufLen = LUCIDMAXBUF;
    AC_ERROR err = acNodeMapGetStringValue(packet.hNodeMap, "DeviceUserID", pDeviceNameBuf, &pBufLen);
    if (err != AC_ERR_SUCCESS) {
        qWarning() << "Failed to read user-defined name for camera" << channel << ":" << errorMessages(err);
        return QString();
    }

    QString name = QString::fromLatin1(pDeviceNameBuf);
    qDebug() << "Read camera" << channel << "(S/N:" << packet.serialString << ") name:" << name;
    return name;
#else
    Q_UNUSED(channel);
    qWarning() << "Camera naming not supported on macOS";
    return QString();
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAULucidCamera::setUserDefinedNames(const QStringList &names, QString &errorMessage, QStringList &progressMessages)
{
    progressMessages.clear();
    errorMessage.clear();

    // Open the Arena SDK system
    acSystem hSystem = NULL;
    AC_ERROR err = acOpenSystem(&hSystem);
    if (err != AC_ERR_SUCCESS || hSystem == NULL) {
        errorMessage = QString("Failed to open Lucid system: %1").arg(errorMessages(err));
        return false;
    }

    progressMessages.append("Lucid system opened successfully");

    // Update device list
    progressMessages.append("Scanning for Lucid cameras...");
    err = acSystemUpdateDevices(hSystem, SYSTEM_TIMEOUT);
    if (err != AC_ERR_SUCCESS) {
        errorMessage = QString("Failed to update device list: %1").arg(errorMessages(err));
        acCloseSystem(hSystem);
        return false;
    }

    // Get number of devices
    size_t numDevices = 0;
    err = acSystemGetNumDevices(hSystem, &numDevices);
    if (err != AC_ERR_SUCCESS) {
        errorMessage = QString("Failed to get device count: %1").arg(errorMessages(err));
        acCloseSystem(hSystem);
        return false;
    }

    progressMessages.append(QString("Found %1 device(s) total").arg(numDevices));

    // First pass: identify valid Lucid cameras (filter out Orbbec and other non-Lucid devices)
    QList<size_t> validLucidIndices;
    for (size_t i = 0; i < numDevices; i++) {
        // Create device temporarily to check serial number
        acDevice hDevice = NULL;
        err = acSystemCreateDevice(hSystem, i, &hDevice);
        if (err != AC_ERR_SUCCESS) {
            progressMessages.append(QString("  Warning: Could not open device %1 for validation").arg(i));
            continue;
        }

        // Get device node map
        acNodeMap hNodeMap = NULL;
        err = acDeviceGetNodeMap(hDevice, &hNodeMap);
        if (err != AC_ERR_SUCCESS) {
            acSystemDestroyDevice(hSystem, hDevice);
            progressMessages.append(QString("  Warning: Could not get node map for device %1").arg(i));
            continue;
        }

        // Get serial number
        char serialBuf[LUCIDMAXBUF];
        size_t serialBufLen = LUCIDMAXBUF;
        err = acNodeMapGetStringValue(hNodeMap, "DeviceSerialNumber", serialBuf, &serialBufLen);
        QString serialNumber = (err == AC_ERR_SUCCESS) ? QString(serialBuf) : QString("");

        // Validate serial number format - Lucid cameras have 9-digit numeric serial numbers
        // Skip devices with invalid serial numbers (e.g., Orbbec cameras that appear in device list)
        bool isValidLucid = false;
        if (serialNumber.length() == 9) {
            bool allDigits = true;
            for (int j = 0; j < serialNumber.length(); j++) {
                if (!serialNumber.at(j).isDigit()) {
                    allDigits = false;
                    break;
                }
            }
            isValidLucid = allDigits;
        }

        if (isValidLucid) {
            validLucidIndices.append(i);
            progressMessages.append(QString("  Device %1: Valid Lucid camera (S/N: %2)").arg(i).arg(serialNumber));
        } else {
            progressMessages.append(QString("  Device %1: Skipping non-Lucid device (S/N: %2)").arg(i).arg(serialNumber));
        }

        // Clean up temporary device
        acSystemDestroyDevice(hSystem, hDevice);
    }

    int numLucidCameras = validLucidIndices.count();
    progressMessages.append(QString("Found %1 valid Lucid camera(s)").arg(numLucidCameras));

    // Check if we have the right number of names for the Lucid cameras
    if (names.count() != numLucidCameras) {
        errorMessage = QString("Name count mismatch: provided %1 names but found %2 Lucid cameras").arg(names.count()).arg(numLucidCameras);
        acCloseSystem(hSystem);
        return false;
    }

    // Second pass: set user-defined names for valid Lucid cameras only
    for (int i = 0; i < numLucidCameras; i++) {
        size_t deviceIndex = validLucidIndices.at(i);
        progressMessages.append(QString("Processing Lucid camera %1 (device index %2)...").arg(i).arg(deviceIndex));

        // Create device
        acDevice hDevice = NULL;
        err = acSystemCreateDevice(hSystem, deviceIndex, &hDevice);
        if (err != AC_ERR_SUCCESS) {
            errorMessage = QString("Failed to create device %1: %2").arg(deviceIndex).arg(errorMessages(err));
            acCloseSystem(hSystem);
            return false;
        }

        // Get device node map
        acNodeMap hNodeMap = NULL;
        err = acDeviceGetNodeMap(hDevice, &hNodeMap);
        if (err != AC_ERR_SUCCESS) {
            errorMessage = QString("Failed to get node map for device %1: %2").arg(deviceIndex).arg(errorMessages(err));
            acSystemDestroyDevice(hSystem, hDevice);
            acCloseSystem(hSystem);
            return false;
        }

        // Get current serial number for progress reporting
        char serialBuf[LUCIDMAXBUF];
        size_t serialBufLen = LUCIDMAXBUF;
        err = acNodeMapGetStringValue(hNodeMap, "DeviceSerialNumber", serialBuf, &serialBufLen);
        QString serialNumber = (err == AC_ERR_SUCCESS) ? QString(serialBuf) : QString("Unknown");

        // Set the user-defined name
        QString newName = names.at(i);
        progressMessages.append(QString("  Setting camera %1 (S/N: %2) to '%3'...").arg(i).arg(serialNumber).arg(newName));

        err = acNodeMapSetStringValue(hNodeMap, "DeviceUserID", newName.toLatin1().constData());
        if (err != AC_ERR_SUCCESS) {
            errorMessage = QString("Failed to set user-defined name for camera %1 (S/N: %2): %3").arg(i).arg(serialNumber).arg(errorMessages(err));
            acSystemDestroyDevice(hSystem, hDevice);
            acCloseSystem(hSystem);
            return false;
        }

        progressMessages.append(QString("  ✓ Camera %1 successfully set to '%2'").arg(i).arg(newName));

        // Clean up device
        acSystemDestroyDevice(hSystem, hDevice);
    }

    // Close system
    acCloseSystem(hSystem);
    progressMessages.append("All cameras configured successfully!");

    return true;
}
#endif
