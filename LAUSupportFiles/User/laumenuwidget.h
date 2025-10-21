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

#ifndef LAUMENUWIDGET_H
#define LAUMENUWIDGET_H

#include <QPushButton>
#include <QSettings>
#include <QMenuBar>
#include <QDialog>
#include <QString>
#include <QObject>
#include <QDebug>

#include "laudocumentwidget.h"

class LAUMenuWidget : public QMenuBar
{
    Q_OBJECT

public:
    explicit LAUMenuWidget(QWidget *parent = NULL);
    ~LAUMenuWidget();

    void setSplashScreenWidget(QWidget *widget)
    {
        splashScreen = widget;
    }

public slots:
    void onCreateNewDocument(QString string = QString());
    void onCreateNewDocument(LAUScan scan, QString string = QString());
    void onCreateNewDocument(QList<LAUScan> scans, QString string = QString());
    void onLoadDocumentFromDisk();
    void onSaveDocumentToDisk();
    void onSaveDocumentToDiskAs();
    void onCloseCurrentDocument();
    void onCloseAllDocuments();
    void onUpdateMenuTexts();
    void onDisableAllMenus();
    void onActionAboutBox();
    void onActionSettings();
    void onFilterCurrentDocument();

    void onDocumentWidgetClosed();
    void onApplicationFocusChanged(QWidget *previous, QWidget *current);

protected:

private:
    QStringList filterStringList;

    QMenu *filtersMenu;

    QAction *createNewDocumentAction;
    QAction *loadDocumentFromDiskAction;
    QAction *saveDocumentToDiskAction;
    QAction *saveDocumentToDiskActionAs;
    QAction *closeCurrentDocumentAction;
    QAction *closeAllDocumentsAction;
    QAction *showAboutBoxAction;
    QAction *settingsAction;

    QList<QAction *> filterActionList;

    QList<LAUDocumentWidget *> documentList;

    QWidget *splashScreen;

signals:
    void showSplashScreen();
    void hideSplashScreen();
};

class LAUSplashScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LAUSplashScreen(QWidget *parent = nullptr);
    ~LAUSplashScreen();

protected:
    inline void showEvent(QShowEvent *)
    {
        menuBar->onDisableAllMenus();
    }

    inline void hideEvent(QHideEvent *)
    {
        menuBar->onUpdateMenuTexts();
    }

private:
    QLabel *splashLabel;
    LAUMenuWidget *menuBar;
};

#endif // LAUMENUWIDGET_H
