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

#ifndef LAUONTRAKWIDGET_H
#define LAUONTRAKWIDGET_H

#include <QMainWindow>
#include <QWidget>
#include <QPalette>
#include <QPushButton>
#include <QTimer>
#include <QLocalServer>
#include <QDate>
#include <QSettings>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

class LAUOnTrakWidget : public QMainWindow
{
    Q_OBJECT

public:
    LAUOnTrakWidget(QWidget *parent = nullptr);
    ~LAUOnTrakWidget();

public slots:
    void onToggleK0();
    void onToggleK1();
    void onCycleRelays();
    void onRelayOffTimeout();
    void onNewConnection();
    void onReadyRead();
    void onAbout();
    void onResetDailyCounter();
    void onShowStatus();

private:
    void setRelayK0(bool state);
    void setRelayK1(bool state);
    void startIPCServer();
    bool checkDailyLimit();
    void incrementDailyCounter();
    void createMenuBar();
    bool checkForExistingInstance();
    void setSlaveMode();
    QString loadHelpContent();

private:
#ifdef Q_OS_WIN
    void *handle;
#else
    bool demoModeK0State;
    bool demoModeK1State;
#endif

    QPalette paletteUp;
    QPalette paletteDn;
    QPalette paletteWait;

    QPushButton *buttonA;
    QPushButton *buttonB;

    QTimer *relayOffTimer;
    QLocalServer *ipcServer;
    QLocalSocket *masterCheckSocket;
    bool cyclingInProgress;
    bool isMasterInstance;
    
    QSettings *settings;
    static const int DAILY_CYCLE_LIMIT = 3;
    
    // Menu related members
    QAction *aboutAction;
    QAction *resetCounterAction;
    QAction *statusAction;
    QAction *exitAction;
};
#endif // LAUONTRAKWIDGET_H
