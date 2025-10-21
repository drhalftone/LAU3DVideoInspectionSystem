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

#include "lausystemcheckdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QClipboard>
#include <QApplication>
#include <QTimer>

LAUSystemCheckDialog::LAUSystemCheckDialog(QWidget *parent)
    : QDialog(parent), m_allPassed(true)
{
    setWindowTitle("System Check");
    setMinimumSize(700, 600);

    QString results = performSystemCheck();

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Title with status
    QLabel *titleLabel = new QLabel(this);
    titleLabel->setWordWrap(true);
    if (m_allPassed) {
        titleLabel->setText("<h2 style='color: green;'>✓ All System Checks Passed</h2>");
    } else {
        titleLabel->setText("<h2 style='color: red;'>✗ Some System Checks Failed</h2>");
    }
    layout->addWidget(titleLabel);

    // Critical warning for missing Lucid SDK
    bool lucidInstalled = false;
    QStringList lucidPaths = {
        "C:/Program Files/Lucid Vision Labs/Arena SDK",
        "C:/Program Files (x86)/Lucid Vision Labs/Arena SDK"
    };
    for (const QString &path : lucidPaths) {
        if (QDir(path).exists()) {
            lucidInstalled = true;
            break;
        }
    }

    if (!lucidInstalled) {
        QLabel *warningLabel = new QLabel(
            "<p style='color: red; font-weight: bold;'>"
            "⚠ CRITICAL: Lucid Arena SDK drivers are required for camera operation.<br>"
            "Please install from: <a href='https://thinklucid.com/downloads-hub/'>https://thinklucid.com/downloads-hub/</a>"
            "</p>",
            this
        );
        warningLabel->setWordWrap(true);
        warningLabel->setOpenExternalLinks(true);
        layout->addWidget(warningLabel);
    }

    // Results text area
    QTextEdit *textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setPlainText(results);
    textEdit->setFont(QFont("Courier New", 12));
    layout->addWidget(textEdit);

    // Button layout - Copy to Clipboard (left) and Continue/Close (right)
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *copyButton = new QPushButton("Copy to Clipboard", this);
    buttonLayout->addWidget(copyButton);

    buttonLayout->addStretch(); // Push buttons to left and right edges

    QPushButton *actionButton = new QPushButton(this);
    if (m_allPassed) {
        actionButton->setText("Continue");
    } else {
        actionButton->setText("Close");
    }
    actionButton->setDefault(true);
    buttonLayout->addWidget(actionButton);

    layout->addLayout(buttonLayout);

    // Connect copy button to copy results to clipboard
    connect(copyButton, &QPushButton::clicked, [this, results]() {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(results);

        // Show brief confirmation in the title
        QString originalTitle = windowTitle();
        setWindowTitle("System Check - Copied to Clipboard!");
        QTimer::singleShot(1500, this, [this, originalTitle]() {
            setWindowTitle(originalTitle);
        });
    });

    connect(actionButton, &QPushButton::clicked, this, m_allPassed ? &QDialog::accept : &QDialog::reject);

    setLayout(layout);
}

LAUSystemCheckDialog::~LAUSystemCheckDialog()
{
}

QString LAUSystemCheckDialog::performSystemCheck()
{
    QString results;
    int passCount = 0;
    int failCount = 0;

    results += "=== SYSTEM CHECK RESULTS ===\n\n";

    // ========================================================================
    // CHECK LUCID ARENA SDK
    // ========================================================================
    results += "LUCID ARENA SDK (Camera Drivers):\n";
    results += "────────────────────────────────────────\n";

    bool lucidInstalled = false;
    QStringList lucidPaths = {
        "C:/Program Files/Lucid Vision Labs/Arena SDK",
        "C:/Program Files (x86)/Lucid Vision Labs/Arena SDK"
    };

    for (const QString &path : lucidPaths) {
        if (QDir(path).exists()) {
            lucidInstalled = true;
            results += "✓ PASS - Lucid Arena SDK installed\n";
            results += "  Location: " + path + "\n";

            // Check for specific SDK components
            QDir sdkDir(path);
            if (sdkDir.exists("x64Release")) {
                results += "  ✓ ArenaC drivers found\n";
            }
            if (sdkDir.exists("include")) {
                results += "  ✓ SDK headers found\n";
            }
            passCount++;
            break;
        }
    }

    if (!lucidInstalled) {
        m_allPassed = false;
        failCount++;
        results += "✗ FAIL - Lucid Arena SDK NOT INSTALLED\n";
        results += "  Status: REQUIRED for Lucid camera operation\n";
        results += "  Action: Download and install from:\n";
        results += "          https://thinklucid.com/downloads-hub/\n";
        results += "  Note: System will not work without this SDK\n";
    }
    results += "\n";

    // ========================================================================
    // CHECK REQUIRED EXECUTABLES
    // ========================================================================
    results += "REQUIRED APPLICATIONS:\n";
    results += "────────────────────────────────────────\n";

    QDir appDir = QDir::current();
    struct ExeInfo {
        QStringList names;  // Can have multiple possible names
        QString description;
    };

    QList<ExeInfo> requiredExes = {
        {{"LAU3DVideoRecorderMini.exe", "LAU3DVideoRecorder.exe"}, "Main recording application"},
        {{"LAUProcessVideos.exe"}, "Video processing tool"},
        {{"LAUEncodeObjectIDFilter.exe"}, "RFID object ID encoder"},
        {{"LAUOnTrakWidget.exe"}, "OnTrak USB relay controller"}
    };

    for (const ExeInfo &exe : requiredExes) {
        QString foundName;
        bool found = false;

        // Check all possible names
        for (const QString &name : exe.names) {
            if (appDir.exists(name)) {
                foundName = name;
                found = true;
                break;
            }
        }

        if (found) {
            results += "✓ PASS - " + foundName + "\n";
            results += "  Purpose: " + exe.description + "\n";

            // Get file size
            QFileInfo fileInfo(appDir.filePath(foundName));
            double sizeMB = fileInfo.size() / (1024.0 * 1024.0);
            results += QString("  Size: %1 MB\n").arg(sizeMB, 0, 'f', 2);
            passCount++;
        } else {
            m_allPassed = false;
            failCount++;
            results += "✗ FAIL - " + exe.names.first() + " NOT FOUND\n";
            if (exe.names.size() > 1) {
                results += "  (Also checked: " + exe.names.mid(1).join(", ") + ")\n";
            }
            results += "  Purpose: " + exe.description + "\n";
            results += "  Action: Reinstall RemoteRecordingTools\n";
        }
        results += "\n";
    }

    // ========================================================================
    // CHECK OPENCV DEPENDENCIES
    // ========================================================================
    results += "OPENCV DEPENDENCIES:\n";
    results += "────────────────────────────────────────\n";

    QStringList openCVDlls = {
        "opencv_core490.dll",
        "opencv_imgproc490.dll",
        "opencv_highgui490.dll"
    };

    int openCVCount = 0;
    for (const QString &dll : openCVDlls) {
        if (appDir.exists(dll)) {
            openCVCount++;
        }
    }

    if (openCVCount == openCVDlls.size()) {
        results += "✓ PASS - OpenCV libraries found\n";
        results += QString("  Found: %1 core OpenCV DLLs\n").arg(openCVCount);
        passCount++;
    } else {
        m_allPassed = false;
        failCount++;
        results += "✗ FAIL - OpenCV libraries incomplete\n";
        results += QString("  Found: %1 of %2 required DLLs\n").arg(openCVCount).arg(openCVDlls.size());
        results += "  Action: Reinstall RemoteRecordingTools\n";
    }
    results += "\n";

    // ========================================================================
    // CHECK ORBBEC SDK
    // ========================================================================
    results += "ORBBEC SDK (Depth Camera Support):\n";
    results += "────────────────────────────────────────\n";

    if (appDir.exists("OrbbecSDK.dll")) {
        results += "✓ PASS - OrbbecSDK.dll found\n";

        QFileInfo orbbecInfo(appDir.filePath("OrbbecSDK.dll"));
        double sizeMB = orbbecInfo.size() / (1024.0 * 1024.0);
        results += QString("  Size: %1 MB\n").arg(sizeMB, 0, 'f', 2);

        if (appDir.exists("OrbbecSDKConfig.xml")) {
            results += "  ✓ Configuration file found\n";
        }
        passCount++;
    } else {
        m_allPassed = false;
        failCount++;
        results += "✗ FAIL - OrbbecSDK.dll NOT FOUND\n";
        results += "  Status: Required for Orbbec depth cameras\n";
        results += "  Action: Reinstall RemoteRecordingTools\n";
    }
    results += "\n";

    // ========================================================================
    // SUMMARY
    // ========================================================================
    results += "=== SUMMARY ===\n";
    results += QString("Total Tests: %1\n").arg(passCount + failCount);
    results += QString("Passed: %1\n").arg(passCount);
    results += QString("Failed: %1\n").arg(failCount);
    results += "\n";

    if (m_allPassed) {
        results += "✓ ALL CHECKS PASSED - System ready for operation\n";
    } else {
        results += "✗ SOME CHECKS FAILED - Please resolve issues before proceeding\n";
    }

    return results;
}
