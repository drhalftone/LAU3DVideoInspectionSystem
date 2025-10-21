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

#ifndef LAUCONTROLLER_H
#define LAUCONTROLLER_H

#include <QDebug>
#include <QThread>
#include <QObject>
#ifdef HEADLESS
#include <QCoreApplication>
#else
#include <QApplication>
#endif

class LAUController : public QObject
{
    Q_OBJECT

public:
    explicit LAUController(QObject *obj, QObject *parent = NULL) : QObject(parent), object(obj), thread(nullptr)
    {
        // CREATE A THREAD TO HOST THE INCOMING OBJECT
        thread = new QThread();

        // CONNECT THE THREAD AND OBJECT TO COORDINATE DECONSTRUCTOR
        connect(thread, SIGNAL(finished()), object, SLOT(deleteLater()));
        connect(object, SIGNAL(destroyed()), thread, SLOT(deleteLater()));

        // MOVE OBJECT TO THREAD AND START THE THREAD
        object->moveToThread(thread);
        thread->start();

        // WAIT HERE UNTIL WE KNOW THREAD IS RUNNING
        while (thread->isRunning() == false) {
            qApp->processEvents();
        }
    }

    ~LAUController()
    {
        // STOP AND DELETE THREAD
        if (thread) {
            thread->quit();
            //while (thread->isRunning()) {
            //    if (qApp) {
            //        qApp->processEvents();
            //    }
            //}
            //thread->deleteLater();
        } else if (object) {
            object->deleteLater();
        }
        qDebug() << QString("LAUController::~LAUController()");
    }

private:
    QThread *thread;
    QObject *object;
};

#endif // LAUCONTROLLER_H
