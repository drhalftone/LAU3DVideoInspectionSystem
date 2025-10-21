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

#include "laubackgroundfiltermainwindow.h"
#include <QProcess>
#include <QDir>
#include <QCoreApplication>

using namespace LAU3DVideoParameters;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUBackgroundFilterMainWindow::LAUBackgroundFilterMainWindow(QWidget *parent) : QWidget(parent)
{
    // Set window properties
    this->setWindowTitle(QString("LAU Background Filter"));
    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);

    // Multi-sensor video widget color mode
#ifdef RAW_NIR_VIDEO
    // Raw NIR video mode - display passive grayscale without background filters
    LAUVideoPlaybackColor color = ColorGray;
#else
    // Normal mode - display ColorXYZG with background filters
    LAUVideoPlaybackColor color = ColorXYZG;
#endif

    // Build list of camera devices in the correct order
    // IMPORTANT: Orbbec must be first, followed by Lucid
    // This matches the setup in LAUProcessVideos tool:
    // - 1x Orbbec Femto Mega (first, has 1 sensor)
    // - 1x LAULucidCamera instance (manages 2x Lucid Helios cameras as multiple sensors)
    //
    // Note: LAULucidCamera automatically detects and manages ALL connected Lucid cameras.
    // A single LAULucidCamera instance will expose each physical Lucid camera as a separate sensor.
    // So one DeviceLucid in the list will connect to both Lucid Helios cameras.
    // PTP synchronization happens within LAULucidCamera among Lucid cameras only.
    QList<LAUVideoPlaybackDevice> availableDevices;

#ifdef ORBBEC
    // Orbbec Femto Mega must be first in the list
    availableDevices.append(DeviceOrbbec);
#endif
#ifdef LUCID
    // LAULucidCamera will automatically detect all connected Lucid cameras
    // and expose them as multiple sensors (cameras.count())
    availableDevices.append(DeviceLucid);
#endif

    // Check if any devices were enabled
    if (availableDevices.isEmpty()) {
        QMessageBox::warning(this, QString("LAU Background Filter"),
                           QString("No camera device enabled!\n\nPlease enable LUCID and ORBBEC in the project configuration."));
        QTimer::singleShot(0, qApp, SLOT(quit()));
        return;
    }

    // Create the multi-sensor video widget
    // The widget will create camera instances for each device in the list
    // The final sensor order will be:
    //   Sensor 0: Orbbec Femto Mega
    //   Sensor 1: Lucid Helios #1 (managed by LAULucidCamera, PTP master)
    //   Sensor 2: Lucid Helios #2 (managed by LAULucidCamera, PTP slave)
    widget = new LAU3DMultiSensorVideoWidget(availableDevices, color, this);

    // Check if widget initialized successfully and validate camera configuration
    if (widget->isNull()) {
        // Build detailed camera detection summary
        QString detectedCameras;
        if (widget->cameraCount() == 0) {
            detectedCameras = "  <font color='red'>No cameras detected</font>";
        } else {
            detectedCameras = QString("Detected %1 camera device(s):").arg(widget->cameraCount());
            for (int i = 0; i < widget->cameraCount(); i++) {
                QString make = widget->cameraMake(i);
                QString model = widget->cameraModel(i);
                int sensorCount = widget->cameraSensors(i);
                detectedCameras += QString("<br>  • %1 %2 (%3 sensor%4)")
                    .arg(make)
                    .arg(model)
                    .arg(sensorCount)
                    .arg(sensorCount > 1 ? "s" : "");
            }
        }

        // Show detailed error message with Retry/Abort options
        QString errorMessage = QString(
            "<b>Failed to initialize cameras</b><br><br>"
            "Expected configuration:<br>"
            "  • 1x Orbbec Femto Mega (depth camera)<br>"
            "  • 2x Lucid Helios (ToF cameras with PTP sync)<br><br>"
            "%1<br><br>"
            "<b>Error details:</b><br>%2<br><br>"
            "<b>Common issues:</b><br>"
            "  • OnTrak relay is OFF (cameras have no PoE power)<br>"
            "  • Cameras not connected to GigE network<br>"
            "  • Lucid Arena SDK not installed<br>"
            "  • Camera drivers not initialized<br>"
            "  • Network switch not configured for camera multicast traffic<br><br>"
            "<b>Troubleshooting:</b><br>"
            "  1. Check that OnTrak widget shows <font color='green'>GREEN</font> buttons (PoE power ON)<br>"
            "  2. Verify all GigE Ethernet cables are connected<br>"
            "  3. Check network switch activity lights for all cameras<br>"
            "  4. Wait 30 seconds for cameras to initialize<br>"
            "  5. Click <b>Retry</b> to try again"
        ).arg(detectedCameras).arg(widget->error());

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Camera Initialization Failed");
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setText(errorMessage);
        msgBox.setIcon(QMessageBox::Critical);

        QPushButton *retryButton = msgBox.addButton("Retry", QMessageBox::AcceptRole);
        QPushButton *abortButton = msgBox.addButton("Abort", QMessageBox::RejectRole);
        msgBox.setDefaultButton(retryButton);

        msgBox.exec();

        if (msgBox.clickedButton() == retryButton) {
            // User wants to retry - close this window and relaunch the application
            QTimer::singleShot(0, this, [this]() {
                qDebug() << "User clicked Retry - relaunching LAUBackgroundFilter";
                // Restart the application
                qApp->quit();
                QProcess::startDetached(qApp->applicationFilePath(), qApp->arguments());
            });
        } else {
            // User clicked Abort - quit application
            QTimer::singleShot(0, qApp, SLOT(quit()));
        }
        return;
    }

    // Validate that we have the expected sensor count (3 total: 1 Orbbec + 2 Lucid)
    int expectedSensorCount = 3;  // 1 Orbbec + 2 Lucid Helios
    if (widget->sensors() != expectedSensorCount) {
        // Build detailed camera detection summary
        QString detectedCameras;
        int orbbecCount = 0;
        int lucidCount = 0;

        for (int i = 0; i < widget->cameraCount(); i++) {
            QString make = widget->cameraMake(i);
            QString model = widget->cameraModel(i);
            int sensorCount = widget->cameraSensors(i);
            LAUVideoPlaybackDevice device = widget->cameraDevice(i);

            if (device == DeviceOrbbec) {
                orbbecCount += sensorCount;
            } else if (device == DeviceLucid) {
                lucidCount += sensorCount;
            }

            detectedCameras += QString("<br>  • %1 %2 (%3 sensor%4)")
                .arg(make)
                .arg(model)
                .arg(sensorCount)
                .arg(sensorCount > 1 ? "s" : "");
        }

        // Determine what's missing
        QStringList missingCameras;
        if (orbbecCount == 0) {
            missingCameras << "Orbbec Femto Mega";
        }
        if (lucidCount < 2) {
            int missing = 2 - lucidCount;
            missingCameras << QString("%1 Lucid Helios camera%2").arg(missing).arg(missing > 1 ? "s" : "");
        }

        QString missingInfo;
        if (!missingCameras.isEmpty()) {
            missingInfo = QString("<br><br><font color='red'><b>Missing:</b> %1</font>").arg(missingCameras.join(", "));
        }

        // Show warning about unexpected sensor count
        QString warningMessage = QString(
            "<b>Unexpected camera configuration detected</b><br><br>"
            "Expected: %1 sensors (1 Orbbec + 2 Lucid)<br>"
            "Detected: %2 sensors (%3 Orbbec + %4 Lucid)%5<br><br>"
            "Detected cameras:%6<br><br>"
            "<b>Possible issues:</b><br>"
            "  • One or more Lucid cameras not detected<br>"
            "  • Extra cameras connected<br>"
            "  • PTP synchronization still initializing<br><br>"
            "<b>Do you want to continue anyway?</b><br>"
            "Proceeding with incorrect camera count may result in incomplete calibration."
        ).arg(expectedSensorCount)
         .arg(widget->sensors())
         .arg(orbbecCount)
         .arg(lucidCount)
         .arg(missingInfo)
         .arg(detectedCameras);

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Camera Configuration Warning");
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setText(warningMessage);
        msgBox.setIcon(QMessageBox::Warning);

        QPushButton *continueButton = msgBox.addButton("Continue Anyway", QMessageBox::AcceptRole);
        QPushButton *retryButton = msgBox.addButton("Retry", QMessageBox::RejectRole);
        QPushButton *abortButton = msgBox.addButton("Abort", QMessageBox::DestructiveRole);
        msgBox.setDefaultButton(retryButton);

        msgBox.exec();

        if (msgBox.clickedButton() == retryButton) {
            // User wants to retry - relaunch application
            QTimer::singleShot(0, this, [this]() {
                qDebug() << "User clicked Retry - relaunching LAUBackgroundFilter";
                qApp->quit();
                QProcess::startDetached(qApp->applicationFilePath(), qApp->arguments());
            });
            return;
        } else if (msgBox.clickedButton() == abortButton) {
            // User clicked Abort - quit application
            QTimer::singleShot(0, qApp, SLOT(quit()));
            return;
        }
        // If Continue Anyway was clicked, proceed with initialization
        qWarning() << "User chose to continue with" << widget->sensors() << "sensors instead of expected" << expectedSensorCount;
    }

    this->layout()->addWidget(widget);

    qDebug() << "LAU Background Filter initialized with" << widget->sensors() << "sensors";

    // Resize window to fit contents
    this->adjustSize();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUBackgroundFilterMainWindow::~LAUBackgroundFilterMainWindow()
{
    // Widget is deleted automatically as a child widget
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUBackgroundFilterMainWindow::closeEvent(QCloseEvent *event)
{
    // Simply close the window - calibration editing is now handled by the palette
    event->accept();
}
