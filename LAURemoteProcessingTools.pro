TEMPLATE = subdirs

# ============================================================================
# LAU 3D Video Inspection System
# ============================================================================
# Comprehensive suite for 3D video inspection and monitoring
#
# This project combines 9 essential tools for video inspection systems:
#   1. LAU3DVideoInspector        - 3D video inspection application
#   2. LAUMonitorLiveVideo        - Live video monitoring and object detection
#   3. LAUEncodeObjectIDFilter    - Object ID extraction and encoding
#   4. LAUBackgroundCapture       - Background capture and filtering tool (depth mode)
#   5. LAUCameraPositionLabeler   - Camera position labeling tool (NIR mode)
#   6. LAURemoteToolsScheduler    - System setup and configuration
#   7. LAUOnTrakWidget            - USB relay control for camera power cycling
#   8. LAU3DVideoCalibrator       - JETR calibration vector editor for camera setup
#   9. LAUInstallerPalette        - Central hub for managing tools (setup assistant)
# ============================================================================

# Define the subprojects
SUBDIRS += \
    LAU3DVideoInspector/LAU3DVideoInspector.pro \
    LAUMonitorLiveVideo/LAUMonitorLiveVideo.pro \
    LAUEncodeObjectIDFilter/LAUEncodeObjectIDFilter.pro \
    LAUBackgroundCapture/LAUBackgroundCapture.pro \
    LAUBackgroundCapture/LAUCameraPositionLabeler.pro \
    LAURemoteToolsScheduler/LAURemoteToolsScheduler.pro \
    LAUOnTrakWidget/LAUOnTrakWidget.pro \
    LAU3DVideoCalibrator/LAU3DVideoCalibrator.pro \
    LAUInstallerPalette/LAUInstallerPalette.pro

# Optional: Define dependencies between projects if needed
# LAUMonitorLiveVideo.depends = LAU3DVideoInspector/LAU3DVideoInspector.pro
# LAUEncodeObjectIDFilter.depends = LAU3DVideoInspector/LAU3DVideoInspector.pro
# LAURemoteToolsScheduler.depends = LAU3DVideoInspector/LAU3DVideoInspector.pro
# LAUOnTrakWidget.depends = LAU3DVideoInspector/LAU3DVideoInspector.pro

# Windows deployment and packaging script generation
win32 {
    message("=== LAU 3D Video Inspection System Build Configuration ===")
    message("Platform: Windows")
    message("CONFIG: $$CONFIG")
    message("OUT_PWD: $$OUT_PWD")

    !debug_and_release|CONFIG(release, debug|release) {
        message("Build Mode: RELEASE - Generating deployment scripts")
        # Create deploy.bat script for Windows
        DEPLOY_SCRIPT = $$OUT_PWD/deploy.bat
        PACKAGE_SCRIPT = $$OUT_PWD/package.bat

        # Write deploy.bat using write_file
        DEPLOY_CONTENT = "@echo off"
        DEPLOY_CONTENT += "echo Deploying Qt dependencies for all subprojects..."
        DEPLOY_CONTENT += "echo."
        DEPLOY_CONTENT += ""
        DEPLOY_CONTENT += "REM Deploy LAU3DVideoInspector"
        DEPLOY_CONTENT += "echo Deploying LAU3DVideoInspector..."
        DEPLOY_CONTENT += "windeployqt --release LAU3DVideoInspector\\release\\LAU3DVideoInspector.exe"
        DEPLOY_CONTENT += "if errorlevel 1 echo WARNING: Failed to deploy LAU3DVideoInspector"
        DEPLOY_CONTENT += "echo."
        DEPLOY_CONTENT += ""
        DEPLOY_CONTENT += "REM Deploy LAUMonitorLiveVideo"
        DEPLOY_CONTENT += "echo Deploying LAUMonitorLiveVideo..."
        DEPLOY_CONTENT += "windeployqt --release LAUMonitorLiveVideo\\release\\LAUMonitorLiveVideo.exe"
        DEPLOY_CONTENT += "if errorlevel 1 echo WARNING: Failed to deploy LAUMonitorLiveVideo"
        DEPLOY_CONTENT += "echo."
        DEPLOY_CONTENT += ""
        DEPLOY_CONTENT += "REM Deploy LAUEncodeObjectIDFilter"
        DEPLOY_CONTENT += "echo Deploying LAUEncodeObjectIDFilter..."
        DEPLOY_CONTENT += "windeployqt --release LAUEncodeObjectIDFilter\\release\\LAUEncodeObjectIDFilter.exe"
        DEPLOY_CONTENT += "if errorlevel 1 echo WARNING: Failed to deploy LAUEncodeObjectIDFilter"
        DEPLOY_CONTENT += "echo."
        DEPLOY_CONTENT += ""
        DEPLOY_CONTENT += "REM Deploy LAUBackgroundCapture (Depth Mode)"
        DEPLOY_CONTENT += "echo Deploying LAUBackgroundCapture (Depth Mode)..."
        DEPLOY_CONTENT += "windeployqt --release LAUBackgroundCapture\\release\\LAUBackgroundCapture.exe"
        DEPLOY_CONTENT += "if errorlevel 1 echo WARNING: Failed to deploy LAUBackgroundCapture"
        DEPLOY_CONTENT += "echo."
        DEPLOY_CONTENT += ""
        DEPLOY_CONTENT += "REM Deploy LAUCameraPositionLabeler (Camera Labeling Mode)"
        DEPLOY_CONTENT += "echo Deploying LAUCameraPositionLabeler (Camera Labeling Mode)..."
        DEPLOY_CONTENT += "windeployqt --release LAUCameraPositionLabeler\\release\\LAUCameraPositionLabeler.exe"
        DEPLOY_CONTENT += "if errorlevel 1 echo WARNING: Failed to deploy LAUCameraPositionLabeler"
        DEPLOY_CONTENT += "echo."
        DEPLOY_CONTENT += ""
        DEPLOY_CONTENT += "REM Deploy LAURemoteToolsScheduler"
        DEPLOY_CONTENT += "echo Deploying LAURemoteToolsScheduler..."
        DEPLOY_CONTENT += "windeployqt --release LAURemoteToolsScheduler\\release\\LAURemoteToolsScheduler.exe"
        DEPLOY_CONTENT += "if errorlevel 1 echo WARNING: Failed to deploy LAURemoteToolsScheduler"
        DEPLOY_CONTENT += "echo."
        DEPLOY_CONTENT += ""
        DEPLOY_CONTENT += "REM Deploy LAUOnTrakWidget"
        DEPLOY_CONTENT += "echo Deploying LAUOnTrakWidget..."
        DEPLOY_CONTENT += "windeployqt --release LAUOnTrakWidget\\release\\LAUOnTrakWidget.exe"
        DEPLOY_CONTENT += "if errorlevel 1 echo WARNING: Failed to deploy LAUOnTrakWidget"
        DEPLOY_CONTENT += "echo."
        DEPLOY_CONTENT += ""
        DEPLOY_CONTENT += "REM Deploy LAU3DVideoCalibrator"
        DEPLOY_CONTENT += "echo Deploying LAU3DVideoCalibrator..."
        DEPLOY_CONTENT += "windeployqt --release LAU3DVideoCalibrator\\release\\LAU3DVideoCalibrator.exe"
        DEPLOY_CONTENT += "if errorlevel 1 echo WARNING: Failed to deploy LAU3DVideoCalibrator"
        DEPLOY_CONTENT += "echo."
        DEPLOY_CONTENT += ""
        DEPLOY_CONTENT += "REM Deploy LAUInstallerPalette"
        DEPLOY_CONTENT += "echo Deploying LAUInstallerPalette..."
        DEPLOY_CONTENT += "windeployqt --release LAUInstallerPalette\\release\\LAUInstallerPalette.exe"
        DEPLOY_CONTENT += "if errorlevel 1 echo WARNING: Failed to deploy LAUInstallerPalette"
        DEPLOY_CONTENT += "echo."
        DEPLOY_CONTENT += ""
        DEPLOY_CONTENT += "echo Deployment complete!"
        DEPLOY_CONTENT += "pause"

        write_file($$DEPLOY_SCRIPT, DEPLOY_CONTENT)

        message("Created: $$DEPLOY_SCRIPT")
    } else {
        message("Build Mode: DEBUG - Skipping deployment script generation")
    }
} else {
    message("Platform: NOT Windows - Skipping deployment script generation")
}
