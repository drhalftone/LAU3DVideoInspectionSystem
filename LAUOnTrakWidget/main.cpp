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

#include "lauontrakwidget.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_WIN
#include "AduHid.h"
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
// Relay availability check function (no GUI)
int checkRelayAvailable()
{
#ifdef Q_OS_WIN
    // Check for OnTrak USB relay
    int count = ADUCount(0);

    qInfo() << "OnTrak Relay Check Results:";
    qInfo() << "  USB relays detected:" << count;

    if (count > 0) {
        // Try to open the device to verify it's accessible
        void *handle = OpenAduDevice(0);
        if (handle) {
            qInfo() << "✓ OnTrak USB relay is available and accessible";
            CloseAduDevice(handle);
            return 0; // Success
        } else {
            qInfo() << "✗ OnTrak USB relay detected but failed to open";
            qInfo() << "  Check permissions or try running as Administrator";
            return 1; // Failure
        }
    } else {
        qInfo() << "✗ No OnTrak USB relay detected";
        qInfo() << "  Check USB connection and Device Manager";
        return 1; // Failure
    }
#else
    qInfo() << "OnTrak Relay Check Results:";
    qInfo() << "  Platform: Non-Windows (demo mode)";
    qInfo() << "✓ OnTrak relay check not applicable on this platform";
    return 0; // Success (demo mode)
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int main(int argc, char *argv[])
{
    // Check for command-line mode first (before creating GUI)
    for (int i = 1; i < argc; i++) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--check-relay" || arg == "-r") {
            // Run in console mode - no GUI needed
            QCoreApplication a(argc, argv);
            a.setOrganizationName(QString("Lau Consulting Inc"));
            a.setOrganizationDomain(QString("drhalftone.com"));
            a.setApplicationName(QString("LAUOnTrakWidget"));
            return checkRelayAvailable();
        }
    }

    // Normal GUI mode
    QApplication a(argc, argv);
    LAUOnTrakWidget w;
    w.show();
    return a.exec();
}
