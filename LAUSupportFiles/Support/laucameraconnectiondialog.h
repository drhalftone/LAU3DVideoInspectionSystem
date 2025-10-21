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

#ifndef LAUCAMERACONNECTIONDIALOG_H
#define LAUCAMERACONNECTIONDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QMutex>
#include <QMutexLocker>
#include <QtGlobal>

class LAUCameraConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUCameraConnectionDialog(QWidget *parent = nullptr);
    ~LAUCameraConnectionDialog();

    // Install/uninstall custom message handler
    void installMessageHandler();
    void uninstallMessageHandler();

    // Add message to dialog
    void appendMessage(const QString &message);

    // Static instance for message handler
    static LAUCameraConnectionDialog *instance();

private:
    QLabel *titleLabel;
    QTextEdit *outputText;
    QMutex mutex;

    // Previous message handler
    static QtMessageHandler previousHandler;
    static LAUCameraConnectionDialog *staticInstance;

    // Custom message handler function
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
};

#endif // LAUCAMERACONNECTIONDIALOG_H
