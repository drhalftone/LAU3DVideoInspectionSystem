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

#include "lau3dmultiscanglwidget.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DMultiScanGLWidget::~LAU3DMultiScanGLWidget()
{
    if (wasInitialized()) {
        // MAKE THIS THE CURRENT OPENGL CONTEXT
        makeCurrent();

        // DELETE PLAYBACK TEXTURES
        while (packetList.count() > 0) {
            delete packetList.takeFirst().texture;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onInsertScans(QList<LAUScan> scans, QList<bool> flags, QList<QList<QVector3D> > fiducials, QList<QList<QVector3D> > colors)
{
    for (int n = 0; n < scans.count(); n++) {
        if (n < flags.count() && n < fiducials.count()) {
            onInsertScan(scans.at(n), flags.at(n), fiducials.at(n), colors.at(n));
        } else if (n < fiducials.count()) {
            onInsertScan(scans.at(n), flags.at(n));
        } else if (n < flags.count()) {
            onInsertScan(scans.at(n), false, fiducials.at(n), colors.at(n));
        } else {
            onInsertScan(scans.at(n));
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onUpdateScans(QList<LAUScan> scans, QList<QList<QVector3D> > fiducials, QList<QList<QVector3D> > colors)
{
    for (int n = 0; n < scans.count(); n++) {
        if (n < fiducials.count()) {
            onUpdateScan(scans.at(n), fiducials.at(n), colors.at(n));
        } else {
            onUpdateScan(scans.at(n));
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onRemoveScans(QList<LAUScan> scans)
{
    for (int n = 0; n < scans.count(); n++) {
        onRemoveScan(scans.at(n));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onInsertScan(LAUScan scan, bool flag, QList<QVector3D> fiducials, QList<QVector3D> colors)
{
    // FIRST, MAKE SURE INCOMING BUFFER ISN'T ALREADY IN THE LIST
    if (indexOf(scan) == -1) {
        // MAKE SURE SCAN IS CORRECT FORMAT
        scan = formatScan(scan);

        QOpenGLTexture *texture = nullptr;
        if (wasInitialized()) {
            // MAKE THIS THE CURRENT OPENGL CONTEXT
            makeCurrent();

            // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE DEPTH TO COLOR VIDEO MAPPING
            texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
            if (texture) {
                if ((color() == ColorXYZRGB) || (color() == ColorXYZWRGBA)) {
                    texture->setSize(2 * scan.width(), scan.height());
                } else {
                    texture->setSize(scan.width(), scan.height());
                }
                texture->setFormat(QOpenGLTexture::RGBA32F);
                texture->setWrapMode(QOpenGLTexture::ClampToBorder);
                texture->setMinificationFilter(QOpenGLTexture::Nearest);
                texture->setMagnificationFilter(QOpenGLTexture::Nearest);
                texture->allocateStorage();

                if (glGetError() == GL_NO_ERROR){
                    switch (color()) {
                        case ColorGray:
                            texture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                            break;
                        case ColorRGB:
                        case ColorXYZRGB:
                            texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                            break;
                        case ColorRGBA:
                        case ColorXYZG:
                        case ColorXYZWRGBA:
                            texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                            break;
                        default:
                            break;
                    }
                } else {
                    qDebug() << "OpenGL error!";
                }
            }
            Packet packet = { flag, scan.parentName(), texture, fiducials, colors, scan.minX(), scan.centroid().x(), scan.maxX(), scan.minY(), scan.centroid().y(), scan.maxY(), scan.minZ(), scan.centroid().z(), scan.maxZ(), scan.transform(), LAUScan() };
            packetList.prepend(packet);
            updateLimits();

            // MAKE THIS THE CURRENT TEXTURE IN THE WIDGET
            onSetTexture(packet.texture);
        } else {
            // SAVE THE SCAN UNTIL THAT TIME WHEN THIS WIDGET IS INITIALIZED
            Packet packet = { flag, scan.parentName(), NULL, fiducials, colors, scan.minX(), scan.centroid().x(), scan.maxX(), scan.minY(), scan.centroid().y(), scan.maxY(), scan.minZ(), scan.centroid().z(), scan.maxZ(), scan.transform(), scan };
            packetList.prepend(packet);
        }
    } else {
        // SEE IF WE NEED TO ENABLE/DISABLE TEXTURE AND THEN DECIDE
        // WHEN TO UPDATE THE TEXTURE
        if (flag) {
            onUpdateScan(scan, fiducials, colors);
            onEnableScan(scan);
        } else {
            onDisableScan(scan);
            onUpdateScan(scan, fiducials, colors);
        }
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onUpdateScan(LAUScan scan, QList<QVector3D> fiducials, QList<QVector3D> colors)
{
    // FIRST, MAKE SURE INCOMING BUFFER ISN'T ALREADY IN THE LIST
    int index = indexOf(scan);
    if (index != -1) {
        // MAKE SURE SCAN IS CORRECT FORMAT
        scan = formatScan(scan);

        // GRAB THE ASSOCIATED PACKET FROM THE LIST
        Packet packet = packetList.takeAt(index);
        if (packet.texture) {
            makeCurrent();
            switch (color()) {
                case ColorGray:
                    packet.texture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                    break;
                case ColorRGB:
                case ColorXYZRGB:
                    packet.texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                    break;
                case ColorRGBA:
                case ColorXYZG:
                case ColorXYZWRGBA:
                    packet.texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                    break;
                default:
                    break;
            }
        }
        packet.transform = scan.transform();
        packet.filename = scan.parentName();
        packet.fiducials = fiducials;
        packet.colors = colors;

        // PLACE THE PACKET AT THE HEAD OF THE LIST
        packetList.prepend(packet);
        updateLimits();

        // ONLY UPDATE THE DISPLAY IF THE CURRENT TEXTURE IS ENABLED
        if (packet.enabled) {
            update();
        }
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onSetTransform(QString string, QMatrix4x4 transform)
{
    int index = indexOf(string);
    if (index >= 0) {
        Packet packet = packetList.at(index);
        packet.transform = transform;
        packetList.replace(index, packet);
        update();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onSetFiducials(QString string, QList<QVector3D> fiducials, QList<QVector3D> colors)
{
    int index = indexOf(string);
    if (index >= 0) {
        Packet packet = packetList.at(index);
        packet.fiducials = fiducials;
        packet.colors = colors;
        packetList.replace(index, packet);
        update();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onSetFiducials(QString string, QList<QVector3D> fiducials)
{
    int index = indexOf(string);
    if (index >= 0) {
        Packet packet = packetList.at(index);
        packet.fiducials = fiducials;
        while (packet.colors.count() << packet.fiducials.count()) {
            packet.colors << QVector3D(0.0, 0.0, 0.0);
        }
        packetList.replace(index, packet);
        update();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onRemoveScan(QString string)
{
    // FIRST, MAKE SURE INCOMING BUFFER ISN'T ALREADY IN THE LIST
    int index = indexOf(string);
    if (index != -1) {
        Packet packet = packetList.takeAt(index);
        if (packet.texture) {
            makeCurrent();
            delete packet.texture;
        }
        updateLimits();
        update();
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onEnableScan(QString string)
{
    // FIND PACKET THAT GOES WITH THIS STRING
    int index = indexOf(string);
    if (index != -1) {
        // WE NEED TO COPY THE FIDUCIALS AND COLORS FROM THE CURRENT SCAN IN THE BASE FIDUCIALGLWIDGET CLASS
        int previous = currentIndex();
        if (previous != -1) {
            Packet packet = packetList.takeAt(previous);
            packet.fiducials = LAU3DFiducialGLWidget::fiducials();
            packet.colors = LAU3DFiducialGLWidget::colors();
            if (mutuallyExclusiveFlag) {
                packet.enabled = false;
            }
            packetList.prepend(packet);
        }
        Packet packet = packetList.takeAt(index);
        packet.enabled = true;
        packetList.prepend(packet);

        // MAKE THIS PACKET THE CURRENT PACKET ON SCREEN
        LAU3DFiducialGLWidget::onSetTexture(packet.texture);
        LAU3DFiducialGLWidget::onSetFiducials(packet.fiducials, packet.colors);
        update();
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onDisableScan(QString string)
{
    // FIRST, MAKE SURE INCOMING BUFFER ISN'T ALREADY IN THE LIST
    int index = indexOf(string);
    if (index != -1) {
        Packet packet = packetList.takeAt(index);
        packet.enabled = false;
        packetList.prepend(packet);

        // FIND THE NEW ACTIVE PACKET
        int previous = currentIndex();
        if (previous != -1) {
            Packet packet = packetList.takeAt(previous);
            packetList.prepend(packet);

            // MAKE THIS PACKET THE CURRENT PACKET ON SCREEN
            LAU3DFiducialGLWidget::onSetTexture(packet.texture);
            LAU3DFiducialGLWidget::onSetFiducials(packet.fiducials, packet.colors);
        }
        update();
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onEnableAll()
{
    for (int n = 0; n < packetList.count(); n++) {
        if (packetList.at(n).enabled == false) {
            Packet packet = packetList.at(n);
            packet.enabled = true;
            packetList.replace(n, packet);
        }
    }
    update();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::onDisableAll()
{
    for (int n = 0; n < packetList.count(); n++) {
        if (packetList.at(n).enabled == true) {
            Packet packet = packetList.at(n);
            packet.enabled = false;
            packetList.replace(n, packet);
        }
    }
    update();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::updateLimits()
{
    xMin = 1e9;
    xMax = -1e9;
    xMen = 0.0;

    yMin = 1e9;
    yMax = -1e9;
    yMen = 0.0;

    zMin = 1e9;
    zMax = -1e9;
    zMen = 0.0;

    for (int n = 0; n < packetList.count(); n++) {
        xMin = qMin(xMin, packetList.at(n).xMin);
        xMax = qMax(xMax, packetList.at(n).xMax);
        yMin = qMin(yMin, packetList.at(n).yMin);
        yMax = qMax(yMax, packetList.at(n).yMax);
        zMin = qMin(zMin, packetList.at(n).zMin);
        zMax = qMax(zMax, packetList.at(n).zMax);

        xMen = xMen + packetList.at(n).xMen;
        yMen = yMen + packetList.at(n).yMen;
        zMen = zMen + packetList.at(n).zMen;
    }

    xMen = xMen / (float)packetList.count();
    yMen = yMen / (float)packetList.count();
    zMen = zMen / (float)packetList.count();

    setLimits(xMin, xMax, yMin, yMax, zMin, zMax, xMen, yMen, zMen);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QVector3D> LAU3DMultiScanGLWidget::fiducials(QString string) const
{
    int index = indexOf(string);
    if (index >= 0) {
        return (packetList.at(index).fiducials);
    }
    return (QList<QVector3D>());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QVector3D> LAU3DMultiScanGLWidget::colors(QString string) const
{
    int index = indexOf(string);
    if (index >= 0) {
        return (packetList.at(index).colors);
    }
    return (QList<QVector3D>());
}

#ifdef DONTCOMPILE
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::mousePressEvent(QMouseEvent *event)
{
    // SET THE FIDUCIAL DRAGGING FLAGS TO FALSE UNTIL WE KNOW BETTER
    fiducialDragMode = false;

    if (event->button() == Qt::LeftButton && packetList.count() > 0) {
        // SEE IF WE ARE IN CLOSE PROXIMITY TO A FIDUCIAL
        float x = 2.0f * (float)event->pos().x() / (float)this->width() - 1.0f;
        float y = 2.0f * (float)event->pos().y() / (float)this->height() - 1.0f;

        // SET THE THRESHOLD FOR HOW CLOSE A MOUSE CLICK NEEDS TO BE TO BE CONSIDERED A FIDUCIAL CLICK
        float tolerance = fiducialRadius * width() / zoomFactor;

        // STORE THE CLOSEST FIDUCIAL
        QVector3D closestFiducial(1e9, 1e9, -1e9);

        fiducialList = packetList.first().fiducials;
        for (int n = 0; n < fiducialList.count(); n++) {
            // CALCULATE THE WINDOW COORDINATE OF THE CURRENT FIDUCIAL
            QVector3D fiducial = fiducialList.at(n);
            QVector4D coordinate = projection * QVector4D(fiducial.x(), fiducial.y(), fiducial.z(), 1.0f);
            coordinate = coordinate / coordinate.w();

            // CALCULATE THE DISTANCE FROM THE FIDUCIAL TO THE EVENT COORDINATE IN PIXELS
            QVector2D position((coordinate.x() - x) / 2.0f * (float)width(), (coordinate.y() + y) / 2.0f * (float)height());

            // SEE IF THIS FIDUCIAL IS WITHIN A CLOSE PROXIMITY OF THE MOUSE CLICK
            if (position.length() < tolerance) {
                if (fiducial.z() > closestFiducial.z()) {
                    fiducialDragMode = true;
                    currentActivePointIndex = n;
                    closestFiducial = fiducial;
                }
            }
        }

        // CHECK TO SEE IF WE SHOULD ENABLE FIDUCIAL TRACKING
        if (fiducialDragMode) {
            // AND WE NEED TO GRAB A COPY OF THE MOUSE BUFFER TO DRAGGING THE FIDUCIAL
            screenMap = grabMouseBuffer();
            // UPDATE THE SCREEN SO THAT WE AT LEAST CHANGE THE COLOR OF THE CURRENT FIDUCIAL
            update();
        } else {
            // LET THE UNDERLYING CLASS HANDLE THE EVENT
            LAU3DScanGLWidget::mousePressEvent(event);
        }
    } else {
        // LET THE UNDERLYING CLASS HANDLE THE EVENT
        LAU3DScanGLWidget::mousePressEvent(event);
        // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
        updateFiducialProjectionMatrix();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (fiducialDragMode) {
        fiducialDragMode = false;
    } else {
        // LET THE UNDERLYING CLASS HANDLE THE EVENT
        LAU3DScanGLWidget::mouseReleaseEvent(event);
        // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
        updateFiducialProjectionMatrix();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && packetList.count() > 0) {
        // WE NEED TO GRAB A COPY OF THE MOUSE BUFFER TO POSITION THE FIDUCIAL
        screenMap = grabMouseBuffer();

        // CONVERT THE WIDGET COORDINATE TO A SCREEN MAP COORDINATE
        int row = (float)event->pos().y() / (float)height() * (int)screenMap.height();
        int col = (float)event->pos().x() / (float)width() * (int)screenMap.width();

        if (row >= 0 && row < (int)screenMap.height() && col >= 0 && col < (int)screenMap.width()) {
            // GRAB THE XYZW-PIXEL FOR THE CURRENT MOUSE POSITION
            float *pixel = &((float *)screenMap.constScanLine(row))[4 * col];

            // MAKE SURE THAT THE W-COORDINATE IS NOT EQUAL TO 0.0F
            if (pixel[3] > 0.5f) {
                // APPEND THE FIDUCIAL LIST WITH THE CURRENT MOUSE POSITION
                QVector3D fiducial(pixel[0], pixel[1], pixel[2]);

                Packet packet = packetList.takeFirst();
                packet.fiducials.append(fiducial);
                currentActivePointIndex = packet.fiducials.count() - 1;
                packetList.prepend(packet);

                // DRAW THE NEW FIDUCIAL ON SCREEN
                update();
            } else {
                // IF THE USER DOUBLE CLICKS AN AREA WITH NO SCAN DATA, THEN CALL
                // THE UNDERLYING CLASS TO HANDLE THE DOUBLE CLICK
                LAU3DScanGLWidget::mouseDoubleClickEvent(event);
                // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
                updateFiducialProjectionMatrix();
            }
        }
    } else {
        // LET THE UNDERLYING CLASS HANDLE THE EVENT
        LAU3DScanGLWidget::mouseDoubleClickEvent(event);
        // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
        updateFiducialProjectionMatrix();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (fiducialDragMode && currentActivePointIndex >= 0) {
        // CONVERT THE WIDGET COORDINATE TO A SCREEN MAP COORDINATE
        int row = (float)event->pos().y() / (float)height() * (int)screenMap.height();
        int col = (float)event->pos().x() / (float)width() * (int)screenMap.width();

        if (row >= 0 && row < (int)screenMap.height() && col >= 0 && col < (int)screenMap.width()) {
            // GRAB THE XYZW-PIXEL FOR THE CURRENT MOUSE POSITION
            float *pixel = &((float *)screenMap.constScanLine(row))[4 * col];

            // MAKE SURE THAT THE W-COORDINATE IS NOT EQUAL TO 0.0F
            if (pixel[3] > 0.5f) {
                // REPLACE THE FIDUCIAL WITH THE CURRENT MOUSE POSITION
                QVector3D fiducial(pixel[0], pixel[1], pixel[2]);

                Packet packet = packetList.takeFirst();
                packet.fiducials.replace(currentActivePointIndex, fiducial);
                packetList.prepend(packet);

                update();
            }
        }
    } else {
        // LET THE UNDERLYING CLASS HANDLE THE EVENT
        LAU3DScanGLWidget::mouseMoveEvent(event);
        // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
        updateFiducialProjectionMatrix();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::keyPressEvent(QKeyEvent *event)
{
    // IF THERE IS NO CURRENTLY ACTIVE POINTS, THEN QUIT
    if (currentActivePointIndex == -1) {
        return;
    }

    // CHECK FOR AN UP OR DOWN ARROW TO CHANGE THE CURRENT FIDUCIAL
    // CHECK FOR DELETE TO REMOVE A FIDUCIAL FROM THE LIST
    if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Up) {
        currentActivePointIndex = (currentActivePointIndex + 1) % (packetList.first().fiducials.count());
    } else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Down) {
        currentActivePointIndex = (currentActivePointIndex + (packetList.first().fiducials.count()) - 1) % (packetList.first().fiducials.count());
    } else if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        Packet packet = packetList.takeFirst();
        packet.fiducials.removeAt(currentActivePointIndex);
        currentActivePointIndex = qMin(currentActivePointIndex, packet.fiducials.count() - 1);
        packetList.prepend(packet);
    }
    update();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::wheelEvent(QWheelEvent *event)
{
    // IGNORE WHEEL EVENTS IF WE ARE IN DRAG MODE
    if (fiducialDragMode == false) {
        // LET THE UNDERLYING CLASS HANDLE THE EVENT
        LAU3DScanGLWidget::wheelEvent(event);
        // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
        updateFiducialProjectionMatrix();
    }
}
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAU3DMultiScanGLWidget::formatScan(LAUScan scan)
{
    // MAKE SURE SCAN IS THE APPROPRIATE SIZE
    if (scan.width() != static_cast<unsigned int>(size().width()) || scan.height() != static_cast<unsigned int>(size().height())) {
        scan = scan.resize(static_cast<unsigned int>(size().width()), static_cast<unsigned int>(size().height()));
    }

    // MAKE SURE INCOMING SCAN IS THE RIGHT PLAYBACK COLOR
    if (scan.color() != color()) {
        scan = scan.convertToColor(color());
    }
    return (scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAU3DMultiScanGLWidget::currentIndex() const
{
    for (int n = 0; n < packetList.count(); n++) {
        if (packetList.at(n).enabled) {
            return (n);
        }
    }
    return (-1);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAU3DMultiScanGLWidget::indexOf(QString string) const
{
    for (int n = 0; n < packetList.count(); n++) {
        if (packetList.at(n).filename == string) {
            return (n);
        }
    }
    return (-1);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::initializeGL()
{
    // CALL THE BASECLASS METHOD FOR CREATING VERTICES, INDICES, TEXTURES, AND SHADER PROGRAMS
    LAU3DFiducialGLWidget::initializeGL();

    // CREATE TEXTURES FOR ALL THE PACKETS THAT HAVE BEEN WAITING FOR A VALID CONTEXT
    for (int n = 0; n < packetList.count(); n++) {
        Packet packet = packetList.at(n);
        if (packet.texture == nullptr) {
            // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE DEPTH TO COLOR VIDEO MAPPING
            QOpenGLTexture *texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
            if (texture) {
                if ((color() == ColorXYZRGB) || (color() == ColorXYZWRGBA)) {
                    texture->setSize(2 * packet.scan.width(), packet.scan.height());
                } else {
                    texture->setSize(packet.scan.width(), packet.scan.height());
                }
                texture->setFormat(QOpenGLTexture::RGBA32F);
                texture->setWrapMode(QOpenGLTexture::ClampToBorder);
                texture->setMinificationFilter(QOpenGLTexture::Nearest);
                texture->setMagnificationFilter(QOpenGLTexture::Nearest);
                texture->allocateStorage();
                switch (color()) {
                    case ColorGray:
                        texture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)packet.scan.constPointer());
                        break;
                    case ColorRGB:
                    case ColorXYZRGB:
                        texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)packet.scan.constPointer());
                        break;
                    case ColorRGBA:
                    case ColorXYZG:
                    case ColorXYZWRGBA:
                        texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)packet.scan.constPointer());
                        break;
                    default:
                        break;
                }
                packet.texture = texture;
                packet.scan = LAUScan();
                packetList.replace(n, packet);
            }
        }
    }

    int index = currentIndex();
    if (index > -1) {
        Packet packet = packetList.takeAt(index);
        packetList.prepend(packet);
        LAU3DFiducialGLWidget::onSetTexture(packet.texture);
        LAU3DFiducialGLWidget::onSetFiducials(packet.fiducials, packet.colors);
    }

    updateLimits();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DMultiScanGLWidget::paintGL()
{
    // STOP ITERATING THROUGH FOR LOOP BECAUSE ONLY ONE PACKET CAN BE ENABLED
    if (mutuallyExclusiveFlag) {
        LAU3DFiducialGLWidget::paintGL();
    } else {
        // Qt 6 compatibility: Clear buffer once before drawing multiple scans
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ITERATE THROUGH ALL PACKETS
        for (int n = 0; n < packetList.count(); n++) {
            if (packetList.at(n).enabled) {
                // SEND THE TEXTURE AND FIDUCIALS TO THE UNDERLYING CLASSES
                LAU3DFiducialGLWidget::onSetTexture(packetList.at(n).texture);
                LAU3DFiducialGLWidget::onUpdateScanTransform(packetList.at(n).transform);
                //LAU3DFiducialGLWidget::onSetFiducials(packetList.at(n).fiducials, packetList.at(n).colors);

                LAU3DFiducialGLWidget::paintGL();
            }
        }
    }
    return;
}
