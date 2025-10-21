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

#include "lausystemsetupwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStandardPaths>
#include <QScrollArea>
#include <QTextEdit>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSysInfo>
#include <QClipboard>
#include <QApplication>
#include <QTimer>
#include <QKeyEvent>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

LAUSystemSetupWidget::LAUSystemSetupWidget(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("3D Video Inspection System Setup");
    setMinimumSize(600, 400); // Set minimum size
    setMaximumHeight(750); // Prevent window from being too tall
    resize(700, 650); // Set initial size to fit on smaller laptop screens

    // Initialize previous path with default
#ifdef Q_OS_WIN
    previousLocalTempPath = QDir::toNativeSeparators("C:/Users/Public/Pictures");
#else
    previousLocalTempPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
#endif

    setupUI();
    loadConfiguration();
}

LAUSystemSetupWidget::~LAUSystemSetupWidget()
{
}

void LAUSystemSetupWidget::keyPressEvent(QKeyEvent *event)
{
    // Ignore Enter and Return keys to prevent accidental dialog acceptance
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        event->ignore();
        return;
    }
    // Pass all other keys to the base class
    QDialog::keyPressEvent(event);
}

void LAUSystemSetupWidget::setupUI()
{
    // Main layout (contains scroll area and buttons)
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Create scroll area for content
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setMinimumHeight(200); // Allow scroll area to shrink
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Create widget to hold all form content
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(15);
    contentLayout->setContentsMargins(10, 10, 10, 10);

    // ========================================================================
    // SYSTEM IDENTIFICATION
    // ========================================================================
    QGroupBox *systemGroup = new QGroupBox("System Identification", contentWidget);
    QFormLayout *systemLayout = new QFormLayout(systemGroup);
    systemLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    systemLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    systemLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    systemLayout->setVerticalSpacing(10); // Reduce vertical spacing between rows

    systemCodeEdit = new QLineEdit(contentWidget);
    systemCodeEdit->setMaxLength(3);
    systemCodeEdit->setPlaceholderText("e.g., WKU");
    systemCodeEdit->setMinimumHeight(30);
    systemCodeEdit->setMinimumWidth(150);
    systemCodeEdit->setMaximumWidth(150);

    // Validator: exactly 3 uppercase letters
    QRegularExpression systemCodeRegex("^[A-Z]{0,3}$");
    QRegularExpressionValidator *systemCodeValidator = new QRegularExpressionValidator(systemCodeRegex, contentWidget);
    systemCodeEdit->setValidator(systemCodeValidator);

    connect(systemCodeEdit, &QLineEdit::textChanged, this, &LAUSystemSetupWidget::onSystemCodeChanged);

    systemCodeValidationLabel = new QLabel(contentWidget);
    systemCodeValidationLabel->setStyleSheet("QLabel { color: red; }");
    systemCodeValidationLabel->hide();

    QVBoxLayout *systemCodeLayout = new QVBoxLayout();
    systemCodeLayout->addWidget(systemCodeEdit);
    systemCodeLayout->addWidget(systemCodeValidationLabel);
    systemCodeLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *systemCodeWidget = new QWidget(contentWidget);
    systemCodeWidget->setLayout(systemCodeLayout);

    QLabel *systemCodeLabel = new QLabel("System Code (3 letters):", contentWidget);
    systemCodeLabel->setMinimumWidth(180);
    systemLayout->addRow(systemCodeLabel, systemCodeWidget);
    contentLayout->addWidget(systemGroup);

    // ========================================================================
    // RECORDING SCHEDULE
    // ========================================================================
    QGroupBox *scheduleGroup = new QGroupBox("Recording Schedule", contentWidget);
    QFormLayout *scheduleLayout = new QFormLayout(scheduleGroup);
    scheduleLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    scheduleLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    scheduleLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    scheduleLayout->setVerticalSpacing(10); // Reduce vertical spacing between rows

    // Start Time
    startTimeEdit = new QTimeEdit(contentWidget);
    startTimeEdit->setDisplayFormat("hh:mm AP"); // 12-hour format with AM/PM
    startTimeEdit->setTime(QTime(6, 0)); // Default 06:00 AM
    startTimeEdit->setMinimumHeight(30);
    startTimeEdit->setMinimumWidth(150);
    startTimeEdit->setMaximumWidth(150);

    QLabel *startTimeLabel = new QLabel("Start Time (daily):", contentWidget);
    startTimeLabel->setMinimumWidth(180);
    scheduleLayout->addRow(startTimeLabel, startTimeEdit);

    // Recording Duration
    QHBoxLayout *durationLayout = new QHBoxLayout();
    durationLayout->setContentsMargins(0, 0, 0, 0); // Remove margins
    durationLayout->setSpacing(10); // Space between hours and minutes

    durationHoursSpinBox = new QSpinBox(contentWidget);
    durationHoursSpinBox->setRange(0, 23);
    durationHoursSpinBox->setValue(2);
    durationHoursSpinBox->setSuffix(" hours");
    durationHoursSpinBox->setMinimumHeight(30);
    durationHoursSpinBox->setMinimumWidth(150);
    durationHoursSpinBox->setMaximumWidth(150);
    durationLayout->addWidget(durationHoursSpinBox);

    durationMinutesSpinBox = new QSpinBox(contentWidget);
    durationMinutesSpinBox->setRange(0, 59);
    durationMinutesSpinBox->setValue(45);
    durationMinutesSpinBox->setSuffix(" minutes");
    durationMinutesSpinBox->setMinimumHeight(30);
    durationMinutesSpinBox->setMinimumWidth(150);
    durationMinutesSpinBox->setMaximumWidth(150);
    durationLayout->addWidget(durationMinutesSpinBox);
    durationLayout->addStretch();

    QWidget *durationWidget = new QWidget(contentWidget);
    durationWidget->setLayout(durationLayout);

    QLabel *durationLabel = new QLabel("Recording Duration:", contentWidget);
    durationLabel->setMinimumWidth(180);
    scheduleLayout->addRow(durationLabel, durationWidget);

    contentLayout->addWidget(scheduleGroup);

    // ========================================================================
    // DESTINATION PATH
    // ========================================================================
    QGroupBox *pathGroup = new QGroupBox("Storage Locations", contentWidget);
    QFormLayout *pathLayout = new QFormLayout(pathGroup);
    pathLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    pathLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    pathLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    pathLayout->setVerticalSpacing(10); // Reduce vertical spacing between rows

    // Destination Path
    QHBoxLayout *destLayout = new QHBoxLayout();
    destLayout->setContentsMargins(0, 0, 0, 0); // Remove margins
    destLayout->setSpacing(10); // Space between edit and button

    destinationPathEdit = new QLineEdit(contentWidget);
#ifdef Q_OS_WIN
    destinationPathEdit->setPlaceholderText("e.g., C:\\Users\\YourName\\OneDrive\\Videos");
#else
    destinationPathEdit->setPlaceholderText("e.g., " + QDir::homePath() + "/OneDrive/Videos");
#endif
    destinationPathEdit->setMinimumHeight(30);
    connect(destinationPathEdit, &QLineEdit::textChanged, this, &LAUSystemSetupWidget::onDestinationPathChanged);
    connect(destinationPathEdit, &QLineEdit::editingFinished, this, &LAUSystemSetupWidget::onDestinationPathEditingFinished);
    destLayout->addWidget(destinationPathEdit);

    browseDestinationButton = new QPushButton("Browse...", contentWidget);
    browseDestinationButton->setMinimumHeight(30);
    connect(browseDestinationButton, &QPushButton::clicked, this, &LAUSystemSetupWidget::onBrowseDestinationClicked);
    destLayout->addWidget(browseDestinationButton);

    QWidget *destWidget = new QWidget(contentWidget);
    destWidget->setLayout(destLayout);

    QLabel *destPathLabel = new QLabel("Cloud Storage Path:", contentWidget);
    destPathLabel->setMinimumWidth(180);
    pathLayout->addRow(destPathLabel, destWidget);

#ifdef Q_OS_WIN
    QLabel *destHintLabel = new QLabel("Videos will be saved to: [Path]\\system[CODE]\\Folder[YYYYMMDD]\\", contentWidget);
#else
    QLabel *destHintLabel = new QLabel("Videos will be saved to: [Path]/system[CODE]/Folder[YYYYMMDD]/", contentWidget);
#endif
    destHintLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    QLabel *emptyLabel = new QLabel("", contentWidget);
    emptyLabel->setMinimumWidth(180);
    pathLayout->addRow(emptyLabel, destHintLabel);

    contentLayout->addWidget(pathGroup);

    // ========================================================================
    // ENCODING OPTIONS
    // ========================================================================
    QGroupBox *encodingGroup = new QGroupBox("Object ID Encoding", contentWidget);
    QVBoxLayout *encodingLayout = new QVBoxLayout(encodingGroup);

    enableEncodingCheckBox = new QCheckBox("Enable object ID encoding (LAUEncodeObjectIDFilter)", contentWidget);
    enableEncodingCheckBox->setChecked(true); // Default enabled
    encodingLayout->addWidget(enableEncodingCheckBox);

    QLabel *encodingHintLabel = new QLabel(
        "Extracts RFID tags from video frames and embeds object IDs into metadata.\n"
        "Runs after recording completes, before uploading to cloud storage.",
        contentWidget
    );
    encodingHintLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    encodingHintLabel->setWordWrap(true);
    encodingLayout->addWidget(encodingHintLabel);

    contentLayout->addWidget(encodingGroup);

    // ========================================================================
    // ADVANCED SETTINGS (COLLAPSIBLE)
    // ========================================================================
    advancedGroupBox = new QGroupBox("Advanced Settings", contentWidget);
    advancedGroupBox->setCheckable(true);
    advancedGroupBox->setChecked(false); // Collapsed by default
    connect(advancedGroupBox, &QGroupBox::toggled, this, &LAUSystemSetupWidget::onAdvancedToggled);

    QFormLayout *advancedLayout = new QFormLayout(advancedGroupBox);
    advancedLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    advancedLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    advancedLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    advancedLayout->setVerticalSpacing(10); // Reduce vertical spacing between rows

    // Local Temporary Path
    QHBoxLayout *localPathLayout = new QHBoxLayout();
    localPathLayout->setContentsMargins(0, 0, 0, 0); // Remove margins
    localPathLayout->setSpacing(10); // Space between edit and button

    localTempPathEdit = new QLineEdit(contentWidget);
#ifdef Q_OS_WIN
    localTempPathEdit->setText(QDir::toNativeSeparators("C:/Users/Public/Pictures")); // Windows default
#else
    localTempPathEdit->setText(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)); // Mac/Linux default
#endif
    localTempPathEdit->setMinimumHeight(30);
    connect(localTempPathEdit, &QLineEdit::editingFinished, this, &LAUSystemSetupWidget::onLocalTempPathEditingFinished);
    localPathLayout->addWidget(localTempPathEdit);

    browseLocalPathButton = new QPushButton("Browse...", contentWidget);
    browseLocalPathButton->setMinimumHeight(30);
    connect(browseLocalPathButton, &QPushButton::clicked, this, &LAUSystemSetupWidget::onBrowseLocalPathClicked);
    localPathLayout->addWidget(browseLocalPathButton);

    QWidget *localPathWidget = new QWidget(contentWidget);
    localPathWidget->setLayout(localPathLayout);

    QLabel *localPathLabel = new QLabel("Local Temporary Storage:", contentWidget);
    localPathLabel->setMinimumWidth(180);
    advancedLayout->addRow(localPathLabel, localPathWidget);

    contentLayout->addWidget(advancedGroupBox);

    // ========================================================================
    // TASK SCHEDULER
    // ========================================================================
    schedulerGroupBox = new QGroupBox("Windows Task Scheduler (runs daily at start time)", contentWidget);
    schedulerGroupBox->setCheckable(true);
    schedulerGroupBox->setChecked(true); // Default enabled
    QVBoxLayout *schedulerLayout = new QVBoxLayout(schedulerGroupBox);

    runAsSystemCheckBox = new QCheckBox("Run as SYSTEM account (not recommended for OneDrive)", contentWidget);
    runAsSystemCheckBox->setChecked(false); // Default disabled - user context needed for OneDrive
    schedulerLayout->addWidget(runAsSystemCheckBox);

    QLabel *schedulerHintLabel = new QLabel(
        "Task name: LAU3DVideoRecording-[LOCATIONCODE]\n"
        "Command: recordVideo.cmd (reads systemConfig.ini)\n"
        "Note: Run as current user to access OneDrive/cloud storage",
        contentWidget
    );
    schedulerHintLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    schedulerHintLabel->setWordWrap(true);
    schedulerLayout->addWidget(schedulerHintLabel);

    contentLayout->addWidget(schedulerGroupBox);

    // ========================================================================
    // AUTO-LOGIN CONFIGURATION
    // ========================================================================
    autoLoginGroupBox = new QGroupBox("Windows Auto-Login (Optional)", contentWidget);
    autoLoginGroupBox->setCheckable(true);
    autoLoginGroupBox->setChecked(false); // Default disabled
    QVBoxLayout *autoLoginLayout = new QVBoxLayout(autoLoginGroupBox);

    QLabel *autoLoginWarningLabel = new QLabel(
        "WARNING: Password is stored in Windows registry in plaintext.\n"
        "Only enable on physically secure systems.\n"
        "Required for scheduled tasks to access OneDrive on system startup.",
        contentWidget
    );
    autoLoginWarningLabel->setStyleSheet("QLabel { color: #ff6600; font-weight: bold; }");
    autoLoginWarningLabel->setWordWrap(true);
    autoLoginLayout->addWidget(autoLoginWarningLabel);

    QFormLayout *autoLoginFormLayout = new QFormLayout();
    autoLoginFormLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    autoLoginFormLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    autoLoginFormLayout->setVerticalSpacing(10);

    autoLoginUsernameEdit = new QLineEdit(contentWidget);
    autoLoginUsernameEdit->setPlaceholderText("Windows username");
    autoLoginUsernameEdit->setMinimumHeight(30);
    connect(autoLoginUsernameEdit, &QLineEdit::editingFinished, this, &LAUSystemSetupWidget::onAutoLoginUsernameEditingFinished);

    QLabel *usernameLabel = new QLabel("Username:", contentWidget);
    usernameLabel->setMinimumWidth(120);
    autoLoginFormLayout->addRow(usernameLabel, autoLoginUsernameEdit);

    autoLoginPasswordEdit = new QLineEdit(contentWidget);
    autoLoginPasswordEdit->setPlaceholderText("Windows password");
    autoLoginPasswordEdit->setEchoMode(QLineEdit::Password);
    autoLoginPasswordEdit->setMinimumHeight(30);
    connect(autoLoginPasswordEdit, &QLineEdit::editingFinished, this, &LAUSystemSetupWidget::onAutoLoginPasswordEditingFinished);

    QLabel *passwordLabel = new QLabel("Password:", contentWidget);
    passwordLabel->setMinimumWidth(120);
    autoLoginFormLayout->addRow(passwordLabel, autoLoginPasswordEdit);

    autoLoginLayout->addLayout(autoLoginFormLayout);

    QLabel *autoLoginHintLabel = new QLabel(
        "Configures Windows to automatically log in to the specified account.\n"
        "This ensures OneDrive is available for video uploads.",
        contentWidget
    );
    autoLoginHintLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    autoLoginHintLabel->setWordWrap(true);
    autoLoginLayout->addWidget(autoLoginHintLabel);

    contentLayout->addWidget(autoLoginGroupBox);

    // ========================================================================
    // STATUS LABEL
    // ========================================================================
    statusLabel = new QLabel(contentWidget);
    statusLabel->setWordWrap(true);
    statusLabel->hide();
    contentLayout->addWidget(statusLabel);

    // Finish content layout and add to scroll area
    contentWidget->setLayout(contentLayout);
    scrollArea->setWidget(contentWidget);

    // Add scroll area to main layout
    mainLayout->addWidget(scrollArea);

    // ========================================================================
    // DIALOG BUTTONS
    // ========================================================================
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Save Configuration && Schedule Task");
    buttonBox->button(QDialogButtonBox::Help)->setText("About");

    // Disable default button behavior - require explicit button clicks
    buttonBox->button(QDialogButtonBox::Ok)->setAutoDefault(false);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(false);
    buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
    buttonBox->button(QDialogButtonBox::Cancel)->setDefault(false);
    buttonBox->button(QDialogButtonBox::Help)->setAutoDefault(false);
    buttonBox->button(QDialogButtonBox::Help)->setDefault(false);

    QPushButton *uninstallButton = new QPushButton("Uninstall", this);
    uninstallButton->setAutoDefault(false);
    uninstallButton->setDefault(false);
    buttonBox->addButton(uninstallButton, QDialogButtonBox::DestructiveRole);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &LAUSystemSetupWidget::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Help), &QPushButton::clicked, this, &LAUSystemSetupWidget::onAboutClicked);
    connect(uninstallButton, &QPushButton::clicked, this, &LAUSystemSetupWidget::onUninstallClicked);

    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set initial focus to system code field instead of buttons
    // This prevents Enter key from triggering the OK button
    systemCodeEdit->setFocus();
}

void LAUSystemSetupWidget::loadConfiguration()
{
    // Determine INI file path (same directory as executable)
    QString iniPath = QDir::currentPath() + "/systemConfig.ini";

    // Check if file exists
    if (!QFile::exists(iniPath)) {
        return; // No saved configuration, use defaults
    }

    QSettings settings(iniPath, QSettings::IniFormat);

    // System Identification
    if (settings.contains("SystemCode")) {
        systemCodeEdit->setText(settings.value("SystemCode").toString());
    }

    // Recording Schedule
    if (settings.contains("StartTime")) {
        QTime time = QTime::fromString(settings.value("StartTime").toString(), "HH:mm");
        if (time.isValid()) {
            startTimeEdit->setTime(time);
        }
    }

    if (settings.contains("DurationMinutes")) {
        int totalMinutes = settings.value("DurationMinutes").toInt();
        durationHoursSpinBox->setValue(totalMinutes / 60);
        durationMinutesSpinBox->setValue(totalMinutes % 60);
    }

    // Storage Paths
    if (settings.contains("DestinationPath")) {
        QString loadedDestPath = QDir::toNativeSeparators(settings.value("DestinationPath").toString());
        destinationPathEdit->setText(loadedDestPath);
        previousDestinationPath = loadedDestPath; // Initialize previous path from loaded config
    }

    if (settings.contains("LocalTempPath")) {
        QString loadedLocalPath = QDir::toNativeSeparators(settings.value("LocalTempPath").toString());
        localTempPathEdit->setText(loadedLocalPath);
        previousLocalTempPath = loadedLocalPath; // Initialize previous path from loaded config

        // If the loaded path is not the default, check the Advanced Settings box
        QString defaultPath;
#ifdef Q_OS_WIN
        defaultPath = QDir::toNativeSeparators("C:/Users/Public/Pictures");
#else
        defaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
#endif
        if (loadedLocalPath != defaultPath && !loadedLocalPath.isEmpty()) {
            advancedGroupBox->setChecked(true);
        }
    }

    // Encoding Options
    if (settings.contains("EnableEncoding")) {
        enableEncodingCheckBox->setChecked(settings.value("EnableEncoding").toBool());
    }

    // Task Scheduler Options
    if (settings.contains("EnableScheduling")) {
        schedulerGroupBox->setChecked(settings.value("EnableScheduling").toBool());
    }

    if (settings.contains("RunAsSystem")) {
        runAsSystemCheckBox->setChecked(settings.value("RunAsSystem").toBool());
    }

    // Auto-Login Options (username only - password never saved to INI)
    if (settings.contains("EnableAutoLogin")) {
        autoLoginGroupBox->setChecked(settings.value("EnableAutoLogin").toBool());
    }

    if (settings.contains("AutoLoginUsername")) {
        autoLoginUsernameEdit->setText(settings.value("AutoLoginUsername").toString());
    }
}

void LAUSystemSetupWidget::onSystemCodeChanged(const QString &text)
{
    QString upperText = text.toUpper();
    if (upperText != text) {
        systemCodeEdit->setText(upperText);
    }

    if (upperText.length() == 3) {
        systemCodeValidationLabel->setText("✓ Valid system code");
        systemCodeValidationLabel->setStyleSheet("QLabel { color: green; }");
        systemCodeValidationLabel->show();
        // Clear any error message from the bottom status label
        if (statusLabel->isVisible() && statusLabel->text().contains("system code", Qt::CaseInsensitive)) {
            statusLabel->hide();
        }
    } else if (upperText.length() > 0) {
        systemCodeValidationLabel->setText("✗ System code must be exactly 3 letters");
        systemCodeValidationLabel->setStyleSheet("QLabel { color: red; }");
        systemCodeValidationLabel->show();
    } else {
        systemCodeValidationLabel->hide();
    }
}

void LAUSystemSetupWidget::onDestinationPathChanged(const QString &text)
{
    Q_UNUSED(text);
    // Clear any error message from the bottom status label when user types
    if (statusLabel->isVisible() &&
        (statusLabel->text().contains("Cloud storage", Qt::CaseInsensitive) ||
         statusLabel->text().contains("Destination", Qt::CaseInsensitive))) {
        statusLabel->hide();
    }
}

void LAUSystemSetupWidget::onDestinationPathEditingFinished()
{
    // Validate the destination path and check for systemXXX patterns
    QString destinationPath = destinationPathEdit->text().trimmed();
    QString userSystemCode = systemCodeEdit->text().trimmed();

    if (destinationPath.isEmpty()) {
        return;
    }

    // First, validate that the path exists
    if (!QDir(destinationPath).exists()) {
        QString errorMsg = "Cloud storage path does not exist:\n\n" +
                          QDir::toNativeSeparators(destinationPath) +
                          "\n\nPlease enter a valid existing directory path.\n\n" +
                          "Restoring previous path: " + QDir::toNativeSeparators(previousDestinationPath);
        statusLabel->setText("Error: Cloud storage path does not exist");
        statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        statusLabel->show();
        QMessageBox::critical(this, "Invalid Path", errorMsg);

        // Restore previous path
        destinationPathEdit->blockSignals(true);
        destinationPathEdit->setText(previousDestinationPath);
        destinationPathEdit->blockSignals(false);
        destinationPathEdit->setFocus();
        destinationPathEdit->selectAll();
        return;
    }

    // Path exists - now check for systemXXX pattern
    QRegularExpression systemPattern("system([A-Z]{3})", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = systemPattern.match(destinationPath);

    if (match.hasMatch()) {
        // Found a system code in the path
        QString detectedSystemCode = match.captured(1).toUpper();
        QString detectedSystemFolder = "system" + detectedSystemCode;

        // Check if the user has entered a system code
        if (!userSystemCode.isEmpty() && userSystemCode.length() == 3) {
            QString userSystemFolder = "system" + userSystemCode.toUpper();

            // Always warn when ANY system code is detected in the path
            // This will create systemXXX/systemWWW structure (either nested duplicates or different systems)
            QString nestedPath = QDir::toNativeSeparators(QDir(destinationPath).filePath(userSystemFolder));

            QMessageBox msgBox(this);
            msgBox.setWindowTitle("System Folder Detected in Path");
            msgBox.setIcon(QMessageBox::Warning);

            if (detectedSystemCode == userSystemCode.toUpper()) {
                // Same system code - will create systemLAU/systemLAU
                msgBox.setText(QString("The path already contains '%1'").arg(detectedSystemFolder));
                msgBox.setInformativeText(QString(
                    "Current path: %1\n"
                    "Your system code: %2\n\n"
                    "This will create a duplicate nested structure:\n"
                    "%3\n\n"
                    "Is this what you want?"
                ).arg(QDir::toNativeSeparators(destinationPath))
                 .arg(userSystemCode.toUpper())
                 .arg(nestedPath));
            } else {
                // Different system codes - will create systemABC/systemDEF
                msgBox.setText(QString("The path contains '%1' but you've specified system code '%2'")
                              .arg(detectedSystemFolder)
                              .arg(userSystemCode.toUpper()));
                msgBox.setInformativeText(QString(
                    "Current path: %1\n"
                    "Your system code: %2\n\n"
                    "This will create a nested structure:\n"
                    "%3\n\n"
                    "Is this what you want?"
                ).arg(QDir::toNativeSeparators(destinationPath))
                 .arg(userSystemCode.toUpper())
                 .arg(nestedPath));
            }

            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::No);

            int result = msgBox.exec();

            if (result == QMessageBox::No) {
                // User doesn't want nested folders - restore previous path
                destinationPathEdit->blockSignals(true);
                destinationPathEdit->setText(previousDestinationPath);
                destinationPathEdit->blockSignals(false);
                destinationPathEdit->setFocus();
                destinationPathEdit->selectAll();
            } else {
                // User accepted the nested structure - update previous path
                previousDestinationPath = destinationPath;
            }
        } else {
            // No system code entered yet - just update previous path
            previousDestinationPath = destinationPath;
        }
    } else {
        // No system pattern detected - update previous path
        previousDestinationPath = destinationPath;
    }
}

void LAUSystemSetupWidget::onLocalTempPathEditingFinished()
{
    // Validate the local temp path after user finishes editing
    QString localTempPath = localTempPathEdit->text().trimmed();

    // If the path is empty, that's fine (it's optional)
    if (localTempPath.isEmpty()) {
        // Clear any existing error messages
        if (statusLabel->isVisible() &&
            (statusLabel->text().contains("Local temporary", Qt::CaseInsensitive) ||
             statusLabel->text().contains("Local temp", Qt::CaseInsensitive))) {
            statusLabel->hide();
        }
        // Update previous path
        previousLocalTempPath = localTempPath;
        return;
    }

    // If path is not empty, validate that it exists
    if (!QDir(localTempPath).exists()) {
        QString errorMsg = "Local temporary storage path does not exist:\n\n" +
                          QDir::toNativeSeparators(localTempPath) +
                          "\n\nPlease enter a valid existing directory path or leave it empty to use the cloud storage path.\n\n" +
                          "Restoring previous path: " + QDir::toNativeSeparators(previousLocalTempPath);
        statusLabel->setText("Error: Local temporary storage path does not exist");
        statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        statusLabel->show();
        QMessageBox::critical(this, "Invalid Path", errorMsg);

        // Restore previous path
        localTempPathEdit->blockSignals(true);
        localTempPathEdit->setText(previousLocalTempPath);
        localTempPathEdit->blockSignals(false);
        localTempPathEdit->setFocus();
        localTempPathEdit->selectAll();
    } else {
        // Path is valid - clear any existing error messages and update previous path
        if (statusLabel->isVisible() &&
            (statusLabel->text().contains("Local temporary", Qt::CaseInsensitive) ||
             statusLabel->text().contains("Local temp", Qt::CaseInsensitive))) {
            statusLabel->hide();
        }
        previousLocalTempPath = localTempPath;
    }
}

void LAUSystemSetupWidget::onAutoLoginUsernameEditingFinished()
{
    // Only validate if auto-login is enabled
    if (!autoLoginGroupBox->isChecked()) {
        return;
    }

    QString username = autoLoginUsernameEdit->text().trimmed();

    // If empty, that's an error but will be caught at save time
    if (username.isEmpty()) {
        return;
    }

    // Check if the username exists on the system
    bool userExists = false;

#ifdef Q_OS_WIN
    // On Windows, check both local and domain accounts
    // First try local account
    QProcess localProcess;
    localProcess.start("net", QStringList() << "user" << username);
    localProcess.waitForFinished(3000); // 3 second timeout

    if (localProcess.exitCode() == 0) {
        userExists = true;
    } else {
        // If local check failed, try domain account
        QProcess domainProcess;
        domainProcess.start("net", QStringList() << "user" << username << "/domain");
        domainProcess.waitForFinished(5000); // 5 second timeout for domain queries

        if (domainProcess.exitCode() == 0) {
            userExists = true;
        }

        // If domain check also failed, check if user profile exists in C:/Users/
        if (!userExists) {
            QString userProfilePath = QString("C:/Users/") + username;
            if (QDir(userProfilePath).exists()) {
                userExists = true;
            }
        }
    }
#else
    // On Linux/Mac, check if user exists by running "id -u <username>"
    QProcess process;
    process.start("id", QStringList() << "-u" << username);
    process.waitForFinished(3000); // 3 second timeout

    // If exit code is 0, user exists
    if (process.exitCode() == 0) {
        userExists = true;
    }
#endif

    if (!userExists) {
        QString errorMsg = QString("Username '%1' could not be verified on this system.\n\n"
                                  "Please ensure you've entered the correct username.\n"
                                  "For domain accounts, use just the username without the domain.").arg(username);
        statusLabel->setText("Warning: Username could not be verified");
        statusLabel->setStyleSheet("QLabel { color: orange; font-weight: bold; }");
        statusLabel->show();
        QMessageBox::warning(this, "Username Verification", errorMsg);
        autoLoginUsernameEdit->setFocus();
        autoLoginUsernameEdit->selectAll();
    } else {
        // Username is valid - clear any existing error messages
        if (statusLabel->isVisible() &&
            (statusLabel->text().contains("Username", Qt::CaseInsensitive) ||
             statusLabel->text().contains("auto-login", Qt::CaseInsensitive))) {
            statusLabel->hide();
        }
    }
}

void LAUSystemSetupWidget::onAutoLoginPasswordEditingFinished()
{
    // Only validate if auto-login is enabled
    if (!autoLoginGroupBox->isChecked()) {
        return;
    }

    QString username = autoLoginUsernameEdit->text().trimmed();
    QString password = autoLoginPasswordEdit->text();

    // If either is empty, skip validation (will be caught at save time)
    if (username.isEmpty() || password.isEmpty()) {
        return;
    }

    // Try to verify the password with the username
    bool passwordValid = false;

#ifdef Q_OS_WIN
    // On Windows, use LogonUser API to verify credentials
    // Convert QString to wide character strings for Windows API
    std::wstring usernameW = username.toStdWString();
    std::wstring passwordW = password.toStdWString();

    HANDLE hToken = NULL;

    // Try local logon first
    passwordValid = LogonUserW(
        usernameW.c_str(),
        NULL,  // Use local machine
        passwordW.c_str(),
        LOGON32_LOGON_INTERACTIVE,
        LOGON32_PROVIDER_DEFAULT,
        &hToken
    );

    if (passwordValid && hToken) {
        CloseHandle(hToken);
    } else {
        // If local logon failed, try domain logon by getting the domain name
        DWORD domainSize = 256;
        wchar_t domain[256];
        if (GetComputerNameExW(ComputerNameDnsDomain, domain, &domainSize) && domainSize > 0) {
            // Try with domain
            passwordValid = LogonUserW(
                usernameW.c_str(),
                domain,
                passwordW.c_str(),
                LOGON32_LOGON_INTERACTIVE,
                LOGON32_PROVIDER_DEFAULT,
                &hToken
            );

            if (passwordValid && hToken) {
                CloseHandle(hToken);
            }
        }
    }
#else
    // On Linux/Mac, password verification is more complex and typically requires root privileges
    // Skip validation for now
    passwordValid = true;
#endif

    if (!passwordValid) {
        QString errorMsg = QString("Password verification failed for username '%1'.\n\n"
                                  "Please ensure you've entered the correct password.").arg(username);
        statusLabel->setText("Warning: Password could not be verified");
        statusLabel->setStyleSheet("QLabel { color: orange; font-weight: bold; }");
        statusLabel->show();
        QMessageBox::warning(this, "Password Verification", errorMsg);
        autoLoginPasswordEdit->setFocus();
        autoLoginPasswordEdit->selectAll();
    } else {
        // Password is valid - clear any existing error messages
        if (statusLabel->isVisible() &&
            (statusLabel->text().contains("Password", Qt::CaseInsensitive) ||
             statusLabel->text().contains("auto-login", Qt::CaseInsensitive))) {
            statusLabel->hide();
        }
    }
}

void LAUSystemSetupWidget::onAdvancedToggled(bool checked)
{
    if (!checked) {
        // When unchecking, reset local temp path to default
#ifdef Q_OS_WIN
        QString defaultPath = QDir::toNativeSeparators("C:/Users/Public/Pictures");
#else
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
#endif
        localTempPathEdit->setText(defaultPath);
        previousLocalTempPath = defaultPath; // Update previous path to default

        // Clear any error messages related to local temp path
        if (statusLabel->isVisible() &&
            (statusLabel->text().contains("Local temporary", Qt::CaseInsensitive) ||
             statusLabel->text().contains("Local temp", Qt::CaseInsensitive))) {
            statusLabel->hide();
        }
    }
}

void LAUSystemSetupWidget::onBrowseDestinationClicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Cloud Storage Base Directory",
        destinationPathEdit->text().isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) : destinationPathEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty()) {
        destinationPathEdit->setText(QDir::toNativeSeparators(dir));
        // Manually trigger the systemXXX duplication check since editingFinished won't fire for programmatic setText
        onDestinationPathEditingFinished();
    }
}

void LAUSystemSetupWidget::onBrowseLocalPathClicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Local Temporary Storage Directory",
        localTempPathEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty()) {
        localTempPathEdit->setText(QDir::toNativeSeparators(dir));
    }
}

bool LAUSystemSetupWidget::validateInputs()
{
    // System Code
    QString systemCode = systemCodeEdit->text().trimmed();
    qDebug() << "Validating system code:" << systemCode << "Length:" << systemCode.length();

    if (systemCode.length() != 3) {
        statusLabel->setText("⚠ Error: System code must be exactly 3 uppercase letters");
        statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        statusLabel->show();

        QString errorMsg;
        if (systemCode.isEmpty()) {
            errorMsg = "System code is required.\n\n"
                      "Please enter exactly 3 uppercase letters.\n\n"
                      "Examples: ABC, XYZ, LAU";
        } else {
            errorMsg = "System code must be exactly 3 uppercase letters.\n\n"
                      "Examples: ABC, XYZ, LAU\n\n"
                      "Current value: \"" + systemCode + "\" (length: " + QString::number(systemCode.length()) + ")";
        }

        QMessageBox::critical(this, "Invalid System Code", errorMsg);
        systemCodeEdit->setFocus();
        systemCodeEdit->selectAll();
        return false;
    }

    // Destination Path - check not empty
    if (destinationPathEdit->text().trimmed().isEmpty()) {
        statusLabel->setText("⚠ Error: Cloud storage path is required");
        statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        statusLabel->show();

        QMessageBox::critical(this, "Missing Required Field",
                            "Cloud storage path is required.\n\n"
                            "Please enter the path to your network storage location\n"
                            "where recording files will be saved.");
        destinationPathEdit->setFocus();
        return false;
    }

    // Destination Path - check directory exists
    QString destinationPath = destinationPathEdit->text().trimmed();
    if (!QDir(destinationPath).exists()) {
        QString errorMsg = "Cloud storage path does not exist:\n\n" + QDir::toNativeSeparators(destinationPath) +
                          "\n\nPlease enter a valid existing directory path.";

        statusLabel->setText("Error: Cloud storage path does not exist");
        statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        statusLabel->show();

        QMessageBox::critical(this, "Invalid Path", errorMsg);
        destinationPathEdit->setFocus();
        destinationPathEdit->selectAll();
        return false;
    }

    // Local Temp Path - check directory exists
    QString localTempPath = localTempPathEdit->text().trimmed();
    if (!localTempPath.isEmpty() && !QDir(localTempPath).exists()) {
        QString errorMsg = "Local temporary storage path does not exist:\n\n" + QDir::toNativeSeparators(localTempPath) +
                          "\n\nPlease enter a valid existing directory path or leave it empty.";

        statusLabel->setText("Error: Local temporary storage path does not exist");
        statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        statusLabel->show();

        QMessageBox::critical(this, "Invalid Path", errorMsg);
        localTempPathEdit->setFocus();
        localTempPathEdit->selectAll();
        return false;
    }

    // Recording Duration (at least 1 minute)
    if (durationHoursSpinBox->value() == 0 && durationMinutesSpinBox->value() == 0) {
        statusLabel->setText("⚠ Error: Recording duration must be at least 1 minute");
        statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        statusLabel->show();
        durationMinutesSpinBox->setFocus();
        return false;
    }

    statusLabel->hide();
    return true;
}

void LAUSystemSetupWidget::onAccepted()
{
    qDebug() << "Save Configuration button clicked - validating inputs...";

    if (!validateInputs()) {
        qDebug() << "Validation failed - returning without saving";
        return;
    }

    qDebug() << "Validation passed - proceeding with configuration...";

    // ========================================================================
    // CHECK FOR SYSTEM FOLDER
    // ========================================================================
    // Note: systemXXX duplication check now happens in real-time via onDestinationPathEditingFinished()
    QString destinationPath = destinationPathEdit->text().trimmed();
    QString systemCode = systemCodeEdit->text();
    QString expectedSystemFolderName = "system" + systemCode;
    QString systemFolderPath = QDir::toNativeSeparators(QDir(destinationPath).filePath(expectedSystemFolderName));

    if (!QDir(systemFolderPath).exists()) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("System Folder Not Found");
        msgBox.setText("The system folder does not exist:");
        msgBox.setInformativeText(systemFolderPath + "\n\nWould you like to create it?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Yes);

        int result = msgBox.exec();

        if (result == QMessageBox::Cancel) {
            return; // Return to dialog without saving
        } else if (result == QMessageBox::Yes) {
            // Create the system folder
            QDir dir;
            if (!dir.mkpath(systemFolderPath)) {
                QMessageBox::critical(
                    this,
                    "Error Creating Folder",
                    "Failed to create system folder:\n" + systemFolderPath + "\n\nPlease check permissions."
                );
                return;
            }
        }
        // If No, continue without creating the folder
    }

    // ========================================================================
    // SAVE CONFIGURATION TO INI FILE
    // ========================================================================
    // Determine INI file path (same directory as executable)
    QString iniPath = QDir::currentPath() + "/systemConfig.ini";

    QSettings settings(iniPath, QSettings::IniFormat);

    // System Identification
    settings.setValue("SystemCode", systemCodeEdit->text());

    // Recording Schedule
    settings.setValue("StartTime", startTimeEdit->time().toString("HH:mm")); // 24-hour format for batch script
    int totalMinutes = durationHoursSpinBox->value() * 60 + durationMinutesSpinBox->value();
    settings.setValue("DurationMinutes", totalMinutes);

    // Storage Paths
    // Use forward slashes to avoid double-backslash escaping in INI files
    settings.setValue("DestinationPath", QDir::fromNativeSeparators(destinationPathEdit->text()));
    settings.setValue("LocalTempPath", QDir::fromNativeSeparators(localTempPathEdit->text()));

    // Encoding Options
    settings.setValue("EnableEncoding", enableEncodingCheckBox->isChecked());

    // Task Scheduler Options
    settings.setValue("EnableScheduling", schedulerGroupBox->isChecked());
    settings.setValue("RunAsSystem", runAsSystemCheckBox->isChecked());

    // Auto-Login Options
    settings.setValue("EnableAutoLogin", autoLoginGroupBox->isChecked());
    if (autoLoginGroupBox->isChecked()) {
        settings.setValue("AutoLoginUsername", autoLoginUsernameEdit->text());
        // Note: Password is NOT saved to INI file for security
    }

    settings.sync(); // Force write to disk

#ifdef Q_OS_WIN
    // ========================================================================
    // DEPLOY RECORDVIDEO.CMD FROM RESOURCES
    // ========================================================================
    QString recordVideoCmdPath = QDir::toNativeSeparators(QDir::currentPath() + "/recordVideo.cmd");

    // Extract recordVideo.cmd from Qt resources if it doesn't exist
    if (!QFile::exists(recordVideoCmdPath)) {
        QFile resourceFile(":/recordVideo.cmd");
        if (resourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QFile outputFile(recordVideoCmdPath);
            if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                outputFile.write(resourceFile.readAll());
                outputFile.close();
            }
            resourceFile.close();
        }
    }

    // ========================================================================
    // CONFIGURE LAU ONTRAK WIDGET TO START ON SYSTEM BOOT
    // ========================================================================
    QString onTrakExePath = QDir::toNativeSeparators(QDir::currentPath() + "/LAUOnTrakWidget.exe");
    bool onTrakConfigured = false;
    QString onTrakError;

    if (QFile::exists(onTrakExePath)) {
        // Clean up old Startup folder shortcut (from previous installer versions)
        QString startupFolder = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/Startup";
        QString oldShortcutPath = QDir::toNativeSeparators(startupFolder + "/LAUOnTrakWidget.lnk");
        if (QFile::exists(oldShortcutPath)) {
            QFile::remove(oldShortcutPath); // Remove old startup shortcut
        }

        QString onTrakTaskName = "LAUOnTrakWidget-Login";

        // Delete any existing task with the same name (prevents duplicates)
        QString deleteCommand = QString("schtasks /Delete /TN \"%1\" /F").arg(onTrakTaskName);
        QProcess::execute(deleteCommand); // Ignore result - it's OK if task doesn't exist

        // Also delete old task name from previous versions
        QProcess::execute("schtasks /Delete /TN \"LAUOnTrakWidget-Startup\" /F");

        // Create Task Scheduler entry that runs on user login
        // /SC ONLOGON = Run when user logs in (after login)
        // /DELAY 0000:30 = Wait 30 seconds after login for system to stabilize
        QProcess process;
        QStringList args;
        args << "/Create" << "/TN" << onTrakTaskName;
        args << "/TR" << QString("\"%1\"").arg(onTrakExePath);
        args << "/SC" << "ONLOGON";
        args << "/DELAY" << "0000:30";
        args << "/F";

        process.start("schtasks", args);
        process.waitForFinished();
        int result = process.exitCode();
        QString stdOut = process.readAllStandardOutput();
        QString stdErr = process.readAllStandardError();

        if (result == 0) {
            onTrakConfigured = true;
        } else {
            QString createCommand = QString("schtasks %1").arg(args.join(" "));
            onTrakError = QString("schtasks returned error code %1\n\nCommand: %2\n\nOutput: %3\nError: %4")
                              .arg(result).arg(createCommand).arg(stdOut).arg(stdErr);
        }
    }

    // ========================================================================
    // CREATE TASK SCHEDULER ENTRY FOR DAILY RECORDING
    // ========================================================================
    bool taskSchedulerConfigured = false;
    QString taskSchedulerError;

    if (schedulerGroupBox->isChecked()) {
        QString taskName = "LAU3DVideoRecording-" + systemCode;
        QString recordVideoCmdPath = QDir::toNativeSeparators(QDir::currentPath() + "/recordVideo.cmd");

        // Verify recordVideo.cmd exists
        if (!QFile::exists(recordVideoCmdPath)) {
            taskSchedulerError = "recordVideo.cmd not found at:\n" + recordVideoCmdPath;
        } else {
            // Delete any existing task with the same name (prevents duplicates)
            QString deleteCommand = QString("schtasks /Delete /TN \"%1\" /F").arg(taskName);
            QProcess::execute(deleteCommand); // Ignore result - it's OK if task doesn't exist

            // Build schtasks /Create command
            QString startTimeStr = startTimeEdit->time().toString("HH:mm");

            // Execute task creation
            QProcess process;

            // Build command - must use cmd /c to run .cmd files from Task Scheduler
            // Also redirect output to log file for troubleshooting
            QString logPath = "C:\\Users\\Public\\Documents\\videoRecording.txt";
            QString taskCommand = QString("cmd /c \"\"%1\" > \"%2\" 2>&1\"").arg(recordVideoCmdPath, logPath);

            // Build argument list for schtasks
            QStringList args;
            args << "/Create" << "/TN" << taskName;
            args << "/TR" << taskCommand;
            args << "/SC" << "DAILY" << "/ST" << startTimeStr;
            if (runAsSystemCheckBox->isChecked()) {
                args << "/RU" << "SYSTEM";
            }
            args << "/F";

            process.start("schtasks", args);
            process.waitForFinished();
            int result = process.exitCode();
            QString stdOut = process.readAllStandardOutput();
            QString stdErr = process.readAllStandardError();

            if (result == 0) {
                taskSchedulerConfigured = true;
            } else {
                QString createCommand = QString("schtasks %1").arg(args.join(" "));
                taskSchedulerError = QString("schtasks returned error code %1\n\n"
                                           "Command: %2\n\n"
                                           "Output: %3\n"
                                           "Error: %4").arg(result).arg(createCommand).arg(stdOut).arg(stdErr);
            }
        }
    }

    // ========================================================================
    // CONFIGURE WINDOWS AUTO-LOGIN
    // ========================================================================
    bool autoLoginConfigured = false;
    QString autoLoginError;

    if (autoLoginGroupBox->isChecked()) {
        QString username = autoLoginUsernameEdit->text().trimmed();
        QString password = autoLoginPasswordEdit->text();

        if (username.isEmpty()) {
            autoLoginError = "Username is required for auto-login";
        } else if (password.isEmpty()) {
            autoLoginError = "Password is required for auto-login";
        } else {
            // Configure Windows auto-login via registry
            // HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon

            bool allSucceeded = true;
            QString failedCommand;
            int failedResult = 0;

            // Set AutoAdminLogon = 1
            QStringList args1;
            args1 << "add" << "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
                  << "/v" << "AutoAdminLogon" << "/t" << "REG_SZ" << "/d" << "1" << "/f";
            int result1 = QProcess::execute("reg", args1);
            if (result1 != 0) {
                allSucceeded = false;
                failedCommand = QString("reg %1").arg(args1.join(" "));
                failedResult = result1;
            }

            // Set DefaultUserName
            if (allSucceeded) {
                QStringList args2;
                args2 << "add" << "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
                      << "/v" << "DefaultUserName" << "/t" << "REG_SZ" << "/d" << username << "/f";
                int result2 = QProcess::execute("reg", args2);
                if (result2 != 0) {
                    allSucceeded = false;
                    failedCommand = QString("reg %1").arg(args2.join(" "));
                    failedResult = result2;
                }
            }

            // Set DefaultPassword
            if (allSucceeded) {
                QStringList args3;
                args3 << "add" << "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
                      << "/v" << "DefaultPassword" << "/t" << "REG_SZ" << "/d" << password << "/f";
                int result3 = QProcess::execute("reg", args3);
                if (result3 != 0) {
                    allSucceeded = false;
                    failedCommand = QString("reg %1").arg(args3.join(" "));
                    failedResult = result3;
                }
            }

            if (allSucceeded) {
                autoLoginConfigured = true;
            } else {
                autoLoginError = QString("Registry command failed (code %1):\n%2")
                                    .arg(failedResult)
                                    .arg(failedCommand);
            }
        }
    }

    // ========================================================================
    // BUILD SUCCESS MESSAGE
    // ========================================================================
    QString buildVersion = "1.1.2";  // Update this with each release
    QString compileDateTime = QString("%1 %2").arg(__DATE__).arg(__TIME__);

    QString message = QString("LAURemoteToolsInstaller v%1 (Built: %2)\n\n").arg(buildVersion, compileDateTime);
    message += "System recording configuration has been saved to:\n" + iniPath + "\n\n";

    if (onTrakConfigured) {
        message += "✓ LAUOnTrakWidget configured to start on user login\n"
                  "  Task name: LAUOnTrakWidget-Login\n"
                  "  Runs as: Logged-in user\n"
                  "  (Relay control for camera power cycling)\n\n";
    } else if (QFile::exists(onTrakExePath)) {
        message += "⚠ Warning: Could not configure LAUOnTrakWidget startup\n";
        if (!onTrakError.isEmpty()) {
            message += "  Error: " + onTrakError + "\n";
        }
        message += "  Please run this installer with administrator privileges.\n\n";
    } else {
        message += "⚠ Warning: LAUOnTrakWidget.exe not found\n"
                  "  Relay control will not be available\n\n";
    }

    if (schedulerGroupBox->isChecked()) {
        if (taskSchedulerConfigured) {
            QString taskName = "LAU3DVideoRecording-" + systemCode;
            QString startTimeStr = startTimeEdit->time().toString("hh:mm AP");
            QString accountType = runAsSystemCheckBox->isChecked() ? "SYSTEM account" : "current user";

            message += QString("✓ Task Scheduler entry created successfully\n"
                             "  Task name: %1\n"
                             "  Start time: %2 daily\n"
                             "  Run as: %3\n\n")
                      .arg(taskName, startTimeStr, accountType);

            message += "The task will run recordVideo.cmd, which reads systemConfig.ini.\n"
                      "You can verify this in Task Scheduler (taskschd.msc).\n\n";
        } else {
            message += "⚠ Warning: Could not create Task Scheduler entry\n";
            if (!taskSchedulerError.isEmpty()) {
                message += "  Error: " + taskSchedulerError + "\n";
            }
            message += "  Please run this installer with administrator privileges.\n\n";
        }
    } else {
        message += "ℹ Task Scheduler entry was not created (checkbox unchecked)\n"
                  "  You can run recordVideo.cmd manually if needed.\n\n";
    }

    if (autoLoginGroupBox->isChecked()) {
        if (autoLoginConfigured) {
            QString username = autoLoginUsernameEdit->text().trimmed();
            message += QString("✓ Windows auto-login configured successfully\n"
                             "  Username: %1\n"
                             "  System will automatically log in on boot\n"
                             "  OneDrive will be available for video uploads\n\n"
                             "⚠ Security Note: Password stored in plaintext in registry\n"
                             "  Location: HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\n\n")
                      .arg(username);
        } else {
            message += "⚠ Warning: Could not configure Windows auto-login\n";
            if (!autoLoginError.isEmpty()) {
                message += "  Error: " + autoLoginError + "\n";
            }
            message += "  Please run this installer with administrator privileges.\n\n";
        }
    } else {
        message += "ℹ Windows auto-login was not configured (checkbox unchecked)\n"
                  "  If using OneDrive, ensure user is logged in before scheduled task runs.\n\n";
    }

    // Create a custom message box with a Copy button
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setWindowTitle("Success");
    msgBox->setText(message);
    msgBox->setIcon(QMessageBox::Information);

    // Add buttons
    QPushButton *copyButton = msgBox->addButton("Copy to Clipboard", QMessageBox::ActionRole);
    QPushButton *okButton = msgBox->addButton(QMessageBox::Ok);

    msgBox->setDefaultButton(okButton);

    // Connect copy button to copy without closing dialog
    connect(copyButton, &QPushButton::clicked, [msgBox, message]() {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(message);

        // Show brief confirmation in the title
        QString originalTitle = msgBox->windowTitle();
        msgBox->setWindowTitle("Success - Copied to Clipboard!");
        QTimer::singleShot(1500, [msgBox, originalTitle]() {
            if (msgBox) {
                msgBox->setWindowTitle(originalTitle);
            }
        });
    });

    // Show the dialog modally
    msgBox->exec();
    delete msgBox;

    // ========================================================================
    // LAUNCH LAU ONTRAK WIDGET IMMEDIATELY (Don't wait for restart)
    // ========================================================================
    if (onTrakConfigured && QFile::exists(onTrakExePath)) {
        // Check if LAUOnTrakWidget is already running
        QProcess checkProcess;
        checkProcess.start("tasklist", QStringList() << "/FI" << "IMAGENAME eq LAUOnTrakWidget.exe");
        checkProcess.waitForFinished();
        QString output = checkProcess.readAllStandardOutput();
        bool alreadyRunning = output.contains("LAUOnTrakWidget.exe", Qt::CaseInsensitive);

        if (alreadyRunning) {
            qDebug() << "LAUOnTrakWidget is already running - skipping launch";
        } else {
            // Launch the OnTrak widget in a detached process
            // This starts it immediately instead of waiting for user to login/restart
            bool launched = QProcess::startDetached(onTrakExePath);

            if (!launched) {
                QMessageBox::warning(
                    this,
                    "Warning",
                    "LAUOnTrakWidget scheduled successfully but failed to launch immediately.\n\n"
                    "It will start automatically on next login."
                );
            }
        }
    }
#else
    // Development mode on Mac/Linux
    QMessageBox::information(
        this,
        "Success (Development Mode)",
        "System recording configuration has been saved to:\n" + iniPath + "\n\n"
        "Note: Running on " + QSysInfo::productType() + "\n"
        "Task Scheduler integration is Windows-only.\n\n"
        "You can inspect the INI file to verify the settings."
    );
#endif

    // Accept the dialog (closes it with QDialog::Accepted)
    accept();
}

QString LAUSystemSetupWidget::loadHelpContent()
{
    QFile file(":/help/resources/help/about.html");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load help content: about.html";
        return "";
    }
    return QString::fromUtf8(file.readAll());
}

void LAUSystemSetupWidget::onAboutClicked()
{
    QString compileDateTime = QString("Compiled: %1 at %2").arg(__DATE__).arg(__TIME__);

    // Load About content from HTML resource
    QString aboutMessage = loadHelpContent();

    // Replace placeholders
    aboutMessage.replace("{{COMPILE_DATE}}", compileDateTime);

    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About LAURemoteToolsInstaller");
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(aboutMessage);
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}

void LAUSystemSetupWidget::onUninstallClicked()
{
    // Confirm uninstall action
    QMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Confirm Uninstall");
    confirmBox.setText("This will remove all system recording automation:");
    confirmBox.setInformativeText(
        "• Disable Windows auto-login\n"
        "• Delete scheduled tasks (LAU3DVideoRecording-* and LAUOnTrakWidget-Login)\n"
        "• Remove systemConfig.ini configuration file\n"
        "• Clear shared folder files (background.tif, LUTX files, etc.)\n\n"
        "Are you sure you want to continue?"
    );
    confirmBox.setIcon(QMessageBox::Warning);
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    confirmBox.setDefaultButton(QMessageBox::Cancel);

    if (confirmBox.exec() != QMessageBox::Yes) {
        return; // User cancelled
    }

#ifdef Q_OS_WIN
    QString uninstallReport;
    bool hasErrors = false;
    QString iniPath = QDir::currentPath() + "/systemConfig.ini";

    // ========================================================================
    // DISABLE AUTO-LOGIN
    // ========================================================================
    uninstallReport += "Disabling Windows auto-login:\n";

    // Set AutoAdminLogon = 0
    QStringList args1;
    args1 << "add" << "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
          << "/v" << "AutoAdminLogon" << "/t" << "REG_SZ" << "/d" << "0" << "/f";
    int result1 = QProcess::execute("reg", args1);

    if (result1 == 0) {
        uninstallReport += "  ✓ Disabled AutoAdminLogon\n";
    } else {
        uninstallReport += QString("  ⚠ Failed to disable AutoAdminLogon (error code %1)\n").arg(result1);
        hasErrors = true;
    }

    // Delete DefaultPassword
    QStringList args2;
    args2 << "delete" << "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
          << "/v" << "DefaultPassword" << "/f";
    int result2 = QProcess::execute("reg", args2);

    if (result2 == 0) {
        uninstallReport += "  ✓ Removed DefaultPassword from registry\n";
    } else {
        // Error 1 means the value doesn't exist, which is fine
        if (result2 == 1) {
            uninstallReport += "  ✓ DefaultPassword was not set (already clean)\n";
        } else {
            uninstallReport += QString("  ⚠ Failed to remove DefaultPassword (error code %1)\n").arg(result2);
            hasErrors = true;
        }
    }

    uninstallReport += "\n";

    // ========================================================================
    // DELETE TASK SCHEDULER ENTRIES
    // ========================================================================
    uninstallReport += "Removing Task Scheduler entries:\n";

    // First, query all tasks to find our tasks
    uninstallReport += "\n  DEBUG: Querying Task Scheduler for LAU tasks...\n";
    QProcess queryProcess;
    queryProcess.start("schtasks", QStringList() << "/Query" << "/FO" << "LIST" << "/V");
    queryProcess.waitForFinished();
    QString allTasks = queryProcess.readAllStandardOutput();
    QString queryError = queryProcess.readAllStandardError();
    int queryExitCode = queryProcess.exitCode();

    uninstallReport += QString("  DEBUG: schtasks query exit code: %1\n").arg(queryExitCode);
    if (!queryError.isEmpty()) {
        uninstallReport += QString("  DEBUG: schtasks query error output: %1\n").arg(queryError);
    }
    uninstallReport += QString("  DEBUG: Total output length: %1 characters\n").arg(allTasks.length());

    // Find all LAU3DVideoRecording-* and LAUOnTrakWidget tasks
    QStringList tasksToDelete;
    QStringList allTaskNames; // For debug - show all task names found
    QStringList lines = allTasks.split('\n');

    uninstallReport += QString("  DEBUG: Parsing %1 lines of output...\n").arg(lines.count());

    for (const QString &line : lines) {
        if (line.startsWith("TaskName:")) {
            QString taskName = line.mid(9).trimmed(); // Skip "TaskName:"
            allTaskNames.append(taskName); // Track all tasks for debug

            // Check task name without leading backslash, but preserve original for deletion
            QString taskNameToCheck = taskName;
            if (taskNameToCheck.startsWith("\\")) {
                taskNameToCheck = taskNameToCheck.mid(1);
            }

            if (taskNameToCheck.startsWith("LAU3DVideoRecording-") || taskNameToCheck == "LAUOnTrakWidget-Login") {
                // Keep original task name (with backslash if it had one) for schtasks to find it
                tasksToDelete.append(taskName);
                uninstallReport += QString("  DEBUG: Found matching task: %1\n").arg(taskName);
            }
        }
    }

    uninstallReport += QString("\n  DEBUG: Found %1 total task(s) in Task Scheduler\n").arg(allTaskNames.count());
    if (!allTaskNames.isEmpty() && allTaskNames.count() < 20) {
        uninstallReport += "  DEBUG: All task names found:\n";
        for (const QString &name : allTaskNames) {
            uninstallReport += QString("    - %1\n").arg(name);
        }
    }
    uninstallReport += "\n";

    if (tasksToDelete.isEmpty()) {
        uninstallReport += "  ℹ No LAU tasks found in Task Scheduler\n";
        uninstallReport += "  Note: Looking for tasks starting with 'LAU3DVideoRecording-' or named 'LAUOnTrakWidget-Login'\n";
    } else {
        uninstallReport += QString("  Found %1 LAU task(s) to delete:\n").arg(tasksToDelete.count());

        // Delete each task
        for (const QString &taskName : tasksToDelete) {
            // Call schtasks directly with arguments (not through cmd.exe)
            // This properly handles task names with backslashes
            QStringList args;
            args << "/Delete" << "/TN" << taskName << "/F";

            uninstallReport += QString("\n  DEBUG: Executing: schtasks %1\n").arg(args.join(" "));

            QProcess deleteProcess;
            deleteProcess.start("schtasks", args);
            deleteProcess.waitForFinished();
            int result = deleteProcess.exitCode();
            QString deleteOutput = deleteProcess.readAllStandardOutput();
            QString deleteError = deleteProcess.readAllStandardError();

            if (result == 0) {
                uninstallReport += QString("    ✓ Deleted: %1\n").arg(taskName);
            } else {
                uninstallReport += QString("    ⚠ Failed to delete: %1 (error code %2)\n").arg(taskName).arg(result);
                if (!deleteOutput.isEmpty()) {
                    uninstallReport += QString("    Output: %1\n").arg(deleteOutput.trimmed());
                }
                if (!deleteError.isEmpty()) {
                    uninstallReport += QString("    Error: %1\n").arg(deleteError.trimmed());
                }
                hasErrors = true;
            }
        }
    }

    uninstallReport += "\n";

    // ========================================================================
    // DELETE SYSTEMCONFIG.INI
    // ========================================================================
    uninstallReport += "Removing configuration file:\n";

    if (QFile::exists(iniPath)) {
        if (QFile::remove(iniPath)) {
            uninstallReport += QString("  ✓ Deleted: %1\n").arg(iniPath);
        } else {
            uninstallReport += QString("  ⚠ Failed to delete: %1\n").arg(iniPath);
            hasErrors = true;
        }
    } else {
        uninstallReport += QString("  ℹ File not found: %1\n").arg(iniPath);
    }

    uninstallReport += "\n";

    // ========================================================================
    // CLEAR SHARED FOLDER FILES
    // ========================================================================
    uninstallReport += "Clearing shared folder files:\n";

    QString sharedFolderPath;
#ifdef Q_OS_WIN
    sharedFolderPath = "C:/ProgramData/3DVideoInspectionTools";
#elif defined(Q_OS_MAC)
    sharedFolderPath = "/Users/Shared/3DVideoInspectionTools";
#else
    sharedFolderPath = "/var/lib/3DVideoInspectionTools";
#endif

    QDir sharedDir(sharedFolderPath);
    if (sharedDir.exists()) {
        // List files to delete
        QStringList files = sharedDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

        if (files.isEmpty()) {
            uninstallReport += QString("  ℹ Shared folder is already empty: %1\n").arg(sharedFolderPath);
        } else {
            uninstallReport += QString("  Found %1 file(s) in shared folder:\n").arg(files.count());

            int deletedCount = 0;
            int errorCount = 0;

            for (const QString &fileName : files) {
                QString filePath = sharedDir.filePath(fileName);
                if (QFile::remove(filePath)) {
                    uninstallReport += QString("    ✓ Deleted: %1\n").arg(fileName);
                    deletedCount++;
                } else {
                    uninstallReport += QString("    ⚠ Failed to delete: %1\n").arg(fileName);
                    errorCount++;
                    hasErrors = true;
                }
            }

            uninstallReport += QString("  Summary: %1 deleted, %2 failed\n").arg(deletedCount).arg(errorCount);

            // Try to remove the directory if it's now empty
            if (errorCount == 0) {
                if (sharedDir.rmdir(sharedFolderPath)) {
                    uninstallReport += QString("  ✓ Removed empty shared folder: %1\n").arg(sharedFolderPath);
                } else {
                    // Directory might not be empty if there are subdirectories
                    uninstallReport += QString("  ℹ Shared folder remains (may contain subdirectories): %1\n").arg(sharedFolderPath);
                }
            }
        }
    } else {
        uninstallReport += QString("  ℹ Shared folder does not exist: %1\n").arg(sharedFolderPath);
    }

    uninstallReport += "\n";

    // ========================================================================
    // CLEAR QSETTINGS
    // ========================================================================
    uninstallReport += "Clearing application settings:\n";

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Lau Consulting Inc", "Remote Recording Tools");
    QString settingsPath = settings.fileName();

    // Clear all settings
    settings.clear();
    settings.sync();

    // Check if settings file exists and try to delete it
    if (QFile::exists(settingsPath)) {
        if (QFile::remove(settingsPath)) {
            uninstallReport += QString("  ✓ Deleted settings file: %1\n").arg(settingsPath);
        } else {
            uninstallReport += QString("  ⚠ Failed to delete settings file: %1\n").arg(settingsPath);
            hasErrors = true;
        }
    } else {
        uninstallReport += QString("  ℹ Settings file not found: %1\n").arg(settingsPath);
    }

    uninstallReport += "\n";

    // ========================================================================
    // SHOW RESULTS
    // ========================================================================
    if (hasErrors) {
        uninstallReport += "⚠ Uninstall completed with errors.\n"
                          "Some items may require administrator privileges to remove.\n";
    } else {
        uninstallReport += "✓ Uninstall completed successfully.\n";
    }

    // Create a custom message box with a Copy button
    QMessageBox *resultBox = new QMessageBox(this);
    resultBox->setWindowTitle("Uninstall Results");
    resultBox->setText(uninstallReport);
    resultBox->setIcon(hasErrors ? QMessageBox::Warning : QMessageBox::Information);

    // Add buttons
    QPushButton *copyButton = resultBox->addButton("Copy to Clipboard", QMessageBox::ActionRole);
    QPushButton *okButton = resultBox->addButton(QMessageBox::Ok);

    resultBox->setDefaultButton(okButton);

    // Connect copy button to copy without closing dialog
    connect(copyButton, &QPushButton::clicked, [resultBox, uninstallReport]() {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(uninstallReport);

        // Show brief confirmation in the title
        QString originalTitle = resultBox->windowTitle();
        resultBox->setWindowTitle("Uninstall Results - Copied to Clipboard!");
        QTimer::singleShot(1500, [resultBox, originalTitle]() {
            if (resultBox) {
                resultBox->setWindowTitle(originalTitle);
            }
        });
    });

    // Show the dialog modally
    resultBox->exec();
    delete resultBox;

    // Close the dialog after uninstall
    accept();
#else
    QMessageBox::information(
        this,
        "Development Mode",
        "Uninstall functionality is Windows-only.\n\n"
        "This is a development build on " + QSysInfo::productType()
    );
#endif
}
