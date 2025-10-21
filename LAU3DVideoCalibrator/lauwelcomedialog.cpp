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

#include "lauwelcomedialog.h"
#include "laucamerainventorydialog.h"
#include <QMessageBox>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUWelcomeDialog::LAUWelcomeDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUWelcomeDialog::~LAUWelcomeDialog()
{
    // Qt handles cleanup automatically
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUWelcomeDialog::setupUI()
{
    setWindowTitle("Welcome to JETR Standalone Editor");
    setFixedSize(650, 500);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // Check if calibrations exist to customize the message
    QList<LAUCameraCalibration> calibrations = LAUCameraInventoryDialog::getAllCameraCalibrations();
    bool hasCalibrations = !calibrations.isEmpty();

    // Title label
    titleLabel = new QLabel(hasCalibrations ?
        "Welcome to JETR Standalone Editor" :
        "No Camera Calibrations Found");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(hasCalibrations ? "color: #1976d2;" : "color: #d32f2f;");
    mainLayout->addWidget(titleLabel);

    // Message label
    QString message;
    if (hasCalibrations) {
        message =
            "<p><b>About this application:</b></p>"
            "<p>The JETR Standalone Editor allows you to:</p>"
            "<ul>"
            "<li><b>Open TIFF depth files</b> containing JETR camera parameters</li>"
            "<li><b>Edit camera calibrations</b> - adjust intrinsic parameters, transforms, and bounding boxes</li>"
            "<li><b>Set XY plane transforms</b> - align 3D point clouds to a ground plane</li>"
            "<li><b>Save LUTX files</b> - export calibrated lookup tables for use in processing</li>"
            "<li><b>Import LUTX files</b> - load previously saved calibrations</li>"
            "<li><b>Preview results</b> - visualize raw depth images and transformed 3D scans</li>"
            "</ul>"
            "<p><b>Getting Started:</b></p>"
            "<ol>"
            "<li>Click <b>Continue</b> to open a TIFF file</li>"
            "<li>Or click <b>Manage Calibrations</b> to import/export camera calibrations</li>"
            "</ol>";
    } else {
        message =
            "<p>The JETR Editor requires camera calibration data to function properly.</p>"
            "<p>Camera calibrations are stored in <b>LUTX</b> files, which contain the "
            "intrinsic parameters needed to convert depth images to 3D point clouds.</p>"
            "<p><b>To get started:</b></p>"
            "<ol>"
            "<li>Click <b>Manage Calibrations</b> to open the Camera Inventory Manager</li>"
            "<li>Import one or more LUTX calibration files</li>"
            "<li>Click OK to save the calibrations</li>"
            "<li>Return here to begin editing</li>"
            "</ol>"
            "<p>Once calibrations are imported, you'll be able to select camera make/model "
            "when opening TIFF files with JETR vectors.</p>";
    }

    messageLabel = new QLabel(message);
    messageLabel->setWordWrap(true);
    messageLabel->setTextFormat(Qt::RichText);
    mainLayout->addWidget(messageLabel);

    mainLayout->addStretch();

    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    quitButton = new QPushButton(hasCalibrations ? "Quit" : "Quit");
    quitButton->setMinimumHeight(35);
    quitButton->setMinimumWidth(120);
    connect(quitButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(quitButton);

    buttonLayout->addStretch();

    importButton = new QPushButton(hasCalibrations ? "Manage Calibrations" : "Manage Calibrations");
    importButton->setMinimumHeight(35);
    importButton->setMinimumWidth(180);
    connect(importButton, &QPushButton::clicked, this, &LAUWelcomeDialog::onImportClicked);
    buttonLayout->addWidget(importButton);

    QPushButton *continueButton = new QPushButton("Continue");
    continueButton->setMinimumHeight(35);
    continueButton->setMinimumWidth(120);
    continueButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #1976d2;"
        "  color: white;"
        "  font-weight: bold;"
        "  border-radius: 4px;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1565c0;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #0d47a1;"
        "}"
    );
    connect(continueButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(continueButton);

    mainLayout->addLayout(buttonLayout);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUWelcomeDialog::onImportClicked()
{
    // Open the Camera Inventory Dialog
    LAUCameraInventoryDialog inventoryDialog(this);

    if (inventoryDialog.exec() == QDialog::Accepted) {
        // Force QSettings to sync to disk
        QSettings settings;
        settings.sync();

        qDebug() << "Camera Inventory Dialog accepted";
    } else {
        qDebug() << "Camera Inventory Dialog was cancelled or rejected";
    }
}
