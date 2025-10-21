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

#include "laurfidwidget.h"

QTextStream GS(stdout);

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAURFIDWidget::LAURFIDWidget(QString string, QWidget *parent) : QWidget(parent), textEdit(NULL), serial(NULL)
{
    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);

    QGroupBox *box = new QGroupBox(QString("History"));
    this->layout()->addWidget(box);
    box->setLayout(new QHBoxLayout());
    box->layout()->setContentsMargins(6, 6, 6, 6);

    textEdit = new QTextEdit;
    textEdit->setReadOnly(true);
    box->layout()->addWidget(textEdit);

    serial = new LAURFIDObject(string);
    connect(serial, SIGNAL(emitError(QString)), this, SLOT(onError(QString)));
    connect(serial, SIGNAL(emitRFID(QString, QTime)), this, SLOT(onRFID(QString, QTime)));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAURFIDWidget::~LAURFIDWidget()
{
    if (serial) {
        delete serial;
    }
}
#endif // HEADLESS

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAURFIDObject::LAURFIDObject(QString string, QObject *parent) : QObject(parent), portString(string)
{
    if (portString.isEmpty()) {
        // GET A LIST OF ALL POSSIBLE SERIAL PORTS CURRENTLY AVAILABLE
        QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
        QStringList strings;
        for (int m = 0; m < portList.count(); m++) {
            if (portList.at(m).productIdentifier() == quint16(24577)) {
                portString = portList.at(m).portName();
                break;
            } else {
                strings << QString("%1::%2").arg(portList.at(m).portName()).arg(portList.at(m).productIdentifier());
            }
        }
#ifndef HEADLESS
        if (portString.isEmpty()) {
            if (strings.count() > 1) {
                portString = QInputDialog::getItem(NULL, QString("Serial Port"), QString("Select serial port:"), strings).split("::").first();
            } else if (strings.count() == 1) {
                portString = strings.first().split("::").first();
            }
        }
#endif // HEADLESS
    }

    if (portString.isEmpty() == false) {
        // SET THE SERIAL PORT SETTINGS
        port.setPortName(portString);
#ifdef USE4800BAUDRATE
        port.setBaudRate(QSerialPort::Baud4800);
#else
        port.setBaudRate(QSerialPort::Baud9600);
#endif
        port.setDataBits(QSerialPort::Data8);
        port.setStopBits(QSerialPort::OneStop);
        port.setParity(QSerialPort::NoParity);
        port.setFlowControl(QSerialPort::NoFlowControl);

        // CONNECT THE SERIAL PORT TO THE READY READ SLOT
        connect(&port, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

        // OPEN CONNECTION
        if (port.open(QIODevice::ReadWrite) == false) {
            GS << "Error, can't open serial port!" << port.error() << "\n";
        } else {
            GS << "Serial port is open!" << port.error() << "\n";
        }
        GS.flush();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAURFIDObject::~LAURFIDObject()
{
    if (port.isOpen()) {
        port.close();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAURFIDObject::onReadyRead()
{
    // CREATE A STATIC BYTE ARRAY IN ORDER TO REMEMBER DATA FROM PREVIOUS CALLS
    static QByteArray byteArray;

    // APPEND THE INCOMING BYTES TO OUR CURRENT BYTE ARRAY
    byteArray.append(port.readAll());

    // SEE IF WE HAVE AT LEAST ONE RFID TAG
    QList<QByteArray> byteArrays = byteArray.split('\n');
    while (byteArrays.count() > 1) {
        rfid = QString(byteArrays.takeFirst()).simplified();
        emit emitRFID(rfid, QTime::currentTime());
    }

    // BECAUSE RFID TAGS END IN A NEW LINE CHARACTER, THE LAST BYTE ARRAY
    // IS A PARTIAL RFID TAG OR ITS EMPTY
    byteArray = byteArrays.first();
}
