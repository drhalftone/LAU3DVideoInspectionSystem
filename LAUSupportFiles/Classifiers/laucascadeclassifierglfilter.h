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

#ifndef LAUCASCADECLASSIFIERGLFILTER_H
#define LAUCASCADECLASSIFIERGLFILTER_H

#include <QTime>
#include <QObject>

#include "laumemoryobject.h"
#include "lauabstractfilter.h"

#ifdef ENABLEFILTERS
#include "lau3dcameras.h"
#include "laurfidwidget.h"
#include "lauobjecthashtable.h"
#include "lau3dvideoglwidget.h"
#include "lausavetodiskfilter.h"
#endif

#ifndef EXCLUDE_LAU3DVIDEOWIDGET
#include "lau3dvideorecordingwidget.h"
#endif

#ifdef ENABLECASCADE
#include "opencv2/objdetect/objdetect.hpp"
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCascadeClassifierFilter : public LAUAbstractFilter
{
    Q_OBJECT

public:
    explicit LAUCascadeClassifierFilter(QString filename, unsigned int cols, unsigned int rows, QWidget *parent = NULL) : LAUAbstractFilter(cols, rows, parent)
    {
#ifdef ENABLECASCADE
        // LOAD CLASSIFIER FROM DISK
        if (QFile(filename).exists()) {
            classifier = cv::CascadeClassifier(filename.toUtf8().constData());
            frame = cv::Mat(rows, cols, CV_8U);
        }
#endif
    }

    ~LAUCascadeClassifierFilter()
    {
        qDebug() << QString("LAUCascadeClassifierFilter::~LAUCascadeClassifierFilter()");
    }

    static LAUMemoryObject localObject;

public slots:

protected:
    void updateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

private:
#ifdef ENABLECASCADE
    cv::Mat frame;
    cv::CascadeClassifier classifier;
#endif
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCascadeClassifierGLFilter : public LAUAbstractGLFilter
{
    Q_OBJECT

public:
    explicit LAUCascadeClassifierGLFilter(QString filename, unsigned int depthCols, unsigned int depthRows, unsigned int colorCols, unsigned int colorRows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent = NULL) : LAUAbstractGLFilter(depthCols, depthRows, colorCols, colorRows, color, device, parent)
    {
        filter = new LAUCascadeClassifierFilter(filename, depthCols, depthRows);
    }

    explicit LAUCascadeClassifierGLFilter(QString filename, unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, LAUVideoPlaybackDevice device, QWidget *parent = NULL) : LAUAbstractGLFilter(cols, rows, color, device, parent)
    {
        filter = new LAUCascadeClassifierFilter(filename, cols, rows);
    }

    ~LAUCascadeClassifierGLFilter();

public slots:

protected:
    void initializeGL();
    void updateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

private:
#ifdef ENABLECASCADE
    cv::Mat frame;
    cv::CascadeClassifier classifier;
#endif
    LAUCascadeClassifierFilter *filter;

    QOpenGLTexture *texture;
    QOpenGLFramebufferObject *frameBufferObjectA, *frameBufferObjectB;
    QOpenGLShaderProgram fillHolesProgram;
};

#ifndef HEADLESS
#ifndef EXCLUDE_LAU3DVIDEOWIDGET
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCascadeClassifierWidget : public LAU3DVideoRecordingWidget
{
    Q_OBJECT

public:
    explicit LAUCascadeClassifierWidget(LAUVideoPlaybackColor color = ColorXYZW, LAUVideoPlaybackDevice device = DevicePrimeSense, QWidget *parent = NULL);
    ~LAUCascadeClassifierWidget();

public slots:

private:

signals:

};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCascadeClassifierDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUCascadeClassifierDialog(LAUVideoPlaybackColor color = ColorXYZRGB, LAUVideoPlaybackDevice device = DevicePrimeSense, QWidget *parent = NULL) : QDialog(parent)
    {
        this->setLayout(new QVBoxLayout());
        this->layout()->setContentsMargins(0, 0, 0, 0);
        widget = new LAUCascadeClassifierWidget(color, device);
        this->layout()->addWidget(widget);

        // CONNECT THE WIDGET TO ITSELF FOR REPLAYING VIDEO
        connect(widget, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), widget, SLOT(onReceiveVideoFrames(QList<LAUMemoryObject>)));
        connect(widget, SIGNAL(emitVideoFrames(LAUMemoryObject)), widget, SLOT(onReceiveVideoFrames(LAUMemoryObject)));
    }

    void enableSnapShotMode(bool state)
    {
        widget->enableSnapShotMode(state);
    }

    QSize size()
    {
        return (widget->size());
    }

    int step()
    {
        return (widget->step());
    }

    int depth()
    {
        return (widget->depth());
    }

    int colors()
    {
        return (widget->colors());
    }

private:
    LAUCascadeClassifierWidget *widget;
};
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCascadeClassifierFromDiskDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUCascadeClassifierFromDiskDialog(QString dirString = QString(), QWidget *parent = NULL);
    ~LAUCascadeClassifierFromDiskDialog();

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
    QFile logFile;
    QFile rfdFile;
    int filterCount;
    QTextStream logTS;
    QTextStream rfdTS;
    LAUMemoryObject object;
    QString directoryString;
    QStringList fileStringList;
    QStringList processedStringList;
    QList<LAUModalityObject> framesList;
    QList<LAUAbstractFilterController *> filterControllers;

#ifdef ENABLEFILTERS
    LAUObjectHashTable *rfidHashTable;
    LAUSaveToDiskFilter *saveToDiskFilter;
#else
    QObject *rfidHashTable;
    QObject *saveToDiskFilter;
#endif

signals:
    void emitBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);

};
#endif // LAUCASCADECLASSIFIERGLFILTER_H
#endif
