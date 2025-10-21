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

#include "laucameraselectiondialog.h"
#include "laujetrwidget.h"
#include "laucamerainventorydialog.h"
#include <QSettings>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <algorithm>
#include <limits>
#include "../Support/laulookuptable.h"
#include "../Support/lauscan.h"
#include "../Support/lauscaninspector.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCameraSelectionDialog::LAUCameraSelectionDialog(const LAUMemoryObject &memoryObject, QWidget *parent)
    : QDialog(parent), memoryObject(memoryObject)
{
    // Calculate number of cameras
    int totalHeight = memoryObject.height();
    int cameraHeight = 480; // Standard camera height for 3D video monitoring
    numCameras = totalHeight / cameraHeight;
    
    if (numCameras <= 1) {
        numCameras = 1;
    }
    
    setupUI();
    populateCameraTabs();
    
    // Restore dialog geometry from QSettings
    QSettings settings;
    settings.beginGroup("DialogGeometry");
    QByteArray geometry = settings.value("LAUCameraSelectionDialog/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    settings.endGroup();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCameraSelectionDialog::~LAUCameraSelectionDialog()
{
    // Save dialog geometry to QSettings
    QSettings settings;
    settings.beginGroup("DialogGeometry");
    settings.setValue("LAUCameraSelectionDialog/geometry", saveGeometry());
    settings.endGroup();
    
    // Qt handles cleanup automatically
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraSelectionDialog::setupUI()
{
    setWindowTitle("Select Make/Model for Each Camera");
    resize(450, 400);  // Use resize instead of setFixedSize for flexible height

    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);  // Reduce spacing between widgets

    // Add JETR vector information if available
    QString infoText;
    if (memoryObject.hasValidJETRVector()) {
        QVector<double> jetrVector = memoryObject.jetr();
        int jetrVectorCount = jetrVector.size() / 37; // Number of complete JETR vectors
        if (jetrVectorCount == 1){
            if (numCameras == 1){
                infoText = QString("Found %1 valid JETR vector for %2 camera.&nbsp; Please select make/model for each camera position.").arg(jetrVectorCount).arg(numCameras);
            } else {
                infoText = QString("Found %1 valid JETR vector for %2 cameras.&nbsp; Please select make/model for each camera position.").arg(jetrVectorCount).arg(numCameras);
            };
        } else {
            if (numCameras == 1){
                infoText = QString("Found %1 valid JETR vectors for %2 camera.&nbsp; Please select make/model for each camera position.").arg(jetrVectorCount).arg(numCameras);
            } else {
                infoText = QString("Found %1 valid JETR vectors for %2 cameras.&nbsp; Please select make/model for each camera position.").arg(jetrVectorCount).arg(numCameras);
            };
        }
    } else {
        if (numCameras == 1){
            infoText = QString("No valid JETR vectors found for %1 camera.&nbsp; Please select make/model for each camera position.").arg(numCameras);
        } else {
            infoText = QString("No valid JETR vectors found for %1 cameras.&nbsp; Please select make/model for each camera position.").arg(numCameras);
        };
    }

    infoLabel = new QLabel(infoText);
    infoLabel->setWordWrap(true);
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setStyleSheet("font-weight: bold; margin: 10px;");
    mainLayout->addWidget(infoLabel);

    // Tab widget for camera panels
    tabWidget = new QTabWidget();
    mainLayout->addWidget(tabWidget);

    // Add spacer to keep tab widget small
    mainLayout->addSpacing(20);

    // Button box
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraSelectionDialog::populateCameraTabs()
{
    // Get available make/model pairs
    QList<QPair<QString, QString>> availablePairs = getAllMakeModelPairs();

    if (availablePairs.isEmpty()) {
        QMessageBox::information(this, "No Camera Data",
                                 "No cached camera configurations found.\n\n"
                                 "Please import LUTX files first to populate the camera database.");
        // Reject the dialog so it doesn't show
        QTimer::singleShot(0, this, &QDialog::reject);
        return;
    }

    // Load camera positions from systemConfig.ini if available
    // In normal mode (not RAW_NIR_VIDEO), cameras are sorted by position name
    // So the first camera in the TIFF should match the first position alphabetically
    QStringList positionsFromIni;
    QString iniPath = QDir::currentPath() + "/systemConfig.ini";
    if (QFile::exists(iniPath)) {
        QSettings settings(iniPath, QSettings::IniFormat);
        settings.beginGroup("CameraPosition");
        QStringList serialNumbers = settings.allKeys();

        // Collect all position values
        for (const QString &serialNumber : serialNumbers) {
            QString position = settings.value(serialNumber).toString();
            if (!position.isEmpty() && position != "unknown") {
                positionsFromIni.append(position);
            }
        }
        settings.endGroup();

        // Sort positions alphabetically (this matches camera sort order in normal mode)
        positionsFromIni.sort();

        qDebug() << "Loaded" << positionsFromIni.count() << "camera positions from systemConfig.ini:" << positionsFromIni;
    } else {
        qDebug() << "No systemConfig.ini found, position combo boxes will default to metadata or Unknown";
    }
    
    for (int i = 0; i < numCameras; ++i) {
        // Create tab widget for each camera
        QWidget *cameraTab = new QWidget();
        QVBoxLayout *tabLayout = new QVBoxLayout(cameraTab);
        
        // Image preview at the top
        QLabel *imageLabel = new QLabel();
        imageLabel->setFixedSize(400, 300);
        imageLabel->setScaledContents(true);
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setStyleSheet("border: 1px solid gray; background-color: #f0f0f0;");
        
        // Extract and display camera image
        QImage cameraImage = extractCameraImage(memoryObject, i);
        if (!cameraImage.isNull()) {
            imageLabel->setPixmap(QPixmap::fromImage(cameraImage));
            originalImages.append(cameraImage);
        } else {
            imageLabel->setText(QString("Camera %1\nImage Failed").arg(i + 1));
            imageLabel->setAlignment(Qt::AlignCenter);
            originalImages.append(QImage()); // Add empty image for consistency
        }
        
        imageLabels.append(imageLabel);
        
        tabLayout->addWidget(imageLabel);
        
        // Form layout for controls below the image
        QFormLayout *formLayout = new QFormLayout();
        formLayout->setHorizontalSpacing(20);
        formLayout->setVerticalSpacing(10);
        
        // Make/Model selection
        QComboBox *makeModelBox = new QComboBox();
        makeModelBox->setMinimumHeight(30);
        for (const auto &pair : availablePairs) {
            QString displayText = QString("%1 - %2").arg(pair.first, pair.second);
            makeModelBox->addItem(displayText, QVariant::fromValue(pair));
        }
        formLayout->addRow("Make/Model:", makeModelBox);
        
        // Camera position selection
        QComboBox *positionBox = new QComboBox();
        positionBox->setMinimumHeight(30);
        positionBox->addItem("Top", "A TOP");
        positionBox->addItem("Side", "B SIDE");
        positionBox->addItem("Bottom", "C BOTTOM");
        positionBox->addItem("Front", "D FRONT");
        positionBox->addItem("Back", "E BACK");
        positionBox->addItem("Quarter", "F QUARTER");
        positionBox->addItem("Rump", "G RUMP");
        positionBox->addItem("Unknown", "H UNKNOWN");
        
        formLayout->addRow("Position:", positionBox);
        
        // Rotation checkbox
        QCheckBox *rotationBox = new QCheckBox("Rotate image by 180 degrees");
        connect(rotationBox, &QCheckBox::toggled, this, &LAUCameraSelectionDialog::onRotationChanged);
        formLayout->addRow("Rotation:", rotationBox);
        
        // 3D Preview button
        QPushButton *previewButton = new QPushButton("Preview 3D");
        previewButton->setMinimumHeight(30);
        previewButton->setEnabled(false); // Initially disabled until make/model is selected
        previewButton->setToolTip("Preview depth data in 3D using selected camera calibration");
        formLayout->addRow("3D Preview:", previewButton);
        
        // Use intelligent guessing to select best make/model for this camera
        QPair<QString, QString> bestGuess = guessBestMakeModel(i, availablePairs);
        if (!bestGuess.first.isEmpty()) {
            // Find and select the best guess in the combo box
            QString displayText = QString("%1 - %2").arg(bestGuess.first, bestGuess.second);
            int bestIndex = makeModelBox->findText(displayText);
            if (bestIndex >= 0) {
                makeModelBox->setCurrentIndex(bestIndex);
            }
            
            // Load metadata from settings for the guessed camera
            loadMetadataFromSettings(bestGuess.first, bestGuess.second, positionBox, rotationBox);
        } else if (!availablePairs.isEmpty()) {
            // Fallback to first available if guessing fails
            const auto &firstPair = availablePairs.first();
            loadMetadataFromSettings(firstPair.first, firstPair.second, positionBox, rotationBox);
        }
        
        // Initialize position from systemConfig.ini if available
        // Cameras in the TIFF are sorted by position name, so positionsFromIni[i] should match camera i
        if (i < positionsFromIni.size() && !positionsFromIni[i].isEmpty()) {
            QString iniPosition = positionsFromIni[i];
            int iniIndex = positionBox->findData(iniPosition);
            if (iniIndex >= 0) {
                positionBox->setCurrentIndex(iniIndex);
                qDebug() << "Camera" << i << "position initialized from INI:" << iniPosition;
            }
        }

        // First camera must always be "Top" and cannot be changed (override any loaded settings)
        if (i == 0) {
            // Verify first camera is "top" - if systemConfig.ini says otherwise, there's a problem
            QString currentPosition = positionBox->currentData().toString();
            if (currentPosition != "top") {
                qDebug() << "WARNING: First camera in systemConfig.ini is not 'top', forcing it to 'top'";
            }
            positionBox->setCurrentText("A Top");
            positionBox->setEnabled(false);
            positionBox->setToolTip("First camera is always the top camera (reference position)");
        } else {
            // For other cameras, if no position was set, default to "Unknown"
            if (positionBox->currentText().isEmpty()) {
                positionBox->setCurrentText("H Unknown");
            }
        }
        
        // Connect make/model changes to enable/disable preview button only
        connect(makeModelBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, makeModelBox, previewButton]() {
                    auto pair = makeModelBox->currentData().value<QPair<QString, QString>>();
                    // Enable preview button when a make/model is selected
                    previewButton->setEnabled(!pair.first.isEmpty() && !pair.second.isEmpty());
                });
        
        // Enable preview button for initial selection
        previewButton->setEnabled(true);
        
        // Connect preview button to handler with camera index
        connect(previewButton, &QPushButton::clicked,
                [this, i]() { onPreview3DClicked(i); });
        
        tabLayout->addLayout(formLayout);
        tabLayout->addStretch();
        
        // Add tab with camera number
        tabWidget->addTab(cameraTab, QString("Camera %1").arg(i + 1));
        
        // Store controls for later retrieval
        makeModelBoxes.append(makeModelBox);
        positionBoxes.append(positionBox);
        rotationBoxes.append(rotationBox);
        previewButtons.append(previewButton);
    }
    
    // Ensure all images are displayed with correct rotation after UI setup
    for (int i = 0; i < numCameras; ++i) {
        updateImageRotation(i);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QPair<QString, QString>> LAUCameraSelectionDialog::getMakeModelPairs() const
{
    QList<QPair<QString, QString>> results;
    for (QComboBox *comboBox : makeModelBoxes) {
        QPair<QString, QString> pair = comboBox->currentData().value<QPair<QString, QString>>();
        results.append(pair);
    }
    return results;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUCameraSelectionDialog::getPositions() const
{
    QStringList results;
    for (int i = 0; i < positionBoxes.size(); ++i) {
        QComboBox *comboBox = positionBoxes[i];
        QString position = comboBox->currentData().toString();
        results.append(position);
    }
    return results;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<bool> LAUCameraSelectionDialog::getRotations() const
{
    QList<bool> results;
    for (QCheckBox *checkBox : rotationBoxes) {
        results.append(checkBox->isChecked());
    }
    return results;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QImage LAUCameraSelectionDialog::extractCameraImage(const LAUMemoryObject &memoryObject, int cameraIndex)
{
    // Use the same logic as LAUJETRWidget::extractCameraImage
    if (!memoryObject.isValid()) {
        return QImage();
    }
    
    // For 3D video monitoring systems, cameras are typically stacked vertically
    int cameraHeight = 480;
    int cameraWidth = 640;
    
    // Calculate the starting row for this camera
    int startRow = cameraIndex * cameraHeight;
    int endRow = startRow + cameraHeight;
    
    // Ensure we don't go beyond the image bounds
    if (endRow > memoryObject.height()) {
        return QImage();
    }
    
    // Extract the camera's portion of the image
    QImage fullImage = LAUJETRWidget::memoryObjectToImage(memoryObject);
    if (fullImage.isNull()) {
        return QImage();
    }
    
    // Crop to the camera's region
    QRect cameraRect(0, startRow, cameraWidth, cameraHeight);
    QImage cameraImage = fullImage.copy(cameraRect);
    
    return cameraImage;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraSelectionDialog::onRotationChanged()
{
    // Find which checkbox was toggled
    QCheckBox *senderCheckBox = qobject_cast<QCheckBox*>(sender());
    if (!senderCheckBox) {
        return;
    }
    
    // Find the index of this checkbox and update the image rotation
    int cameraIndex = rotationBoxes.indexOf(senderCheckBox);
    if (cameraIndex >= 0) {
        updateImageRotation(cameraIndex);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QPair<QString, QString>> LAUCameraSelectionDialog::getAllMakeModelPairs()
{
    // Use the same logic as LAUJETRWidget::getAllMakeModelPairs
    QList<QPair<QString, QString>> pairs;
    
    QSettings settings;
    settings.beginGroup("CameraParams");
    QStringList keys = settings.childKeys();
    
    for (const QString &key : keys) {
        // Parse make and model from key (format: Make_Model)
        QString make, model;
        int underscorePos = key.indexOf('_');
        if (underscorePos > 0) {
            make = key.left(underscorePos);
            model = key.mid(underscorePos + 1);
        } else {
            make = key;
            model = "";
        }
        
        pairs.append(QPair<QString, QString>(make, model));
    }
    
    settings.endGroup();
    
    // Sort by make, then by model
    std::sort(pairs.begin(), pairs.end(), [](const QPair<QString, QString> &a, const QPair<QString, QString> &b) {
        if (a.first != b.first) {
            return a.first < b.first;
        }
        return a.second < b.second;
    });
    
    return pairs;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraSelectionDialog::loadMetadataFromSettings(const QString &make, const QString &model, QComboBox *positionBox, QCheckBox *rotationBox)
{
    if (make.isEmpty() || model.isEmpty() || !positionBox || !rotationBox) {
        return;
    }
    
    // Use same key format as LAUJETRWidget
    QString makeModelKey = QString("%1_%2").arg(make, model);
    
    QSettings settings;
    settings.beginGroup("CameraMetadata");
    
    // Load position
    QString position = settings.value(makeModelKey + "_position", "unknown").toString();
    int index = positionBox->findData(position);
    
    if (index >= 0) {
        positionBox->setCurrentIndex(index);
    } else {
        // Find the "Unknown" item by data value and set it properly
        int unknownIndex = positionBox->findData("H UNKNOWN");
        if (unknownIndex >= 0) {
            positionBox->setCurrentIndex(unknownIndex);
        }
    }
    
    // Load rotation
    bool rotation = settings.value(makeModelKey + "_rotation", false).toBool();
    rotationBox->setChecked(rotation);
    
    settings.endGroup();
    
    // Update image display to reflect rotation state
    // Find which camera index this rotationBox belongs to
    int cameraIndex = rotationBoxes.indexOf(rotationBox);
    if (cameraIndex >= 0) {
        updateImageRotation(cameraIndex);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraSelectionDialog::updateImageRotation(int cameraIndex)
{
    // Validate camera index
    if (cameraIndex < 0 || cameraIndex >= rotationBoxes.size() || 
        cameraIndex >= imageLabels.size() || cameraIndex >= originalImages.size()) {
        return;
    }
    
    // Get rotation checkbox and original image
    QCheckBox *rotationBox = rotationBoxes[cameraIndex];
    QImage originalImage = originalImages[cameraIndex];
    
    if (!rotationBox || originalImage.isNull()) {
        return;
    }
    
    // Apply rotation if checkbox is checked
    QImage displayImage = originalImage;
    if (rotationBox->isChecked()) {
        // Rotate 180 degrees
        QTransform transform;
        transform.rotate(180);
        displayImage = originalImage.transformed(transform);
    }
    
    // Update the image label
    imageLabels[cameraIndex]->setPixmap(QPixmap::fromImage(displayImage));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraSelectionDialog::onPreview3DClicked(int cameraIndex)
{
    // Validate camera index
    if (cameraIndex < 0 || cameraIndex >= makeModelBoxes.size()) {
        QMessageBox::warning(this, "Preview 3D Error", "Invalid camera index.");
        return;
    }
    
    // Get the selected make/model from the current camera's combo box
    QPair<QString, QString> makeModelPair = makeModelBoxes[cameraIndex]->currentData().value<QPair<QString, QString>>();
    QString make = makeModelPair.first;
    QString model = makeModelPair.second;
    
    if (make.isEmpty() || model.isEmpty()) {
        QMessageBox::warning(this, "Preview 3D Error", "Please select a camera make/model first.");
        return;
    }
    
    // Load cached JETR vector for the selected make/model
    QString makeModelKey = QString("%1_%2").arg(make, model);
    QSettings settings;
    settings.beginGroup("CameraParams");
    QVariantList jetrVariantList = settings.value(makeModelKey).toList();
    settings.endGroup();
    
    if (jetrVariantList.size() != 37) {
        QMessageBox::warning(this, "Preview 3D Error", 
            QString("No cached calibration found for %1 - %2.\n\n"
            "Please import LUTX files first to populate the camera database.")
            .arg(make, model));
        return;
    }
    
    // Convert QVariantList to QVector<double>
    QVector<double> previewJetr;
    for (const QVariant &variant : jetrVariantList) {
        previewJetr.append(variant.toDouble());
    }
    
    // Create preview JETR vector with:
    // - Keep intrinsic parameters (elements 0-11, 34-36) from cached JETR
    // - Set identity transformation matrix (elements 12-27)
    // - Set infinite bounding box (elements 28-33)
    
    // Set identity transformation matrix [12-27]
    previewJetr[12] = 1.0; previewJetr[13] = 0.0; previewJetr[14] = 0.0; previewJetr[15] = 0.0;  // Row 1
    previewJetr[16] = 0.0; previewJetr[17] = 1.0; previewJetr[18] = 0.0; previewJetr[19] = 0.0;  // Row 2
    previewJetr[20] = 0.0; previewJetr[21] = 0.0; previewJetr[22] = 1.0; previewJetr[23] = 0.0;  // Row 3
    previewJetr[24] = 0.0; previewJetr[25] = 0.0; previewJetr[26] = 0.0; previewJetr[27] = 1.0;  // Row 4
    
    // Set infinite bounding box [28-33]
    previewJetr[28] = -std::numeric_limits<double>::infinity(); // xMin
    previewJetr[29] = +std::numeric_limits<double>::infinity(); // xMax
    previewJetr[30] = -std::numeric_limits<double>::infinity(); // yMin
    previewJetr[31] = +std::numeric_limits<double>::infinity(); // yMax
    previewJetr[32] = -std::numeric_limits<double>::infinity(); // zMin
    previewJetr[33] = +std::numeric_limits<double>::infinity(); // zMax
    
    // Check if LUT is already cached
    qDebug() << "Preview 3D: Checking cached LUT for" << make << model << "640x480";
    if (!LAUCameraInventoryDialog::hasLUTInCache(make, model, 640, 480)) {
        // LUT not cached - need to generate it immediately
        qDebug() << "Preview 3D: LUT not cached, pausing background generation";
        
        // Pause background LUT generation
        LAUCameraInventoryDialog::pauseBackgroundLUTGeneration();
        
        // Generate LUT immediately with progress dialog
        qDebug() << "Preview 3D: Generating LUT immediately";
        // Use invalid date for preview mode - this will preserve old behavior (always rotate Femto Mega)
        LAULookUpTable lookupTable = LAULookUpTable::generateTableFromJETR(640, 480, previewJetr, make, model, QDate(), this);
        
        if (!lookupTable.isValid()) {
            // Resume background generation even if generation failed
            LAUCameraInventoryDialog::resumeBackgroundLUTGeneration();
            QMessageBox::warning(this, "Preview 3D Error", 
                "Failed to generate lookup table from JETR vector.\n\n"
                "The cached calibration data may be invalid.");
            return;
        }
        
        // Add to cache so background doesn't regenerate it
        qDebug() << "Preview 3D: Adding LUT to cache";
        LAUCameraInventoryDialog::addLUTToCache(make, model, 640, 480, lookupTable);
        
        // Resume background generation
        LAUCameraInventoryDialog::resumeBackgroundLUTGeneration();
    }
    
    // Get the LUT from cache (now guaranteed to exist)
    LAULookUpTable lookupTable = LAUCameraInventoryDialog::getCachedLUT(make, model, 640, 480);
    
    // Extract camera's depth data
    LAUMemoryObject cameraMemoryObject = extractCameraMemoryObject(memoryObject, cameraIndex);
    if (!cameraMemoryObject.isValid()) {
        QMessageBox::warning(this, "Preview 3D Error", 
            QString("Failed to extract depth data for Camera %1.\n\n"
            "The camera region may be outside the image bounds.")
            .arg(cameraIndex + 1));
        return;
    }
    
    // Create LAUScan from depth data and lookup table
    LAUScan scan = LAUScan::fromRawDepth(cameraMemoryObject, lookupTable);
    scan.setMake(make);
    scan.setModel(model);
    scan.setParentName(QString("Preview_Camera_%1").arg(cameraIndex + 1));
    scan.updateLimits();

    if (!scan.isValid()) {
        QMessageBox::warning(this, "Preview 3D Error", 
            "Failed to create 3D scan from depth data.\n\n"
            "The depth data may be invalid or incompatible with the selected camera calibration.");
        return;
    }
    
    // Launch LAUScanInspector dialog with the generated scan
    scan.inspectImage();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraSelectionDialog::setMakeModel(int cameraIndex, const QString &make, const QString &model)
{
    if (cameraIndex < 0 || cameraIndex >= makeModelBoxes.size()) {
        return;
    }
    
    QComboBox *makeModelBox = makeModelBoxes[cameraIndex];
    if (!makeModelBox) {
        return;
    }
    
    // Find the item that matches this make/model pair
    QString targetText = QString("%1 - %2").arg(make, model);
    for (int i = 0; i < makeModelBox->count(); ++i) {
        if (makeModelBox->itemText(i) == targetText) {
            makeModelBox->setCurrentIndex(i);
            break;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraSelectionDialog::setPosition(int cameraIndex, const QString &position)
{
    if (cameraIndex < 0 || cameraIndex >= positionBoxes.size()) {
        return;
    }
    
    QComboBox *positionBox = positionBoxes[cameraIndex];
    if (!positionBox) {
        return;
    }
    
    int index = positionBox->findData(position);
    if (index >= 0) {
        positionBox->setCurrentIndex(index);
    } else {
        // Default to unknown if position not found
        int unknownIndex = positionBox->findData("H UNKNOWN");
        if (unknownIndex >= 0) {
            positionBox->setCurrentIndex(unknownIndex);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraSelectionDialog::setRotation(int cameraIndex, bool rotate180)
{
    if (cameraIndex < 0 || cameraIndex >= rotationBoxes.size()) {
        return;
    }
    
    QCheckBox *rotationBox = rotationBoxes[cameraIndex];
    if (!rotationBox) {
        return;
    }
    
    rotationBox->setChecked(rotate180);
    // Trigger image update
    updateImageRotation(cameraIndex);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QPair<QString, QString> LAUCameraSelectionDialog::getMakeModel(int cameraIndex) const
{
    if (cameraIndex < 0 || cameraIndex >= makeModelBoxes.size()) {
        return QPair<QString, QString>();
    }
    
    QComboBox *makeModelBox = makeModelBoxes[cameraIndex];
    if (!makeModelBox) {
        return QPair<QString, QString>();
    }
    
    return makeModelBox->currentData().value<QPair<QString, QString>>();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUCameraSelectionDialog::getPosition(int cameraIndex) const
{
    if (cameraIndex < 0 || cameraIndex >= positionBoxes.size()) {
        return "unknown";
    }
    
    QComboBox *positionBox = positionBoxes[cameraIndex];
    if (!positionBox) {
        return "unknown";
    }
    
    return positionBox->currentData().toString();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUCameraSelectionDialog::getRotation(int cameraIndex) const
{
    if (cameraIndex < 0 || cameraIndex >= rotationBoxes.size()) {
        return false;
    }
    
    QCheckBox *rotationBox = rotationBoxes[cameraIndex];
    if (!rotationBox) {
        return false;
    }
    
    return rotationBox->isChecked();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUCameraSelectionDialog::extractCameraMemoryObject(const LAUMemoryObject &memoryObject, int cameraIndex)
{
    // Validate input parameters
    if (!memoryObject.isValid()) {
        return LAUMemoryObject();
    }
    
    if (cameraIndex < 0 || cameraIndex >= numCameras) {
        return LAUMemoryObject();
    }
    
    // Calculate camera region parameters
    int cameraWidth = 640;
    int cameraHeight = 480;
    int startRow = cameraIndex * cameraHeight;
    int endRow = startRow + cameraHeight;
    
    if (endRow > memoryObject.height()) {
        return LAUMemoryObject();
    }
    
    // Create new LAUMemoryObject for single camera
    LAUMemoryObject cameraObject(cameraWidth, cameraHeight, memoryObject.colors(), memoryObject.depth());
    if (!cameraObject.isValid()) {
        return LAUMemoryObject();
    }
    
    // Copy pixel data from source to destination
    const unsigned char *srcPtr = (const unsigned char*)memoryObject.constPointer();
    unsigned char *dstPtr = (unsigned char*)cameraObject.pointer();
    
    int srcBytesPerPixel = memoryObject.depth();
    int dstBytesPerPixel = cameraObject.depth();
    int srcRowBytes = memoryObject.width() * srcBytesPerPixel;
    int dstRowBytes = cameraWidth * dstBytesPerPixel;
    
    // Copy row by row from the camera's region
    for (int row = 0; row < cameraHeight; ++row) {
        const unsigned char *srcRowPtr = srcPtr + ((startRow + row) * srcRowBytes);
        unsigned char *dstRowPtr = dstPtr + (row * dstRowBytes);
        memcpy(dstRowPtr, srcRowPtr, dstRowBytes);
    }
    
    // Preserve metadata if present
    if (!memoryObject.xml().isEmpty()) {
        cameraObject.setXML(memoryObject.xml());
    }
    
    return cameraObject;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUCameraSelectionDialog::validatePositions()
{
    QStringList positions;
    QStringList duplicatePositions;
    QStringList unknownCameras;
    
    // Collect all position values
    for (int i = 0; i < positionBoxes.size(); ++i) {
        if (positionBoxes[i]) {
            QString position = positionBoxes[i]->currentData().toString();
            
            // Check for "unknown" positions (not allowed)
            if (position == "unknown") {
                unknownCameras.append(QString("Camera %1").arg(i + 1));
            } else {
                // Check for duplicates among defined positions
                if (positions.contains(position)) {
                    if (!duplicatePositions.contains(position)) {
                        duplicatePositions.append(position);
                    }
                } else {
                    positions.append(position);
                }
            }
        }
    }
    
    // Check for unknown positions first
    if (!unknownCameras.isEmpty()) {
        QString unknownList = unknownCameras.join(", ");
        QMessageBox::warning(this, "Undefined Camera Positions",
                           QString("The following cameras have undefined positions: %1\n\n"
                                 "Every camera must have a specific position assigned. "
                                 "Please select a position (Top, Side, Bottom, Front, or Back) "
                                 "for each camera before continuing.")
                           .arg(unknownList));
        return false;
    }
    
    // Then check for duplicates
    if (!duplicatePositions.isEmpty()) {
        QString duplicateList = duplicatePositions.join(", ");
        QMessageBox::warning(this, "Duplicate Camera Positions",
                           QString("The following camera positions are used by multiple cameras: %1\n\n"
                                 "Each camera must have a unique position. Please change the duplicate "
                                 "positions before continuing.")
                           .arg(duplicateList));
        return false;
    }
    
    return true;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraSelectionDialog::accept()
{
    // Validate positions before accepting
    if (validatePositions()) {
        // Pre-generate LUTs for selected cameras to speed up JETR dialog opening
        int cameraHeight = memoryObject.height() / numCameras;
        for (int i = 0; i < numCameras; ++i) {
            if (i < makeModelBoxes.size()) {
                QPair<QString, QString> makeModelPair = makeModelBoxes[i]->currentData().value<QPair<QString, QString>>();
                QString make = makeModelPair.first;
                QString model = makeModelPair.second;
                
                if (!make.isEmpty() && !model.isEmpty()) {
                    // Request priority LUT generation for this camera
                    LAUCameraInventoryDialog::getCachedLUTWithPriority(
                        make, model, memoryObject.width(), cameraHeight, this);
                    qDebug() << "Pre-generating LUT for:" << make << model 
                            << QString("%1x%2").arg(memoryObject.width()).arg(cameraHeight);
                }
            }
        }
        
        QDialog::accept();
    }
    // If validation fails, dialog stays open for user to fix the issues
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QPair<QString, QString> LAUCameraSelectionDialog::guessBestMakeModel(int cameraIndex, const QList<QPair<QString, QString>> &availablePairs)
{
    // Get JETR vectors from memory object
    QVector<double> memoryJetr = memoryObject.jetr();
    if (memoryJetr.isEmpty()) {
        // No JETR data available, return first available pair
        return availablePairs.isEmpty() ? QPair<QString, QString>() : availablePairs.first();
    }
    
    // Calculate how many values per camera (should be 37)
    int valuesPerCamera = memoryJetr.size() / numCameras;
    if (valuesPerCamera != 37) {
        qDebug() << "Warning: Expected 37 JETR values per camera, got" << valuesPerCamera;
    }
    
    // Extract JETR vector for this specific camera
    int startIndex = cameraIndex * valuesPerCamera;
    int endIndex = std::min(static_cast<qsizetype>(startIndex + valuesPerCamera), memoryJetr.size());
    
    if (startIndex >= memoryJetr.size()) {
        // Camera index out of range, return first available
        return availablePairs.isEmpty() ? QPair<QString, QString>() : availablePairs.first();
    }
    
    QVector<double> cameraJetr;
    for (int i = startIndex; i < endIndex; ++i) {
        cameraJetr.append(memoryJetr[i]);
    }
    
    // Compare with all available make/model pairs
    QPair<QString, QString> bestMatch;
    double bestScore = std::numeric_limits<double>::max();
    
    qDebug() << QString("Camera %1: Comparing JETR vector (%2 values) with inventory:")
                .arg(cameraIndex + 1).arg(cameraJetr.size());
    
    int validComparisons = 0;
    for (const auto &pair : availablePairs) {
        QString make = pair.first;
        QString model = pair.second;
        
        // Get cached JETR vector for this make/model from camera inventory
        LAUCameraCalibration calibration = LAUCameraInventoryDialog::getCameraCalibration(make, model);
        if (calibration.isValid()) {
            QVector<double> inventoryJetr = calibration.jetrVector;
            
            qDebug() << QString("  %1 - %2: inventory has %3 values")
                        .arg(make).arg(model).arg(inventoryJetr.size());
            
            // Compare vectors
            double score = compareJETRVectors(cameraJetr, inventoryJetr);
            qDebug() << QString("    Score: %1").arg(score);
            
            if (score < bestScore) {
                bestScore = score;
                bestMatch = pair;
            }
            validComparisons++;
        } else {
            qDebug() << QString("  %1 - %2: NO VALID CALIBRATION").arg(make).arg(model);
        }
    }
    
    qDebug() << QString("Camera %1: %2 valid comparisons, best score: %3")
                .arg(cameraIndex + 1).arg(validComparisons).arg(bestScore);
    
    // If no valid match found, return first available
    if (bestMatch.first.isEmpty() && !availablePairs.isEmpty()) {
        bestMatch = availablePairs.first();
    }
    
    qDebug() << QString("Camera %1: Best guess %2 - %3 (score: %4)")
                .arg(cameraIndex + 1)
                .arg(bestMatch.first)
                .arg(bestMatch.second)
                .arg(bestScore);
    
    return bestMatch;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
double LAUCameraSelectionDialog::compareJETRVectors(const QVector<double> &vector1, const QVector<double> &vector2)
{
    if (vector1.size() < 6 || vector2.size() < 6) {
        return std::numeric_limits<double>::max(); // Need at least 6 core intrinsic parameters
    }
    
    // Only compare core intrinsic parameters (indices 0-5)
    // 0: fx - Focal length in x direction
    // 1: cx - Principal point x coordinate  
    // 2: fy - Focal length in y direction
    // 3: cy - Principal point y coordinate
    // 4: k1 - Primary radial distortion coefficient
    // 5: k2 - Secondary radial distortion coefficient
    
    double sumSquaredDiff = 0.0;
    int validCount = 0;
    
    for (int i = 0; i < 6; ++i) {  // Only compare first 6 core parameters
        double val1 = vector1[i];
        double val2 = vector2[i];
        
        // Skip infinite or NaN values
        if (std::isfinite(val1) && std::isfinite(val2)) {
            double diff = val1 - val2;
            sumSquaredDiff += diff * diff;
            validCount++;
        }
    }
    
    if (validCount == 0) {
        return std::numeric_limits<double>::max();
    }
    
    // Return root mean squared error for core intrinsic parameters only
    return std::sqrt(sumSquaredDiff / validCount);
}
