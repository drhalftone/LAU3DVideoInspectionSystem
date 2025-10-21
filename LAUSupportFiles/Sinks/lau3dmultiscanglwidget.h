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

#ifndef LAU3DMULTISCANGLWIDGET_H
#define LAU3DMULTISCANGLWIDGET_H

#include "lau3dfiducialglwidget.h"

using namespace LAU3DVideoParameters;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DMultiScanGLWidget : public LAU3DFiducialGLWidget
{
    Q_OBJECT

public:
    LAU3DMultiScanGLWidget(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent = NULL) : LAU3DFiducialGLWidget(cols, rows, color, parent), mutuallyExclusiveFlag(true) { ; }
    ~LAU3DMultiScanGLWidget();

    int count() const
    {
        return (packetList.count());
    }

    void setMutuallyExclusive(bool flag)
    {
        mutuallyExclusiveFlag = flag;
    }

    int currentIndex() const;
    QList<QVector3D> fiducials(QString string) const;
    QList<QVector3D> colors(QString string) const;

public slots:
    void onInsertScan(LAUScan scan, bool flag = true, QList<QVector3D> fiducials = QList<QVector3D>(), QList<QVector3D> colors = QList<QVector3D>()); // CREATE A NEW TEXTURE FROM THE SUPPLIED CPU BUFFER
    void onUpdateScan(LAUScan scan, QList<QVector3D> fiducials = QList<QVector3D>(), QList<QVector3D> colors = QList<QVector3D>());                   // UPDATE THE TEXTURE THAT CORRESPONDS TO THIS PARTICULAR CPU BUFFER
    void onInsertScans(QList<LAUScan> scans, QList<bool> flags = QList<bool>(), QList<QList<QVector3D> > fiducials = QList<QList<QVector3D> >(), QList<QList<QVector3D> > colors = QList<QList<QVector3D> >());
    void onUpdateScans(QList<LAUScan> scans, QList<QList<QVector3D> > fiducials = QList<QList<QVector3D> >(), QList<QList<QVector3D> > colors = QList<QList<QVector3D> >());

    void onSetTransform(QString string, QMatrix4x4 mat);
    void onSetFiducials(QString string, QList<QVector3D> fiducials);
    void onSetFiducials(QString string, QList<QVector3D> fiducials,  QList<QVector3D> colors);

    void onRemoveScan(QString string);   // REMOVE THE TEXTURE THAT CORRESPONDS TO THIS PARTICULAR CPU BUFFER
    void onEnableScan(QString string);   // ENABLE THE DISPLAY OF THIS PARTICULAR CPU BUFFER
    void onDisableScan(QString string);  // DISABLE THE DISPLAY OF THIS PARTICULAR CPU BUFFER

    void onRemoveScan(LAUScan scan)
    {
        onRemoveScan(scan.parentName());
    }

    void onEnableScan(LAUScan scan)
    {
        onEnableScan(scan.parentName());
    }

    void onDisableScan(LAUScan scan)
    {
        onDisableScan(scan.parentName());
    }

    void onRemoveScans(QList<LAUScan> scans);

    void onEnableAll();                  // SHOW ALL TEXTURES
    void onDisableAll();                 // HIDE ALL TEXTURES

protected:
    // Override clearGL() to conditionally clear based on mode
    void clearGL()
    {
        // Only clear in single scan mode; in multi-scan mode clearing happens once in paintGL()
        if (mutuallyExclusiveFlag) {
            LAU3DScanGLWidget::clearGL();
        }
    }

    void initializeGL();
    void paintGL();

private:
    typedef struct {
        bool enabled;
        QString filename;
        QOpenGLTexture *texture;
        QList<QVector3D> fiducials;
        QList<QVector3D> colors;
        float xMin, xMen, xMax, yMin, yMen, yMax, zMin, zMen, zMax;
        QMatrix4x4 transform;
        LAUScan scan;
    } Packet;

    void updateLimits();
    QList<Packet> packetList;
    int indexOf(QString string) const;
    int indexOf(LAUScan scan) const
    {
        return (indexOf(scan.parentName()));
    }
    LAUScan formatScan(LAUScan scan);
    bool mutuallyExclusiveFlag;
};

#endif // LAU3DMULTISCANGLWIDGET_H
