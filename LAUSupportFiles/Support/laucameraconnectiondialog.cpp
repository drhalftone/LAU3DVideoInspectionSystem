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

#include "laucameraconnectiondialog.h"
#include <QVBoxLayout>
#include <QFont>
#include <QPushButton>
#include <QScrollBar>

// Static members
QtMessageHandler LAUCameraConnectionDialog::previousHandler = nullptr;
LAUCameraConnectionDialog *LAUCameraConnectionDialog::staticInstance = nullptr;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCameraConnectionDialog::LAUCameraConnectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Connecting to Cameras");
    setModal(true);
    setMinimumSize(600, 400);
    resize(700, 500);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Title label
    titleLabel = new QLabel("Connecting to cameras and synchronizing...");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    // Output text area
    outputText = new QTextEdit();
    outputText->setReadOnly(true);
    outputText->setFont(QFont("Courier", 9));
    outputText->setStyleSheet("QTextEdit { background-color: #f5f5f5; color: #333; }");
    layout->addWidget(outputText);

    // Info label
    QLabel *infoLabel = new QLabel(
        "Camera connection and PTP synchronization may take 30-60 seconds.\n"
        "This window will close automatically when cameras are ready.");
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    layout->addWidget(infoLabel);

    // Cancel button
    QPushButton *cancelButton = new QPushButton("Cancel");
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(cancelButton);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCameraConnectionDialog::~LAUCameraConnectionDialog()
{
    uninstallMessageHandler();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCameraConnectionDialog *LAUCameraConnectionDialog::instance()
{
    return staticInstance;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraConnectionDialog::installMessageHandler()
{
    staticInstance = this;
    previousHandler = qInstallMessageHandler(messageHandler);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraConnectionDialog::uninstallMessageHandler()
{
    if (staticInstance == this) {
        qInstallMessageHandler(previousHandler);
        staticInstance = nullptr;
        previousHandler = nullptr;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraConnectionDialog::appendMessage(const QString &message)
{
    QMutexLocker locker(&mutex);

    // Append to text edit (must be done in GUI thread)
    QMetaObject::invokeMethod(outputText, [this, message]() {
        outputText->append(message);
        // Auto-scroll to bottom
        outputText->verticalScrollBar()->setValue(outputText->verticalScrollBar()->maximum());
    }, Qt::QueuedConnection);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraConnectionDialog::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // Forward to previous handler if it exists
    if (previousHandler) {
        previousHandler(type, context, msg);
    }

    // Capture qDebug(), qInfo(), qWarning(), and qCritical() messages
    // Include qDebug to show camera connection status messages
    if (type == QtDebugMsg || type == QtInfoMsg || type == QtWarningMsg || type == QtCriticalMsg) {
        if (staticInstance) {
            QString prefix;
            switch (type) {
                case QtDebugMsg:    prefix = "[DEBUG] "; break;
                case QtInfoMsg:     prefix = "[INFO] "; break;
                case QtWarningMsg:  prefix = "[WARN] "; break;
                case QtCriticalMsg: prefix = "[ERROR] "; break;
                default: prefix = ""; break;
            }

            staticInstance->appendMessage(prefix + msg);
        }
    }
}
