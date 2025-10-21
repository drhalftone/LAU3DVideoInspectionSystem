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

#include <QApplication>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QSettings>
#include <QFile>
#include <QDir>
#include "laucascadeclassifierfromlivevideo.h"
#ifdef LUCID
#include "laulucidcamera.h"
#endif

namespace libtiff
{
#include "tiffio.h"
}
using namespace libtiff;
using namespace LAU3DVideoParameters;

QTextStream console(stdout);

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int main(int argc, char *argv[])
{
    QSurfaceFormat format;
    format.setDepthBufferSize(10);
    format.setMajorVersion(4);
    format.setMinorVersion(1);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#ifndef Q_PROCESSOR_ARM
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#endif

    QApplication a(argc, argv);
    a.setOrganizationName(QString("Lau Consulting Inc"));
    a.setOrganizationDomain(QString("drhalftone.com"));
    a.setApplicationName(QString("LAU3DVideoRecorder"));

    qRegisterMetaType<LAUMemoryObject>("LAUMemoryObject");

    // SET THE TIFF ERROR AND WARNING HANDLERS
    libtiff::TIFFSetErrorHandler(myTIFFErrorHandler);
    libtiff::TIFFSetWarningHandler(myTIFFWarningHandler);

    // CHECK FOR HELP ARGUMENT
    if (argc >= 2) {
        QString arg1 = QString::fromUtf8(argv[1]);
        if (arg1 == "-h" || arg1 == "--help" || arg1 == "-?" || arg1.toLower() == "help") {
            console << "LAUProcessVideos - 3D Video Recording and Processing Tool\n";
            console << "================================================================\n";
            console << "Compiled: " << __DATE__ << " " << __TIME__ << "\n\n";
            console << "DESCRIPTION:\n";
            console << "  This application records video from 3D cameras and processes the video\n";
            console << "  stream to detect and track objects using RFID tags and cascade classifiers\n";
            console << "  or green screen filtering.\n\n";
            console << "  The system captures depth and color data, applies real-time filtering to\n";
            console << "  detect foreground objects, and saves the processed video to disk.\n";
            console << "  Recording automatically stops after the specified duration.\n\n";
            console << "USAGE:\n";
            console << "  LAUProcessVideos <output_directory> <duration_HH:MM:SS> [threshold]\n\n";
            console << "ARGUMENTS:\n";
            console << "  output_directory  Directory path where video files will be saved.\n";
            console << "                    Must be an existing, writable directory.\n";
            console << "                    Maximum path length: 4096 characters.\n\n";
            console << "  duration          Recording duration in HH:MM:SS format.\n";
            console << "                    Maximum duration: 11:59:59 (under 12 hours).\n";
            console << "                    Examples: 00:30:00 (30 minutes), 02:15:30 (2h 15m 30s)\n\n";
            console << "  threshold         Optional. Foreground pixel count threshold for object detection.\n";
            console << "                    Range: 0-307200 (corresponding to 640x480 resolution).\n";
            console << "                    Default: 30000 pixels.\n";
            console << "                    Higher values require more pixels to trigger recording.\n\n";
            console << "EXAMPLES:\n";
            console << "  # Record for 1 hour to C:/Videos with default threshold\n";
            console << "  LAUProcessVideos C:/Videos 01:00:00\n\n";
            console << "  # Record for 30 minutes with custom threshold of 50000 pixels\n";
            console << "  LAUProcessVideos /home/user/videos 00:30:00 50000\n\n";
            console << "RETURN CODES:\n";
            console << "  0  - Success (recording completed normally)\n";
            console << "  1  - Dialog rejected by user\n";
            console << "  2  - Camera initialization failed\n";
            console << "  3  - Directory does not exist\n";
            console << "  4  - Invalid time format\n";
            console << "  5  - Invalid threshold value\n";
            console << "  6  - Insufficient arguments\n";
            console << "  7  - Path string too long\n";
            console << "  8  - Path string empty\n";
            console << "  9  - Invalid time string length\n";
            console << "  10 - Path traversal detected\n";
            console << "  11 - Invalid hours value\n";
            console << "  12 - Invalid minutes value\n";
            console << "  13 - Invalid seconds value\n";
            console << "  14 - Threshold string too long\n";
            console << "  15 - Threshold string empty\n\n";
            console << "HARDWARE REQUIREMENTS:\n";
            console << "  Supported 3D cameras (this build):\n";
#if defined(LUCID)
            console << "    - Lucid Vision Labs cameras\n";
#endif
#if defined(ORBBEC)
            console << "    - Orbbec depth cameras\n";
#endif
            console << "\n  Optional hardware:\n";
            console << "    - RFID reader connected to COM1 (for object identification)\n\n";
            console << "For more information, visit: drhalftone.com\n";
            console << "Copyright (c) 2017, Lau Consulting Inc\n";
            return (0);
        }
    }

    if (argc >= 3) {
        // VALIDATE INPUT LENGTHS FIRST (before any operations on untrusted input)
        QString pathString = QString::fromUtf8(argv[1]);
        QString timeString = QString::fromUtf8(argv[2]);

        // CHECK PATH STRING LENGTH
        if (pathString.length() > 4096) {
            console << "Error: Path string too long (max 4096 characters)\n";
            return (7);
        }

        if (pathString.length() == 0) {
            console << "Error: Path string is empty\n";
            return (8);
        }

        // CHECK TIME STRING LENGTH (HH:MM:SS = 8 characters max)
        if (timeString.length() > 8 || timeString.length() < 8) {
            console << "Error: Invalid time string length. Expected HH:MM:SS format\n";
            return (9);
        }

        // SANITIZE AND VALIDATE PATH STRING
        pathString = pathString.replace(QString("\\"), QString("/"));

        // CHECK FOR PATH TRAVERSAL ATTEMPTS
        if (pathString.contains("..")) {
            console << "Error: Path traversal detected. Path cannot contain '..'\n";
            return (10);
        }

        // VALIDATE PATH STRING
        QDir directory(pathString);
        if (!directory.exists()) {
            console << "Error: Directory does not exist: " << pathString << "\n";
            console << "Usage: LAUProcessVideos <output_directory> <duration_HH:MM:SS> [threshold]\n";
            console << "  output_directory: Valid directory path for saving files\n";
            console << "  duration: Recording duration in HH:MM:SS format (max 11:59:59)\n";
            console << "  threshold: Optional foreground pixel count threshold, 0-307200 (default: 30000)\n";
            return (3);
        }

        // VALIDATE TIME STRING FORMAT AND VALUES
        QStringList timeComponents = timeString.split(":");
        if (timeComponents.count() != 3) {
            console << "Error: Invalid time format: " << timeString << "\n";
            console << "Expected format: HH:MM:SS (e.g., 01:30:00 for 1 hour 30 minutes)\n";
            return (4);
        }

        // VALIDATE HOURS COMPONENT
        bool ok;
        int hours = timeComponents.at(0).toInt(&ok);
        if (!ok || hours < 0 || hours >= 12) {
            console << "Error: Invalid hours value: " << timeComponents.at(0) << "\n";
            console << "Hours must be 00-11 (recording time limited to under 12 hours)\n";
            return (11);
        }

        // VALIDATE MINUTES COMPONENT
        int minutes = timeComponents.at(1).toInt(&ok);
        if (!ok || minutes < 0 || minutes >= 60) {
            console << "Error: Invalid minutes value: " << timeComponents.at(1) << "\n";
            console << "Minutes must be 00-59\n";
            return (12);
        }

        // VALIDATE SECONDS COMPONENT
        int seconds = timeComponents.at(2).toInt(&ok);
        if (!ok || seconds < 0 || seconds >= 60) {
            console << "Error: Invalid seconds value: " << timeComponents.at(2) << "\n";
            console << "Seconds must be 00-59\n";
            return (13);
        }

        // VALIDATE THRESHOLD
        int threshold = 30000;
        if (argc >= 4) {
            QString thresholdString = QString::fromUtf8(argv[3]);

            // CHECK THRESHOLD STRING LENGTH
            if (thresholdString.length() > 10) {
                console << "Error: Threshold string too long\n";
                return (14);
            }

            if (thresholdString.length() == 0) {
                console << "Error: Threshold string is empty\n";
                return (15);
            }

            threshold = thresholdString.toInt(&ok);
            if (!ok || threshold < 0 || threshold > 307200) {
                console << "Error: Invalid threshold value: " << thresholdString << "\n";
                console << "Threshold must be an integer between 0 and 307200 (640x480 pixels)\n";
                return (5);
            }
        }

        console << pathString << " :: " << timeString << "\n";

#ifdef LUCID
        // Program Lucid camera labels from systemConfig.ini before creating camera objects
        // This is necessary because OnTrak power cycles clear the camera's DeviceUserID fields
        QString iniPath = QDir::currentPath() + "/systemConfig.ini";
        if (QFile::exists(iniPath)) {
            qDebug() << "Found systemConfig.ini, loading camera positions...";
            QSettings settings(iniPath, QSettings::IniFormat);

            // Read all camera positions from INI [CameraPosition] section
            settings.beginGroup("CameraPosition");
            QStringList serialNumbers = settings.allKeys();
            settings.endGroup();

            if (serialNumbers.count() >= 2) {
                qDebug() << "Found" << serialNumbers.count() << "camera positions in INI";

                // Build list of positions in order (we expect 2 Lucid cameras)
                QStringList positions;
                for (const QString &serialNumber : serialNumbers) {
                    QString position = settings.value(QString("CameraPosition/%1").arg(serialNumber)).toString();
                    positions.append(position);
                    qDebug() << "  S/N" << serialNumber << "->" << position;
                }

                // Program the cameras with these positions
                QString errorMessage;
                QStringList progressMessages;
                bool success = LAULucidCamera::setUserDefinedNames(positions, errorMessage, progressMessages);

                if (success) {
                    qDebug() << "Successfully programmed Lucid camera labels from INI";
                } else {
                    qWarning() << "Failed to program Lucid camera labels:" << errorMessage;
                    // Don't exit - continue anyway, cameras will use serial numbers
                }
            } else {
                qDebug() << "No camera positions found in INI, cameras will use serial numbers";
            }
        } else {
            qDebug() << "No systemConfig.ini found, cameras will use serial numbers";
        }
#endif

        LAUCascadeClassifierFromLiveVideo dialog(pathString, threshold);
        if (dialog.isValid()) {
            dialog.setTimer(timeString);
            if (dialog.exec() == QDialog::Accepted) {
                return 0;
            } else {
                return 1;
            }
        } else {
            console << dialog.error() << "\n";
            return (2);
        }
    } else {
        console << "Error: Insufficient arguments.\n";
        console << "Usage: LAUProcessVideos <output_directory> <duration_HH:MM:SS> [threshold]\n";
        console << "  output_directory: Valid directory path for saving files\n";
        console << "  duration: Recording duration in HH:MM:SS format (max 11:59:59)\n";
        console << "  threshold: Optional foreground pixel count threshold, 0-307200 (default: 30000)\n";
        return (6);
    }
    return (0);
}
