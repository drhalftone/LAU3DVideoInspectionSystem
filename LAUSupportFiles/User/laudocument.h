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

#ifndef LAUDOCUMENT_H
#define LAUDOCUMENT_H

#include <QWidget>
#include <QString>
#include <QIODevice>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QProgressDialog>

#include "lauscan.h"

#ifdef AZUREIOT
#include "lauazureiotwidget.h"
#endif

class LAUDocument
{
public:
    LAUDocument(QString filenameString = QString(), QWidget *widget = NULL);
    ~LAUDocument();

    QStringList parentStringList();

    bool saveToDisk(QString filename = QString());

    void sendToCloud();
    void loadFromDisk(QString filenameString = QString());
    void orderChannels(QStringList orderList);

    void makeClean()
    {
        editFlag = false;
    }

    void makeDirty()
    {
        editFlag = true;
    }

    QString duplicateImage(QString string);

    bool save()
    {
        if (editFlag) {
            return (saveToDisk(fileString));
        }
        return (true);
    }

    bool isValid()
    {
        return (imageList.count() > 0);
    }

    QString filename() const
    {
        return (fileString);
    }

    bool isDirty() const
    {
        return (editFlag);
    }

    int count() const
    {
        return (imageList.count());
    }

    QWidget *widget() const
    {
        return (widgetInterface);
    }

    void setWidget(QWidget *widget)
    {
        widgetInterface = widget;
    }

    int indexOf(QString filename);
    LAUScan image(int index);
    LAUScan image(QString filename, int *index = NULL);
    LAUScan takeImage(QString string, int *index = NULL);
    QList<LAUScan> takeImages(QStringList strings);
    QList<LAUScan> images() const
    {
        return (imageList);
    }
    QImage preview(QString parentName);
    QList<QImage> previews();

    void removeImage(LAUScan image);
    void replaceImage(LAUScan image);
    void removeImage(QString filename);
    QStringList insertImage(QString filename);
    void insertImage(LAUScan image, int index = -1);
    void insertImages(QList<LAUScan> scans);
    int inspectImage(QString filename);
    bool exists(QString filename);
    QString getNextAvailableFilename();

    LAUVideoPlaybackColor color() const
    {
        if (imageList.isEmpty()) {
            return (ColorUndefined);
        }
        return (imageList.first().color());
    }

    static int allDocumentCounter;
    static int untitledDocumentCounter;

private:
    bool editFlag;
    QString fileString;
    QWidget *widgetInterface;
    QList<LAUScan> imageList;
};

#endif // LAUDOCUMENT_H
