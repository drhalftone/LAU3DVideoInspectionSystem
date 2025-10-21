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
#include "laumenuwidget.h"

namespace libtiff
{
#include "tiffio.h"
}
using namespace LAU3DVideoParameters;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int main(int argc, char *argv[])
{
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
#if !defined(Q_PROCESSOR_ARM) && (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    // Qt 6 enables high DPI scaling by default, so only set this for Qt 5
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication a(argc, argv);

    a.setOrganizationName(QString("Lau Consulting Inc"));
    a.setOrganizationDomain(QString("drhalftone.com"));
    a.setApplicationName(QString("LAU3DVideoRecorder"));
    a.setQuitOnLastWindowClosed(false);

    // Register meta types for Qt signal/slot system
    qRegisterMetaType<LAUMemoryObject>("LAUMemoryObject");
    qRegisterMetaType<QList<LAUScan>>("QList<LAUScan>");
    qRegisterMetaType<LAUScan>("LAUScan");

    // Set TIFF error and warning handlers
    libtiff::TIFFSetErrorHandler(myTIFFErrorHandler);
    libtiff::TIFFSetWarningHandler(myTIFFWarningHandler);

    // Show main splash screen
    LAUSplashScreen w;
    w.show();

    return a.exec();
}
