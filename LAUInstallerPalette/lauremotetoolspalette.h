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

#ifndef LAUREMOTETOOLSPALETTE_H
#define LAUREMOTETOOLSPALETTE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QEnterEvent>
#include <QDateTime>
#include <QCursor>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QScrollBar>
#include <QStyle>
#include <QThread>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
// Worker thread for executing PowerShell status check scripts
class LAUStatusCheckWorker : public QObject
{
    Q_OBJECT

public:
    explicit LAUStatusCheckWorker(QObject *parent = nullptr);
    void setInstallFolder(const QString &path) { installFolderPath = path; }

public slots:
    void checkStatus();

signals:
    void statusReady(bool status1, bool status2, bool status3, bool status4, bool status5, int status6);

private:
    QString installFolderPath;
    int executeButtonScript(int buttonId);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUStatusButton : public QPushButton
{
    Q_OBJECT

public:
    enum Status {
        Unknown,    // GRAY (initial state before status check)
        NotReady,   // RED
        WarmingUp,  // YELLOW
        Ready       // GREEN
    };

    explicit LAUStatusButton(const QString &text, QWidget *parent = nullptr);

    void setStatus(Status status);
    void setSelected(bool selected);
    bool isReady() const { return currentStatus == Ready; }
    Status status() const { return currentStatus; }

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void mouseEntered(int buttonId);
    void mouseLeft();

private:
    int buttonId;
    Status currentStatus;
    bool isSelected;

    friend class LAURemoteToolsPalette;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAURemoteToolsPalette : public QWidget
{
    Q_OBJECT

public:
    explicit LAURemoteToolsPalette(QWidget *parent = nullptr);
    ~LAURemoteToolsPalette();

private slots:
    void onCheckStatus();
    void onStatusCheckComplete(bool status1, bool status2, bool status3, bool status4, bool status5, int status6);
    void onButton1Clicked();  // System Configuration
    void onButton2Clicked();  // OnTrak Power Control
    void onButton3Clicked();  // Label Cameras
    void onButton4Clicked();  // Record Background
    void onButton5Clicked();  // Calibrate System
    void onButton6Clicked();  // Process Videos
    void onButtonHovered(int buttonId);
    void onAbout();  // Show About dialog

private:
    // UI Components
    QMenuBar *menuBar;
    LAUStatusButton *button1;  // System Configuration
    LAUStatusButton *button2;  // OnTrak Power Control
    LAUStatusButton *button3;  // Label Cameras
    LAUStatusButton *button4;  // Record Background
    LAUStatusButton *button5;  // Calibrate System
    LAUStatusButton *button6;  // Process Videos
    QTextEdit *descriptionPanel;

    // Status checking
    QTimer *statusTimer;
    QThread *workerThread;
    LAUStatusCheckWorker *statusWorker;
    QString installFolderPath;
    QString sharedFolderPath;
    QString localTempPath;

    // OnTrak power-on timing and camera detection
    QDateTime onTrakStartTime;
    bool previousOnTrakStatus;
    static const int CAMERA_WARMUP_SECONDS = 120;  // 2 minutes initial warmup (cameras need ~2min to fully power on)
    static const int CAMERA_RETRY_SECONDS = 30;    // 30 seconds between camera checks
    static const int STATUS_UPDATE_INTERVAL_MS = 5000;  // Status check interval in milliseconds (5 seconds to reduce PowerShell overhead)
    bool camerasDetected;
    int cameraCheckAttempts;

    // Launch protection flags to prevent multiple instances during startup
    bool installerLaunching;
    bool cameraLabelingLaunching;
    bool backgroundFilterLaunching;
    bool jetrStandaloneLaunching;

    // Track which button description is currently displayed
    int currentDescriptionButtonId;

    // Helper methods
    void setupUI();
    void updateDescriptionPanel(int buttonId);
    QString getDescriptionContent(int buttonId, bool isReady);
    QString loadHelpContent(int buttonId);
    QString extractMetadata(const QString &content, const QString &key);

    // Status checking methods
    int executeButtonScript(int buttonId);  // Execute PowerShell test script for button, returns exit code (0=GREEN, 1=RED)
    int executeButtonActionScript(int buttonId);  // Execute PowerShell action script for button, returns exit code

    bool isProcessRunning(const QString &processName);
    bool hasValidTransformMatrix(const QString &filePath);
    QString readLocalTempPathFromConfig();
    bool hasNoCalFiles();  // Check if noCal*.tif files exist in temp folder
    bool isCameraWarmupComplete() const;
    QString getWarmupTimeRemaining() const;
    bool checkCamerasAvailable();
    bool checkRelayAvailable();

    // Tool launching methods
    void launchTool(const QString &toolName);
    void showTaskSchedulerDialog();
    void openTaskScheduler();
};

#endif // LAUREMOTETOOLSPALETTE_H
