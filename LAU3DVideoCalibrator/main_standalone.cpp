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

#include <QtGui>
#include <QtCore>
#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QDebug>

#include "laujetrstandalonedialog.h"
#include "laucamerainventorydialog.h"
#include "lauwelcomedialog.h"

#ifdef TEST_XY_PLANE
#include "../Calibration/lausetxyplanewidget.h"
#include "../Support/lauscan.h"
#endif

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
    a.setApplicationName("JETR Calibration Manager");
    a.setApplicationVersion("1.0.0");
    a.setOrganizationName(QString("Lau Consulting Inc"));
    a.setOrganizationDomain(QString("drhalftone.com"));

    qRegisterMetaType<LAUMemoryObject>("LAUMemoryObject");

    // SET THE TIFF ERROR AND WARNING HANDLERS
    libtiff::TIFFSetErrorHandler(myTIFFErrorHandler);
    libtiff::TIFFSetWarningHandler(myTIFFWarningHandler);

#ifdef TEST_XY_PLANE
    // XY Plane Dialog Test Mode
    qDebug() << "=== XY Plane Dialog Test Mode ===";
    qDebug() << "Loading scan from file dialog...";

    LAUScan scan(QString());  // Empty string triggers file dialog

    if (!scan.isValid()) {
        qDebug() << "No scan loaded, exiting";
        return 0;
    }

    qDebug() << "Scan loaded:" << scan.width() << "x" << scan.height();
    qDebug() << "Filename:" << scan.filename();
    qDebug() << "Opening XY Plane Dialog...";

    LAUSetXYPlaneDialog dialog(scan);
    int result = dialog.exec();

    qDebug() << "Dialog result:" << (result == QDialog::Accepted ? "Accepted" : "Cancelled");
    if (result == QDialog::Accepted) {
        qDebug() << "Transform matrix:" << dialog.transform();
    }

    qDebug() << "Test completed, exiting";
    return 0;
#else
    // Normal Standalone Mode

    // Check if a file path was passed as a command-line argument
    // This happens when LAUBackgroundFilter launches us with a background file
    QString initialFile;
    if (argc > 1) {
        initialFile = QString(argv[1]);
        qDebug() << "Launched with file argument:" << initialFile;

        // SECURITY: Validate the file path to prevent malicious input
        bool isValid = true;

        // Check for reasonable path length (prevent buffer overflow attempts)
        if (initialFile.length() > 4096) {
            qWarning() << "File path too long (>4096 chars), rejecting for security";
            isValid = false;
        }

        // Check for null bytes (injection attempt)
        if (isValid && initialFile.contains(QChar('\0'))) {
            qWarning() << "File path contains null bytes, rejecting for security";
            isValid = false;
        }

        // Check for path traversal attempts
        if (isValid && (initialFile.contains("../") || initialFile.contains("..\\"))) {
            qWarning() << "File path contains path traversal sequences, rejecting for security";
            isValid = false;
        }

        // Verify file extension is .tif or .tiff (TIFF image format expected)
        if (isValid) {
            QString extension = QFileInfo(initialFile).suffix().toLower();
            if (extension != "tif" && extension != "tiff") {
                qWarning() << "File extension must be .tif or .tiff, got:" << extension;
                isValid = false;
            }
        }

        // Use QFileInfo to get canonical path (resolves symlinks and normalizes)
        // This prevents various path manipulation attacks
        if (isValid) {
            QFileInfo fileInfo(initialFile);
            if (fileInfo.exists()) {
                QString canonicalPath = fileInfo.canonicalFilePath();
                if (canonicalPath.isEmpty()) {
                    qWarning() << "Unable to resolve canonical path for:" << initialFile;
                    isValid = false;
                } else {
                    initialFile = canonicalPath;
                    qDebug() << "Canonical file path:" << canonicalPath;
                }
            } else {
                qWarning() << "File does not exist:" << initialFile;
                isValid = false;
            }
        }

        // Clear the file path if validation failed
        if (!isValid) {
            qWarning() << "File path validation failed, ignoring command-line argument";
            initialFile.clear();
        }
    }

    // If launched with a file argument, open it directly without welcome dialog
    if (!initialFile.isEmpty()) {
        qDebug() << "Opening file directly:" << initialFile;
        LAUJETRStandaloneDialog dialog(initialFile);
        return dialog.exec();
    }

    // Otherwise, loop to keep returning to welcome dialog until user quits
    while (true) {
        // Always show welcome dialog to explain what this application does
        LAUWelcomeDialog welcomeDialog;
        if (welcomeDialog.exec() != QDialog::Accepted) {
            // User chose to quit from welcome dialog - this is the only way to exit
            return 0;
        }

        // Create and show the standalone dialog
        LAUJETRStandaloneDialog dialog((QString()));

        // When dialog closes (for any reason), loop back to welcome screen
        // User can only quit from the welcome dialog
        dialog.exec();

        // Loop continues, returning to welcome dialog
    }

    return 0;
#endif
}
