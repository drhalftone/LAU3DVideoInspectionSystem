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

#ifndef LAUSAVETODISKFILTER_H
#define LAUSAVETODISKFILTER_H

#include "lauabstractfilter.h"

#define NUMBER_HEADER_FRAMES 30

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUSaveToDiskFilter : public LAUAbstractFilter
{
    Q_OBJECT

public:
    explicit LAUSaveToDiskFilter(QString dirString, QObject *parent = NULL);
    ~LAUSaveToDiskFilter()
    {
        qDebug() << QString("LAUSaveToDiskFilter::~LAUSaveToDiskFilter()");
    }

    bool isValid()
    {
        return (directoryString.isEmpty() == false);
    }

    bool isNull()
    {
        return (directoryString.isEmpty());
    }

    void setHeader(LAUMemoryObject object)
    {
        header = object;
    }

    QStringList newFiles() const
    {
        return (newFileList);
    }

public slots:
    void onRecordButtonClicked(bool flag)
    {
        recordFlag = flag;
    }

protected:
    void onStart();
    void onFinish();
    void updateBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);

private:
    bool recordFlag;
    int frameCounter;
    libtiff::TIFF *file;
    QString currentFileString;
    LAUMemoryObject header;
    QStringList newFileList;
    QString directoryString;

    typedef struct {
        LAUMemoryObject depth;
        LAUMemoryObject color;
    } LAUFrame;

    QList<LAUFrame> headerFrames;
    QList<LAUFrame> trailerFrames;

    bool closeOldFile(int frames = -1);
    bool openNewFile();
    QString getNextFilestring();

    QFile logFile;
    QTextStream logTS;

signals:
    void emitNewRecordingOpened(int index);
};

#endif // LAUSAVETODISKFILTER_H
