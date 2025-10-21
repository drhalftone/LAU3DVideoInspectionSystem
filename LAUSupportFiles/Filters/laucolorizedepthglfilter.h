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

#ifndef LAUCOLORIZEDEPTHGLFILTER_H
#define LAUCOLORIZEDEPTHGLFILTER_H

#include "lauabstractfilter.h"

#ifdef ENABLECASCADE
#ifndef HEADLESS
#include <QDialog>
#include <QTimer>
#include <QDialog>
#include <QShowEvent>
#include <QPushButton>
#include <QOpenGLWidget>
#ifndef EXCLUDE_LAU3DVIDEOWIDGET
#include "lau3dvideowidget.h"
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUColorizerFromDiskDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUColorizerFromDiskDialog(QString dirString = QString(), QWidget *parent = NULL);
    ~LAUColorizerFromDiskDialog();

    bool isValid() const
    {
        return (object.isValid());
    }

    bool isNull() const
    {
        return (object.isNull());
    }

public slots:
    void onUpdateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());
    void onFilterDestroyed()
    {
        filterCount--;
    }

protected:
    void accept();
    void showEvent(QShowEvent *event)
    {
        // CALL THE UNDERLYING WIDGET'S SHOW EVENT HANDLER
        QWidget::showEvent(event);

        // MAKE THE FIRST CALL TO ONUPDATEBUFFER TO START FRAME PROCESSING
        QTimer::singleShot(1000, this, SLOT(onUpdateBuffer()));
    }

private:
    QFile rfdFile;
    int filterCount;
    QTextStream rfdTS;
    libtiff::TIFF *outFile;
    LAUMemoryObject object;
    QString directoryString;
    QStringList fileStringList;
    QStringList processedStringList;
    QStringList newlyCreatedFileList;
    QList<LAUModalityObject> framesList;
    QList<LAUAbstractFilterController *> filterControllers;


signals:
    void emitBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);

};
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUColorizeDepthGLFilter : public LAUAbstractGLFilter
{
    Q_OBJECT

public:
    explicit LAUColorizeDepthGLFilter(unsigned int depthCols, unsigned int depthRows, unsigned int colorCols, unsigned int colorRows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent = nullptr) : LAUAbstractGLFilter(depthCols, depthRows, colorCols, colorRows, color, device, parent), maskFlag(false), qtRadius(1), texture(nullptr), colorFBO(nullptr), depthFBO(nullptr) { ; }
    explicit LAUColorizeDepthGLFilter(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent = nullptr) : LAUAbstractGLFilter(cols, rows, color, device, parent), qtRadius(1), texture(nullptr), colorFBO(nullptr), maskFlag(false), depthFBO(nullptr) { ; }
    ~LAUColorizeDepthGLFilter();

    unsigned int radius() const
    {
        return (qtRadius);
    }

    void setRadius(unsigned int val)
    {
        qtRadius = val;
    }

    void enableMask(bool flag)
    {
        maskFlag = flag;
    }

public slots:
    void onSetRadius(unsigned int val)
    {
        setRadius(val);
    }

protected:
    void initializeGL();
    void updateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

private:
    bool maskFlag;
    unsigned int qtRadius;
    QOpenGLTexture *texture;
    QOpenGLFramebufferObject *colorFBO, *depthFBO;
    QOpenGLShaderProgram colorProgram, depthProgram;
};
#endif
#endif // LAUCOLORIZEDEPTHGLFILTER_H
