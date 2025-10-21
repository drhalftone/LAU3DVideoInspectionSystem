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
#include <QIcon>
#include <QCommandLineParser>
#include <QProcess>
#include <QDir>
#include "lausystemsetupwidget.h"
#include "lausystemcheckdialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Lau Consulting Inc");
    app.setOrganizationDomain("drhalftone.com");
    app.setApplicationName("LAURemoteToolsInstaller");

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("LAU 3D Video Inspection System Setup Tool");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption testOption("test", "Test mode - skip system check enforcement");
    parser.addOption(testOption);

    parser.process(app);

    bool testMode = parser.isSet(testOption);

    // Also skip checks in debug builds
#ifdef QT_DEBUG
    testMode = true;
    qDebug() << "Debug build detected - bypassing system checks";
#endif

    // Show system check dialog first
    LAUSystemCheckDialog systemCheck;
    int result = systemCheck.exec();

    // In test mode (or debug build), always continue to system setup
    // In normal mode, only continue if checks passed
    if (!testMode && (result == QDialog::Rejected || !systemCheck.allChecksPassed())) {
        return 0;
    }

    // All checks passed (or test mode) - show system setup dialog
    LAUSystemSetupWidget dialog;
    dialog.resize(650, 600);

    // Show the dialog
    dialog.exec();

    return 0;
}
