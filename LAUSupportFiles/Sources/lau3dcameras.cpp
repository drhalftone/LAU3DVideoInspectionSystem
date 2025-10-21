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

#include "lau3dcameras.h"

#if defined(XIMEA)
#include "lauframeaveragingfilter.h"
#include "lauximeacamera.h"
#include "laudemocamera.h"
#include "lau3dmachinevisionscannerwidget.h"
#include "laumachinelearningvideoframelabelerwidget.h"
#endif

#if defined(IDS)
#include "lauidscamera.h"
#include "laudemocamera.h"
#include "lau3dmachinevisionscannerwidget.h"
#include "laumachinelearningvideoframelabelerwidget.h"
#endif


#if defined(SEEK)
#include "lauseekcamera.h"
#include "lau3dmachinevisionscannerwidget.h"
#endif

#if defined(PRIMESENSE)
#include "lauprimesensecamera.h"
#endif

#if defined(REALSENSE)
#include "lau3dmachinevisionscannerwidget.h"
#include "laurealsensecamera.h"
#endif

#if defined(KINECT)
#include "laukinectcamera.h"
#endif

#if defined(LUCID)
#include "laulucidcamera.h"
#endif

#if defined(VIDU)
#include "lauviducamera.h"
#endif

#if defined(ORBBEC)
#include "lauorbbeccamera.h"
#endif

#if defined(VZENSE)
#include "lauvzensecamera.h"
#endif

LAU3DCamera *LAU3DCameras::getCamera(LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device)
{
    LAU3DCamera *camera = nullptr;

    switch (device){
    case DevicePrimeSense:
#if defined(PRIMESENSE)
        camera = new LAUPrimeSenseCamera(color);
#endif
        break;
    case DeviceRealSense:
#if defined(REALSENSE)
        camera = new LAURealSenseCamera(color);
#endif
        break;
    case DeviceLucid:
#if defined(LUCID)
        camera = new LAULucidCamera(color);
#endif
        break;
    case DeviceVidu:
#if defined(VIDU)
        camera = new LAUViduCamera(color);
#endif
        break;
    case DeviceOrbbec:
#if defined(ORBBEC)
        camera = new LAUOrbbecCamera(color);
#endif
        break;
    case DeviceSeek:
#if defined(SEEK)
        camera = new LAUSeekCamera(color);
#endif
        break;
    case DeviceVZense:
#if defined(VZENSE)
        camera = new LAUVZenseCamera(color);
#endif
        break;
#if defined(PROSILICA)
    case DeviceProsilicaGRY:
        camera = new LAUProsilicaCamera(color, ModeMono, SchemeNone, LAUDFTFilter::PatternNone);
        break;
    case DeviceProsilicaLCG:
#if defined(ENABLEPROSILICAFPGA)
        camera = new LAUProsilicaCamera(color, ModeSlave, SchemeNone, LAUDFTFilter::PatternEightEightEight);
#else
        camera = new LAUProsilicaCamera(color, ModeMaster, SchemeFlashingSequence, LAUDFTFilter::PatternEightEightEight);
#endif
        break;
    case DeviceProsilicaDPR:
        break;
    case DeviceProsilicaIOS:
        camera = new LAUProsilicaCamera(color, ModeMaster, SchemePatternBit, LAUDFTFilter::PatternDualFrequency);
        break;
    case DeviceProsilicaPST:
        break;
    case DeviceProsilicaAST:
        break;
    case DeviceProsilicaRGB:
        break;
    case DeviceProsilicaTOF:
        break;
#elif defined(VIMBA)
    case DeviceProsilicaGRY:
        for (int counter = 0; counter < 2; counter++){
            camera = new LAUVimbaCamera(color, ModeMono, SchemeNone, LAUDFTFilter::PatternNone);
            if (camera->isValid() || camera->error() == QString("No cameras found.")){
                break;
            } else if (counter == 0){
                camera->reset();
                delete camera;
            }
        }
        break;
    case DeviceProsilicaLCG:
#if defined(ENABLEPROSILICAFPGA)
        for (int counter = 0; counter < 2; counter++){
            camera = new LAUVimbaCamera(color, ModeSlave, SchemeNone, LAUDFTFilter::PatternEightEightEight);
            if (camera->isValid() || camera->error() == QString("No cameras found.")){
                break;
            } else if (counter == 0){
                camera->reset();
                delete camera;
            }
        }
#else
        for (int counter = 0; counter < 2; counter++){
            camera = new LAUVimbaCamera(color, ModeMaster, SchemeFlashingSequence, LAUDFTFilter::PatternEightEightEight);
            if (camera->isValid() || camera->error() == QString("No cameras found.")){
                break;
            } else if (counter == 0){
                camera->reset();
                delete camera;
            }
        }
#endif
        break;
    case DeviceProsilicaDPR:
        break;
    case DeviceProsilicaIOS:
        for (int counter = 0; counter < 2; counter++){
            camera = new LAUVimbaCamera(color, ModeMaster, SchemePatternBit, LAUDFTFilter::PatternDualFrequency);
            if (camera->isValid() || camera->error() == QString("No cameras found.")){
                break;
            } else if (counter == 0){
                camera->reset();
                delete camera;
            }
        }
        break;
    case DeviceProsilicaPST:
        break;
    case DeviceProsilicaAST:
        break;
    case DeviceProsilicaRGB:
        for (int counter = 0; counter < 2; counter++){
            camera = new LAUVimbaCamera(color, ModeMono, SchemeNone, LAUDFTFilter::PatternNone);
            if (camera->isValid() || camera->error() == QString("No cameras found.")){
                break;
            } else if (counter == 0){
                camera->reset();
                delete camera;
            }
        }
        break;
    case DeviceProsilicaTOF:
        break;
#elif defined(BASLERUSB)
    case DeviceProsilicaGRY:
        camera = new LAUBaslerUSBCamera(color, ModeMono, SchemeNone, LAUDFTFilter::PatternNone);
        break;
    case DeviceProsilicaRGB:
        camera = new LAUBaslerUSBCamera(color, ModeMono, SchemeNone, LAUDFTFilter::PatternNone);
        break;
    case DeviceProsilicaLCG:
        camera = new LAUBaslerUSBCamera(color, ModeMaster, SchemeNone, LAUDFTFilter::PatternEightEightEight);
        break;
    case DeviceProsilicaDPR:
#if defined(ENABLECALIBRATION)
        camera = new LAUBaslerUSBCamera(color, ModeMaster, SchemeFlashingSequence, LAUDFTFilter::PatternEightEightEight);
#else
        camera = new LAUBaslerUSBCamera(color, ModeMaster, SchemeNone, LAUDFTFilter::PatternEightEightEight);
#endif
        break;
    case DeviceProsilicaIOS:
#if defined(ENABLECALIBRATION)
        camera = new LAUBaslerUSBCamera(color, ModeMaster, SchemeFlashingSequence, LAUDFTFilter::PatternDualFrequency);
#else
        camera = new LAUBaslerUSBCamera(color, ModeMaster, SchemeNone, LAUDFTFilter::PatternDualFrequency);
#endif
        break;
    case DeviceProsilicaAST:
#if defined(ENABLECALIBRATION)
        camera = new LAUBaslerUSBCamera(color, ModeMaster, SchemeFlashingSequence, LAUDFTFilter::PatternEightEightEight);
#else
        camera = new LAUBaslerUSBCamera(color, ModeMaster, SchemeNone, LAUDFTFilter::PatternEightEightEight);
#endif
        break;
    case DeviceProsilicaPST:
        camera = new LAUBaslerUSBCamera(color, ModeMono, SchemeNone, LAUDFTFilter::PatternNone);
        break;
    case DeviceProsilicaTOF:
#ifdef KINECT
        camera = new LAUBaslerKinectCamera(color, ModeMaster, SchemeFlashingSequence, LAUDFTFilter::PatternEightEightEight);
#endif
        break;
#endif
    case DeviceXimea:
#ifdef XIMEA
#ifdef CASSI
            camera = new LAUXimeaCamera(color, ModeSlave);
#else
            camera = new LAUXimeaCamera(color);
#endif
#endif
        break;
    case DeviceKinect:
#ifdef KINECT
        camera = new LAUKinectCamera(color);
#endif
        break;
    case DeviceIDS:
#ifdef IDS
        idsFilter = NULL;
        idsController = NULL;
        camera = new LAUIDSCamera(color);
#endif
        break;
    case Device2DCamera:
    case DeviceUndefined:
    case DeviceDemo:
        break;
    }

    return (camera);
}
