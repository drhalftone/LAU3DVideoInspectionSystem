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

#include "lauremotetoolspalette.h"
#include <QStandardPaths>
#include <QTextStream>
#include <QCoreApplication>
#include <QXmlStreamReader>
#include <QFileDialog>
#include <QRegularExpression>
#include <tiffio.h>
#include <cmath>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUStatusCheckWorker::LAUStatusCheckWorker(QObject *parent)
    : QObject(parent)
{
}

void LAUStatusCheckWorker::checkStatus()
{
    // Execute all 6 PowerShell scripts and emit results
    bool status1 = (executeButtonScript(1) == 0);
    bool status2 = (executeButtonScript(2) == 0);
    bool status3 = (executeButtonScript(3) == 0);
    bool status4 = (executeButtonScript(4) == 0);
    bool status5 = (executeButtonScript(5) == 0);
    int status6 = (executeButtonScript(6) == 0) ? 1 : 0;  // Invert for internal representation

    emit statusReady(status1, status2, status3, status4, status5, status6);
}

int LAUStatusCheckWorker::executeButtonScript(int buttonId)
{
    // Extract the PowerShell test script from resources to a temporary location
    QString scriptResource = QString(":/scripts/resources/button%1_test.ps1").arg(buttonId);

    QFile resourceFile(scriptResource);
    if (!resourceFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open script resource:" << scriptResource;
        return 1;  // Return RED (failure) if script can't be loaded
    }

    // Create temp file for the script
    QString tempScriptPath = QDir::temp().filePath(QString("button%1_test.ps1").arg(buttonId));
    QFile tempFile(tempScriptPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to create temp script file:" << tempScriptPath;
        return 1;
    }

    tempFile.write(resourceFile.readAll());
    tempFile.close();
    resourceFile.close();

    // Build PowerShell command with parameters
    QStringList arguments;
    arguments << "-ExecutionPolicy" << "Bypass" << "-File" << tempScriptPath;
    arguments << "-InstallPath" << installFolderPath;

    // Execute PowerShell script
    QProcess process;
    process.start("powershell.exe", arguments);
    process.waitForFinished(5000);  // 5 second timeout

    int exitCode = process.exitCode();

    // Clean up temp file
    QFile::remove(tempScriptPath);

    return exitCode;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUStatusButton::LAUStatusButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent), buttonId(-1), currentStatus(Unknown), isSelected(false)
{
    setMinimumHeight(80);
    setMinimumWidth(180);
    setStatus(Unknown);  // Start as GRAY until status check completes
    setMouseTracking(true);
}

void LAUStatusButton::setStatus(Status status)
{
    currentStatus = status;

    QString stylesheet;
    QString borderStyle = isSelected ? "3px solid #000000" : "2px solid";

    switch (status) {
        case Ready:
            // GREEN button
            stylesheet = "QPushButton { "
                        "background-color: #4CAF50; "
                        "color: white; "
                        "font-size: 14px; "
                        "font-weight: bold; "
                        "border: " + (isSelected ? QString("3px solid #000000") : QString("2px solid #45a049")) + "; "
                        "border-radius: 8px; "
                        "padding: 10px; "
                        "text-align: left; "
                        "} "
                        "QPushButton:hover { "
                        "background-color: #45a049; "
                        "border: 3px solid #000000; "
                        "} "
                        "QPushButton:disabled { "
                        "background-color: #9E9E9E; "
                        "border: " + (isSelected ? QString("3px solid #000000") : QString("2px solid #757575")) + "; "
                        "color: #CCCCCC; "
                        "}";
            break;

        case WarmingUp:
            // YELLOW button
            stylesheet = "QPushButton { "
                        "background-color: #FFC107; "
                        "color: white; "
                        "font-size: 14px; "
                        "font-weight: bold; "
                        "border: " + (isSelected ? QString("3px solid #000000") : QString("2px solid #FFA000")) + "; "
                        "border-radius: 8px; "
                        "padding: 10px; "
                        "text-align: left; "
                        "} "
                        "QPushButton:hover { "
                        "background-color: #FFA000; "
                        "border: 3px solid #000000; "
                        "} "
                        "QPushButton:disabled { "
                        "background-color: #9E9E9E; "
                        "border: " + (isSelected ? QString("3px solid #000000") : QString("2px solid #757575")) + "; "
                        "color: #CCCCCC; "
                        "}";
            break;

        case NotReady:
            // RED button
            stylesheet = "QPushButton { "
                        "background-color: #f44336; "
                        "color: white; "
                        "font-size: 14px; "
                        "font-weight: bold; "
                        "border: " + (isSelected ? QString("3px solid #000000") : QString("2px solid #da190b")) + "; "
                        "border-radius: 8px; "
                        "padding: 10px; "
                        "text-align: left; "
                        "} "
                        "QPushButton:hover { "
                        "background-color: #da190b; "
                        "border: 3px solid #000000; "
                        "} "
                        "QPushButton:disabled { "
                        "background-color: #9E9E9E; "
                        "border: " + (isSelected ? QString("3px solid #000000") : QString("2px solid #757575")) + "; "
                        "color: #CCCCCC; "
                        "}";
            break;

        case Unknown:
        default:
            // GRAY button (waiting for status check)
            stylesheet = "QPushButton { "
                        "background-color: #9E9E9E; "
                        "color: white; "
                        "font-size: 14px; "
                        "font-weight: bold; "
                        "border: " + (isSelected ? QString("3px solid #000000") : QString("2px solid #757575")) + "; "
                        "border-radius: 8px; "
                        "padding: 10px; "
                        "text-align: left; "
                        "} "
                        "QPushButton:hover { "
                        "background-color: #757575; "
                        "border: 3px solid #000000; "
                        "} "
                        "QPushButton:disabled { "
                        "background-color: #9E9E9E; "
                        "border: " + (isSelected ? QString("3px solid #000000") : QString("2px solid #757575")) + "; "
                        "color: #CCCCCC; "
                        "}";
            break;
    }

    setStyleSheet(stylesheet);
}

void LAUStatusButton::setSelected(bool selected)
{
    isSelected = selected;
    setStatus(currentStatus);  // Refresh the stylesheet
}

void LAUStatusButton::enterEvent(QEnterEvent *event)
{
    QPushButton::enterEvent(event);
    if (buttonId >= 0) {
        emit mouseEntered(buttonId);
    }
}

void LAUStatusButton::leaveEvent(QEvent *event)
{
    QPushButton::leaveEvent(event);
    emit mouseLeft();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAURemoteToolsPalette::LAURemoteToolsPalette(QWidget *parent)
    : QWidget(parent), statusTimer(nullptr), workerThread(nullptr), statusWorker(nullptr), previousOnTrakStatus(false),
      camerasDetected(false), cameraCheckAttempts(0),
      installerLaunching(false), cameraLabelingLaunching(false), backgroundFilterLaunching(false), jetrStandaloneLaunching(false),
      currentDescriptionButtonId(1)
{
    // In debug mode, use the installed location for tools
    // In release mode, use the directory where the palette executable is located
#ifdef QT_DEBUG
    // Debug mode - look for tools in the installed location
    #ifdef Q_OS_WIN
        installFolderPath = "C:/Program Files (x86)/RemoteRecordingTools";
        qDebug() << "Debug mode - using installed tools at:" << installFolderPath;
    #elif defined(Q_OS_MAC)
        installFolderPath = "/Applications";
    #else
        installFolderPath = "/usr/local/bin";
    #endif
#else
    // Release mode - use the directory where this executable is located
    installFolderPath = QCoreApplication::applicationDirPath();
#endif

    // Shared folder paths remain system-specific
#ifdef Q_OS_WIN
    sharedFolderPath = "C:/ProgramData/3DVideoInspectionTools";
#elif defined(Q_OS_MAC)
    sharedFolderPath = "/Users/Shared/3DVideoInspectionTools";
#else
    sharedFolderPath = "/var/lib/3DVideoInspectionTools";
#endif

    setupUI();

    // Set up worker thread for status checking
    workerThread = new QThread(this);
    statusWorker = new LAUStatusCheckWorker();
    statusWorker->setInstallFolder(installFolderPath);
    statusWorker->moveToThread(workerThread);

    // Connect signals
    connect(workerThread, &QThread::finished, statusWorker, &QObject::deleteLater);
    connect(this, &LAURemoteToolsPalette::destroyed, workerThread, &QThread::quit);
    connect(statusWorker, &LAUStatusCheckWorker::statusReady, this, &LAURemoteToolsPalette::onStatusCheckComplete);

    // Start the worker thread
    workerThread->start();

    // Initialize previousOnTrakStatus to false
    // The first status check will detect the actual state
    previousOnTrakStatus = false;

    // Start status checking timer
    statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &LAURemoteToolsPalette::onCheckStatus);
    statusTimer->start(STATUS_UPDATE_INTERVAL_MS);

    // Set button 1 as initially selected
    button1->setSelected(true);

    // Do initial status check (runs asynchronously in worker thread)
    onCheckStatus();
}

LAURemoteToolsPalette::~LAURemoteToolsPalette()
{
    // Clean up worker thread
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
    }
}

void LAURemoteToolsPalette::setupUI()
{
    // Set fixed window size - height sized to fit menu bar + 6 buttons with spacing
    // Height calculation: menu bar (~25px) + 6 buttons (80px each) + 5 gaps (15px each) + top/bottom margins (10px each) + title bar (~30px) = 630px
    setFixedSize(600, 630);
    setWindowTitle("LAU Remote Tools Palette - Setup Assistant");

    // Create menu bar
    menuBar = new QMenuBar(this);
    QMenu *helpMenu = menuBar->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction(style()->standardIcon(QStyle::SP_MessageBoxInformation), "&About");
    connect(aboutAction, &QAction::triggered, this, &LAURemoteToolsPalette::onAbout);

    // Create main vertical layout to hold menu bar and content
    QVBoxLayout *topLevelLayout = new QVBoxLayout(this);
    topLevelLayout->setContentsMargins(0, 0, 0, 0);
    topLevelLayout->setSpacing(0);
    topLevelLayout->addWidget(menuBar);

    // Create widget to hold the main content
    QWidget *contentWidget = new QWidget();
    topLevelLayout->addWidget(contentWidget);

    // Create main horizontal layout (two panels) for content
    QHBoxLayout *mainLayout = new QHBoxLayout(contentWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // LEFT PANEL: Button stack
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(15);

    // Create 6 status buttons with labels from resource files
    QString label1 = extractMetadata(loadHelpContent(1), "BUTTON_LABEL");
    button1 = new LAUStatusButton(label1.isEmpty() ? "1. System\nConfiguration" : label1);
    button1->buttonId = 1;
    connect(button1, &LAUStatusButton::mouseEntered, this, &LAURemoteToolsPalette::onButtonHovered);
    connect(button1, &QPushButton::clicked, this, &LAURemoteToolsPalette::onButton1Clicked);
    buttonLayout->addWidget(button1);

    QString label2 = extractMetadata(loadHelpContent(2), "BUTTON_LABEL");
    button2 = new LAUStatusButton(label2.isEmpty() ? "2. OnTrak\nPower Control" : label2);
    button2->buttonId = 2;
    connect(button2, &LAUStatusButton::mouseEntered, this, &LAURemoteToolsPalette::onButtonHovered);
    connect(button2, &QPushButton::clicked, this, &LAURemoteToolsPalette::onButton2Clicked);
    buttonLayout->addWidget(button2);

    QString label3 = extractMetadata(loadHelpContent(3), "BUTTON_LABEL");
    button3 = new LAUStatusButton(label3.isEmpty() ? "3. Label\nCameras" : label3);
    button3->buttonId = 3;
    connect(button3, &LAUStatusButton::mouseEntered, this, &LAURemoteToolsPalette::onButtonHovered);
    connect(button3, &QPushButton::clicked, this, &LAURemoteToolsPalette::onButton3Clicked);
    buttonLayout->addWidget(button3);

    QString label4 = extractMetadata(loadHelpContent(4), "BUTTON_LABEL");
    button4 = new LAUStatusButton(label4.isEmpty() ? "4. Record\nBackground" : label4);
    button4->buttonId = 4;
    connect(button4, &LAUStatusButton::mouseEntered, this, &LAURemoteToolsPalette::onButtonHovered);
    connect(button4, &QPushButton::clicked, this, &LAURemoteToolsPalette::onButton4Clicked);
    buttonLayout->addWidget(button4);

    QString label5 = extractMetadata(loadHelpContent(5), "BUTTON_LABEL");
    button5 = new LAUStatusButton(label5.isEmpty() ? "5. Monitor Live\nVideo" : label5);
    button5->buttonId = 5;
    connect(button5, &LAUStatusButton::mouseEntered, this, &LAURemoteToolsPalette::onButtonHovered);
    connect(button5, &QPushButton::clicked, this, &LAURemoteToolsPalette::onButton5Clicked);
    buttonLayout->addWidget(button5);

    QString label6 = extractMetadata(loadHelpContent(6), "BUTTON_LABEL");
    button6 = new LAUStatusButton(label6.isEmpty() ? "6. Calibrate\nSystem" : label6);
    button6->buttonId = 6;
    connect(button6, &LAUStatusButton::mouseEntered, this, &LAURemoteToolsPalette::onButtonHovered);
    connect(button6, &QPushButton::clicked, this, &LAURemoteToolsPalette::onButton6Clicked);
    buttonLayout->addWidget(button6);

    buttonLayout->addStretch();

    // RIGHT PANEL: Description
    descriptionPanel = new QTextEdit();
    descriptionPanel->setReadOnly(true);
    descriptionPanel->setMinimumWidth(380);
    descriptionPanel->setStyleSheet("QTextEdit { "
                                   "background-color: #f5f5f5; "
                                   "border: 2px solid #ddd; "
                                   "border-radius: 8px; "
                                   "padding: 15px; "
                                   "font-size: 13px; "
                                   "}");

    // Show default content (Button 1)
    updateDescriptionPanel(1);

    // Add panels to main layout
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(descriptionPanel);
}

void LAURemoteToolsPalette::onCheckStatus()
{
    // Trigger the worker to check status (runs in background thread)
    QMetaObject::invokeMethod(statusWorker, "checkStatus", Qt::QueuedConnection);
}

void LAURemoteToolsPalette::onStatusCheckComplete(bool status1, bool status2, bool status3, bool status4, bool status5, int status6)
{
    // This runs in the main thread after the worker finishes

    // Detect OnTrak transition from OFF to ON
    if (status2 && !previousOnTrakStatus) {
        // OnTrak just turned on - start the warmup timer
        onTrakStartTime = QDateTime::currentDateTime();
        camerasDetected = false;
        cameraCheckAttempts = 0;
    }
    previousOnTrakStatus = status2;

    // Check if camera warmup timer is complete
    bool warmupComplete = isCameraWarmupComplete();

    // When warmup completes, check if cameras are available
    if (status2 && warmupComplete && !camerasDetected) {
        // Timer just completed - check for cameras
        qDebug() << "Warmup timer complete - checking for cameras (attempt" << (cameraCheckAttempts + 1) << ")";

        if (checkCamerasAvailable()) {
            // Cameras detected!
            camerasDetected = true;
            qDebug() << "✓ Cameras detected successfully";
        } else {
            // Cameras not ready yet - restart timer for another retry interval
            cameraCheckAttempts++;
            onTrakStartTime = QDateTime::currentDateTime();
            qDebug() << "✗ Cameras not detected yet - restarting timer for" << CAMERA_RETRY_SECONDS << "seconds";
        }
    }

    // If OnTrak turns off, reset camera detection
    if (!status2) {
        camerasDetected = false;
        cameraCheckAttempts = 0;
    }

    // Update button colors
    button1->setStatus(status1 ? LAUStatusButton::Ready : LAUStatusButton::NotReady);

    // Button 2: Show yellow during warmup/camera detection, green when cameras ready, red when off
    if (status2) {
        if (camerasDetected) {
            button2->setStatus(LAUStatusButton::Ready);
        } else {
            button2->setStatus(LAUStatusButton::WarmingUp);
        }
    } else {
        button2->setStatus(LAUStatusButton::NotReady);
    }

    button3->setStatus(status3 ? LAUStatusButton::Ready : LAUStatusButton::NotReady);
    button4->setStatus(status4 ? LAUStatusButton::Ready : LAUStatusButton::NotReady);
    button5->setStatus(status5 ? LAUStatusButton::Ready : LAUStatusButton::NotReady);

    // Button 6: Special handling for calibration status (red/green only, no yellow)
    // Red = noCal files exist (need calibration), Green = calibrated (LUTX files exist)
    button6->setStatus(status6 == 1 ? LAUStatusButton::Ready : LAUStatusButton::NotReady);

    // Enable/disable buttons based on prerequisites
    // Button 1 is always enabled (no prerequisites)
    button1->setEnabled(true);

    // Button 2: enabled only if button 1 is green
    button2->setEnabled(status1);

    // Button 3 (Label Cameras): enabled only if buttons 1 AND 2 are green AND cameras detected
    // No cameras detected = gray (disabled)
    button3->setEnabled(status1 && status2 && camerasDetected);

    // Button 4 (Record Background): enabled only if buttons 1, 2, AND 3 are green AND cameras detected
    // No cameras detected = gray (disabled)
    button4->setEnabled(status1 && status2 && status3 && camerasDetected);

    // Button 5 (Process Videos): enabled only if buttons 1, 2, 3, AND 4 are green AND cameras detected
    // background.tif (button 4) is required by LAUProcessVideos for background subtraction
    // No cameras detected = gray (disabled)
    button5->setEnabled(status1 && status2 && status3 && status4 && camerasDetected);

    // Button 6 (Calibrate System): enabled only if OnTrak running AND noCal files exist AND cameras detected
    // OnTrak not running OR no cameras detected OR no noCal files = gray (disabled)
    button6->setEnabled(status2 && hasNoCalFiles() && camerasDetected);

    // Always update the currently displayed description panel
    // This ensures timers and dynamic content update even when not hovering
    updateDescriptionPanel(currentDescriptionButtonId);
}

void LAURemoteToolsPalette::onButtonHovered(int buttonId)
{
    // Clear selection from all buttons
    button1->setSelected(false);
    button2->setSelected(false);
    button3->setSelected(false);
    button4->setSelected(false);
    button5->setSelected(false);
    button6->setSelected(false);

    // Set selection on the hovered button
    switch (buttonId) {
        case 1: button1->setSelected(true); break;
        case 2: button2->setSelected(true); break;
        case 3: button3->setSelected(true); break;
        case 4: button4->setSelected(true); break;
        case 5: button5->setSelected(true); break;
        case 6: button6->setSelected(true); break;
    }

    currentDescriptionButtonId = buttonId;
    updateDescriptionPanel(buttonId);
}

void LAURemoteToolsPalette::updateDescriptionPanel(int buttonId)
{
    bool isReady = false;

    switch (buttonId) {
        case 1: isReady = button1->isReady(); break;
        case 2: isReady = button2->isReady(); break;
        case 3: isReady = button3->isReady(); break;
        case 4: isReady = button4->isReady(); break;
        case 5: isReady = button5->isReady(); break;
        case 6: isReady = button6->isReady(); break;
    }

    QString content = getDescriptionContent(buttonId, isReady);

    // Only update HTML if content has actually changed to preserve scroll position
    if (descriptionPanel->toHtml() != content) {
        // Save current scroll position
        int scrollPosition = descriptionPanel->verticalScrollBar()->value();

        // Update content
        descriptionPanel->setHtml(content);

        // Restore scroll position
        descriptionPanel->verticalScrollBar()->setValue(scrollPosition);
    }
}

QString LAURemoteToolsPalette::getDescriptionContent(int buttonId, bool isReady)
{
    // Load HTML content from resource file
    QString html = loadHelpContent(buttonId);

    if (html.isEmpty()) {
        return "<h2>Error</h2><p>Failed to load help content for this button.</p>";
    }

    // Common replacements for all buttons
    QString statusIcon = isReady ? "✓" : "✗";
    QString statusColor = isReady ? "green" : "red";
    html.replace("{{STATUS_ICON}}", statusIcon);
    html.replace("{{STATUS_COLOR}}", statusColor);

    // Button-specific dynamic content
    switch (buttonId) {
        case 1:  // System Configuration
            {
                QString statusText = isReady ? "systemConfig.ini found" : "systemConfig.ini not found";
                html.replace("{{STATUS_TEXT}}", statusText);
            }
            break;

        case 2:  // OnTrak Power Control
            {
                QString warmupStatus = getWarmupTimeRemaining();
                QString warmupColor = camerasDetected ? "green" : "orange";
                bool isRunning = button2->status() != LAUStatusButton::NotReady;

                QString statusText;
                if (!isRunning) {
                    statusText = "LAUOnTrakWidget.exe not running";
                } else if (camerasDetected) {
                    statusText = "LAUOnTrakWidget.exe running - cameras detected (1 Orbbec + 2 Lucid)";
                } else {
                    statusText = "LAUOnTrakWidget.exe running - waiting for cameras";
                }

                QString warmupStatusHtml;
                if (isRunning && !camerasDetected) {
                    warmupStatusHtml = QString("<p><b>CAMERA DETECTION:</b> <span style='color:%1; font-size:14px; font-weight:bold;'>%2</span></p>")
                        .arg(warmupColor).arg(warmupStatus);
                }

                // Add warning if multiple camera check attempts have failed
                QString relayWarning;
                if (isRunning && !camerasDetected && cameraCheckAttempts >= 3) {
                    bool relayDetected = checkRelayAvailable();

                    if (!relayDetected) {
                        relayWarning = "<p><b><span style='color:red;'>WARNING:</span></b> "
                                      "OnTrak is running but <b>USB relay is NOT detected</b>!<br>"
                                      "<b>Action required:</b></p>"
                                      "<ul>"
                                      "<li><b>Check USB relay connection</b> - Verify OnTrak device is plugged in</li>"
                                      "<li><b>Check Device Manager</b> - Look for OnTrak USB device</li>"
                                      "<li><b>Try different USB port</b> - Some ports may not work</li>"
                                      "<li><b>Restart OnTrak</b> - Close and reopen LAUOnTrakWidget</li>"
                                      "</ul>";
                    } else {
                        relayWarning = "<p><b><span style='color:orange;'>WARNING:</span></b> "
                                      "OnTrak is running and relay is detected, but cameras not found after multiple attempts.<br>"
                                      "<b>Possible causes:</b></p>"
                                      "<ul>"
                                      "<li>PoE power not reaching cameras - Check cables and PoE injector</li>"
                                      "<li>Cameras not connected or powered</li>"
                                      "<li>Camera startup delay - Wait longer and retry</li>"
                                      "<li>Faulty cameras or cables</li>"
                                      "</ul>";
                    }
                }

                html.replace("{{STATUS_TEXT}}", statusText);
                html.replace("{{WARMUP_STATUS}}", warmupStatusHtml);
                html.replace("{{RELAY_WARNING}}", relayWarning);
            }
            break;

        case 3:  // Label Cameras
            {
                QString statusText;
                int positionCount = 0;
                QString configPath = QDir(installFolderPath).filePath("systemConfig.ini");
                QFile file(configPath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    bool inCameraSection = false;
                    while (!in.atEnd()) {
                        QString line = in.readLine().trimmed();
                        if (line == "[CameraPosition]") {
                            inCameraSection = true;
                        } else if (line.startsWith("[") && line.endsWith("]")) {
                            inCameraSection = false;
                        } else if (inCameraSection && line.contains("=")) {
                            positionCount++;
                        }
                    }
                    file.close();
                }

                if (positionCount >= 2) {
                    statusText = QString("Camera positions found in systemConfig.ini (%1 cameras labeled)").arg(positionCount);
                } else if (positionCount == 0) {
                    statusText = "No camera positions found - ready to label";
                } else {
                    statusText = QString("Incomplete camera labels (%1/2 found)").arg(positionCount);
                }

                html.replace("{{STATUS_TEXT}}", statusText);
            }
            break;

        case 4:  // Record Background
            {
                QString statusText;
                QString backgroundPath = QDir(sharedFolderPath).filePath("background.tif");
                if (QFile::exists(backgroundPath)) {
                    statusText = "background.tif found (already recorded)";
                } else {
                    statusText = "background.tif not found - ready to record";
                }

                html.replace("{{STATUS_TEXT}}", statusText);
            }
            break;

        case 5:  // Process Videos
            {
                QString statusText = isReady ? "LAUProcessVideos.exe running" : "LAUProcessVideos.exe not running";
                html.replace("{{STATUS_TEXT}}", statusText);
            }
            break;

        case 6:  // Calibrate System
            {
                // Use the current button status instead of re-running the script
                bool isCalibrated = button6->isReady();
                QString statusText;
                QString whatHappens;

                if (isCalibrated) {
                    // GREEN - Calibrated (LUTX files exist)
                    statusText = "System calibrated";
                    whatHappens = "<p><b>WHAT HAPPENS WHEN YOU CLICK:</b><br>"
                                 "Opens file dialog to select a noCal file. You can recalibrate or adjust existing calibration.</p>";
                } else if (hasNoCalFiles()) {
                    // RED - noCal files exist, need calibration
                    statusText = "⚠ noCal files detected - calibration required";
                    whatHappens = "<p><b>WHAT HAPPENS WHEN YOU CLICK:</b><br>"
                                 "Opens file dialog in temporary folder to select a noCal*.tif file for calibration.<br><br>"
                                 "<b style='color:red;'>Action Required:</b> noCal files indicate recording started without calibration. "
                                 "You must calibrate using one of these files to set bounding boxes and transform matrices.</p>";
                } else {
                    // RED - No noCal files available
                    statusText = "No noCal files available";
                    whatHappens = "<p><b>WHAT HAPPENS WHEN YOU CLICK:</b><br>"
                                 "Button is disabled. noCal files are generated when LAUProcessVideos runs without calibration.</p>";
                }

                html.replace("{{STATUS_TEXT}}", statusText);
                html.replace("{{WHAT_HAPPENS}}", whatHappens);
            }
            break;
    }

    return html;
}

QString LAURemoteToolsPalette::loadHelpContent(int buttonId)
{
    QString filename;
    switch (buttonId) {
        case 0: filename = ":/help/resources/about.html"; break;  // About dialog
        case 1: filename = ":/help/resources/button1.html"; break;
        case 2: filename = ":/help/resources/button2.html"; break;
        case 3: filename = ":/help/resources/button3.html"; break;
        case 4: filename = ":/help/resources/button4.html"; break;
        case 5: filename = ":/help/resources/button5.html"; break;
        case 6: filename = ":/help/resources/button6.html"; break;
        default: return "";
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load help content:" << filename;
        return "";
    }

    return QString::fromUtf8(file.readAll());
}

QString LAURemoteToolsPalette::extractMetadata(const QString &content, const QString &key)
{
    // Extract metadata from HTML comment block
    // Format: <!--METADATA\nBUTTON_LABEL: value\nTITLE: value\n-->

    QRegularExpression metadataRegex("<!--METADATA\\s*([\\s\\S]*?)-->");
    QRegularExpressionMatch match = metadataRegex.match(content);

    if (!match.hasMatch()) {
        return "";
    }

    QString metadata = match.captured(1);

    // Extract the specific key
    QRegularExpression keyRegex(QString("%1:\\s*(.+)").arg(QRegularExpression::escape(key)));
    QRegularExpressionMatch keyMatch = keyRegex.match(metadata);

    if (!keyMatch.hasMatch()) {
        return "";
    }

    QString value = keyMatch.captured(1).trimmed();

    // Replace literal \n with actual newline
    value.replace("\\n", "\n");

    return value;
}

int LAURemoteToolsPalette::executeButtonScript(int buttonId)
{
    // Extract the PowerShell test script from resources to a temporary location
    QString scriptResource = QString(":/scripts/resources/button%1_test.ps1").arg(buttonId);

    QFile resourceFile(scriptResource);
    if (!resourceFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open script resource:" << scriptResource;
        return 1;  // Return RED (failure) if script can't be loaded
    }

    // Create temp file for the script
    QString tempScriptPath = QDir::temp().filePath(QString("button%1_test.ps1").arg(buttonId));
    QFile tempFile(tempScriptPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to create temp script file:" << tempScriptPath;
        return 1;
    }

    tempFile.write(resourceFile.readAll());
    tempFile.close();
    resourceFile.close();

    // Build PowerShell command with parameters
    QStringList arguments;
    arguments << "-ExecutionPolicy" << "Bypass" << "-File" << tempScriptPath;
    arguments << "-InstallPath" << installFolderPath;
    arguments << "-SharedPath" << sharedFolderPath;

    // Add TempPath if available
    if (!localTempPath.isEmpty()) {
        arguments << "-TempPath" << localTempPath;
    }

    // Execute PowerShell script
    QProcess process;
    process.start("powershell.exe", arguments);
    process.waitForFinished(5000);  // 5 second timeout

    int exitCode = process.exitCode();

    // Clean up temp file
    QFile::remove(tempScriptPath);

    return exitCode;
}

int LAURemoteToolsPalette::executeButtonActionScript(int buttonId)
{
    // Extract the PowerShell action script from resources to a temporary location
    QString scriptResource = QString(":/scripts/resources/button%1_action.ps1").arg(buttonId);

    QFile resourceFile(scriptResource);
    if (!resourceFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open action script resource:" << scriptResource;
        return 1;  // Return failure if script can't be loaded
    }

    // Create temp file for the script
    QString tempScriptPath = QDir::temp().filePath(QString("button%1_action.ps1").arg(buttonId));
    QFile tempFile(tempScriptPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to create temp action script file:" << tempScriptPath;
        return 1;
    }

    tempFile.write(resourceFile.readAll());
    tempFile.close();
    resourceFile.close();

    // Build PowerShell command with parameters
    QStringList arguments;
    arguments << "-ExecutionPolicy" << "Bypass" << "-File" << tempScriptPath;
    arguments << "-InstallPath" << installFolderPath;
    arguments << "-SharedPath" << sharedFolderPath;

    // Add TempPath if available
    if (!localTempPath.isEmpty()) {
        arguments << "-TempPath" << localTempPath;
    }

    // Execute PowerShell action script
    QProcess process;
    process.start("powershell.exe", arguments);
    process.waitForFinished(30000);  // 30 second timeout for action scripts (they may do more work)

    int exitCode = process.exitCode();

    // Log output for debugging
    QString output = process.readAllStandardOutput();
    QString errors = process.readAllStandardError();
    if (!output.isEmpty()) {
        qDebug() << "Action script output:" << output;
    }
    if (!errors.isEmpty()) {
        qWarning() << "Action script errors:" << errors;
    }

    // Clean up temp file
    QFile::remove(tempScriptPath);

    return exitCode;
}

bool LAURemoteToolsPalette::hasNoCalFiles()
{
    // Check if noCal*.tif files exist in the temporary video recording folder
    QString tempPath = readLocalTempPathFromConfig();
    if (tempPath.isEmpty()) {
        return false;
    }

    QDir tempDir(tempPath);
    QStringList noCalFiles = tempDir.entryList(QStringList() << "noCal*.tif", QDir::Files);
    return !noCalFiles.isEmpty();
}

bool LAURemoteToolsPalette::isProcessRunning(const QString &processName)
{
#ifdef Q_OS_WIN
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << QString("IMAGENAME eq %1").arg(processName));
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    return output.contains(processName, Qt::CaseInsensitive);
#else
    QProcess process;
    process.start("ps", QStringList() << "aux");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    return output.contains(processName, Qt::CaseInsensitive);
#endif
}

QString LAURemoteToolsPalette::readLocalTempPathFromConfig()
{
    QString configPath = QDir(installFolderPath).filePath("systemConfig.ini");
    QFile file(configPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("LocalTempPath=", Qt::CaseInsensitive)) {
            QString value = line.mid(14).trimmed();  // Skip "LocalTempPath="
            file.close();
            return value;
        }
    }

    file.close();
    return QString();
}

bool LAURemoteToolsPalette::hasValidTransformMatrix(const QString &filePath)
{
    // Open the TIFF file
    TIFF *tiff = TIFFOpen(filePath.toUtf8().constData(), "r");
    if (!tiff) {
        return false;
    }

    // Read the XML packet from the TIFF tags
    int dataLength = 0;
    char *dataString = nullptr;
    bool dataPresent = TIFFGetField(tiff, TIFFTAG_XMLPACKET, &dataLength, &dataString);

    if (!dataPresent || !dataString || dataLength <= 0) {
        TIFFClose(tiff);
        return false;
    }

    // Parse the XML to extract jetrVector
    QByteArray xmlByteArray(dataString, dataLength);
    TIFFClose(tiff);

    // Parse XML using QXmlStreamReader
    QHash<QString, QString> hashTable;
    QXmlStreamReader reader(xmlByteArray);

    if (reader.readNextStartElement()) {
        while (!reader.atEnd()) {
            if (reader.readNextStartElement()) {
                QString nameText = reader.name().toString();
                if (!nameText.isEmpty()) {
                    QString elementText = reader.readElementText();
                    if (!elementText.isEmpty()) {
                        hashTable[nameText] = elementText;
                    }
                }
            }
        }
    }

    // Check if jetrVector exists
    if (!hashTable.contains("jetrVector")) {
        return false;
    }

    // Parse the jetrVector comma-separated values
    QString jetrString = hashTable["jetrVector"];
    QStringList doubleList = jetrString.split(",");

    if (doubleList.count() < 28) {
        // Need at least 28 values (12 initial values + 16 matrix values)
        return false;
    }

    // Check if the first camera's transform matrix (indices 12-27) is identity
    // Transform matrix starts at index 12, it's a 4x4 matrix in row-major order
    bool isIdentity = true;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int idx = 12 + (row * 4 + col);
            double value = doubleList[idx].toDouble();
            double expected = (row == col) ? 1.0 : 0.0;

            if (std::abs(value - expected) > 0.001) {
                isIdentity = false;
                break;
            }
        }
        if (!isIdentity) break;
    }

    // Return true if NOT identity (i.e., transform has been set)
    return !isIdentity;
}

void LAURemoteToolsPalette::onButton1Clicked()
{
    // Execute the action script - all logic is now in PowerShell
    executeButtonActionScript(1);
}

void LAURemoteToolsPalette::onButton2Clicked()
{
    // Execute the action script - all logic is now in PowerShell
    executeButtonActionScript(2);
}

void LAURemoteToolsPalette::onButton3Clicked()
{
    // Execute the action script - all logic is now in PowerShell
    executeButtonActionScript(3);
}

void LAURemoteToolsPalette::onButton4Clicked()
{
    // Execute the action script - all logic is now in PowerShell
    executeButtonActionScript(4);
}

void LAURemoteToolsPalette::onButton5Clicked()
{
    // Execute the action script - all logic is now in PowerShell
    executeButtonActionScript(5);
}

void LAURemoteToolsPalette::onButton6Clicked()
{
    // Execute the action script - all logic is now in PowerShell
    executeButtonActionScript(6);
}

void LAURemoteToolsPalette::launchTool(const QString &toolName)
{
    QString toolPath = QDir(installFolderPath).filePath(toolName);

    if (!QFile::exists(toolPath)) {
        QMessageBox::warning(this, "Tool Not Found",
                           QString("Could not find %1 at:\n%2\n\nPlease verify the installation.")
                           .arg(toolName).arg(toolPath));
        return;
    }

    bool success = false;

    // Launch Background Filter tools through cmd.exe to keep console visible for debugging
    if (toolName.contains("BackgroundFilter", Qt::CaseInsensitive)) {
        // Use cmd.exe /K to keep console window open after application exits
        // This allows viewing debug output and error messages
        QStringList arguments;
        arguments << "/K" << toolPath;
        success = QProcess::startDetached("cmd.exe", arguments);
    } else {
        // Launch other tools directly without console
        success = QProcess::startDetached(toolPath);
    }

    if (!success) {
        QMessageBox::warning(this, "Launch Failed",
                           QString("Failed to launch %1.\n\nPlease try running it manually.")
                           .arg(toolName));
    }
}

void LAURemoteToolsPalette::showTaskSchedulerDialog()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("OnTrak Power Control Setup");
    msgBox.setIcon(QMessageBox::Information);

    QString message =
        "<h3>LAUOnTrakWidget Task Scheduler Setup</h3>"
        "<p>OnTrak must run continuously via Windows Task Scheduler (not launched directly).</p>"
        "<hr>"
        "<p><b>To set up the scheduled task:</b></p>"
        "<ol>"
        "<li>Press <b>Windows+R</b> and type: <code>taskschd.msc</code></li>"
        "<li>Click \"Create Task\" in the right panel</li>"
        "<li><b>General tab:</b>"
        "<ul><li>Name: LAUOnTrakWidget</li>"
        "<li>Check: \"Run with highest privileges\"</li></ul></li>"
        "<li><b>Triggers tab:</b>"
        "<ul><li>New trigger → \"At startup\"</li></ul></li>"
        "<li><b>Actions tab:</b>"
        "<ul><li>New action → Start a program</li>"
        "<li>Program: <code>" + installFolderPath + "/LAUOnTrakWidget.exe</code></li></ul></li>"
        "<li>Click OK to save</li>"
        "</ol>"
        "<hr>"
        "<p><b>To start the task now:</b></p>"
        "<p>Right-click the task and select \"Run\"</p>";

    msgBox.setText(message);
    msgBox.setTextFormat(Qt::RichText);

    QPushButton *openSchedulerBtn = msgBox.addButton("Open Task Scheduler", QMessageBox::ActionRole);
    QPushButton *closeBtn = msgBox.addButton("Close", QMessageBox::RejectRole);
    msgBox.setDefaultButton(closeBtn);

    msgBox.exec();

    // Check which button was clicked
    if (msgBox.clickedButton() == openSchedulerBtn) {
        openTaskScheduler();
    }
}

void LAURemoteToolsPalette::openTaskScheduler()
{
    qDebug() << "Opening Task Scheduler...";

    // Try multiple methods to open Task Scheduler
    bool success = false;

    // Method 1: Direct launch of taskschd.msc
    success = QProcess::startDetached("taskschd.msc", QStringList());

    if (!success) {
        qWarning() << "Method 1 failed, trying cmd.exe...";
        // Method 2: Use cmd.exe to launch it
        success = QProcess::startDetached("cmd.exe", QStringList() << "/c" << "taskschd.msc");
    }

    if (!success) {
        qWarning() << "Method 2 failed, trying full path...";
        // Method 3: Try full path
        success = QProcess::startDetached("C:/Windows/System32/taskschd.msc", QStringList());
    }

    if (!success) {
        QMessageBox::warning(this, "Failed to Open Task Scheduler",
                           "Could not automatically open Task Scheduler.\n\n"
                           "Please open it manually:\n"
                           "1. Press Windows+R\n"
                           "2. Type: taskschd.msc\n"
                           "3. Press Enter");
    } else {
        qDebug() << "Task Scheduler launched successfully";
    }
}

bool LAURemoteToolsPalette::isCameraWarmupComplete() const
{
    // If OnTrak hasn't started yet, warmup is not complete
    if (!onTrakStartTime.isValid()) {
        return false;
    }

    // Check if enough time has passed
    // Use longer warmup for first attempt, shorter retry interval for subsequent attempts
    qint64 secondsElapsed = onTrakStartTime.secsTo(QDateTime::currentDateTime());
    int requiredSeconds = (cameraCheckAttempts == 0) ? CAMERA_WARMUP_SECONDS : CAMERA_RETRY_SECONDS;

    return secondsElapsed >= requiredSeconds;
}

QString LAURemoteToolsPalette::getWarmupTimeRemaining() const
{
    if (!onTrakStartTime.isValid()) {
        return "Waiting for OnTrak to start...";
    }

    if (camerasDetected) {
        return "✓ Cameras detected and ready!";
    }

    qint64 secondsElapsed = onTrakStartTime.secsTo(QDateTime::currentDateTime());
    int requiredSeconds = (cameraCheckAttempts == 0) ? CAMERA_WARMUP_SECONDS : CAMERA_RETRY_SECONDS;
    qint64 secondsRemaining = requiredSeconds - secondsElapsed;

    if (secondsRemaining <= 0) {
        return "Checking for cameras...";
    }

    int minutes = secondsRemaining / 60;
    int seconds = secondsRemaining % 60;

    if (cameraCheckAttempts == 0) {
        return QString("Waiting %1:%2 for cameras to power on...")
            .arg(minutes)
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("Retrying camera detection in %1:%2 (attempt %3)...")
            .arg(minutes)
            .arg(seconds, 2, 10, QChar('0'))
            .arg(cameraCheckAttempts + 1);
    }
}

bool LAURemoteToolsPalette::checkCamerasAvailable()
{
    // Check if cameras are available using LAUBackgroundFilter --check-cameras
    QString toolName;
#ifdef Q_OS_WIN
    toolName = "LAUBackgroundFilter.exe";
#else
    toolName = "LAUBackgroundFilter";
#endif

    QString toolPath = QDir(installFolderPath).filePath(toolName);
    if (!QFile::exists(toolPath)) {
        qWarning() << "LAUBackgroundFilter not found at" << toolPath;
        return false; // Tool not found
    }

    // Run the camera check (synchronously with timeout)
    QProcess process;
    process.start(toolPath, QStringList() << "--check-cameras");

    // Wait up to 5 seconds for the check to complete
    if (!process.waitForFinished(5000)) {
        qWarning() << "Camera check timed out";
        process.kill();
        return false; // Timeout
    }

    // Log the output for debugging
    QString output = process.readAllStandardOutput();
    if (!output.isEmpty()) {
        qDebug() << "Camera check output:" << output;
    }

    // Return true if exit code is 0 (cameras available)
    return process.exitCode() == 0;
}

bool LAURemoteToolsPalette::checkRelayAvailable()
{
    // Check if OnTrak USB relay is available using LAUOnTrakWidget --check-relay
    QString toolName;
#ifdef Q_OS_WIN
    toolName = "LAUOnTrakWidget.exe";
#else
    toolName = "LAUOnTrakWidget";
#endif

    QString toolPath = QDir(installFolderPath).filePath(toolName);
    if (!QFile::exists(toolPath)) {
        qWarning() << "LAUOnTrakWidget not found at" << toolPath;
        return false; // Tool not found
    }

    // Run the relay check (synchronously with timeout)
    QProcess process;
    process.start(toolPath, QStringList() << "--check-relay");

    // Wait up to 3 seconds for the check to complete
    if (!process.waitForFinished(3000)) {
        qWarning() << "Relay check timed out";
        process.kill();
        return false; // Timeout
    }

    // Log the output for debugging
    QString output = process.readAllStandardOutput();
    if (!output.isEmpty()) {
        qDebug() << "Relay check output:" << output;
    }

    // Return true if exit code is 0 (relay available)
    return process.exitCode() == 0;
}

void LAURemoteToolsPalette::onAbout()
{
    // Read recordVideo.cmd version
    QString scriptVersion = "Unknown";
    QString scriptVersionDisplay;
    QString scriptPath = QDir(installFolderPath).filePath("recordVideo.cmd");

    if (QFile::exists(scriptPath)) {
        QFile file(scriptPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                // Look for line like: ECHO recordVideo.cmd - Script Version: 1041
                if (line.contains("Script Version:", Qt::CaseInsensitive)) {
                    // Extract version number
                    int versionIndex = line.indexOf("Script Version:", 0, Qt::CaseInsensitive);
                    if (versionIndex != -1) {
                        QString versionPart = line.mid(versionIndex + 15).trimmed();
                        // Remove any trailing text after the version number
                        scriptVersion = versionPart.split(' ').first().trimmed();
                        break;
                    }
                }
            }
            file.close();
        }

        if (scriptVersion == "Unknown") {
            scriptVersionDisplay = "<span style='color:red;'><b>Can't parse version from recordVideo.cmd</b></span>";
        } else {
            scriptVersionDisplay = "Version " + scriptVersion;
        }
    } else {
        scriptVersionDisplay = "<span style='color:red;'><b>Can't find recordVideo.cmd</b></span>";
    }

    // Get palette executable compile date/time
    QString compileDate = QString("%1 %2").arg(__DATE__).arg(__TIME__);

    // Load About content from HTML resource
    QString aboutMessage = loadHelpContent(0);  // Use button ID 0 for About

    // Replace placeholders
    aboutMessage.replace("{{COMPILE_DATE}}", compileDate);
    aboutMessage.replace("{{SCRIPT_VERSION}}", scriptVersionDisplay);
    aboutMessage.replace("{{INSTALL_PATH}}", installFolderPath);

    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About LAU Remote Tools Palette");
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(aboutMessage);
    aboutBox.setIcon(QMessageBox::Information);
    aboutBox.exec();
}
