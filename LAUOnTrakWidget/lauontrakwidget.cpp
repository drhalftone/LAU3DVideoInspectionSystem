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

#include "lauontrakwidget.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QLocalSocket>
#include <QMessageBox>
#include <QThread>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QStyle>
#include <QFile>

#ifdef Q_OS_WIN
#include "AduHid.h"
#endif

LAUOnTrakWidget::LAUOnTrakWidget(QWidget *parent) : QMainWindow(parent), cyclingInProgress(false), isMasterInstance(true)
#ifndef Q_OS_WIN
    , demoModeK0State(false), demoModeK1State(false)
#endif
{
    // SET THE WINDOW PROPERTIES
    this->setWindowTitle(QString("LAU On Trak Widget"));
    
    // Create central widget and layout
    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);
    centralWidget->setLayout(new QVBoxLayout());
    centralWidget->layout()->setContentsMargins(6, 6, 6, 6);
    
    // Create menu bar
    createMenuBar();

    buttonA = new QPushButton("On Track K0");
    buttonA->setAutoFillBackground(true);
    buttonA->setFixedSize(320,90);
    buttonA->setCheckable(false);
    buttonA->setFlat(true);
    buttonA->setToolTip(QString("OnTrak Relay K0 Control\n\n"
                                "Click to toggle relay K0 on/off\n\n"
                                "Color meanings:\n"
                                "• Yellow: No OnTrak device connected (demo mode)\n"
                                "• Red: Relay K0 is OFF\n"
                                "• Green: Relay K0 is ON\n"
                                "• Blue: Processing/waiting state\n\n"
                                "Daily limit: 3 cycles per day for equipment protection"));
    connect(buttonA, SIGNAL(clicked()), this, SLOT(onToggleK0()));
    centralWidget->layout()->addWidget(buttonA);

    buttonB = new QPushButton("On Track K1");
    buttonB->setAutoFillBackground(true);
    buttonB->setFixedSize(320,90);
    buttonB->setCheckable(false);
    buttonB->setFlat(true);
    buttonB->setToolTip(QString("OnTrak Relay K1 Control\n\n"
                                "Click to toggle relay K1 on/off\n\n"
                                "Color meanings:\n"
                                "• Yellow: No OnTrak device connected (demo mode)\n"
                                "• Red: Relay K1 is OFF\n"
                                "• Green: Relay K1 is ON\n"
                                "• Blue: Processing/waiting state\n\n"
                                "Daily limit: 3 cycles per day for equipment protection"));
    connect(buttonB, SIGNAL(clicked()), this, SLOT(onToggleK1()));
    centralWidget->layout()->addWidget(buttonB);

    paletteUp = buttonA->palette();
    paletteDn = buttonB->palette();
    paletteWait = buttonA->palette();

    paletteUp.setColor(QPalette::Base , QColor(196, 64, 64));
    paletteUp.setColor(QPalette::Button , QColor(196, 64, 64));
    paletteUp.setColor(QPalette::Window , QColor(196, 64, 64));
    paletteUp.setColor(QPalette::WindowText , QColor(196, 64, 64));

    paletteDn.setColor(QPalette::Base , QColor(64, 196, 64));
    paletteDn.setColor(QPalette::Button , QColor(64, 196, 64));
    paletteDn.setColor(QPalette::Window , QColor(64, 196, 64));
    paletteDn.setColor(QPalette::WindowText , QColor(64, 196, 64));

    paletteWait.setColor(QPalette::Base , QColor(64, 64, 196));
    paletteWait.setColor(QPalette::Button , QColor(64, 64, 196));
    paletteWait.setColor(QPalette::Window , QColor(64, 64, 196));
    paletteWait.setColor(QPalette::WindowText , QColor(64, 64, 196));

    QPalette paletteNo = buttonA->palette();
    paletteNo.setColor(QPalette::Base , QColor(196, 196, 64));
    paletteNo.setColor(QPalette::Button , QColor(196, 196, 64));
    paletteNo.setColor(QPalette::Window , QColor(196, 196, 64));
    paletteNo.setColor(QPalette::WindowText , QColor(196, 196, 64));

    buttonA->setPalette(paletteNo);
    buttonA->repaint();

    buttonB->setPalette(paletteNo);
    buttonB->repaint();

#ifdef Q_OS_WIN
    int count = ADUCount(0);
    if (count > 0){
        handle = OpenAduDevice(0);
        if (handle){
            onToggleK0();
            onToggleK1();
        }
    }
#else
    // Demo mode - set initial states
    setWindowTitle(QString("LAU On Trak Widget - DEMO MODE"));
    buttonA->setPalette(paletteUp);
    buttonB->setPalette(paletteUp);
#endif

    // Initialize relay off timer
    relayOffTimer = new QTimer(this);
    relayOffTimer->setSingleShot(true);
    connect(relayOffTimer, SIGNAL(timeout()), this, SLOT(onRelayOffTimeout()));

    // Check if another instance is already running
    if (checkForExistingInstance()) {
        // Another instance is running - become slave
        setSlaveMode();
    } else {
        // First instance - start IPC server
        startIPCServer();
    }
    
    // Initialize settings for daily limit tracking
    settings = new QSettings("LAUOnTrak", "RelayController");
}

LAUOnTrakWidget::~LAUOnTrakWidget()
{
#ifdef Q_OS_WIN
    if (handle) {
        CloseAduDevice(handle);
    }
#endif
    if (ipcServer) {
        ipcServer->close();
        delete ipcServer;
    }
    if (masterCheckSocket) {
        masterCheckSocket->disconnectFromServer();
        delete masterCheckSocket;
    }
    delete settings;
}

void LAUOnTrakWidget::createMenuBar()
{
    // Get the menu bar
    QMenuBar *mainMenuBar = this->menuBar();
    
    // Create File menu
    QMenu *fileMenu = mainMenuBar->addMenu("&File");
    
    exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setStatusTip("Exit the application");
    exitAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    fileMenu->addAction(exitAction);
    
    // Create Tools menu
    QMenu *toolsMenu = mainMenuBar->addMenu("&Tools");
    
    resetCounterAction = new QAction("&Reset Daily Counter", this);
    resetCounterAction->setStatusTip("Reset the daily cycle counter");
    resetCounterAction->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(resetCounterAction, SIGNAL(triggered()), this, SLOT(onResetDailyCounter()));
    toolsMenu->addAction(resetCounterAction);
    
    statusAction = new QAction("Show &Status", this);
    statusAction->setStatusTip("Show current status and daily cycle count");
    statusAction->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(statusAction, SIGNAL(triggered()), this, SLOT(onShowStatus()));
    toolsMenu->addAction(statusAction);
    
    // Create Help menu
    QMenu *helpMenu = mainMenuBar->addMenu("&Help");
    
    aboutAction = new QAction("&About", this);
    aboutAction->setStatusTip("Show information about this application");
    aboutAction->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(onAbout()));
    helpMenu->addAction(aboutAction);
    
    // Add status bar
    statusBar()->showMessage("Ready");
}

void LAUOnTrakWidget::onToggleK0()
{
    // Don't allow relay control in slave mode
    if (!isMasterInstance) {
        statusBar()->showMessage("Relay control disabled - Slave mode", 3000);
        return;
    }
    
    static bool state = false;
#ifdef Q_OS_WIN
    if (handle){
        // SEND SK0 TO TURN ON RELAY FOR OUTPUT K0
        char bytes[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        if (state){
            int ret = WriteAduDevice(handle, "RK0", 4, 0, 0);
            if (ret == 0){
                return;
            }

            // SEND RPK0 TO REQUEST READ OUTPUT K0
            ret = WriteAduDevice(handle, "RPK0", 4, 0, 0);
            if (ret == 1){
                // READ OUTPUT K0
                ret = ReadAduDevice(handle, bytes, 4, 0, 0);
                if (ret == 1){
                    if (bytes[0] == 0x30){
                        state = false;
                        buttonA->setPalette(paletteUp);
                        buttonA->repaint();
                    }
                }
            }
        } else {
            int ret = WriteAduDevice(handle, "SK0", 4, 0, 0);
            if (ret == 0){
                return;
            }

            // SEND RPK0 TO REQUEST READ OUTPUT K0
            ret = WriteAduDevice(handle, "RPK0", 4, 0, 0);
            if (ret == 1){
                // READ OUTPUT K0
                ret = ReadAduDevice(handle, bytes, 4, 0, 0);
                if (ret == 1){
                    if (bytes[0] == 0x31){
                        state = true;
                        buttonA->setPalette(paletteDn);
                        buttonA->repaint();
                    }
                }
            }
        }
    }
#else
    // Demo mode - just toggle the state and update UI
    demoModeK0State = !demoModeK0State;
    state = demoModeK0State;
    buttonA->setPalette(demoModeK0State ? paletteDn : paletteUp);
    buttonA->repaint();
#endif
}

void LAUOnTrakWidget::onToggleK1()
{
    // Don't allow relay control in slave mode
    if (!isMasterInstance) {
        statusBar()->showMessage("Relay control disabled - Slave mode", 3000);
        return;
    }
    
    static bool state = false;
#ifdef Q_OS_WIN
    if (handle){
        // SEND SK0 TO TURN ON RELAY FOR OUTPUT K0
        char bytes[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        if (state){
            int ret = WriteAduDevice(handle, "RK1", 4, 0, 0);
            if (ret == 0){
                return;
            }

            // SEND RPK0 TO REQUEST READ OUTPUT K0
            ret = WriteAduDevice(handle, "RPK1", 4, 0, 0);
            if (ret == 1){
                // READ OUTPUT K0
                ret = ReadAduDevice(handle, bytes, 4, 0, 0);
                if (ret == 1){
                    if (bytes[0] == 0x30){
                        state = false;
                        buttonB->setPalette(paletteUp);
                        buttonB->repaint();
                    }
                }
            }
        } else {
            int ret = WriteAduDevice(handle, "SK1", 4, 0, 0);
            if (ret == 0){
                return;
            }

            // SEND RPK0 TO REQUEST READ OUTPUT K0
            ret = WriteAduDevice(handle, "RPK1", 4, 0, 0);
            if (ret == 1){
                // READ OUTPUT K0
                ret = ReadAduDevice(handle, bytes, 4, 0, 0);
                if (ret == 1){
                    if (bytes[0] == 0x31){
                        state = true;
                        buttonB->setPalette(paletteDn);
                        buttonB->repaint();
                    }
                }
            }
        }
    }
#else
    // Demo mode - just toggle the state and update UI
    demoModeK1State = !demoModeK1State;
    state = demoModeK1State;
    buttonB->setPalette(demoModeK1State ? paletteDn : paletteUp);
    buttonB->repaint();
#endif
}

void LAUOnTrakWidget::setRelayK0(bool state)
{
#ifdef Q_OS_WIN
    if (handle){
        char bytes[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        
        if (state){
            int ret = WriteAduDevice(handle, "SK0", 4, 0, 0);
            if (ret == 0) return;
            
            ret = WriteAduDevice(handle, "RPK0", 4, 0, 0);
            if (ret == 1){
                ret = ReadAduDevice(handle, bytes, 4, 0, 0);
                if (ret == 1 && bytes[0] == 0x31){
                    buttonA->setPalette(paletteDn);
                    buttonA->repaint();
                }
            }
        } else {
            int ret = WriteAduDevice(handle, "RK0", 4, 0, 0);
            if (ret == 0) return;
            
            ret = WriteAduDevice(handle, "RPK0", 4, 0, 0);
            if (ret == 1){
                ret = ReadAduDevice(handle, bytes, 4, 0, 0);
                if (ret == 1 && bytes[0] == 0x30){
                    buttonA->setPalette(paletteUp);
                    buttonA->repaint();
                }
            }
        }
    }
#else
    // Demo mode - just update the state and UI
    demoModeK0State = state;
    buttonA->setPalette(state ? paletteDn : paletteUp);
    buttonA->repaint();
#endif
}

void LAUOnTrakWidget::setRelayK1(bool state)
{
#ifdef Q_OS_WIN
    if (handle){
        char bytes[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        
        if (state){
            int ret = WriteAduDevice(handle, "SK1", 4, 0, 0);
            if (ret == 0) return;
            
            ret = WriteAduDevice(handle, "RPK1", 4, 0, 0);
            if (ret == 1){
                ret = ReadAduDevice(handle, bytes, 4, 0, 0);
                if (ret == 1 && bytes[0] == 0x31){
                    buttonB->setPalette(paletteDn);
                    buttonB->repaint();
                }
            }
        } else {
            int ret = WriteAduDevice(handle, "RK1", 4, 0, 0);
            if (ret == 0) return;
            
            ret = WriteAduDevice(handle, "RPK1", 4, 0, 0);
            if (ret == 1){
                ret = ReadAduDevice(handle, bytes, 4, 0, 0);
                if (ret == 1 && bytes[0] == 0x30){
                    buttonB->setPalette(paletteUp);
                    buttonB->repaint();
                }
            }
        }
    }
#else
    // Demo mode - just update the state and UI
    demoModeK1State = state;
    buttonB->setPalette(state ? paletteDn : paletteUp);
    buttonB->repaint();
#endif
}

void LAUOnTrakWidget::onCycleRelays()
{
    // Don't allow relay control in slave mode
    if (!isMasterInstance) {
        statusBar()->showMessage("Relay cycling disabled - Slave mode", 3000);
        return;
    }
    
    if (cyclingInProgress) return;
    
    // Check daily limit
    if (!checkDailyLimit()) {
        return;
    }
    
    cyclingInProgress = true;
    
    // Increment daily counter
    incrementDailyCounter();
    
    // Disable buttons during cycling
    buttonA->setEnabled(false);
    buttonB->setEnabled(false);
    
    // Set buttons to "waiting" color
    buttonA->setPalette(paletteWait);
    buttonB->setPalette(paletteWait);
    buttonA->repaint();
    buttonB->repaint();
    
    // Turn off both relays
    setRelayK0(false);
    setRelayK1(false);
    
    // Start 30 second timer
    relayOffTimer->start(5000);
}

void LAUOnTrakWidget::onRelayOffTimeout()
{
    // Turn relays back on
    setRelayK0(true);
    setRelayK1(true);
    
    // Re-enable buttons
    buttonA->setEnabled(true);
    buttonB->setEnabled(true);
    
    cyclingInProgress = false;
}

void LAUOnTrakWidget::startIPCServer()
{
    ipcServer = new QLocalServer(this);
    
    // Remove any existing server
    QLocalServer::removeServer("LAUOnTrakWidget");
    
    if (!ipcServer->listen("LAUOnTrakWidget")) {
        qCritical() << "Unable to start IPC server:" << ipcServer->errorString();
        // Continue anyway - widget will still function for manual relay control
        return;
    }
    
    connect(ipcServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

void LAUOnTrakWidget::onNewConnection()
{
    QLocalSocket *socket = ipcServer->nextPendingConnection();
    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
}

void LAUOnTrakWidget::onReadyRead()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) return;

    QByteArray data = socket->readAll();
    QString command = QString::fromUtf8(data).trimmed();

    if (command == "CYCLE_RELAYS") {
        // Track total requests received (regardless of whether they're allowed)
        int requestCount = settings->value("dailyRequestCount", 0).toInt();
        settings->setValue("dailyRequestCount", requestCount + 1);
        settings->sync();

        if (!checkDailyLimit()) {
            socket->write("ERROR: Daily limit exceeded (3 cycles per day)\n");
        } else {
            onCycleRelays();
            socket->write("OK\n");
        }
    } else if (command == "STATUS") {
        QString status = cyclingInProgress ? "CYCLING" : "READY";
        socket->write((status + "\n").toUtf8());
    } else if (command == "GET_LIMIT") {
        int count = 0;
        QDate lastDate = settings->value("lastCycleDate").toDate();
        if (lastDate == QDate::currentDate()) {
            count = settings->value("dailyCycleCount", 0).toInt();
        }
        QString response = QString("CYCLES_TODAY: %1/%2\n").arg(count).arg(DAILY_CYCLE_LIMIT);
        socket->write(response.toUtf8());
    } else {
        socket->write("ERROR: Unknown command\n");
    }
    
    socket->flush();
}

bool LAUOnTrakWidget::checkDailyLimit()
{
    QDate currentDate = QDate::currentDate();
    QDate lastDate = settings->value("lastCycleDate").toDate();

    // If it's a new day, reset the counters
    if (lastDate != currentDate) {
        settings->setValue("lastCycleDate", currentDate);
        settings->setValue("dailyCycleCount", 0);
        settings->setValue("dailyRequestCount", 0);
        return true;
    }

    // Check if we've exceeded the daily limit
    int count = settings->value("dailyCycleCount", 0).toInt();
    return count < DAILY_CYCLE_LIMIT;
}

void LAUOnTrakWidget::incrementDailyCounter()
{
    int count = settings->value("dailyCycleCount", 0).toInt();
    settings->setValue("dailyCycleCount", count + 1);
    settings->sync();
}

QString LAUOnTrakWidget::loadHelpContent()
{
    QFile file(":/help/resources/help/about.html");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load help content: about.html";
        return "";
    }
    return QString::fromUtf8(file.readAll());
}

void LAUOnTrakWidget::onAbout()
{
    // Load About content from HTML resource
    QString aboutMessage = loadHelpContent();

    QMessageBox::about(this, "About LAU OnTrak Widget", aboutMessage);
}

void LAUOnTrakWidget::onResetDailyCounter()
{
    int ret = QMessageBox::question(this, "Reset Daily Counter",
        "Are you sure you want to reset the daily cycle counter?\n"
        "This will allow up to 3 more cycles today.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        settings->setValue("dailyCycleCount", 0);
        settings->sync();
        statusBar()->showMessage("Daily counter reset", 3000);
        QMessageBox::information(this, "Counter Reset", 
            "Daily cycle counter has been reset to 0.");
    }
}

void LAUOnTrakWidget::onShowStatus()
{
    QDate currentDate = QDate::currentDate();
    QDate lastDate = settings->value("lastCycleDate").toDate();

    int count = 0;
    int requestCount = 0;
    if (lastDate == currentDate) {
        count = settings->value("dailyCycleCount", 0).toInt();
        requestCount = settings->value("dailyRequestCount", 0).toInt();
    }

    QString deviceStatus;
    if (!isMasterInstance) {
        deviceStatus = "Slave mode (another instance has control)";
    } else {
#ifdef Q_OS_WIN
        if (handle) {
            deviceStatus = "OnTrak device connected (Master)";
        } else {
            deviceStatus = "OnTrak device not found (Master)";
        }
#else
        deviceStatus = "Demo mode (Master instance)";
#endif
    }

    QString cycleStatus = cyclingInProgress ? "Currently cycling relays" : "Ready";

    QMessageBox::information(this, "Status Information",
        QString("<h3>OnTrak Widget Status</h3>"
                "<p><b>Device:</b> %1</p>"
                "<p><b>State:</b> %2</p>"
                "<p><b>Daily Cycles Performed:</b> %3 / %4</p>"
                "<p><b>Total Reset Requests Received:</b> %5</p>"
                "<p><b>Date:</b> %6</p>")
                .arg(deviceStatus)
                .arg(cycleStatus)
                .arg(count)
                .arg(DAILY_CYCLE_LIMIT)
                .arg(requestCount)
                .arg(currentDate.toString("yyyy-MM-dd")));
}

bool LAUOnTrakWidget::checkForExistingInstance()
{
    masterCheckSocket = new QLocalSocket(this);
    masterCheckSocket->connectToServer("LAUOnTrakWidget");
    
    // Wait a short time for connection
    if (masterCheckSocket->waitForConnected(1000)) {
        // Another instance is running
        return true;
    }
    
    // No other instance found
    masterCheckSocket->deleteLater();
    masterCheckSocket = nullptr;
    return false;
}

void LAUOnTrakWidget::setSlaveMode()
{
    isMasterInstance = false;
    
    // Update window title to indicate slave mode
    setWindowTitle(QString("LAU On Trak Widget - SLAVE MODE (Another instance has device control)"));
    
    // Set buttons to yellow (indicating no device control)
    QPalette paletteNoControl = buttonA->palette();
    paletteNoControl.setColor(QPalette::Base, QColor(196, 196, 64));
    paletteNoControl.setColor(QPalette::Button, QColor(196, 196, 64));
    paletteNoControl.setColor(QPalette::Window, QColor(196, 196, 64));
    paletteNoControl.setColor(QPalette::WindowText, QColor(196, 196, 64));
    
    buttonA->setPalette(paletteNoControl);
    buttonB->setPalette(paletteNoControl);
    buttonA->repaint();
    buttonB->repaint();
    
    // Disable relay control buttons
    buttonA->setEnabled(false);
    buttonB->setEnabled(false);
    
    // Update tooltips to explain slave mode
    buttonA->setToolTip(QString("OnTrak Relay K0 Control - DISABLED\n\n"
                               "This instance is in SLAVE MODE because another\n"
                               "OnTrak Widget instance is already running and\n"
                               "has control of the device.\n\n"
                               "Close the other instance to regain control,\n"
                               "or use this instance for monitoring only."));
    
    buttonB->setToolTip(QString("OnTrak Relay K1 Control - DISABLED\n\n"
                               "This instance is in SLAVE MODE because another\n"
                               "OnTrak Widget instance is already running and\n"
                               "has control of the device.\n\n"
                               "Close the other instance to regain control,\n"
                               "or use this instance for monitoring only."));
    
    // Update status bar
    statusBar()->showMessage("Slave mode - Another instance has device control");
    
    // Don't initialize hardware in slave mode
#ifdef Q_OS_WIN
    handle = nullptr;
#endif
}
