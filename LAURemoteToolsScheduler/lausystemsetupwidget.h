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

#ifndef LAUSYSTEMSETUPWIDGET_H
#define LAUSYSTEMSETUPWIDGET_H

#include <QDialog>
#include <QLineEdit>
#include <QTimeEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>

class LAUSystemSetupWidget : public QDialog
{
    Q_OBJECT

public:
    explicit LAUSystemSetupWidget(QWidget *parent = nullptr);
    ~LAUSystemSetupWidget();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onBrowseDestinationClicked();
    void onBrowseLocalPathClicked();
    void onAccepted();
    void onSystemCodeChanged(const QString &text);
    void onDestinationPathChanged(const QString &text);
    void onDestinationPathEditingFinished();
    void onLocalTempPathEditingFinished();
    void onAutoLoginUsernameEditingFinished();
    void onAutoLoginPasswordEditingFinished();
    void onAdvancedToggled(bool checked);
    void onAboutClicked();
    void onUninstallClicked();

private:
    void setupUI();
    void loadConfiguration();
    bool validateInputs();
    QString loadHelpContent();

    // System Identification
    QLineEdit *systemCodeEdit;
    QLabel *systemCodeValidationLabel;

    // Recording Schedule
    QTimeEdit *startTimeEdit;
    QSpinBox *durationHoursSpinBox;
    QSpinBox *durationMinutesSpinBox;

    // Paths
    QLineEdit *destinationPathEdit;
    QPushButton *browseDestinationButton;

    // Advanced Settings (collapsible)
    QGroupBox *advancedGroupBox;
    QLineEdit *localTempPathEdit;
    QPushButton *browseLocalPathButton;

    // Encoding
    QCheckBox *enableEncodingCheckBox;

    // Task Scheduler
    QGroupBox *schedulerGroupBox;
    QCheckBox *runAsSystemCheckBox;

    // Auto-Login
    QGroupBox *autoLoginGroupBox;
    QLineEdit *autoLoginUsernameEdit;
    QLineEdit *autoLoginPasswordEdit;

    // Dialog Buttons
    QDialogButtonBox *buttonBox;

    // Status Label
    QLabel *statusLabel;

    // Previous path storage for validation
    QString previousDestinationPath;
    QString previousLocalTempPath;
};

#endif // LAUSYSTEMSETUPWIDGET_H
