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

#ifndef LAUOBJECTHASHTABLE_H
#define LAUOBJECTHASHTABLE_H

#include <QTime>
#include <QHash>
#include <QDebug>
#include <QFile>
#include <QSettings>
#ifndef HEADLESS
#include <QFileDialog>
#endif
#include <QStandardPaths>

class LAUObjectHashTable
{
public:
    typedef struct {
        QString string;
        QTime time;
    } Request;

    LAUObjectHashTable(QString filename = QString());
    ~LAUObjectHashTable();

    bool load(QString filename = QString());
    bool save(QString filename);

    QString idString(QString string, QTime time);

    bool contains(QString string) const
    {
        return (hash.contains(string));
    }

    int id(QString string) const
    {
        return (hash.value(string.simplified(), -1));
    }

private:
    QList<int> objectIDs;
    QHash<QString, int> hash;
    QList<Request> requests;
};

#endif // LAUOBJECTHASHTABLE_H
