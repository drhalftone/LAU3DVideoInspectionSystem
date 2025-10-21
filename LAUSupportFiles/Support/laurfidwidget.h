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

#ifndef LAUSERIALWIDGET_H
#define LAUSERIALWIDGET_H

#include <QTime>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>

#ifndef HEADLESS
#include <QWidget>
#include <QDialog>
#include <QTextEdit>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QDialogButtonBox>
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAURFIDObject : public QObject
{
    Q_OBJECT

public:
    LAURFIDObject(QString string = QString(), QObject *parent = 0);
    ~LAURFIDObject();

    bool isValid() const
    {
        return (port.isOpen());
    }

    bool isNull() const
    {
        return (!isValid());
    }

    QString lastRFID() const
    {
        return (rfid);
    }

private:
    QString portString;        // PORT STRING
    QSerialPort port;          // INSTANCE OF THE SERIAL PORT
    QString rfid;

private slots:
    void onReadyRead();

signals:
    void emitError(QString);
    void emitRFID(QString, QTime);
};

#ifndef HEADLESS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAURFIDWidget : public QWidget
{
    Q_OBJECT

public:
    LAURFIDWidget(QString string = QString(), QWidget *parent = 0);
    ~LAURFIDWidget();

public slots:
    void onError(QString string)
    {
        qDebug() << string;
    }

    void onRFID(QString string, QTime time)
    {
        // ADD THE INCOMING RFID TAG TO THE TEXT EDIT WINDOW FOR THE USER TO SEE
        textEdit->append(string);

        // PRINT IN THE APPLICATION OUTPUT THE RFID TAG AND THE TIME IT WAS RECORDED
        qDebug() << string << time.toString(Qt::ISODate);
    }

private:
    QTextEdit *textEdit;
    LAURFIDObject *serial;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAURFIDDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAURFIDDialog(QString string = QString(), QWidget *parent = 0) : QDialog(parent), widget(NULL)
    {
        this->setLayout(new QVBoxLayout());
#ifdef Q_OS_WIN
        this->layout()->setContentsMargins(6, 6, 6, 6);
#else
        this->layout()->setContentsMargins(6, 6, 6, 6);
#endif
        this->setWindowTitle(QString("RFID Tag Reader"));

        widget = new LAURFIDWidget(string);
        this->layout()->addWidget(widget);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
        connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
        this->layout()->addWidget(buttonBox);
    }

private:
    LAURFIDWidget *widget;
};
#endif // HEADLESS
#endif // LAUSERIALWIDGET_H
