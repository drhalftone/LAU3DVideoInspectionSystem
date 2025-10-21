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

#ifndef LAUCONSTANTS_H
#define LAUCONSTANTS_H

/*********************************************************************************
 *                                                                               *
 * LAU System Constants - Centralized Configuration                             *
 *                                                                               *
 * This header defines all magic numbers and configuration constants used       *
 * throughout the LAU 3D Video Inspection System.                               *
 *                                                                               *
 *********************************************************************************/

// ============================================================================
// CAMERA RESOLUTION CONSTANTS
// ============================================================================

// Standard VGA resolution (640x480) - Used by most depth cameras
#define LAU_CAMERA_DEFAULT_WIDTH            640
#define LAU_CAMERA_DEFAULT_HEIGHT           480

// Orbbec Gemini 2 specific resolution (640x576 with pseudo mode to 640x480)
#define LAU_ORBBEC_GEMINI2_NATIVE_WIDTH     640
#define LAU_ORBBEC_GEMINI2_NATIVE_HEIGHT    576
#define LAU_ORBBEC_PSEUDO_HEIGHT            480

// RealSense D435 native resolution (848x480)
#define LAU_REALSENSE_D435_WIDTH            848
#define LAU_REALSENSE_D435_HEIGHT           480

// PrimeSense/Kinect resolution (512x424)
#define LAU_PRIMESENSE_WIDTH                512
#define LAU_PRIMESENSE_HEIGHT               424

// Lucid Helios2 camera resolutions
#define LAU_LUCID_DEPTH_SENSOR_WIDTH        640
#define LAU_LUCID_DEPTH_SENSOR_HEIGHT       480
#define LAU_LUCID_COLOR_SENSOR_WIDTH        640
#define LAU_LUCID_COLOR_SENSOR_HEIGHT       480

// Higher resolution options
#define LAU_CAMERA_HD_WIDTH                 1280
#define LAU_CAMERA_HD_HEIGHT                720
#define LAU_CAMERA_FULLHD_WIDTH             1920
#define LAU_CAMERA_FULLHD_HEIGHT            1080

// Combined multi-sensor frame dimensions (3 sensors stacked vertically)
#define LAU_MULTISENSOR_COMBINED_WIDTH      640
#define LAU_MULTISENSOR_COMBINED_HEIGHT     1440  // 480 * 3 sensors

// ============================================================================
// CAMERA SYSTEM CONFIGURATION
// ============================================================================

// Standard system configuration: 1 Orbbec + 2 Lucid cameras = 3 sensors total
#define LAU_MIN_CAMERA_COUNT                3     // Minimum cameras required for full system
#define LAU_EXPECTED_ORBBEC_COUNT           1     // Number of Orbbec depth cameras
#define LAU_EXPECTED_LUCID_COUNT            2     // Number of Lucid ToF cameras
#define LAU_MIN_CAMERA_POSITIONS            2     // Minimum labeled camera positions (Lucid cameras only)

// Camera check retry configuration
#define LAU_MAX_CAMERA_CHECK_ATTEMPTS       3     // Maximum camera detection retry attempts

// ============================================================================
// TIMING AND DELAY CONSTANTS (in milliseconds unless noted)
// ============================================================================

// Camera warmup and initialization timing
#define LAU_CAMERA_WARMUP_SECONDS           120   // 2 minutes initial warmup (cameras need ~2min to fully power on)
#define LAU_CAMERA_RETRY_SECONDS            30    // 30 seconds between camera detection retries
#define LAU_CAMERA_STARTUP_DELAY_MS         2000  // 2 second delay after launching camera tools
#define LAU_CAMERA_BRIEF_WAIT_MS            100   // Brief wait for camera operations
#define LAU_CAMERA_POLL_DELAY_MS            10    // Polling delay for camera status checks

// Process execution timeouts
#define LAU_PROCESS_DEFAULT_TIMEOUT_MS      5000  // 5 second default timeout for process operations
#define LAU_PROCESS_SHORT_TIMEOUT_MS        3000  // 3 second timeout for quick checks
#define LAU_NETWORK_CONNECT_TIMEOUT_MS      3000  // 3 second TCP connection timeout

// Status timer and UI update intervals
#define LAU_STATUS_TIMER_INTERVAL_MS        1000  // 1 second status check interval
#define LAU_FPS_WARNING_THRESHOLD_MS        5000  // 5 seconds before showing FPS warning

// User recommendation delays
#define LAU_RECOMMENDED_WAIT_SECONDS        10    // Recommended wait time in error messages

// ============================================================================
// CALIBRATION AND TRANSFORM CONSTANTS
// ============================================================================

// JETR (Joint Extrinsic Transform Rotation) vector size
#define LAU_JETR_VECTOR_SIZE                37    // Required JETR vector components for calibration

// Minimum fiducials required for calibration
#define LAU_MIN_FIDUCIAL_COUNT              3     // Minimum fiducial markers needed

// ============================================================================
// FRAME RATE AND PERFORMANCE CONSTANTS
// ============================================================================

// FPS counter update frequency
#define LAU_FPS_COUNTER_FRAMES              30    // Number of frames before updating FPS display

// Frame capture retry limit
#define LAU_FRAME_CAPTURE_RETRY_LIMIT       30    // Maximum retries for frame capture

// ============================================================================
// STRING FORMATTING CONSTANTS
// ============================================================================

// Minimum string padding length for formatted output
#define LAU_MIN_STRING_LENGTH               3     // Minimum length for zero-padded strings

// ============================================================================
// DEFAULT WIDGET SIZES
// ============================================================================

// Minimum widget dimensions
#define LAU_MIN_WIDGET_WIDTH                640
#define LAU_MIN_WIDGET_HEIGHT               480

// ============================================================================
// LEGACY COMPATIBILITY (Deprecated - To be removed)
// ============================================================================

// These constants are kept for backward compatibility but should not be used
// in new code. Use the LAU_* prefixed constants instead.
#define LUCIDDEPTHSENSORWIDTH   LAU_LUCID_DEPTH_SENSOR_WIDTH
#define LUCIDDEPTHSENSORHEIGHT  LAU_LUCID_DEPTH_SENSOR_HEIGHT
#define LUCIDCOLORSENSORWIDTH   LAU_LUCID_COLOR_SENSOR_WIDTH
#define LUCIDCOLORSENSORHEIGHT  LAU_LUCID_COLOR_SENSOR_HEIGHT

#endif // LAUCONSTANTS_H
