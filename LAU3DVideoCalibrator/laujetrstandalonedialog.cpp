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

#include "laujetrstandalonedialog.h"
#include "lautiffviewerdialog.h"
#include <QRegularExpression>
#include <QDate>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUJETRStandaloneDialog::LAUJETRStandaloneDialog(QWidget *parent) :
    QDialog(parent),
    mainLayout(nullptr),
    fileInfoGroupBox(nullptr),
    fileInfoLayout(nullptr),
    filePathLabel(nullptr),
    filePathLineEdit(nullptr),
    openFileButton(nullptr),
    statusGroupBox(nullptr),
    statusLayout(nullptr),
    statusLabel(nullptr),
    progressBar(nullptr),
    tabWidget(nullptr),
    saveLUTXButton(nullptr),
    closeButton(nullptr),
    fileLoaded(false),
    widgetsModified(false),
    skipIntrinsicsDuringImport(false)
{
    // Initialize settings
    settings = new QSettings("LAU", "JETRStandalone", this);
    
    setupUI();
    setWindowTitle("JETR Standalone Editor");
    resize(800, 600);
    
    // Show file dialog immediately if no file path provided
    QTimer::singleShot(100, this, SLOT(onOpenFileClicked()));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUJETRStandaloneDialog::LAUJETRStandaloneDialog(const QString &filePath, QWidget *parent) :
    QDialog(parent),
    mainLayout(nullptr),
    fileInfoGroupBox(nullptr),
    fileInfoLayout(nullptr),
    filePathLabel(nullptr),
    filePathLineEdit(nullptr),
    openFileButton(nullptr),
    statusGroupBox(nullptr),
    statusLayout(nullptr),
    statusLabel(nullptr),
    progressBar(nullptr),
    tabWidget(nullptr),
    saveLUTXButton(nullptr),
    closeButton(nullptr),
    fileLoaded(false),
    widgetsModified(false),
    skipIntrinsicsDuringImport(false)
{
    // Initialize settings
    settings = new QSettings("LAU", "JETRStandalone", this);
    
    setupUI();
    setWindowTitle("JETR Standalone Editor");
    resize(800, 600);
    
    // Load the specified file
    if (!filePath.isEmpty()) {
        QTimer::singleShot(100, [this, filePath]() {
            loadTiffFile(filePath);
        });
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUJETRStandaloneDialog::~LAUJETRStandaloneDialog()
{
    // Qt will handle cleanup of child widgets
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::setupUI()
{
    // Create main layout
    mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    // Combined File Information and Status group
    fileInfoGroupBox = new QGroupBox("File Information and Status", this);
    QVBoxLayout *combinedLayout = new QVBoxLayout(fileInfoGroupBox);
    combinedLayout->setContentsMargins(6, 6, 6, 6);
    combinedLayout->setSpacing(2);  // Reduce vertical spacing

    // File selection row
    QHBoxLayout *fileRowLayout = new QHBoxLayout();
    filePathLabel = new QLabel("TIFF File:", fileInfoGroupBox);
    filePathLineEdit = new QLineEdit(fileInfoGroupBox);
    filePathLineEdit->setReadOnly(true);
    openFileButton = new QPushButton("Open TIFF...", fileInfoGroupBox);

    fileRowLayout->addWidget(filePathLabel);
    fileRowLayout->addWidget(filePathLineEdit);
    fileRowLayout->addWidget(openFileButton);
    combinedLayout->addLayout(fileRowLayout);

    // Status section (hidden until user tries to load a file)
    statusLabel = new QLabel("No file loaded", fileInfoGroupBox);
    statusLabel->setVisible(false);
    progressBar = new QProgressBar(fileInfoGroupBox);
    progressBar->setVisible(false);

    combinedLayout->addWidget(statusLabel);
    combinedLayout->addWidget(progressBar);

    mainLayout->addWidget(fileInfoGroupBox);

    // Keep statusGroupBox pointer for compatibility (point to same group box)
    statusGroupBox = fileInfoGroupBox;
    statusLayout = combinedLayout;
    fileInfoLayout = nullptr;
    
    // JETR tabs container
    tabWidget = new QTabWidget(this);
    tabWidget->setEnabled(false);
    mainLayout->addWidget(tabWidget);

    // Add vertical spacer to compress tab widget to minimum size
    mainLayout->addStretch();

    // Custom button layout with left and right sections
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    saveLUTXButton = new QPushButton("Save LUTX", this);
    importLUTXButton = new QPushButton("Import LUTX", this);
    closeButton = new QPushButton("Close", this);

    saveLUTXButton->setEnabled(false);
    importLUTXButton->setEnabled(false);

    // Add buttons to left side
    buttonLayout->addWidget(importLUTXButton);
    buttonLayout->addWidget(saveLUTXButton);
    buttonLayout->addStretch();  // Push Close button to the right
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(openFileButton, SIGNAL(clicked()), this, SLOT(onOpenFileClicked()));
    connect(saveLUTXButton, SIGNAL(clicked()), this, SLOT(onSaveLUTXClicked()));
    connect(importLUTXButton, SIGNAL(clicked()), this, SLOT(onImportLUTXClicked()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    // Allow dialog to be resizable - don't fix size
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::onOpenFileClicked()
{
    // Get the last used directory from settings
    QString lastDirectory = settings->value("lastDirectory", QDir::homePath()).toString();
    
    QString fileName = QFileDialog::getOpenFileName(this, 
                                                  "Open TIFF File", 
                                                  lastDirectory, 
                                                  "TIFF Files (*.tif *.tiff)");
    
    if (!fileName.isEmpty()) {
        // Save the directory for next time
        QFileInfo fileInfo(fileName);
        settings->setValue("lastDirectory", fileInfo.absolutePath());
        
        loadTiffFile(fileName);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::loadTiffFile(const QString &filePath)
{
    if (!QFileInfo::exists(filePath)) {
        statusLabel->setText("Error: File does not exist");
        statusLabel->setVisible(true);
        return;
    }

    statusLabel->setText("Loading TIFF file...");
    statusLabel->setVisible(true);
    progressBar->setVisible(true);
    progressBar->setRange(0, 0); // Indeterminate progress
    enableControls(false);
    
    QApplication::processEvents();
    
    try {
        // Load the memory object from the TIFF file
        memoryObject = LAUMemoryObject(filePath);
        
        if (memoryObject.isNull()) {
            statusLabel->setText("Error: Failed to load TIFF file");
            progressBar->setVisible(false);
            enableControls(true);
            return;
        }
        
        // Update UI
        currentTiffPath = filePath;
        filePathLineEdit->setText(QFileInfo(filePath).absoluteFilePath());
        filePathLineEdit->setToolTip(QFileInfo(filePath).absoluteFilePath()); // Also show in tooltip
        
        // Extract JETR vectors from memory object
        QVector<double> objectJetr = memoryObject.jetr();

        if (!objectJetr.isEmpty() && (objectJetr.size() % 37 == 0)) {
            // Memory object has JETR data - identify cameras and create tabs
            int numCameras = objectJetr.size() / 37;
            QList<QVector<double>> jetrVectors;
            QList<LAUCameraInfo> identifiedCameras;

            statusLabel->setText("Identifying cameras from embedded JETR data...");
            QApplication::processEvents();

            for (int i = 0; i < numCameras; ++i) {
                QVector<double> cameraJetr(37);
                int startIndex = i * 37;
                for (int j = 0; j < 37; ++j) {
                    cameraJetr[j] = objectJetr[startIndex + j];
                }
                jetrVectors.append(cameraJetr);
                
                // Try to identify what camera this JETR vector represents
                QPair<QString, QString> guess = LAUJETRWidget::guessCameraFromJETR(cameraJetr);
                if (!guess.first.isEmpty() && !guess.second.isEmpty()) {
                    LAUCameraInfo info(guess.first, guess.second, 
                                     (i == 0) ? "top" : (i == 1) ? "middle" : "bottom", false);
                    identifiedCameras.append(info);
                    statusLabel->setText(QString("Camera %1 identified as: %2 %3").arg(i+1).arg(guess.first).arg(guess.second));
                    QApplication::processEvents();
                } else {
                    // Couldn't identify - use default
                    LAUCameraInfo info("Unknown", "Unknown", 
                                     (i == 0) ? "top" : (i == 1) ? "middle" : "bottom", false);
                    identifiedCameras.append(info);
                }
            }
            
            // Set up multi-camera interface with identified cameras
            setJETRVectors(jetrVectors);
            
            // Parse date from file path for date-aware LUT generation
            QFileInfo fileInfo(filePath);
            QString fileName = fileInfo.fileName();
            QString folderName = fileInfo.dir().dirName();

            // Try to get date from TIFF DateTime tag first (most reliable)
            QDate folderDate;
            QDateTime tiffDateTime = LAUMemoryObject::getTiffDateTime(filePath, 0);
            if (tiffDateTime.isValid()) {
                folderDate = tiffDateTime.date();
                qDebug() << "LAUJETRStandaloneDialog: Using date from TIFF DateTime tag:"
                         << folderDate.toString("yyyy-MM-dd");
            } else {
                // Fallback to parsing folder name or filename
                folderDate = LAULookUpTable::parseFolderDate(folderName);
                if (!folderDate.isValid()) {
                    folderDate = LAULookUpTable::parseFolderDate(fileName);
                }
                qDebug() << "LAUJETRStandaloneDialog: Parsed date from file path:"
                         << (folderDate.isValid() ? folderDate.toString("yyyy-MM-dd") : "INVALID")
                         << "from folder:" << folderName << "or filename:" << fileName;
            }

            // Read camera positions from systemConfig.ini if available
            QStringList lucidPositions = readCameraPositionsFromConfig();

            // Set make/model information for each tab based on identification
            for (int i = 0; i < identifiedCameras.size() && i < jetrWidgets.size(); ++i) {
                if (jetrWidgets[i]) {
                    jetrWidgets[i]->setCameraMake(identifiedCameras[i].make);
                    jetrWidgets[i]->setCameraModel(identifiedCameras[i].model);
                    jetrWidgets[i]->setCameraRotation(identifiedCameras[i].rotated);
                    jetrWidgets[i]->setCurrentDate(folderDate);  // Set date BEFORE position to ensure LUT uses correct date

                    // Determine position for this camera
                    QString position = identifiedCameras[i].position; // Default from identification
                    if (i == 0) {
                        // Camera 0 is always the Orbbec (top camera)
                        position = "A TOP";
                    } else if (!lucidPositions.isEmpty()) {
                        // Cameras 1 and 2 are Lucid cameras - use positions from config
                        // Camera 1 gets lucidPositions[0], Camera 2 gets lucidPositions[1]
                        int lucidIndex = i - 1; // Convert to 0-based index for Lucid cameras
                        if (lucidIndex < lucidPositions.size()) {
                            position = lucidPositions[lucidIndex];
                            qDebug() << QString("Using position '%1' from systemConfig.ini for camera %2 (%3 %4)")
                                        .arg(position).arg(i+1).arg(identifiedCameras[i].make).arg(identifiedCameras[i].model);
                        }
                    }

                    jetrWidgets[i]->setCameraPosition(position);  // This triggers LUT generation
                    jetrWidgets[i]->setMemoryObjectOnly(memoryObject, i);
                }
            }

            fileLoaded = true;
            widgetsModified = true; // Loading a file is a modification that should be saved

            // Store original TIFF JETR vectors AFTER widget setup is complete
            // This avoids capturing vectors that were modified during the setup process
            originalTiffJETRVectors.clear();
            for (int i = 0; i < jetrWidgets.size(); ++i) {
                if (jetrWidgets[i]) {
                    QVector<double> jetrVec = jetrWidgets[i]->getJETRVector();
                    originalTiffJETRVectors.append(jetrVec);
                    qDebug() << QString("Stored original TIFF JETR for camera %1: fx=%2, fy=%3, cx=%4, cy=%5")
                                .arg(i+1).arg(jetrVec[0], 0, 'f', 6).arg(jetrVec[2], 0, 'f', 6).arg(jetrVec[1], 0, 'f', 6).arg(jetrVec[3], 0, 'f', 6);
                }
            }

            statusLabel->setText(QString("TIFF loaded: %1x%2 pixels, %3 camera(s) identified")
                               .arg(memoryObject.width())
                               .arg(memoryObject.height())
                               .arg(numCameras));
        } else {
            // No JETR data - clear stored vectors and try to guess cameras
            originalTiffJETRVectors.clear();
            statusLabel->setText("Analyzing image for camera identification...");
            QApplication::processEvents();
            
            // Determine number of cameras from image dimensions
            int imageHeight = memoryObject.height();
            int numCameras = (imageHeight == 1440) ? 3 : 1;  // Common multi-camera case
            
            QList<LAUCameraInfo> cameraInfos;
            
            // Try to guess camera make/model for each camera
            bool guessSuccessful = true;
            for (int i = 0; i < numCameras; ++i) {
                QPair<QString, QString> guess = LAUJETRWidget::guessCameraFromMemoryObject(memoryObject, i);
                if (!guess.first.isEmpty() && !guess.second.isEmpty()) {
                    LAUCameraInfo info(guess.first, guess.second, 
                                     (i == 0) ? "top" : (i == 1) ? "middle" : "bottom", false);
                    cameraInfos.append(info);
                    statusLabel->setText(QString("Identified Camera %1: %2 %3").arg(i+1).arg(guess.first).arg(guess.second));
                    QApplication::processEvents();
                } else {
                    guessSuccessful = false;
                    break;
                }
            }
            
            // If guessing failed or user wants to verify, show manual selection
            if (!guessSuccessful || cameraInfos.isEmpty()) {
                statusLabel->setText("Camera identification inconclusive - please select manually");
                QApplication::processEvents();
                
                cameraInfos = LAUJETRWidget::getMultiCameraMakeAndModel(memoryObject, this);
                
                if (cameraInfos.isEmpty()) {
                    statusLabel->setText("No cameras selected");
                    progressBar->setVisible(false);
                    enableControls(true);
                    return;
                }
            } else {
                // Show a dialog asking if user wants to confirm the guesses
                QString guessText = "Detected cameras:\n";
                for (int i = 0; i < cameraInfos.size(); ++i) {
                    guessText += QString("Camera %1: %2 %3 (%4)\n")
                                .arg(i+1).arg(cameraInfos[i].make).arg(cameraInfos[i].model).arg(cameraInfos[i].position);
                }
                guessText += "\nUse these cameras or select manually?";
                
                QMessageBox msgBox(this);
                msgBox.setWindowTitle("Camera Detection");
                msgBox.setText(guessText);
                msgBox.addButton("Use Detected", QMessageBox::AcceptRole);
                msgBox.addButton("Select Manually", QMessageBox::RejectRole);
                
                if (msgBox.exec() == QMessageBox::RejectRole) {
                    cameraInfos = LAUJETRWidget::getMultiCameraMakeAndModel(memoryObject, this);
                    
                    if (cameraInfos.isEmpty()) {
                        statusLabel->setText("No cameras selected");
                        progressBar->setVisible(false);
                        enableControls(true);
                        return;
                    }
                }
            }
            
            // Create default JETR vectors and set up tabs
            QList<QVector<double>> jetrVectors;
            for (int i = 0; i < cameraInfos.size(); ++i) {
                jetrVectors.append(LAUJETRWidget::createDefaultJETR());
            }
            
            setJETRVectors(jetrVectors);
            
            // Set make/model information for each tab
            for (int i = 0; i < cameraInfos.size() && i < jetrWidgets.size(); ++i) {
                if (jetrWidgets[i]) {
                    jetrWidgets[i]->setCameraMake(cameraInfos[i].make);
                    jetrWidgets[i]->setCameraModel(cameraInfos[i].model);
                    jetrWidgets[i]->setJETRVector(memoryObject, i, 
                                                cameraInfos[i].make, 
                                                cameraInfos[i].model, true);
                }
            }

            fileLoaded = true;
            widgetsModified = true; // Loading a file is a modification that should be saved
            statusLabel->setText(QString("TIFF loaded: %1x%2 pixels, %3 camera(s)")
                               .arg(memoryObject.width())
                               .arg(memoryObject.height())
                               .arg(cameraInfos.size()));
        }
        
    } catch (const std::exception &e) {
        statusLabel->setText(QString("Error: %1").arg(e.what()));
        fileLoaded = false;
    } catch (...) {
        statusLabel->setText("Error: Unknown error loading TIFF file");
        fileLoaded = false;
    }
    
    progressBar->setVisible(false);
    updateUI();
    enableControls(true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::onImportLUTXClicked()
{
    if (!fileLoaded) {
        QMessageBox::warning(this, "Warning", "Please load a TIFF file first before importing calibration");
        return;
    }

    // Warning dialog about overwriting current settings
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("Import Calibration");
    msgBox.setText("Import will overwrite current widget settings.");
    msgBox.setInformativeText("Are you sure you want to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() != QMessageBox::Yes) {
        return;
    }

    // Get the last used directory from settings
    QString lastDirectory = settings->value("lastDirectory", QDir::homePath()).toString();

    QString fileName = QFileDialog::getOpenFileName(this,
                                                  "Import Calibration from LUTX or TIFF",
                                                  lastDirectory,
                                                  "Calibration Files (*.lutx *.tif *.tiff);;LUTX Files (*.lutx);;TIFF Files (*.tif *.tiff);;All Files (*)");

    if (!fileName.isEmpty()) {
        // Save the directory for next time
        QFileInfo fileInfo(fileName);
        settings->setValue("lastDirectory", fileInfo.absolutePath());

        // Check file extension to determine import method
        QString extension = fileInfo.suffix().toLower();
        bool success = false;

        if (extension == "lutx") {
            // Import from LUTX file
            success = importLUTXFile(fileName);
            if (success) {
                statusLabel->setText("LUTX file imported successfully");
                QMessageBox::information(this, "Success", "LUTX file imported successfully!");
            } else {
                statusLabel->setText("Error: Failed to import LUTX file");
                QMessageBox::critical(this, "Error", "Failed to import LUTX file!");
            }
        } else if (extension == "tif" || extension == "tiff") {
            // Import from TIFF file (extract JETR vectors)
            success = importFromTiffFile(fileName);
            if (success) {
                statusLabel->setText("TIFF calibration imported successfully");
                QMessageBox::information(this, "Success", "Calibration imported from TIFF file successfully!");
            } else {
                statusLabel->setText("Error: Failed to import calibration from TIFF file");
                QMessageBox::critical(this, "Error", "Failed to import calibration from TIFF file!");
            }
        } else {
            QMessageBox::warning(this, "Unsupported Format",
                QString("Unsupported file format: .%1\n\nPlease select a .lutx or .tif/.tiff file.").arg(extension));
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::onSaveLUTXClicked()
{
    if (!fileLoaded || jetrWidgets.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No TIFF file loaded or no JETR widgets available");
        return;
    }

    // Validate that all transforms and bounding box are set before allowing save
    for (int i = 0; i < jetrWidgets.size(); ++i) {
        QVector<double> jetrVector = jetrWidgets[i]->getJETRVector();

        // Check transform matrix exists
        if (jetrVector.size() < 28) {
            QMessageBox::warning(this, "Transform Required",
                QString("Camera %1 does not have a valid transform matrix.\n\n"
                        "Before saving LUTX, you must:\n"
                        "1. Camera 1: Click 'Edit Transform Matrix' and fit the XY plane to the floor\n"
                        "2. Camera 2+: Click 'Edit Transform Matrix' and align to Camera 1").arg(i + 1));
            return;
        }

        // Check if transform is identity (not yet set)
        bool isIdentity = true;
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                int idx = 12 + (row * 4 + col);
                double expected = (row == col) ? 1.0 : 0.0;
                if (std::abs(jetrVector[idx] - expected) > 0.001) {
                    isIdentity = false;
                    break;
                }
            }
            if (!isIdentity) break;
        }

        if (isIdentity) {
            if (i == 0) {
                QMessageBox::warning(this, "Top Camera Transform Required",
                    "Camera 1 (top) must have its transform set before saving.\n\n"
                    "Please:\n"
                    "1. Switch to Camera 1 tab\n"
                    "2. Click 'Edit Transform Matrix...'\n"
                    "3. Set the transform (e.g., fit XY plane to floor)\n"
                    "4. Accept the transform\n\n"
                    "Then you can save the LUTX file.");
            } else {
                QMessageBox::warning(this, "Camera Alignment Required",
                    QString("Camera %1 must be aligned before saving.\n\n"
                            "Please:\n"
                            "1. Switch to Camera %1 tab\n"
                            "2. Set the camera position (side, quarter, rump, etc.)\n"
                            "3. Click 'Edit Transform Matrix...'\n"
                            "4. Align this camera to Camera 1 using the merge scan dialog\n"
                            "5. Accept the transform\n\n"
                            "Then you can save the LUTX file.").arg(i + 1));
            }
            return;
        }

        // Check bounding box is set (not all default -1000/1000 values)
        if (jetrVector.size() >= 34) {
            bool hasDefaultBBox = (std::abs(jetrVector[28] - (-1000.0)) < 0.1 &&
                                   std::abs(jetrVector[29] - 1000.0) < 0.1 &&
                                   std::abs(jetrVector[30] - (-1000.0)) < 0.1 &&
                                   std::abs(jetrVector[31] - 1000.0) < 0.1 &&
                                   std::abs(jetrVector[32] - (-1000.0)) < 0.1 &&
                                   std::abs(jetrVector[33] - 1000.0) < 0.1);

            if (hasDefaultBBox) {
                QMessageBox::warning(this, "Bounding Box Required",
                    "The bounding box has not been set.\n\n"
                    "Before saving LUTX, you must:\n"
                    "1. Set all camera transforms (top camera to floor, other cameras aligned)\n"
                    "2. Click 'Edit Bounding Box' button to set the 3D region of interest\n\n"
                    "The bounding box defines the spatial limits for 3D reconstruction.");
                return;
            }
        }
    }

    // All validation passed - proceed with save
    // Get default LUTX path based on TIFF filename (same directory as TIFF)
    QString defaultPath = getDefaultLUTXPath(currentTiffPath);

    QString fileName = QFileDialog::getSaveFileName(this,
                                                  "Save LUTX File",
                                                  defaultPath,
                                                  "LUTX Files (*.lutx)");

    if (!fileName.isEmpty()) {
        QList<QVector<double>> jetrVectors = getJETRVectors();
        if (saveLUTXFile(fileName, jetrVectors)) {
            statusLabel->setText("LUTX file saved successfully");
            widgetsModified = false; // Reset modified flag after successful save

            // Determine install/data folder path based on platform
            QString installFolderPath;
#ifdef Q_OS_WIN
            // Windows: Use ProgramData (writable by all users without admin)
            installFolderPath = "C:/ProgramData/3DVideoInspectionTools";
#elif defined(Q_OS_MAC)
            // macOS: Use /Users/Shared (writable by all users)
            installFolderPath = "/Users/Shared/3DVideoInspectionTools";
#else
            // Linux: Use /var/lib (standard for application data)
            installFolderPath = "/var/lib/3DVideoInspectionTools";
#endif
            QString backgroundFilePath = QDir(installFolderPath).absoluteFilePath("background.tif");

            // Ask user if they want to save background to install folder
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Save Background to Install Folder?");
            msgBox.setText("Would you like to save the background calibration to the install folder?");
            msgBox.setInformativeText(QString("This will save the background with complete JETR vectors to:\n\n%1\n\n"
                                             "This file will be used by LAUProcessVideos as the header for recorded videos.")
                                     .arg(backgroundFilePath));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);

            if (msgBox.exec() == QMessageBox::Yes) {
                if (saveBackgroundToInstallFolder(jetrVectors)) {
                    qDebug() << "Successfully saved background file to install folder";
                    QMessageBox::information(this, "Background Saved",
                                           QString("Background calibration saved to:\n%1").arg(backgroundFilePath));
                } else {
                    qWarning() << "Failed to save background file";
                    QMessageBox::warning(this, "Save Failed",
                                       QString("Failed to save background file to:\n%1").arg(backgroundFilePath));
                }
            }

            // Copy LUTX file to Public Pictures directory for cloud backup
            if (copyLUTXToPublicPictures(fileName)) {
                qDebug() << "Successfully copied LUTX to Public Pictures";
            } else {
                qWarning() << "Failed to copy LUTX to Public Pictures";
            }

            QMessageBox::information(this, "Success", "LUTX file saved successfully!");
        } else {
            statusLabel->setText("Error: Failed to save LUTX file");
            QMessageBox::critical(this, "Error", "Failed to save LUTX file!");
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRStandaloneDialog::saveLUTXFile(const QString &filePath, const QList<QVector<double>> &jetrVectors)
{
    if (!fileLoaded || memoryObject.isNull() || jetrVectors.isEmpty()) {
        return false;
    }
    
    try {
        statusLabel->setText("Generating lookup tables...");
        progressBar->setVisible(true);
        progressBar->setRange(0, jetrVectors.size());
        QApplication::processEvents();
        
        QList<LAULookUpTable> luts;

        // Use cached LUTs from widgets and update transform/bounding box
        for (int i = 0; i < jetrVectors.size(); ++i) {
            progressBar->setValue(i);
            statusLabel->setText(QString("Preparing lookup table %1 of %2...").arg(i+1).arg(jetrVectors.size()));
            QApplication::processEvents();

            QString position = (i < jetrWidgets.size() && jetrWidgets[i]) ? jetrWidgets[i]->getCameraPosition() : QString();

            // Get cached LUT from widget (uses smart caching)
            LAULookUpTable lut;
            if (i < jetrWidgets.size() && jetrWidgets[i]) {
                lut = jetrWidgets[i]->getCachedLUT();
            }

            if (lut.isNull()) {
                statusLabel->setText(QString("Error: Failed to get lookup table for camera %1").arg(i+1));
                progressBar->setVisible(false);
                return false;
            }

            // Update transform from current JETR vector
            if (jetrVectors[i].size() >= 28) {
                QMatrix4x4 transformMatrix;
                // Load transform matrix from JETR vector (elements 12-27)
                // JETR stores in row-major order, but QMatrix4x4 needs (row,col) indexing
                for (int row = 0; row < 4; row++) {
                    for (int col = 0; col < 4; col++) {
                        int jetrIndex = 12 + (row * 4 + col); // Row-major in JETR
                        transformMatrix(row, col) = static_cast<float>(jetrVectors[i][jetrIndex]);
                    }
                }
                lut.setTransform(transformMatrix);
            }

            // Update bounding box from current JETR vector
            if (jetrVectors[i].size() >= 34) {
                LookUpTableBoundingBox bbox;
                bbox.xMin = std::isfinite(jetrVectors[i][28]) ? jetrVectors[i][28] : -1000.0;
                bbox.xMax = std::isfinite(jetrVectors[i][29]) ? jetrVectors[i][29] : 1000.0;
                bbox.yMin = std::isfinite(jetrVectors[i][30]) ? jetrVectors[i][30] : -1000.0;
                bbox.yMax = std::isfinite(jetrVectors[i][31]) ? jetrVectors[i][31] : 1000.0;
                bbox.zMin = std::isfinite(jetrVectors[i][32]) ? jetrVectors[i][32] : -1000.0;
                bbox.zMax = std::isfinite(jetrVectors[i][33]) ? jetrVectors[i][33] : 1000.0;
                lut.setBoundingBox(bbox);
            }
            
            // Set software string with position and date information (matching JETR Manager convention)
            if (!position.isEmpty()) {
                // Try to extract date from TIFF filename, otherwise use current date
                QString date = extractDateFromFilename(currentTiffPath);
                if (date.isEmpty()) {
                    date = QDate::currentDate().toString("yyyy-MM-dd");
                }
                QString softwareInfo = QString("%1 - %2").arg(position, date);
                lut.setSoftwareString(softwareInfo);
            }
            
            luts.append(lut);
        }
        
        statusLabel->setText("Saving LUTX file...");
        QApplication::processEvents();
        
        bool success = LAULookUpTable::saveLookUpTables(luts, filePath);
        
        progressBar->setVisible(false);
        return success;
        
    } catch (const std::exception &e) {
        progressBar->setVisible(false);
        qWarning() << "Exception in saveLUTXFile:" << e.what();
        return false;
    } catch (...) {
        progressBar->setVisible(false);
        qWarning() << "Unknown exception in saveLUTXFile";
        return false;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRStandaloneDialog::importLUTXFile(const QString &filePath)
{
    if (!fileLoaded || memoryObject.isNull()) {
        return false;
    }

    // Reset import flags
    skipIntrinsicsDuringImport = false;

    try {
        statusLabel->setText("Loading LUTX file...");
        progressBar->setVisible(true);
        progressBar->setRange(0, 0); // Indeterminate progress
        QApplication::processEvents();

        // Load lookup tables from LUTX file
        QList<LAULookUpTable> lookupTables = LAULookUpTable::LAULookUpTableX(filePath);

        if (lookupTables.isEmpty()) {
            statusLabel->setText("Error: LUTX file contains no lookup tables");
            progressBar->setVisible(false);
            return false;
        }

        statusLabel->setText(QString("Processing %1 lookup table(s)...").arg(lookupTables.size()));
        QApplication::processEvents();

        // Extract JETR vectors from LUTX for comparison
        QList<QVector<double>> lutxJETRVectors;
        for (int tableIndex = 0; tableIndex < lookupTables.size(); ++tableIndex) {
            const LAULookUpTable &table = lookupTables[tableIndex];
            QVector<double> jetrVector = table.jetr();
            if (jetrVector.size() == 37) {
                // Use transform matrix from LUT (same as will be applied later)
                QMatrix4x4 transformMatrix = table.transform();
                const float *transformData = transformMatrix.constData();
                // Convert from QMatrix4x4 column-major to JETR row-major
                for (int row = 0; row < 4; ++row) {
                    for (int col = 0; col < 4; ++col) {
                        int jetrIndex = 12 + (row * 4 + col);     // JETR uses row-major storage
                        int matrixIndex = col * 4 + row;          // QMatrix4x4 uses column-major storage
                        jetrVector[jetrIndex] = transformData[matrixIndex];
                    }
                }
                lutxJETRVectors.append(jetrVector);
                qDebug() << QString("LUTX JETR for camera %1: fx=%2, fy=%3, cx=%4, cy=%5")
                            .arg(tableIndex+1).arg(jetrVector[0], 0, 'f', 6).arg(jetrVector[2], 0, 'f', 6).arg(jetrVector[1], 0, 'f', 6).arg(jetrVector[3], 0, 'f', 6);
            }
        }

        // Compare with original TIFF JETR vectors if they exist
        if (!originalTiffJETRVectors.isEmpty() && !lutxJETRVectors.isEmpty()) {
            // Check if any vectors differ
            bool anyDifference = false;
            int minCount = std::min(originalTiffJETRVectors.size(), lutxJETRVectors.size());

            for (int i = 0; i < minCount; ++i) {
                if (!compareJETRVectors(originalTiffJETRVectors[i], lutxJETRVectors[i])) {
                    anyDifference = true;
                    break;
                }
            }

            // Also check for camera count mismatch
            if (originalTiffJETRVectors.size() != lutxJETRVectors.size()) {
                anyDifference = true;
            }

            if (anyDifference) {
                QString comparisonSummary = generateJETRComparisonSummary(originalTiffJETRVectors, lutxJETRVectors);

                QMessageBox msgBox(this);
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setWindowTitle("Camera Mismatch Detected");
                msgBox.setText("The camera intrinsic parameters in the LUTX file differ from those in the TIFF file.");
                msgBox.setInformativeText("This suggests the LUTX file may be from different cameras. How would you like to proceed?");
                msgBox.setDetailedText(comparisonSummary);

                QPushButton *importAllButton = msgBox.addButton("Import All", QMessageBox::AcceptRole);
                QPushButton *importExcludeIntrinsicsButton = msgBox.addButton("Import Excluding Intrinsics", QMessageBox::AcceptRole);
                QPushButton *cancelButton = msgBox.addButton("Cancel", QMessageBox::RejectRole);
                msgBox.setDefaultButton(cancelButton);

                importAllButton->setToolTip("Import all parameters including camera intrinsics (may overwrite camera calibration)");
                importExcludeIntrinsicsButton->setToolTip("Import only extrinsics, bounding box, and depth parameters (preserve camera intrinsics)");
                cancelButton->setToolTip("Cancel the import operation");

                int result = msgBox.exec();
                QAbstractButton *clickedButton = msgBox.clickedButton();

                if (clickedButton == cancelButton) {
                    statusLabel->setText("LUTX import cancelled");
                    progressBar->setVisible(false);
                    return false;
                } else if (clickedButton == importExcludeIntrinsicsButton) {
                    // Set flag to skip intrinsics during import
                    skipIntrinsicsDuringImport = true;
                    statusLabel->setText("Importing LUTX (excluding intrinsics)...");
                } else {
                    // Import all (default behavior)
                    skipIntrinsicsDuringImport = false;
                    statusLabel->setText("Importing LUTX (all parameters)...");
                }
            }
        }

        // Clear existing tabs
        clearTabs();

        // Create JETR widgets from lookup tables
        for (int i = 0; i < lookupTables.size(); ++i) {
            const LAULookUpTable &table = lookupTables[i];

            QVector<double> jetrVector = table.jetr();
            if (jetrVector.size() != 37) {
                statusLabel->setText(QString("Error: Invalid JETR vector size in table %1: %2 (expected 37)")
                                   .arg(i + 1).arg(jetrVector.size()));
                progressBar->setVisible(false);
                return false;
            }

            // Always use transform matrix from LUT to override the JETR extrinsic parameters
            // The jetr() method uses projection matrix, but we want transform matrix for user-defined transforms
            QMatrix4x4 transformMatrix = table.transform();

            // Replace elements 12-27 (extrinsic parameters) with transform matrix data
            // NOTE: QMatrix4x4 stores in column-major order, JETR expects row-major order
            const float *transformData = transformMatrix.constData();
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    int jetrIndex = 12 + (row * 4 + col);     // JETR uses row-major storage
                    int matrixIndex = col * 4 + row;          // QMatrix4x4 uses column-major storage
                    jetrVector[jetrIndex] = transformData[matrixIndex];
                }
            }

            qDebug() << QString("LUT %1: Using transform matrix from LUTX file").arg(i + 1);

            // Handle selective import (preserve intrinsics if requested)
            if (skipIntrinsicsDuringImport && i < originalTiffJETRVectors.size()) {
                // Preserve original intrinsic parameters (elements 0-11) from TIFF
                for (int n = 0; n < 12 && n < originalTiffJETRVectors[i].size(); ++n) {
                    jetrVector[n] = originalTiffJETRVectors[i][n];
                }
                qDebug() << QString("LUT %1: Preserved intrinsics from original TIFF").arg(i + 1);
            }

            // Create JETR widget with imported data
            QString make = table.makeString();
            QString model = table.modelString();
            QString tabTitle = QString("LUT %1 - %2 %3").arg(i + 1).arg(make).arg(model);

            // Don't set read-only when importing with a memory object loaded
            addJETRTab(jetrVector, make, model, tabTitle, false);

            // Set memory object and cache LUT to avoid regeneration
            if (i < jetrWidgets.size() && jetrWidgets[i]) {
                // Parse date from TIFF filename for date-aware processing
                QFileInfo fileInfo(currentTiffPath);
                QString fileName = fileInfo.fileName();
                QString folderName = fileInfo.dir().dirName();

                // Try to get date from TIFF DateTime tag first (most reliable)
                QDate folderDate;
                QDateTime tiffDateTime = LAUMemoryObject::getTiffDateTime(currentTiffPath, 0);
                if (tiffDateTime.isValid()) {
                    folderDate = tiffDateTime.date();
                    qDebug() << "LUTX import: Using date from TIFF DateTime tag:"
                             << folderDate.toString("yyyy-MM-dd");
                } else {
                    // Fallback to parsing folder name or filename
                    folderDate = LAULookUpTable::parseFolderDate(folderName);
                    if (!folderDate.isValid()) {
                        folderDate = LAULookUpTable::parseFolderDate(fileName);
                    }
                    qDebug() << "LUTX import: Parsed date from file path:"
                             << (folderDate.isValid() ? folderDate.toString("yyyy-MM-dd") : "INVALID");
                }

                jetrWidgets[i]->setCurrentDate(folderDate);
                jetrWidgets[i]->setMemoryObjectOnly(memoryObject, i);

                // Cache the LUT from LUTX to avoid regeneration
                jetrWidgets[i]->setCachedLUT(table);
                qDebug() << QString("LUTX import: Cached LUT for camera %1 to avoid regeneration").arg(i + 1);

                // Set position information if available from software string
                // This can trigger updateMasterScanIfTop() which will now use cached LUT
                QString softwareString = table.softwareString();
                if (!softwareString.isEmpty()) {
                    // Software string format: "position - date"
                    QStringList parts = softwareString.split(" - ");
                    if (!parts.isEmpty()) {
                        QString position = parts.first().trimmed();
                        if (!position.isEmpty()) {
                            jetrWidgets[i]->setCameraPosition(position);
                        }
                    }
                }
            }
        }

        statusLabel->setText(QString("Imported %1 lookup table(s) from LUTX").arg(lookupTables.size()));
        progressBar->setVisible(false);
        widgetsModified = true; // Importing is a modification that should be saved
        return true;

    } catch (const std::exception &e) {
        progressBar->setVisible(false);
        statusLabel->setText(QString("Error: %1").arg(e.what()));
        qWarning() << "Exception in importLUTXFile:" << e.what();
        return false;
    } catch (...) {
        progressBar->setVisible(false);
        statusLabel->setText("Error: Unknown error importing LUTX file");
        qWarning() << "Unknown exception in importLUTXFile";
        return false;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRStandaloneDialog::importFromTiffFile(const QString &filePath)
{
    if (!fileLoaded || memoryObject.isNull()) {
        return false;
    }

    try {
        statusLabel->setText("Loading TIFF file to extract calibration...");
        progressBar->setVisible(true);
        progressBar->setRange(0, 0); // Indeterminate progress
        QApplication::processEvents();

        // Load the TIFF file to extract JETR vectors
        LAUMemoryObject importObject(filePath);

        if (importObject.isNull()) {
            statusLabel->setText("Error: Failed to load TIFF file");
            progressBar->setVisible(false);
            return false;
        }

        // Extract JETR vectors from the TIFF file
        QVector<double> importJetr = importObject.jetr();

        if (importJetr.isEmpty() || (importJetr.size() % 37 != 0)) {
            statusLabel->setText("Error: TIFF file contains no valid JETR calibration data");
            progressBar->setVisible(false);
            QMessageBox::warning(this, "No Calibration Data",
                "The selected TIFF file does not contain JETR calibration data.\n\n"
                "Please select a background.tif file that was saved with calibration.");
            return false;
        }

        int numCameras = importJetr.size() / 37;
        statusLabel->setText(QString("Found %1 camera calibration(s) in TIFF file...").arg(numCameras));
        QApplication::processEvents();

        // Split the concatenated JETR vector into individual camera vectors
        QList<QVector<double>> tiffJETRVectors;
        for (int i = 0; i < numCameras; ++i) {
            QVector<double> cameraJetr(37);
            int startIndex = i * 37;
            for (int j = 0; j < 37; ++j) {
                cameraJetr[j] = importJetr[startIndex + j];
            }
            tiffJETRVectors.append(cameraJetr);
            qDebug() << QString("TIFF import: Camera %1 JETR: fx=%2, fy=%3, cx=%4, cy=%5")
                        .arg(i+1).arg(cameraJetr[0], 0, 'f', 6).arg(cameraJetr[2], 0, 'f', 6)
                        .arg(cameraJetr[1], 0, 'f', 6).arg(cameraJetr[3], 0, 'f', 6);
        }

        // Compare with original TIFF JETR vectors if they exist
        skipIntrinsicsDuringImport = false;
        if (!originalTiffJETRVectors.isEmpty() && !tiffJETRVectors.isEmpty()) {
            // Check if any vectors differ
            bool anyDifference = false;
            int minCount = std::min(originalTiffJETRVectors.size(), tiffJETRVectors.size());

            for (int i = 0; i < minCount; ++i) {
                if (!compareJETRVectors(originalTiffJETRVectors[i], tiffJETRVectors[i])) {
                    anyDifference = true;
                    break;
                }
            }

            // Also check for camera count mismatch
            if (originalTiffJETRVectors.size() != tiffJETRVectors.size()) {
                anyDifference = true;
            }

            if (anyDifference) {
                QString comparisonSummary = generateJETRComparisonSummary(originalTiffJETRVectors, tiffJETRVectors);

                QMessageBox msgBox(this);
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setWindowTitle("Camera Mismatch Detected");
                msgBox.setText("The camera intrinsic parameters in the import TIFF file differ from the current TIFF file.");
                msgBox.setInformativeText("This suggests the files may be from different cameras. How would you like to proceed?");
                msgBox.setDetailedText(comparisonSummary);

                QPushButton *importAllButton = msgBox.addButton("Import All", QMessageBox::AcceptRole);
                QPushButton *importExcludeIntrinsicsButton = msgBox.addButton("Import Excluding Intrinsics", QMessageBox::AcceptRole);
                QPushButton *cancelButton = msgBox.addButton("Cancel", QMessageBox::RejectRole);
                msgBox.setDefaultButton(cancelButton);

                importAllButton->setToolTip("Import all parameters including camera intrinsics (may overwrite camera calibration)");
                importExcludeIntrinsicsButton->setToolTip("Import only extrinsics, bounding box, and depth parameters (preserve camera intrinsics)");
                cancelButton->setToolTip("Cancel the import operation");

                int result = msgBox.exec();
                QAbstractButton *clickedButton = msgBox.clickedButton();

                if (clickedButton == cancelButton) {
                    statusLabel->setText("TIFF import cancelled");
                    progressBar->setVisible(false);
                    return false;
                } else if (clickedButton == importExcludeIntrinsicsButton) {
                    // Set flag to skip intrinsics during import
                    skipIntrinsicsDuringImport = true;
                    statusLabel->setText("Importing TIFF calibration (excluding intrinsics)...");
                } else {
                    // Import all (default behavior)
                    skipIntrinsicsDuringImport = false;
                    statusLabel->setText("Importing TIFF calibration (all parameters)...");
                }
            }
        }

        // Decide whether to update existing widgets or create new ones
        bool hasExistingWidgets = !jetrWidgets.isEmpty();

        if (hasExistingWidgets) {
            // Update existing widgets (user already has a file loaded)
            // This preserves the user's current view and memory object

            // Verify we have the right number of cameras
            if (tiffJETRVectors.size() != jetrWidgets.size()) {
                statusLabel->setText(QString("Error: Import has %1 cameras but current file has %2")
                    .arg(tiffJETRVectors.size()).arg(jetrWidgets.size()));
                progressBar->setVisible(false);
                QMessageBox::warning(this, "Camera Count Mismatch",
                    QString("The import file has %1 camera(s) but the current file has %2 camera(s).\n\n"
                            "Cannot import - camera counts must match.")
                    .arg(tiffJETRVectors.size()).arg(jetrWidgets.size()));
                return false;
            }

            // Update each existing widget with imported JETR vector
            for (int i = 0; i < tiffJETRVectors.size() && i < jetrWidgets.size(); ++i) {
                if (!jetrWidgets[i]) continue;

                QVector<double> jetrVector = tiffJETRVectors[i];

                // Handle selective import (preserve intrinsics if requested)
                if (skipIntrinsicsDuringImport && i < originalTiffJETRVectors.size()) {
                    // Preserve original intrinsic parameters (elements 0-11) from current TIFF
                    for (int n = 0; n < 12 && n < originalTiffJETRVectors[i].size(); ++n) {
                        jetrVector[n] = originalTiffJETRVectors[i][n];
                    }
                    qDebug() << QString("TIFF import: Camera %1 - Preserved intrinsics from current TIFF").arg(i + 1);
                }

                // Block signals to prevent validation during batch update
                jetrWidgets[i]->blockSignals(true);

                // Update the JETR vector in the existing widget AND update UI
                jetrWidgets[i]->setJETRVector(jetrVector, true);

                // Unblock signals
                jetrWidgets[i]->blockSignals(false);

                qDebug() << QString("TIFF import: Updated camera %1 with imported JETR vector").arg(i + 1);
            }
        } else {
            // No existing widgets - create new tabs from imported TIFF
            // This happens when user imports without loading a file first
            qDebug() << "TIFF import: No existing widgets, creating new tabs";

            // Read camera positions from systemConfig.ini
            QStringList lucidPositions = readCameraPositionsFromConfig();

            // Create JETR widgets from TIFF vectors
            for (int i = 0; i < tiffJETRVectors.size(); ++i) {
                QVector<double> jetrVector = tiffJETRVectors[i];

                // Try to identify camera from JETR vector
                QPair<QString, QString> guess = LAUJETRWidget::guessCameraFromJETR(jetrVector);
                QString make = guess.first.isEmpty() ? "Unknown" : guess.first;
                QString model = guess.second.isEmpty() ? "Unknown" : guess.second;
                QString tabTitle = QString("Camera %1 - %2 %3").arg(i + 1).arg(make).arg(model);

                // Create JETR widget
                addJETRTab(jetrVector, make, model, tabTitle, false);

                // Set memory object and configure widget
                if (i < jetrWidgets.size() && jetrWidgets[i]) {
                    jetrWidgets[i]->setCameraMake(make);
                    jetrWidgets[i]->setCameraModel(model);

                    // Determine position for this camera
                    QString position;
                    if (i == 0) {
                        // Camera 0 is always the Orbbec (top camera)
                        position = "A TOP";
                    } else if (!lucidPositions.isEmpty()) {
                        // Cameras 1 and 2 are Lucid cameras - use positions from config
                        int lucidIndex = i - 1; // Convert to 0-based index for Lucid cameras
                        if (lucidIndex < lucidPositions.size()) {
                            position = lucidPositions[lucidIndex];
                            qDebug() << QString("TIFF import: Using position '%1' from systemConfig.ini for camera %2")
                                        .arg(position).arg(i+1);
                        }
                    }

                    if (!position.isEmpty()) {
                        // Block signals during batch position assignment to avoid validation errors
                        jetrWidgets[i]->blockSignals(true);
                        jetrWidgets[i]->setCameraPosition(position);
                        jetrWidgets[i]->blockSignals(false);
                    }
                }
            }
        }

        fileLoaded = true;
        widgetsModified = true; // Importing is a modification that should be saved

        statusLabel->setText(QString("Imported calibration for %1 camera(s) from TIFF").arg(tiffJETRVectors.size()));
        progressBar->setVisible(false);
        return true;

    } catch (const std::exception &e) {
        progressBar->setVisible(false);
        statusLabel->setText(QString("Error: %1").arg(e.what()));
        qWarning() << "Exception in importFromTiffFile:" << e.what();
        return false;
    } catch (...) {
        progressBar->setVisible(false);
        statusLabel->setText("Error: Unknown error importing TIFF file");
        qWarning() << "Unknown exception in importFromTiffFile";
        return false;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUJETRStandaloneDialog::getDefaultLUTXPath(const QString &tiffPath) const
{
    // Determine install folder path
    // In debug builds, use hardcoded install path
    // In release builds, use directory where executable is located
    QString installFolderPath;
#ifndef NDEBUG
    // Debug mode - use installed tools location
#ifdef Q_OS_WIN
    installFolderPath = "C:/Program Files (x86)/RemoteRecordingTools";
#elif defined(Q_OS_MAC)
    installFolderPath = "/Applications";
#else
    installFolderPath = "/usr/local/bin";
#endif
    qDebug() << "Debug mode - looking for systemConfig.ini at:" << installFolderPath;
#else
    // Release mode - executable directory contains systemConfig.ini
    installFolderPath = QCoreApplication::applicationDirPath();
    qDebug() << "Release mode - looking for systemConfig.ini in executable directory:" << installFolderPath;
#endif

    QString configPath = QDir(installFolderPath).filePath("systemConfig.ini");

    // Read system code and local temp path from systemConfig.ini
    QString systemCode;
    QString localTempPath;

    QFile configFile(configPath);
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&configFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("LocationCode=", Qt::CaseInsensitive)) {
                systemCode = line.mid(13).trimmed(); // Skip "LocationCode="
            } else if (line.startsWith("LocalTempPath=", Qt::CaseInsensitive)) {
                localTempPath = line.mid(14).trimmed(); // Skip "LocalTempPath="
            }

            // Exit early if we found both values
            if (!systemCode.isEmpty() && !localTempPath.isEmpty()) {
                break;
            }
        }
        configFile.close();
    }

    // If no system code found, use "XXX" as default
    if (systemCode.isEmpty()) {
        systemCode = "XXX";
    }

    // Use local temp path if available, otherwise fall back to TIFF file's directory
    QString directory;
    if (!localTempPath.isEmpty() && QDir(localTempPath).exists()) {
        directory = localTempPath;
    } else {
        // Fallback to TIFF file's directory
        QFileInfo fileInfo(tiffPath);
        directory = fileInfo.absolutePath();
    }

    // Get date from TIFF file creation date
    QString dateString;
    QDateTime tiffDateTime = LAUMemoryObject::getTiffDateTime(tiffPath, 0);
    if (tiffDateTime.isValid()) {
        dateString = tiffDateTime.date().toString("yyyyMMdd");
    } else {
        // Fallback to current date if TIFF date not available
        dateString = QDate::currentDate().toString("yyyyMMdd");
    }

    // Generate filename: systemXXX########.lutx
    QString lutxFileName = QString("system%1%2.lutx").arg(systemCode).arg(dateString);

    return QDir(directory).absoluteFilePath(lutxFileName);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::updateUI()
{
    saveLUTXButton->setEnabled(fileLoaded);
    importLUTXButton->setEnabled(fileLoaded);
    tabWidget->setEnabled(fileLoaded);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::enableControls(bool enabled)
{
    openFileButton->setEnabled(enabled);
    saveLUTXButton->setEnabled(enabled && fileLoaded);
    importLUTXButton->setEnabled(enabled && fileLoaded);
    tabWidget->setEnabled(enabled && fileLoaded);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::clearTabs()
{
    // Clear existing tabs and widgets
    while (tabWidget->count() > 0) {
        QWidget *widget = tabWidget->widget(0);
        tabWidget->removeTab(0);
        if (widget) {
            widget->deleteLater();
        }
    }
    jetrWidgets.clear();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::addJETRTab(const QVector<double> &jetrVector, const QString &tabTitle)
{
    LAUJETRWidget *widget = new LAUJETRWidget(jetrVector);

    // Connect to receive updates
    connect(widget, &LAUJETRWidget::jetrVectorChanged,
            this, &LAUJETRStandaloneDialog::onJETRVectorChanged);
    connect(widget, &LAUJETRWidget::requestBoundingBoxEdit,
            this, &LAUJETRStandaloneDialog::onEditBoundingBox);

    QString title = tabTitle;
    if (title.isEmpty()) {
        title = QString("Camera %1").arg(tabWidget->count() + 1);
    }

    tabWidget->addTab(widget, title);
    jetrWidgets.append(widget);

    // First camera is always "Top" and read-only
    if (jetrWidgets.size() == 1) {
        widget->setCameraPosition("A TOP");
        widget->setCameraPositionReadOnly(true);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::addJETRTab(const QVector<double> &jetrVector, const QString &make, const QString &model, const QString &tabTitle, bool readOnly)
{
    LAUJETRWidget *widget = new LAUJETRWidget(jetrVector);

    // Set make and model in the widget
    widget->setCameraMake(make);
    widget->setCameraModel(model);

    // Set read-only mode (true when no memory object available, false when importing with data)
    widget->setReadOnly(readOnly);

    // Connect to receive updates
    connect(widget, &LAUJETRWidget::jetrVectorChanged,
            this, &LAUJETRStandaloneDialog::onJETRVectorChanged);
    connect(widget, &LAUJETRWidget::requestBoundingBoxEdit,
            this, &LAUJETRStandaloneDialog::onEditBoundingBox);

    QString title = tabTitle;
    if (title.isEmpty()) {
        title = QString("%1 - %2").arg(make, model);
    }

    tabWidget->addTab(widget, title);
    jetrWidgets.append(widget);

    // First camera is always "Top" and read-only
    if (jetrWidgets.size() == 1) {
        widget->setCameraPosition("A TOP");
        widget->setCameraPositionReadOnly(true);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::setJETRVectors(const QList<QVector<double>> &vectors)
{
    clearTabs();

    if (vectors.isEmpty()) {
        addJETRTab(QVector<double>(37, NAN), "Default");
        return;
    }

    for (int i = 0; i < vectors.size(); ++i) {
        addJETRTab(vectors[i], QString("Camera %1").arg(i + 1));

        // If we have a memory object, set it on the widget without overriding JETR
        if (memoryObject.isValid() && i < jetrWidgets.size()) {
            jetrWidgets[i]->setMemoryObjectOnly(memoryObject, i);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector<double> LAUJETRStandaloneDialog::getJETRVector() const
{
    if (tabWidget->currentWidget()) {
        LAUJETRWidget *widget = qobject_cast<LAUJETRWidget*>(tabWidget->currentWidget());
        if (widget) {
            return widget->getJETRVector();
        }
    }
    return QVector<double>();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QVector<double>> LAUJETRStandaloneDialog::getJETRVectors() const
{
    QList<QVector<double>> vectors;
    for (LAUJETRWidget *widget : jetrWidgets) {
        if (widget) {
            vectors.append(widget->getJETRVector());
        }
    }
    return vectors;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUJETRStandaloneDialog::getMakes() const
{
    QStringList makes;
    for (LAUJETRWidget *widget : jetrWidgets) {
        if (widget) {
            makes.append(widget->getCameraMake());
        }
    }
    return makes;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUJETRStandaloneDialog::getModels() const
{
    QStringList models;
    for (LAUJETRWidget *widget : jetrWidgets) {
        if (widget) {
            models.append(widget->getCameraModel());
        }
    }
    return models;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::onJETRVectorChanged(const QVector<double> &vector)
{
    Q_UNUSED(vector);
    // Enable save button when any JETR vector changes
    saveLUTXButton->setEnabled(true);
    statusLabel->setText("JETR configuration updated");

    // Mark widgets as modified
    widgetsModified = true;

    qDebug() << "LAUJETRStandaloneDialog: Widget modified flag set to true";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::onEditBoundingBox()
{
    if (!fileLoaded || jetrWidgets.isEmpty()) {
        QMessageBox::information(this, "No Data", "No TIFF file loaded or no camera data available.");
        return;
    }

    // Check that all camera transforms are properly set before allowing bounding box editing
    for (int i = 0; i < jetrWidgets.size(); ++i) {
        QVector<double> jetrVector = jetrWidgets[i]->getJETRVector();
        if (jetrVector.size() < 28) {
            QMessageBox::warning(this, "Transform Required",
                QString("Camera %1 does not have a valid transform matrix.\n\n"
                        "Please set transforms for all cameras before editing the bounding box:\n"
                        "1. Camera 1: Click 'Edit Transform Matrix' and fit the XY plane to the floor\n"
                        "2. Camera 2+: Click 'Edit Transform Matrix' and align to Camera 1").arg(i + 1));
            return;
        }

        // Check if transform is identity (not yet set)
        bool isIdentity = true;
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                int idx = 12 + (row * 4 + col); // Row-major in JETR
                double expected = (row == col) ? 1.0 : 0.0;
                if (std::abs(jetrVector[idx] - expected) > 0.001) {
                    isIdentity = false;
                    break;
                }
            }
            if (!isIdentity) break;
        }

        if (isIdentity) {
            if (i == 0) {
                QMessageBox::warning(this, "Top Camera Transform Required",
                    "Camera 1 (top) must have its transform set before editing bounding box.\n\n"
                    "Please:\n"
                    "1. Switch to Camera 1 tab\n"
                    "2. Click 'Edit Transform Matrix...'\n"
                    "3. Set the transform (e.g., fit XY plane to floor)\n"
                    "4. Accept the transform\n\n"
                    "Then you can edit the bounding box.");
            } else {
                QMessageBox::warning(this, "Camera Alignment Required",
                    QString("Camera %1 must be aligned before editing bounding box.\n\n"
                            "Please:\n"
                            "1. Switch to Camera %1 tab\n"
                            "2. Set the camera position (side, quarter, rump, etc.)\n"
                            "3. Click 'Edit Transform Matrix...'\n"
                            "4. Align this camera to Camera 1 using the merge scan dialog\n"
                            "5. Accept the transform\n\n"
                            "Then you can edit the bounding box.").arg(i + 1));
            }
            return;
        }
    }

    // Use cached LUTs if available, otherwise try to populate from widgets, otherwise generate them
    QList<LAULookUpTable> lookupTables;

    // Try to populate cache from widget caches if we don't have cached LUTs
    if (cachedLUTs.isEmpty()) {
        populateCacheFromWidgets();
    }

    // If still no cached LUTs available, we can't edit bounding box without memory object
    if (cachedLUTs.isEmpty()) {
        QMessageBox::information(this, "Bounding Box Editor",
            "The visual bounding box editor requires a depth image for reference.\n\n"
            "When importing LUTX files, the bounding box can only be edited by:\n"
            "1. Loading the original TIFF file first, then importing the LUTX\n"
            "2. Manually editing the bounding box values in the parameter fields");
        return;
    }
    
    if (cachedLUTs.size() == jetrWidgets.size() && !cachedLUTs.isEmpty()) {
        // Use cached LUTs and update their transforms and bounding boxes
        QMatrix4x4 toXYPlaneTransform; // Transform from first camera to XY plane
        
        for (int i = 0; i < jetrWidgets.size(); ++i) {
            LAULookUpTable lut = cachedLUTs[i]; // Copy cached LUT
            
            // Get the current JETR vector from this widget to update transform and bounding box
            QVector<double> jetrVector = jetrWidgets[i]->getJETRVector();
            if (jetrVector.size() >= 28) {
                // Extract the 4x4 transform matrix from JETR elements 12-27
                QMatrix4x4 transformMatrix;
                // Load transform matrix from JETR vector (elements 12-27)
                // JETR stores in row-major order, but QMatrix4x4 needs (row,col) indexing
                for (int row = 0; row < 4; row++) {
                    for (int col = 0; col < 4; col++) {
                        int jetrIndex = 12 + (row * 4 + col); // Row-major in JETR
                        transformMatrix(row, col) = static_cast<float>(jetrVector[jetrIndex]);
                    }
                }

                if (i == 0) {
                    // First camera: this is the transform to XY plane
                    toXYPlaneTransform = transformMatrix;
                    lut.setTransform(transformMatrix);
                } else {
                    // Subsequent cameras: transform is relative to first camera
                    // Need to convert to XY plane: (toXYPlane) * (thisToFirst) = (thisToXYPlane)
                    QMatrix4x4 toXYPlane = toXYPlaneTransform * transformMatrix;
                    lut.setTransform(toXYPlane);
                }
            }
            
            // Set proper bounding box values from JETR vector
            if (jetrVector.size() >= 34) {
                LookUpTableBoundingBox bbox;
                bbox.xMin = std::isfinite(jetrVector[28]) ? jetrVector[28] : -1000.0;
                bbox.xMax = std::isfinite(jetrVector[29]) ? jetrVector[29] : 1000.0;
                bbox.yMin = std::isfinite(jetrVector[30]) ? jetrVector[30] : -1000.0;
                bbox.yMax = std::isfinite(jetrVector[31]) ? jetrVector[31] : 1000.0;
                bbox.zMin = std::isfinite(jetrVector[32]) ? jetrVector[32] : -1000.0;
                bbox.zMax = std::isfinite(jetrVector[33]) ? jetrVector[33] : 1000.0;
                lut.setBoundingBox(bbox);
            } else {
                // Default bounding box if JETR doesn't have bounding box data
                LookUpTableBoundingBox bbox;
                bbox.xMin = -1000.0;
                bbox.xMax = 1000.0;
                bbox.yMin = -1000.0;
                bbox.yMax = 1000.0;
                bbox.zMin = -1000.0;
                bbox.zMax = 1000.0;
                lut.setBoundingBox(bbox);
            }
            
            lookupTables.append(lut);
        }
    } else {
        // No cached LUTs available - need to generate them (fallback)
        QMatrix4x4 toXYPlaneTransform; // Transform from first camera to XY plane
        
        for (int i = 0; i < jetrWidgets.size(); ++i) {
            // Get the current make, model, and position from the JETR widget
            QString make = jetrWidgets[i]->getCameraMake();
            QString model = jetrWidgets[i]->getCameraModel();
            QString position = jetrWidgets[i]->getCameraPosition();
            
            if (make.isEmpty() || model.isEmpty()) {
                QMessageBox::warning(this, "Configuration Required", 
                    QString("Please set camera make and model for Camera %1 before editing bounding box.").arg(i + 1));
                return;
            }
            
            // Get cached LUT from the widget (this will generate if needed)
            LAULookUpTable lut = jetrWidgets[i]->getCachedLUT();
            if (lut.isNull()) {
                QMessageBox::warning(this, "LUT Generation Failed", 
                    QString("Failed to generate lookup table for Camera %1. Please check configuration.").arg(i + 1));
                return;
            }
            
            // Set software string with position and date information (matching JETR Manager convention)
            if (!position.isEmpty()) {
                // Try to extract date from TIFF filename, otherwise use current date
                QString date = extractDateFromFilename(currentTiffPath);
                if (date.isEmpty()) {
                    date = QDate::currentDate().toString("yyyy-MM-dd");
                }
                QString softwareInfo = QString("%1 - %2").arg(position, date);
                lut.setSoftwareString(softwareInfo);
            }
            
            // Get the current JETR vector from this widget
            QVector<double> jetrVector = jetrWidgets[i]->getJETRVector();
            if (jetrVector.size() >= 28) {
                // Extract the 4x4 transform matrix from JETR elements 12-27
                QMatrix4x4 transformMatrix;
                // Load transform matrix from JETR vector (elements 12-27)
                // JETR stores in row-major order, but QMatrix4x4 needs (row,col) indexing
                for (int row = 0; row < 4; row++) {
                    for (int col = 0; col < 4; col++) {
                        int jetrIndex = 12 + (row * 4 + col); // Row-major in JETR
                        transformMatrix(row, col) = static_cast<float>(jetrVector[jetrIndex]);
                    }
                }

                if (i == 0) {
                    // First camera: this is the transform to XY plane
                    toXYPlaneTransform = transformMatrix;
                    lut.setTransform(transformMatrix);
                } else {
                    // Subsequent cameras: transform is relative to first camera
                    // Need to convert to XY plane: (toXYPlane) * (thisToFirst) = (thisToXYPlane)
                    QMatrix4x4 toXYPlane = toXYPlaneTransform * transformMatrix;
                    lut.setTransform(toXYPlane);
                }
            }
            
            // Set proper bounding box values from JETR vector
            if (jetrVector.size() >= 34) {
                LookUpTableBoundingBox bbox;
                bbox.xMin = std::isfinite(jetrVector[28]) ? jetrVector[28] : -1000.0;
                bbox.xMax = std::isfinite(jetrVector[29]) ? jetrVector[29] : 1000.0;
                bbox.yMin = std::isfinite(jetrVector[30]) ? jetrVector[30] : -1000.0;
                bbox.yMax = std::isfinite(jetrVector[31]) ? jetrVector[31] : 1000.0;
                bbox.zMin = std::isfinite(jetrVector[32]) ? jetrVector[32] : -1000.0;
                bbox.zMax = std::isfinite(jetrVector[33]) ? jetrVector[33] : 1000.0;
                lut.setBoundingBox(bbox);
            } else {
                // Default bounding box if JETR doesn't have bounding box data
                LookUpTableBoundingBox bbox;
                bbox.xMin = -1000.0;
                bbox.xMax = 1000.0;
                bbox.yMin = -1000.0;
                bbox.yMax = 1000.0;
                bbox.zMin = -1000.0;
                bbox.zMax = 1000.0;
                lut.setBoundingBox(bbox);
            }
            
            lookupTables.append(lut);
        }
        
        // Cache the generated LUTs for future use
        if (!lookupTables.isEmpty()) {
            cacheLUTs(lookupTables);
        }
    }
    
    if (lookupTables.isEmpty()) {
        QMessageBox::warning(this, "No Lookup Tables", "No valid lookup tables could be generated.");
        return;
    }
    
    // Create and configure the TIFF viewer dialog for bounding box editing
    LAUTiffViewerDialog dialog(this);
    dialog.setWindowTitle("Edit Bounding Box");
    dialog.setTiffFilename(currentTiffPath);
    dialog.setLookupTables(lookupTables);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Get the updated bounding box from the dialog
        LookUpTableBoundingBox bbox = dialog.getBoundingBox();
        
        // Apply the bounding box to all JETR widgets
        for (LAUJETRWidget *widget : jetrWidgets) {
            if (widget) {
                QVector<double> jetr = widget->getJETRVector();
                if (jetr.size() >= 34) {
                    // Update bounding box parameters (indices 28-33)
                    jetr[28] = bbox.xMin;
                    jetr[29] = bbox.xMax;
                    jetr[30] = bbox.yMin;
                    jetr[31] = bbox.yMax;
                    jetr[32] = bbox.zMin;
                    jetr[33] = bbox.zMax;

                    widget->setJETRVector(jetr, true); // Update UI
                }
            }
        }
        
        statusLabel->setText("Bounding box updated for all cameras");
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::cacheLUTs(const QList<LAULookUpTable> &luts)
{
    cachedLUTs = luts;
    qDebug() << QString("Cached %1 LUTs for reuse").arg(luts.size());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::populateCacheFromWidgets()
{
    if (jetrWidgets.isEmpty()) {
        return;
    }
    
    QList<LAULookUpTable> luts;
    bool allValid = true;
    
    for (int i = 0; i < jetrWidgets.size(); ++i) {
        if (!jetrWidgets[i]) {
            allValid = false;
            break;
        }
        
        // Try to get cached LUT from widget
        LAULookUpTable lut = jetrWidgets[i]->getCachedLUT();
        if (lut.isNull()) {
            allValid = false;
            break;
        }
        
        luts.append(lut);
    }
    
    if (allValid && !luts.isEmpty()) {
        cacheLUTs(luts);
        qDebug() << "Populated cache from widget LUTs";
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::closeEvent(QCloseEvent *event)
{
    qDebug() << "LAUJETRStandaloneDialog::closeEvent - fileLoaded:" << fileLoaded << "widgetsModified:" << widgetsModified;

    // Only prompt to save if user modified JETR widgets
    if (widgetsModified) {
        qDebug() << "LAUJETRStandaloneDialog: Showing save prompt dialog (from closeEvent)";

        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowTitle("Save Calibration?");
        msgBox.setText("You have unsaved changes to your JETR configuration.");
        msgBox.setInformativeText("Would you like to save the calibration to a LUTX file before closing?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Yes);

        // Set button text for clarity
        QAbstractButton *yesButton = msgBox.button(QMessageBox::Yes);
        QAbstractButton *noButton = msgBox.button(QMessageBox::No);
        if (yesButton) yesButton->setText("Save LUTX");
        if (noButton) noButton->setText("Close Without Saving");

        int result = msgBox.exec();
        qDebug() << "LAUJETRStandaloneDialog: User chose:" << result;

        if (result == QMessageBox::Cancel) {
            qDebug() << "LAUJETRStandaloneDialog: User cancelled - staying open";
            event->ignore(); // Don't close - stay open
            return;
        } else if (result == QMessageBox::Yes) {
            qDebug() << "LAUJETRStandaloneDialog: User wants to save - triggering save";
            // Trigger the save LUTX operation
            onSaveLUTXClicked();
            // Don't close yet - let user see save result and close manually
            event->ignore();
            return;
        }
        // If No (Close Without Saving) - fall through to accept close
    }

    qDebug() << "LAUJETRStandaloneDialog: Accepting close event";
    event->accept(); // Accept the close event
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRStandaloneDialog::reject()
{
    qDebug() << "LAUJETRStandaloneDialog::reject - fileLoaded:" << fileLoaded << "widgetsModified:" << widgetsModified;

    // Only prompt to save if user modified JETR widgets
    if (widgetsModified) {
        qDebug() << "LAUJETRStandaloneDialog: Showing save prompt dialog";

        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowTitle("Save Calibration?");
        msgBox.setText("You have unsaved changes to your JETR configuration.");
        msgBox.setInformativeText("Would you like to save the calibration to a LUTX file before closing?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Yes);

        // Set button text for clarity
        QAbstractButton *yesButton = msgBox.button(QMessageBox::Yes);
        QAbstractButton *noButton = msgBox.button(QMessageBox::No);
        if (yesButton) yesButton->setText("Save LUTX");
        if (noButton) noButton->setText("Close Without Saving");

        int result = msgBox.exec();
        qDebug() << "LAUJETRStandaloneDialog: User chose:" << result;

        if (result == QMessageBox::Cancel) {
            qDebug() << "LAUJETRStandaloneDialog: User cancelled - staying open";
            return; // Don't close - stay open
        } else if (result == QMessageBox::Yes) {
            qDebug() << "LAUJETRStandaloneDialog: User wants to save - triggering save";
            // Trigger the save LUTX operation
            onSaveLUTXClicked();
            // Don't close yet - let user see save result and close manually
            return;
        }
        // If No (Close Without Saving) - fall through to close
    }

    qDebug() << "LAUJETRStandaloneDialog: Calling parent reject";
    QDialog::reject(); // Call parent reject to close dialog
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRStandaloneDialog::compareJETRVectors(const QVector<double> &vec1, const QVector<double> &vec2, double tolerance) const
{
    if (vec1.size() != vec2.size() || vec1.size() < 37) {
        return false;
    }

    // Only compare intrinsic camera parameters (elements 0-11)
    // These are fundamental properties of the camera hardware:
    // 0-3: Focal lengths and principal point (fx, cx, fy, cy)
    // 4-9: Radial distortion coefficients (k1-k6)
    // 10-11: Tangential distortion coefficients (p1, p2)
    //
    // Skip extrinsic parameters (12-27) - these represent camera pose which can vary
    // Skip bounding box (28-33) - these are scene-specific
    // Skip depth parameters (34-36) - these can be adjusted per scene

    for (int i = 0; i < 12; ++i) { // Only elements 0-11 (intrinsic parameters)
        // Handle NaN values - both must be NaN or both must be finite
        if (std::isnan(vec1[i]) && std::isnan(vec2[i])) {
            continue;
        }
        if (std::isnan(vec1[i]) || std::isnan(vec2[i])) {
            return false;
        }

        // Handle infinite values
        if (std::isinf(vec1[i]) && std::isinf(vec2[i])) {
            // Both infinite - check same sign
            if ((vec1[i] > 0) != (vec2[i] > 0)) {
                return false;
            }
            continue;
        }
        if (std::isinf(vec1[i]) || std::isinf(vec2[i])) {
            return false;
        }

        // Compare finite values with tolerance
        if (std::abs(vec1[i] - vec2[i]) > tolerance) {
            return false;
        }
    }

    return true;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUJETRStandaloneDialog::generateJETRComparisonSummary(const QList<QVector<double>> &tiffVectors, const QList<QVector<double>> &lutxVectors) const
{
    if (tiffVectors.isEmpty()) {
        return "TIFF file contains no JETR vectors to compare.";
    }

    if (lutxVectors.isEmpty()) {
        return "LUTX file contains no JETR vectors to compare.";
    }

    QString summary;
    int minCount = std::min(tiffVectors.size(), lutxVectors.size());

    summary += QString("Camera Intrinsic Parameters Comparison (%1 TIFF vs %2 LUTX cameras):\n\n")
               .arg(tiffVectors.size()).arg(lutxVectors.size());

    summary += "Checking camera intrinsics (focal length, principal point, distortion coefficients)\n";
    summary += "to verify LUTX file matches the cameras used in this TIFF file.\n\n";

    if (tiffVectors.size() != lutxVectors.size()) {
        summary += QString("⚠️ Camera count mismatch: TIFF has %1 cameras, LUTX has %2 cameras\n\n")
                   .arg(tiffVectors.size()).arg(lutxVectors.size());
    }

    for (int i = 0; i < minCount; ++i) {
        bool matches = compareJETRVectors(tiffVectors[i], lutxVectors[i]);
        summary += QString("Camera %1: %2\n")
                   .arg(i + 1)
                   .arg(matches ? "✅ Camera intrinsics match" : "❌ Camera intrinsics differ");

        // If they differ, show detailed comparison of intrinsic parameters
        if (!matches) {
            summary += "  Intrinsic parameter differences:\n";
            for (int j = 0; j < 12; ++j) { // Only intrinsic parameters
                double diff = std::abs(tiffVectors[i][j] - lutxVectors[i][j]);
                if (diff > 5e-5) {
                    QString paramName;
                    switch (j) {
                        case 0: paramName = "fx"; break;
                        case 1: paramName = "cx"; break;
                        case 2: paramName = "fy"; break;
                        case 3: paramName = "cy"; break;
                        case 4: paramName = "k1"; break;
                        case 5: paramName = "k2"; break;
                        case 6: paramName = "k3"; break;
                        case 7: paramName = "k4"; break;
                        case 8: paramName = "k5"; break;
                        case 9: paramName = "k6"; break;
                        case 10: paramName = "p1"; break;
                        case 11: paramName = "p2"; break;
                        default: paramName = QString("param%1").arg(j); break;
                    }
                    summary += QString("    %1: TIFF=%.10g, LUTX=%.10g, diff=%.2e\n")
                               .arg(paramName).arg(tiffVectors[i][j]).arg(lutxVectors[i][j]).arg(diff);
                }
            }
        }
    }

    // Report any extra cameras
    if (tiffVectors.size() > lutxVectors.size()) {
        for (int i = lutxVectors.size(); i < tiffVectors.size(); ++i) {
            summary += QString("Camera %1: ⚠️ Only in TIFF\n").arg(i + 1);
        }
    } else if (lutxVectors.size() > tiffVectors.size()) {
        for (int i = tiffVectors.size(); i < lutxVectors.size(); ++i) {
            summary += QString("Camera %1: ⚠️ Only in LUTX\n").arg(i + 1);
        }
    }

    return summary;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUJETRStandaloneDialog::extractDateFromFilename(const QString &filePath) const
{
    if (filePath.isEmpty()) {
        return QString();
    }
    
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.baseName();
    
    // Look for date patterns like "20220703" or "2022-07-03" in filename
    QRegularExpression dateRegex8("(\\d{4})(\\d{2})(\\d{2})");
    QRegularExpression dateRegexDash("(\\d{4})-(\\d{2})-(\\d{2})");
    
    QRegularExpressionMatch match8 = dateRegex8.match(baseName);
    QRegularExpressionMatch matchDash = dateRegexDash.match(baseName);
    
    if (match8.hasMatch()) {
        // Convert YYYYMMDD to YYYY-MM-DD
        QString year = match8.captured(1);
        QString month = match8.captured(2);
        QString day = match8.captured(3);
        
        // Validate the date
        QDate date(year.toInt(), month.toInt(), day.toInt());
        if (date.isValid()) {
            return date.toString("yyyy-MM-dd");
        }
    } else if (matchDash.hasMatch()) {
        // Already in YYYY-MM-DD format
        QString dateStr = matchDash.captured(0);
        QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
        if (date.isValid()) {
            return dateStr;
        }
    }
    
    return QString(); // No valid date found
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRStandaloneDialog::saveBackgroundToInstallFolder(const QList<QVector<double>> &jetrVectors)
{
    // Save the background memory object with complete JETR vectors to the install folder
    // This file will be loaded by LAUProcessVideos as the header for recorded TIF files

    if (!memoryObject.isValid()) {
        qWarning() << "Cannot save background: invalid memory object";
        return false;
    }

    if (jetrVectors.isEmpty()) {
        qWarning() << "Cannot save background: no JETR vectors provided";
        return false;
    }

    // Create a copy of the memory object with the complete JETR vectors
    LAUMemoryObject backgroundWithJetr = memoryObject;

    // Concatenate all JETR vectors into a single QVector
    QVector<double> completeJetr;
    for (const QVector<double> &jetr : jetrVectors) {
        completeJetr.append(jetr);
    }

    // Set the complete JETR data in the memory object
    backgroundWithJetr.setJetr(completeJetr);

    // Determine install/data folder path based on platform
    // Use writable shared directories that don't require admin privileges
    QString installFolderPath;
#ifdef Q_OS_WIN
    // Windows: Use ProgramData (writable by all users without admin)
    installFolderPath = "C:/ProgramData/3DVideoInspectionTools";
#elif defined(Q_OS_MAC)
    // macOS: Use /Users/Shared (writable by all users)
    installFolderPath = "/Users/Shared/3DVideoInspectionTools";
#else
    // Linux: Use /var/lib (standard for application data)
    installFolderPath = "/var/lib/3DVideoInspectionTools";
#endif

    // Ensure the directory exists
    QDir installDir(installFolderPath);
    if (!installDir.exists()) {
        qWarning() << "Install folder does not exist:" << installFolderPath;
        // Try to create it
        if (!installDir.mkpath(".")) {
            qWarning() << "Failed to create install folder:" << installFolderPath;
            return false;
        }
        qDebug() << "Created install folder:" << installFolderPath;
    }

    // Construct the background file path
    QString backgroundFilePath = installDir.absoluteFilePath("background.tif");

    // Save the memory object to the file
    if (!backgroundWithJetr.save(backgroundFilePath)) {
        qWarning() << "Failed to save background file to:" << backgroundFilePath;
        return false;
    }

    qDebug() << "Successfully saved background with" << jetrVectors.count()
             << "complete JETR vectors to:" << backgroundFilePath;

    return true;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRStandaloneDialog::copyLUTXToPublicPictures(const QString &lutxFilePath)
{
    // Copy LUTX file to Public Pictures directory for cloud backup
    // Windows: C:\Users\Public\Pictures
    // macOS: /Users/Shared
    // Linux: /usr/share/pixmaps (or /home/Shared if available)

    if (lutxFilePath.isEmpty()) {
        qWarning() << "Cannot copy LUTX: empty file path";
        return false;
    }

    QFileInfo sourceFileInfo(lutxFilePath);
    if (!sourceFileInfo.exists()) {
        qWarning() << "Cannot copy LUTX: source file does not exist:" << lutxFilePath;
        return false;
    }

    // Determine public pictures directory based on platform
    QString publicPicturesPath;

#ifdef Q_OS_WIN
    publicPicturesPath = "C:/Users/Public/Pictures";
#elif defined(Q_OS_MAC)
    publicPicturesPath = "/Users/Shared";
#else
    // Linux or other Unix-like systems
    publicPicturesPath = QDir::homePath() + "/Pictures";
#endif

    QDir publicDir(publicPicturesPath);
    if (!publicDir.exists()) {
        qWarning() << "Public directory does not exist:" << publicPicturesPath;
        // Try to create it
        if (!publicDir.mkpath(".")) {
            qWarning() << "Failed to create public directory:" << publicPicturesPath;
            return false;
        }
        qDebug() << "Created public directory:" << publicPicturesPath;
    }

    // Construct destination file path
    QString destinationPath = publicDir.absoluteFilePath(sourceFileInfo.fileName());

    // Check if source and destination are the same (normalize paths for comparison)
    QFileInfo destFileInfo(destinationPath);
    if (sourceFileInfo.canonicalFilePath() == destFileInfo.canonicalFilePath()) {
        qDebug() << "LUTX file already in Public Pictures directory - no copy needed:" << destinationPath;
        return true; // Already in the right place, nothing to do
    }

    // If destination file exists, remove it first
    if (QFile::exists(destinationPath)) {
        if (!QFile::remove(destinationPath)) {
            qWarning() << "Failed to remove existing LUTX file:" << destinationPath;
            return false;
        }
        qDebug() << "Removed existing LUTX file:" << destinationPath;
    }

    // Copy the file
    if (!QFile::copy(lutxFilePath, destinationPath)) {
        qWarning() << "Failed to copy LUTX file from" << lutxFilePath << "to" << destinationPath;
        return false;
    }

    qDebug() << "Successfully copied LUTX file to:" << destinationPath;
    return true;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUJETRStandaloneDialog::readCameraPositionsFromConfig() const
{
    // Read camera position mappings from systemConfig.ini
    // Returns a list of positions for LUCID cameras ONLY (typically 2 entries)
    // Filters out Orbbec camera positions by checking serial number format
    // Lucid cameras have 9-digit numeric serial numbers (e.g., "221300900")
    // Orbbec cameras have alphanumeric serial numbers (e.g., "BX7D6410037")
    QStringList positions;

    // Determine install folder path
    // In debug builds, use hardcoded install path
    // In release builds, use directory where executable is located
    QString installFolderPath;
#ifndef NDEBUG
    // Debug mode - use installed tools location
#ifdef Q_OS_WIN
    installFolderPath = "C:/Program Files (x86)/RemoteRecordingTools";
#elif defined(Q_OS_MAC)
    installFolderPath = "/Applications";
#else
    installFolderPath = "/usr/local/bin";
#endif
    qDebug() << "Debug mode - looking for systemConfig.ini at:" << installFolderPath;
#else
    // Release mode - executable directory contains systemConfig.ini
    installFolderPath = QCoreApplication::applicationDirPath();
    qDebug() << "Release mode - looking for systemConfig.ini in executable directory:" << installFolderPath;
#endif

    QString configPath = QDir(installFolderPath).filePath("systemConfig.ini");
    QFile configFile(configPath);

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open systemConfig.ini to read camera positions";
        return positions;
    }

    QTextStream in(&configFile);
    bool inCameraSection = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line == "[CameraPosition]") {
            inCameraSection = true;
            continue;
        }

        if (line.startsWith("[") && line.endsWith("]")) {
            inCameraSection = false;
            continue;
        }

        if (inCameraSection && line.contains("=")) {
            // Format: serialNumber=POSITION (e.g., "221300900=B Side")
            QStringList parts = line.split("=");
            if (parts.size() == 2) {
                QString serialNumber = parts[0].trimmed();
                QString position = parts[1].trimmed(); // Keep original format with prefix

                // Filter: Only include Lucid cameras (9-digit numeric serial numbers)
                // Skip Orbbec cameras (alphanumeric serial numbers like "BX7D6410037")
                if (serialNumber.length() == 9) {
                    bool allDigits = true;
                    for (int i = 0; i < serialNumber.length(); ++i) {
                        if (!serialNumber.at(i).isDigit()) {
                            allDigits = false;
                            break;
                        }
                    }

                    if (allDigits) {
                        // This is a Lucid camera - add its position
                        positions.append(position);
                        qDebug() << "Found Lucid camera position in systemConfig.ini: S/N" << serialNumber << "=" << position;
                    } else {
                        qDebug() << "Skipping non-Lucid camera (non-numeric S/N):" << serialNumber << "=" << position;
                    }
                } else {
                    qDebug() << "Skipping non-Lucid camera (S/N length != 9):" << serialNumber << "=" << position;
                }
            }
        }
    }

    configFile.close();

    qDebug() << "Read" << positions.size() << "Lucid camera positions from systemConfig.ini";

    return positions;
}