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
#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QCommandLineParser>
#include <QDebug>
#include <QDialog>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QClipboard>
#include <QSettings>
#include <QDir>
#include <QFile>
#include "laubackgroundfiltermainwindow.h"
#include "lauscan.h"

#ifdef LUCID
#include "laulucidcamera.h"
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <stdio.h>
#endif

// Include camera headers for enumeration
#ifdef ORBBEC
#include <libobsensor/ObSensor.hpp>
#endif
#ifdef LUCID
#include <ArenaCApi.h>
#endif

namespace libtiff
{
#include "tiffio.h"
}
using namespace LAU3DVideoParameters;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
// Camera availability check function (no GUI)
int checkCamerasAvailable()
{
    int orbbecCount = 0;
    int lucidCount = 0;

    // Use fprintf to ensure output always appears in console
    fprintf(stdout, "Camera Check Results:\n");
    fflush(stdout);

#ifdef ORBBEC
    // Check for Orbbec cameras
    fprintf(stdout, "  Checking Orbbec cameras...\n");
    fflush(stdout);
    try {
        ob_error *error = NULL;
        ob_context *context = ob_create_context(&error);
        if (!error && context) {
            ob_device_list *deviceList = ob_query_device_list(context, &error);
            if (!error && deviceList) {
                orbbecCount = ob_device_list_device_count(deviceList, &error);
                if (error) {
                    fprintf(stdout, "    Error getting device count\n");
                    fflush(stdout);
                    orbbecCount = 0;
                }
                ob_delete_device_list(deviceList, &error);
            } else {
                fprintf(stdout, "    Error querying device list\n");
                fflush(stdout);
            }
            ob_delete_context(context, &error);
        } else {
            fprintf(stdout, "    Error creating Orbbec context\n");
            fflush(stdout);
        }
    } catch (...) {
        fprintf(stdout, "    Exception checking Orbbec cameras\n");
        fflush(stdout);
        orbbecCount = 0;
    }
    fprintf(stdout, "  Orbbec cameras: %d\n", orbbecCount);
    fflush(stdout);
#else
    fprintf(stdout, "  Orbbec support: NOT COMPILED\n");
    fflush(stdout);
#endif

#ifdef LUCID
    // Check for Lucid cameras using C API
    fprintf(stdout, "  Checking Lucid cameras...\n");
    fflush(stdout);

    acSystem hSystem = NULL;
    AC_ERROR err = acOpenSystem(&hSystem);
    if (err == AC_ERR_SUCCESS && hSystem) {
        // Update device list - some systems need this to enumerate cameras
        fprintf(stdout, "    Updating device list...\n");
        fflush(stdout);
        err = acSystemUpdateDevices(hSystem, 1000); // 1 second timeout
        if (err != AC_ERR_SUCCESS) {
            fprintf(stdout, "    Warning: UpdateDevices returned 0x%08X\n", err);
            fflush(stdout);
        }

        size_t numDevices = 0;
        err = acSystemGetNumDevices(hSystem, &numDevices);
        if (err == AC_ERR_SUCCESS) {
            lucidCount = (int)numDevices;
        } else {
            fprintf(stdout, "    Error getting device count (error code: 0x%08X)\n", err);
            fflush(stdout);
        }
        acCloseSystem(hSystem);
    } else {
        fprintf(stdout, "    Error opening Lucid system (error code: 0x%08X)\n", err);
        fflush(stdout);
    }

    fprintf(stdout, "  Lucid cameras: %d\n", lucidCount);
    fflush(stdout);
#else
    fprintf(stdout, "  Lucid support: NOT COMPILED\n");
    fflush(stdout);
#endif

    // Expected: 1 Orbbec + 2 Lucid = 3 total cameras
    bool camerasAvailable = (orbbecCount >= 1 && lucidCount >= 2);

    if (camerasAvailable) {
        fprintf(stdout, "SUCCESS: All expected cameras are available (1 Orbbec + 2 Lucid)\n");
        fflush(stdout);
        return 0; // Success
    } else {
        fprintf(stdout, "FAILED: Not all cameras available (expected 1 Orbbec + 2 Lucid)\n");
        fflush(stdout);
        return 1; // Failure
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int main(int argc, char *argv[])
{
    // Check for command-line mode first (before creating GUI)
    for (int i = 1; i < argc; i++) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--check-cameras" || arg == "-c") {
#ifdef Q_OS_WIN
            // Attach to parent console on Windows so output is visible
            if (AttachConsole(ATTACH_PARENT_PROCESS)) {
                // Reopen stdout/stderr to the console
                freopen("CONOUT$", "w", stdout);
                freopen("CONOUT$", "w", stderr);
            } else {
                // No parent console, allocate a new one
                AllocConsole();
                freopen("CONOUT$", "w", stdout);
                freopen("CONOUT$", "w", stderr);
            }
#endif
            // Run in console mode - no GUI needed
            QCoreApplication a(argc, argv);
            a.setOrganizationName(QString("Lau Consulting Inc"));
            a.setOrganizationDomain(QString("drhalftone.com"));
            a.setApplicationName(QString("LAUBackgroundFilter"));
            int result = checkCamerasAvailable();
            return result;
        }

        if (arg == "--set-camera-names" || arg == "-s") {
            // Get camera names from next argument
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --set-camera-names requires camera names as argument\n");
                fprintf(stderr, "Usage: LAUBackgroundFilter --set-camera-names \"SIDE,TOP\"\n");
                return 1;
            }

            QString namesArg = QString::fromLocal8Bit(argv[i + 1]);
            QStringList names = namesArg.split(',', Qt::SkipEmptyParts);

            if (names.isEmpty()) {
                fprintf(stderr, "Error: No camera names provided\n");
                return 1;
            }

            // Run in GUI mode for progress dialog
            QApplication a(argc, argv);
            a.setOrganizationName(QString("Lau Consulting Inc"));
            a.setOrganizationDomain(QString("drhalftone.com"));
            a.setApplicationName(QString("LAUBackgroundFilter"));

            // Call the naming dialog (to be implemented next)
            // For now, just call the static method directly
            QString errorMessage;
            QStringList progressMessages;

#ifdef LUCID
            bool success = LAULucidCamera::setUserDefinedNames(names, errorMessage, progressMessages);

            // Show progress dialog with results
            QDialog *dialog = new QDialog();
            dialog->setWindowTitle("Set Camera Names");
            dialog->setLayout(new QVBoxLayout());

            QTextEdit *textEdit = new QTextEdit();
            textEdit->setReadOnly(true);
            textEdit->setMinimumSize(600, 400);

            // Display progress messages
            for (const QString &msg : progressMessages) {
                textEdit->append(msg);
            }

            if (!success) {
                textEdit->append("\n<b>ERROR:</b>");
                textEdit->append(errorMessage);
            }

            dialog->layout()->addWidget(textEdit);

            // Add buttons
            QDialogButtonBox *buttonBox = new QDialogButtonBox();
            if (!success) {
                QPushButton *copyBtn = buttonBox->addButton("Copy Error to Clipboard", QDialogButtonBox::ActionRole);
                QObject::connect(copyBtn, &QPushButton::clicked, [errorMessage, progressMessages]() {
                    QString fullText = progressMessages.join("\n") + "\n\nERROR:\n" + errorMessage;
                    QApplication::clipboard()->setText(fullText);
                });
            }
            QPushButton *closeBtn = buttonBox->addButton(QDialogButtonBox::Close);
            QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
            dialog->layout()->addWidget(buttonBox);

            dialog->exec();
            return success ? 0 : 1;
#else
            fprintf(stderr, "Error: Lucid camera support not compiled in this build\n");
            return 1;
#endif
        }
    }

    // Normal GUI mode
    // Configure OpenGL surface format
    QSurfaceFormat format;
    format.setDepthBufferSize(10);
    format.setMajorVersion(4);
    format.setMinorVersion(1);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    QSurfaceFormat::setDefaultFormat(format);

    // Configure Qt application
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication a(argc, argv);

    a.setOrganizationName(QString("Lau Consulting Inc"));
    a.setOrganizationDomain(QString("drhalftone.com"));
    a.setApplicationName(QString("LAUBackgroundFilter"));

    // Register meta types for Qt signal/slot system
    qRegisterMetaType<LAUMemoryObject>("LAUMemoryObject");
    qRegisterMetaType<QList<LAUScan>>("QList<LAUScan>");
    qRegisterMetaType<LAUScan>("LAUScan");

    // Set TIFF error and warning handlers
    libtiff::TIFFSetErrorHandler(myTIFFErrorHandler);
    libtiff::TIFFSetWarningHandler(myTIFFWarningHandler);

    // Check that we have the expected cameras before proceeding
    qDebug() << "Checking for cameras...";
    int cameraCheckResult = checkCamerasAvailable();
    if (cameraCheckResult != 0) {
        qWarning() << "WARNING: Not all expected cameras detected (expected 1 Orbbec + 2 Lucid)";
        qWarning() << "Application will continue, but may not function correctly";
    } else {
        qDebug() << "Camera check passed: All expected cameras detected";
    }

#ifdef LUCID
    // Program Lucid camera labels from systemConfig.ini before creating camera objects
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

    // Create and show main window
    LAUBackgroundFilterMainWindow window;
    window.show();

    return a.exec();
}
