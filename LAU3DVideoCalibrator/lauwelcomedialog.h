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

#ifndef LAUWELCOMEDIALOG_H
#define LAUWELCOMEDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUWelcomeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUWelcomeDialog(QWidget *parent = nullptr);
    ~LAUWelcomeDialog();

private slots:
    void onImportClicked();

private:
    void setupUI();

    // UI Components
    QVBoxLayout *mainLayout;
    QLabel *titleLabel;
    QLabel *messageLabel;
    QPushButton *importButton;
    QPushButton *quitButton;
};

#endif // LAUWELCOMEDIALOG_H
