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

#include "lauobjecthashtable.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUObjectHashTable_requestLessThan(const LAUObjectHashTable::Request &s1, const LAUObjectHashTable::Request &s2)
{
    int a = s1.string.toInt();
    int b = s2.string.toInt();

    if (a == b){
        return (s1.time < s2.time);
    } else {
        return (a < b);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUObjectHashTable_objectIdLessThan(const int &s1, const int &s2)
{
    return (s1 < s2);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUObjectHashTable::LAUObjectHashTable(QString filename)
{
#ifndef HEADLESS
    if (filename.isEmpty()) {
        QSettings settings;
        QString directory = settings.value("LAUObjectHashTable::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        filename = QFileDialog::getOpenFileName(nullptr, QString("Load RFID table from disk (*.csv)"), directory, QString("*.csv"));
        if (filename.isEmpty() == false) {
            settings.setValue(QString("LAUObjectHashTable::lastUsedDirectory"), QFileInfo(filename).absolutePath());
        }
    }
#endif
    load(filename);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUObjectHashTable::~LAUObjectHashTable()
{
    ;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUObjectHashTable::idString(QString string, QTime time)
{
    // KEEP TRACK OF THE LAST ID STRING REQUEST
    static QString previousString;

    // GET THE OBJECT ID CORRESPONDING TO THE INCOMING RFID STRING
    int index = id(string);
    if (index > -1) {
        string = QString("%1").arg(index);
    }

    // SAVE THE OBJECT ID AND REQUEST TIME
    if (string != previousString) {
        previousString = string;
        Request request = { string, time };
        requests << request;
    }

    // RETURN THE OBJECT ID TO THE USER
    return (string);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUObjectHashTable::load(QString filename)
{
    if (filename.isEmpty()) {
        return (false);
    }

    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        // CREATE A REGULAR EXPRESSION TO TEST IS STRING IS ALL NUMBERS
        // LIKE AND RFID TAG SHOULD BE
        QRegularExpression re("\\d*");

        // READ FILE LINE BY LINE UNTIL WE REACH THE END
        while (file.atEnd() == false) {
            QStringList strings = QString(file.readLine()).simplified().split(",");
            strings.removeDuplicates();
            if (strings.count() > 1) {
                bool flag = false;
                int index = strings.first().simplified().toInt(&flag);
                if (flag && index < 50000) {
                    objectIDs << index;
                    while (strings.count() > 1) {
                        QString string = strings.takeLast().simplified();
                        if (re.match(string).hasMatch() && string.length() > 8) {
                            hash.insert(string, index);
                        }
                    }
                }
            }
        }
        std::sort(objectIDs.begin(), objectIDs.end(), LAUObjectHashTable_objectIdLessThan);
        file.close();
        return (true);
    }
    return (false);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUObjectHashTable::save(QString filename)
{
    // MAKE SURE WE HAVE A VALID FILENAME
    if (filename.isEmpty()) {
        return (false);
    }

    // CREATE A FILE TO READ AND WRITE
    QFile file(filename);

    // OPEN THE FILE FOR READING
    if (file.open(QIODevice::ReadOnly)) {
        // READ FILE LINE BY LINE UNTIL WE REACH THE END
        while (file.atEnd() == false){
            QStringList strings = QString(file.readLine()).split(",");

            // POP TIME STAMPS OFF WORKING FROM LEFT TO RIGHT
            while (strings.count() > 1){
                QStringList timeStrings = strings.takeLast().split(":");
                QTime time(timeStrings.at(0).toInt(), timeStrings.at(1).toInt(), timeStrings.at(2).toInt());
                Request request = { strings.at(0).simplified(), time };
                requests << request;
            }
        }
        file.close();
    }

    // NOW DELETE THE OLD FILE AND CREATE A NEW ONE FOR WRITING
    if (file.open(QIODevice::WriteOnly)) {
        // WRITE FILE LINE BY LINE UNTIL WE REACH THE END
        while (objectIDs.count() > 0) {
            int id = objectIDs.takeFirst();

            // FIND ALL REQUESTS FOR THE CURRENT OBJECT ID
            QList<Request> idRequests;
            for (int m = 0; m < requests.count(); m++) {
                if (requests.at(m).string.toInt() == id) {
                    idRequests << requests.takeAt(m);
                }
            }

            // SORT THE REQUESTS FROM EARLIEST TO LATEST
            std::sort(idRequests.begin(), idRequests.end(), LAUObjectHashTable_requestLessThan);

            // WRITE THE TIME STAMPES FOR THE CURRENT OBJECT ID ON THE SAME ROW OF TEXT
            file.write(QString("%1").arg(id).toLocal8Bit());
            while (idRequests.count() > 0) {
                file.write(QString(", %1").arg(idRequests.takeFirst().time.toString()).toLocal8Bit());
            }
            file.write(QString("\n").toLocal8Bit());
        }
        file.close();
        return (true);
    }
    return (false);
}
