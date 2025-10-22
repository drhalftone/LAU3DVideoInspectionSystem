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

#include "laujetrdialog.h"
#include "laulookuptable.h"
#include "laucamerainventorydialog.h"
#include "laucameraselectiondialog.h"
#include "lautiffviewerdialog.h"
#include <QFileInfo>
#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QStringList>
#include <QDirIterator>
#include <QRegularExpression>
#include <QProcess>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QTimer>
#include <QShowEvent>
#include <QDebug>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUJETRDialog::LAUJETRDialog(QWidget *parent)
    : QDialog(parent), showLoadButton(true), importCancelled(false), hasUnsavedChanges(false)
{
    setupUI();
    addJETRTab(LAUJETRWidget::createDefaultJETR(), "Default");

    // Store original state
    originalJETRVectors = getJETRVectors();

    // Restore dialog geometry from QSettings
    QSettings settings;
    settings.beginGroup("DialogGeometry");
    QByteArray geometry = settings.value("LAUJETRDialog/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    settings.endGroup();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUJETRDialog::LAUJETRDialog(const QString &filename, QWidget *parent)
    : QDialog(parent), showLoadButton(false), importCancelled(false), hasUnsavedChanges(false)
{
    setupUI();
    
    // Restore dialog geometry from QSettings
    QSettings settings;
    settings.beginGroup("DialogGeometry");
    QByteArray geometry = settings.value("LAUJETRDialog/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    settings.endGroup();
    
    // Import the file immediately during construction
    if (!filename.isEmpty()) {
        try {
            FileType fileType = detectFileType(filename);
            
            switch (fileType) {
            case FileTypeMemoryObject:
                importMemoryObject(filename);
                break;
            case FileTypeLookUpTable:
                importLookUpTable(filename);
                break;
            case FileTypeLUTX:
                importLUTX(filename);
                break;
            default:
                // If file type unknown, show default tab with load button
                showLoadButton = true;
                addJETRTab(LAUJETRWidget::createDefaultJETR(), "Default");
                QMessageBox::warning(this, "Unknown File Type",
                                   QString("Could not determine file type for: %1").arg(filename));
                break;
            }
        } catch (const std::exception &e) {
            // If import fails, show default tab with load button
            showLoadButton = true;
            addJETRTab(LAUJETRWidget::createDefaultJETR(), "Default");
            QMessageBox::critical(this, "Import Error",
                                QString("Failed to import file: %1\n\nError: %2")
                                .arg(filename).arg(e.what()));
        }
        
        // If import was cancelled during construction, set cancelled flag
        // The preflight method should have already handled this case
    } else {
        // Empty filename, show default behavior with load button
        showLoadButton = true;
        addJETRTab(LAUJETRWidget::createDefaultJETR(), "Default");
    }
    
    // Store original state after import
    originalJETRVectors = getJETRVectors();
    hasUnsavedChanges = false; // Just loaded, no changes yet

    // Update button visibility based on whether we loaded a file
    updateButtonVisibility();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUJETRDialog::~LAUJETRDialog()
{
    // Save dialog geometry to QSettings
    QSettings settings;
    settings.beginGroup("DialogGeometry");
    settings.setValue("LAUJETRDialog/geometry", saveGeometry());
    settings.endGroup();
    
    // Qt handles cleanup automatically
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::checkAutoImport()
{
    // Check if we should auto-import a file
    QVariant autoImportFile = property("autoImportFile");
    if (autoImportFile.isValid() && !autoImportFile.toString().isEmpty()) {
        // Use a single-shot timer to import after dialog is shown
        QTimer::singleShot(100, [this, autoImportFile]() {
            importMemoryObject(autoImportFile.toString());
        });
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRDialog::importLUTsDirectly()
{
    // Get last used directory from QSettings
    QSettings settings;
    QString lastDir = settings.value("LastImportDirectory", QDir::homePath()).toString();

    QString filename = QFileDialog::getOpenFileName(
        this,
        "Import LUT/LUTX Files",
        lastDir,
        "LUT Files (*.lut *.lutx);;LUT Files (*.lut);;LUTX Files (*.lutx);;All Files (*)"
        );

    if (filename.isEmpty()) {
        return false;
    }

    // Save the directory for next time
    QFileInfo fileInfo(filename);
    settings.setValue("LastImportDirectory", fileInfo.absolutePath());

    // Clear existing tabs before import
    clearTabs();

    // Detect file type and import accordingly
    FileType fileType = detectFileType(filename);

    try {
        switch (fileType) {
        case FileTypeLookUpTable:
            importLookUpTable(filename);
            return true;
        case FileTypeLUTX:
            importLUTX(filename);
            return true;
        default:
            QMessageBox::warning(this, "Invalid File Type",
                               "Please select a valid LUT or LUTX file.");
            return false;
        }
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Import Error",
                            QString("Failed to import file: %1").arg(e.what()));
        return false;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::setDisplayMode(bool displayMode)
{
    if (displayMode) {
        // Hide the Load button
        if (importButton) {
            importButton->setVisible(false);
        }

        // Hide Discard button in display mode (read-only, so only OK to close)
        if (rejectButton) {
            rejectButton->setVisible(false);
        }

        // Change Import button to OK in display mode
        if (acceptButton) {
            acceptButton->setText("OK");
            acceptButton->setToolTip("Close the calibration display");
        }

        // Make all JETR widgets read-only
        for (LAUJETRWidget *widget : jetrWidgets) {
            widget->setReadOnly(true);
        }

        // Update info label to indicate display mode
        if (infoLabel) {
            infoLabel->setText("Displaying cached camera calibration data (read-only).");
        }
    } else {
        // Show the Load button
        if (importButton) {
            importButton->setVisible(true);
        }

        // Show Discard button in edit mode
        if (rejectButton) {
            rejectButton->setVisible(true);
        }

        // Restore Import button text in edit mode
        if (acceptButton) {
            acceptButton->setText("Import");
            acceptButton->setToolTip("Import and save the camera calibration parameters");
        }

        // Make all JETR widgets editable
        for (LAUJETRWidget *widget : jetrWidgets) {
            widget->setReadOnly(false);
        }

        // Restore default info label
        if (infoLabel) {
            infoLabel->setText("Import memory objects, lookup tables, or LUTX files to edit JETR parameters.");
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::setPreloadedMode(bool preloadedMode)
{
    if (preloadedMode) {
        // Hide the Load button (file already loaded)
        if (importButton) {
            importButton->setVisible(false);
        }
        
        // Keep all JETR widgets editable (unlike display mode)
        for (LAUJETRWidget *widget : jetrWidgets) {
            widget->setReadOnly(false);
        }
        
        // Update info label to indicate preloaded mode
        if (infoLabel) {
            infoLabel->setText("Editing calibration data loaded from TIFF file.");
        }
    } else {
        // Show the Load button
        if (importButton) {
            importButton->setVisible(true);
        }
        
        // Keep all JETR widgets editable
        for (LAUJETRWidget *widget : jetrWidgets) {
            widget->setReadOnly(false);
        }
        
        // Restore default info label
        if (infoLabel) {
            infoLabel->setText("Import memory objects, lookup tables, or LUTX files to edit JETR parameters.");
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::setupUI()
{
    setWindowTitle("JETR Vector Editor");
    resize(800, 900);  // Initial size, but allow user to resize smaller

    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);  // Reduce spacing between widgets

    // Info label
    infoLabel = new QLabel("Import memory objects, lookup tables, or LUTX files to edit JETR parameters.");
    infoLabel->setStyleSheet("color: #666; margin: 5px;");
    mainLayout->addWidget(infoLabel);

    // Tab widget for multiple JETR vectors
    tabWidget = new QTabWidget();
    tabWidget->setTabsClosable(false); // Don't allow closing tabs for now
    tabWidget->setToolTip("Each tab represents one camera's calibration parameters");
    mainLayout->addWidget(tabWidget);

    // Add stretch to push buttons to bottom
    mainLayout->addStretch();

    // Button box with custom buttons
    buttonBox = new QDialogButtonBox();

    // Add Load button (for TIFF files only)
    importButton = new QPushButton("Load");
    importButton->setToolTip("Load TIFF memory object file");
    buttonBox->addButton(importButton, QDialogButtonBox::ActionRole);

    // Add Import button (replaces OK)
    acceptButton = new QPushButton("Import");
    acceptButton->setToolTip("Import and save the camera calibration parameters");
    buttonBox->addButton(acceptButton, QDialogButtonBox::AcceptRole);

    // Add Discard button (replaces Cancel)
    rejectButton = new QPushButton("Discard");
    rejectButton->setToolTip("Discard and close without importing");
    buttonBox->addButton(rejectButton, QDialogButtonBox::RejectRole);

    // Note: History-related buttons have been moved to main window
    // Note: Export and Inventory buttons have been moved to main window Tools menu

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(importButton, &QPushButton::clicked, this, &LAUJETRDialog::onImportClicked);

    mainLayout->addWidget(buttonBox);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    // Fix the dialog size after it's been shown
    setFixedSize(this->size());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::updateButtonVisibility()
{
    // Show/hide the Load button based on whether we loaded a file
    if (importButton) {
        importButton->setVisible(showLoadButton);
    }

    // Update info label based on whether we loaded a file
    if (infoLabel) {
        if (showLoadButton) {
            infoLabel->setText("Import memory objects, lookup tables, or LUTX files to edit JETR parameters.");
        } else {
            infoLabel->setText("Imported file loaded. Edit parameters and click OK to save changes.");
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::clearTabs()
{
    while (tabWidget->count() > 0) {
        QWidget *widget = tabWidget->widget(0);
        tabWidget->removeTab(0);
        delete widget;
    }
    jetrWidgets.clear();
    currentMakes.clear();
    currentModels.clear();
    currentPositions.clear();
    currentRotations.clear();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::addJETRTab(const QVector<double> &jetrVector, const QString &tabTitle)
{
    LAUJETRWidget *widget = new LAUJETRWidget(jetrVector);


    // Connect to receive updates
    connect(widget, &LAUJETRWidget::jetrVectorChanged,
            this, &LAUJETRDialog::onJETRVectorChanged);
    connect(widget, &LAUJETRWidget::requestBoundingBoxEdit,
            this, &LAUJETRDialog::onEditBoundingBox);

    QString title = tabTitle;
    if (title.isEmpty()) {
        title = QString("Camera %1").arg(tabWidget->count() + 1);
    }

    tabWidget->addTab(widget, title);
    jetrWidgets.append(widget);

    // First camera is always "Top" and read-only
    if (jetrWidgets.size() == 1) {
        widget->setCameraPosition("top");
        widget->setCameraPositionReadOnly(true);
    }

    // Track current metadata
    currentMakes.append(widget->getCameraMake());
    currentModels.append(widget->getCameraModel());
    currentPositions.append(widget->getCameraPosition());
    currentRotations.append(widget->getCameraRotation());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::addJETRTab(const QVector<double> &jetrVector, const QString &make, const QString &model, const QString &tabTitle)
{
    LAUJETRWidget *widget = new LAUJETRWidget(jetrVector);

    // Set make and model in the widget
    widget->setCameraMake(make);
    widget->setCameraModel(model);


    // Connect to receive updates
    connect(widget, &LAUJETRWidget::jetrVectorChanged,
            this, &LAUJETRDialog::onJETRVectorChanged);
    connect(widget, &LAUJETRWidget::requestBoundingBoxEdit,
            this, &LAUJETRDialog::onEditBoundingBox);

    QString title = tabTitle;
    if (title.isEmpty()) {
        title = QString("%1 - %2").arg(make, model);
    }

    tabWidget->addTab(widget, title);
    jetrWidgets.append(widget);

    // First camera is always "Top" and read-only
    if (jetrWidgets.size() == 1) {
        widget->setCameraPosition("top");
        widget->setCameraPositionReadOnly(true);
    }

    // Track current metadata
    currentMakes.append(widget->getCameraMake());
    currentModels.append(widget->getCameraModel());
    currentPositions.append(widget->getCameraPosition());
    currentRotations.append(widget->getCameraRotation());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::setTabMetadata(const QStringList &makes, const QStringList &models, const QStringList &positions, const QList<bool> &rotations)
{
    // Set metadata for existing tabs
    for (int i = 0; i < jetrWidgets.size(); ++i) {
        LAUJETRWidget *widget = jetrWidgets[i];
        if (!widget) continue;
        
        // Set make and model if available
        if (i < makes.size()) {
            widget->setCameraMake(makes[i]);
        }
        if (i < models.size()) {
            widget->setCameraModel(models[i]);
        }
        
        // Set rotation if available
        if (i < rotations.size()) {
            widget->setCameraRotation(rotations[i]);
        }

        // Server-only feature removed for standalone build
        // if (!dateFolder.isEmpty()) {
        //     QDate folderDate = LAULookUpTable::parseFolderDate(dateFolder);
        //     widget->setCurrentDate(folderDate);
        // }

        // Set position if available (this triggers LUT generation, so date must be set first)
        if (i < positions.size()) {
            widget->setCameraPosition(positions[i]);
        }
        
        // Update tab title to include make/model if both are available
        if (i < makes.size() && i < models.size() && !makes[i].isEmpty() && !models[i].isEmpty()) {
            QString newTitle = QString("%1 - %2").arg(makes[i], models[i]);
            tabWidget->setTabText(i, newTitle);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QVector<double>> LAUJETRDialog::getJETRVectors() const
{
    QList<QVector<double>> vectors;
    for (const LAUJETRWidget *widget : jetrWidgets) {
        vectors.append(widget->getJETRVector());
    }
    return vectors;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector<double> LAUJETRDialog::getCurrentJETRVector() const
{
    int currentIndex = tabWidget->currentIndex();
    if (currentIndex >= 0 && currentIndex < jetrWidgets.size()) {
        return jetrWidgets[currentIndex]->getJETRVector();
    }
    return QVector<double>(37, NAN);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::setJETRVectors(const QList<QVector<double>> &vectors)
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
void LAUJETRDialog::setJETRVector(const QVector<double> &vector)
{
    setJETRVectors(QList<QVector<double>>() << vector);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUJETRDialog::getMakes() const
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
QStringList LAUJETRDialog::getModels() const
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
QStringList LAUJETRDialog::getPositions() const
{
    QStringList positions;
    for (LAUJETRWidget *widget : jetrWidgets) {
        if (widget) {
            positions.append(widget->getCameraPosition());
        }
    }
    return positions;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector<double> LAUJETRDialog::getTopCameraJETRVector() const
{
    for (LAUJETRWidget *widget : jetrWidgets) {
        if (widget && widget->getCameraPosition().toLower().endsWith("top")) {
            return widget->getJETRVector();
        }
    }
    
    // If no "top" camera found, return empty vector
    return QVector<double>();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAUJETRDialog::getTopCameraIndex() const
{
    for (int i = 0; i < jetrWidgets.size(); ++i) {
        if (jetrWidgets[i] && jetrWidgets[i]->getCameraPosition().toLower().endsWith("top")) {
            return i;
        }
    }
    
    // If no "top" camera found, return -1
    return -1;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<bool> LAUJETRDialog::getRotations() const
{
    QList<bool> rotations;
    for (LAUJETRWidget *widget : jetrWidgets) {
        if (widget) {
            rotations.append(widget->getCameraRotation());
        }
    }
    return rotations;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::onExportCSVClicked()
{
    // Get filename from user
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Export JETR Vectors to CSV",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/jetr_vectors.csv",
        "CSV Files (*.csv);;All Files (*)"
    );

    if (filename.isEmpty()) {
        return;
    }

    // Open file for writing
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Export Error", 
                            QString("Failed to open file for writing: %1").arg(file.errorString()));
        return;
    }

    QTextStream stream(&file);
    
    // Write header row
    stream << "Make,Model";
    for (int i = 0; i < 37; ++i) {
        stream << ",JETR_" << i;
    }
    stream << "\n";

    // Get all cached JETR vectors from QSettings
    QSettings settings;
    settings.beginGroup("CameraParams");
    QStringList keys = settings.childKeys();
    
    int exportCount = 0;
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

        // Get JETR vector
        QVariantList variantList = settings.value(key).toList();
        if (variantList.size() != 37) {
            continue; // Skip invalid entries
        }

        // Write row
        stream << make << "," << model;
        for (const QVariant &variant : variantList) {
            stream << "," << QString::number(variant.toDouble(), 'g', 10);
        }
        stream << "\n";
        exportCount++;
    }
    
    settings.endGroup();
    file.close();

    // Show success message
    QMessageBox::information(this, "Export Complete", 
                           QString("Successfully exported %1 JETR vector(s) to:\n%2")
                           .arg(exportCount).arg(filename));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::setMemoryObjectOnAllWidgets(const LAUMemoryObject &memObject)
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        LAUJETRWidget* jetrWidget = qobject_cast<LAUJETRWidget*>(tabWidget->widget(i));
        if (jetrWidget) {
            jetrWidget->setMemoryObjectOnly(memObject, i);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::onInventoryClicked()
{
    LAUCameraInventoryDialog *inventoryDialog = new LAUCameraInventoryDialog(this);
    inventoryDialog->exec();
    inventoryDialog->deleteLater();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::onImportClicked()
{
    // Get last used directory from QSettings
    QSettings settings;
    QString lastDir = settings.value("LastImportDirectory", QDir::homePath()).toString();

    QString filename = QFileDialog::getOpenFileName(
        this,
        "Load TIFF File",
        lastDir,
        "TIFF Files (*.tif *.tiff);;All Files (*)"
        );

    if (filename.isEmpty()) {
        return;
    }

    // Save the directory for next time
    QFileInfo fileInfo(filename);
    settings.setValue("LastImportDirectory", fileInfo.absolutePath());

    // Detect file type and import accordingly
    FileType fileType = detectFileType(filename);

    try {
        switch (fileType) {
        case FileTypeMemoryObject:
            importMemoryObject(filename);
            break;
        case FileTypeLookUpTable:
            importLookUpTable(filename);
            break;
        case FileTypeLUTX:
            importLUTX(filename);
            break;
        default:
            QMessageBox::warning(this, "Import Error",
                                 QString("Unknown file type: %1\n\nSupported formats:\n"
                                         "• TIFF files (memory objects)\n"
                                         "• LUT files (lookup tables)\n"
                                         "• LUTX files (multiple lookup tables)")
                                     .arg(QFileInfo(filename).suffix()));
            return;
        }

        infoLabel->setText(QString("Imported: %1").arg(QFileInfo(filename).fileName()));

    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Import Error",
                              QString("Failed to import file:\n%1\n\nError: %2")
                                  .arg(filename).arg(e.what()));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::onJETRVectorChanged(const QVector<double> &vector)
{
    Q_UNUSED(vector)
    // Mark that changes have been made
    hasUnsavedChanges = true;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::onEditBoundingBox()
{
    // Get cached lookup tables from camera inventory and set transforms from JETR widgets
    QList<LAULookUpTable> lookupTables;
    QMatrix4x4 toXYPlaneTransform; // Transform from first camera to XY plane
    
    for (int i = 0; i < jetrWidgets.size(); ++i) {
        // Get the current make and model directly from the JETR widget
        QString make = jetrWidgets[i]->getCameraMake();
        QString model = jetrWidgets[i]->getCameraModel();
        
        // Get the cached LUT for this camera with priority generation
        LAULookUpTable table = LAUCameraInventoryDialog::getCachedLUTWithPriority(
            make, model,
            memoryObject.width(), memoryObject.height()/jetrWidgets.size(), this);
            
        if (table.isValid()) {
            // Get the current JETR vector from this widget
            QVector<double> jetrVector = jetrWidgets[i]->getJETRVector();

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
                table.setTransform(transformMatrix);
            } else {
                // Subsequent cameras: transform is relative to first camera
                // Need to convert to XY plane: (toXYPlane) * (thisToFirst) = (thisToXYPlane)
                QMatrix4x4 toXYPlane = toXYPlaneTransform * transformMatrix;
                table.setTransform(toXYPlane);
            }

            lookupTables.append(table);
        }
    }
    
    // Check if we have any valid lookup tables
    if (lookupTables.isEmpty()) {
        QMessageBox::warning(this, "No Lookup Tables", 
                           "No cached lookup tables found for the cameras. Please generate LUTs first.");
        return;
    }
    
    // Use the stored TIFF filename
    if (tiffFilename.isEmpty()) {
        QMessageBox::warning(this, "No TIFF File", 
                           "No TIFF file path has been set. Please set the filename first.");
        return;
    }


    LAUTiffViewerDialog dialog(this);
    
    // Get current bounding box from the first (top) camera JETR widget to initialize the dialog
    if (!jetrWidgets.isEmpty()) {
        QVector<double> topCameraJetr = jetrWidgets[0]->getJETRVector();
        if (topCameraJetr.size() >= 37) {
            // Extract bounding box values from JETR elements 28-33
            LookUpTableBoundingBox currentBoundingBox;
            currentBoundingBox.xMin = topCameraJetr[28];
            currentBoundingBox.xMax = topCameraJetr[29];
            currentBoundingBox.yMin = topCameraJetr[30];
            currentBoundingBox.yMax = topCameraJetr[31];
            currentBoundingBox.zMin = topCameraJetr[32];
            currentBoundingBox.zMax = topCameraJetr[33];
            
            // Apply this bounding box to all lookup tables before passing to dialog
            for (int i = 0; i < lookupTables.size(); ++i) {
                lookupTables[i].setBoundingBox(currentBoundingBox);
            }
        }
    }
    
    // Set the filename and lookup tables (no memory objects needed)
    dialog.setTiffFilename(tiffFilename);
    dialog.setLookupTables(lookupTables);
    
    // Show the dialog and handle result
    if (dialog.exec() == QDialog::Accepted) {
        // Get the updated bounding box from the dialog
        LookUpTableBoundingBox bbox = dialog.getBoundingBox();
        
        // Apply the bounding box to all tabs
        applyBoundingBoxToAllTabs(bbox.xMin, bbox.xMax, bbox.yMin, bbox.yMax, bbox.zMin, bbox.zMax);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUJETRDialog::FileType LAUJETRDialog::detectFileType(const QString &filename) const
{
    QFileInfo fileInfo(filename);
    QString suffix = fileInfo.suffix().toLower();

    if (suffix == "tif" || suffix == "tiff") {
        return FileTypeMemoryObject;
    } else if (suffix == "lut") {
        return FileTypeLookUpTable;
    } else if (suffix == "lutx") {
        return FileTypeLUTX;
    }

    return FileTypeUnknown;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::importMemoryObject(const QString &filename)
{
    // File loading is no longer supported - the main window must provide memory objects
    Q_UNUSED(filename);
    throw std::runtime_error("Memory object loading from dialog is no longer supported. The main window must provide memory objects.");
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::importMemoryObjectDirect(const QString &filename)
{
    // File loading is no longer supported - the main window must provide memory objects
    Q_UNUSED(filename);
    throw std::runtime_error("Memory object loading from dialog is no longer supported. The main window must provide memory objects.");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::importLookUpTable(const QString &filename)
{
    // Mock implementation - replace with actual LAULookUpTable loading
    LAULookUpTable lookupTable(filename);

    if (!lookupTable.isValid()) {
        throw std::runtime_error("Failed to load lookup table");
    }

    QVector<double> jetrVector = lookupTable.jetr();
    if (jetrVector.size() != 37) {
        throw std::runtime_error(QString("Invalid JETR vector size: %1 (expected 37)")
                                     .arg(jetrVector.size()).toStdString());
    }

    clearTabs();

    // Lists cleared in clearTabs()

    // Create widget with make/model and set to read-only
    LAUJETRWidget *widget = new LAUJETRWidget(jetrVector);
    widget->setCameraMake(lookupTable.makeString());
    widget->setCameraModel(lookupTable.modelString());

    // Set read-only mode for LUT inspection (no memory object available)
    widget->setReadOnly(true);


    // Connect to receive updates
    connect(widget, &LAUJETRWidget::jetrVectorChanged,
            this, &LAUJETRDialog::onJETRVectorChanged);
    connect(widget, &LAUJETRWidget::requestBoundingBoxEdit,
            this, &LAUJETRDialog::onEditBoundingBox);

    QString tabTitle = QString("%1 - %2").arg(lookupTable.makeString(), lookupTable.modelString());
    tabWidget->addTab(widget, tabTitle);
    jetrWidgets.append(widget);

    // Track current metadata
    currentMakes.append(widget->getCameraMake());
    currentModels.append(widget->getCameraModel());
    currentPositions.append(widget->getCameraPosition());
    currentRotations.append(widget->getCameraRotation());

    infoLabel->setText("Loaded lookup table");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::importLUTX(const QString &filename)
{
    // Mock implementation - replace with actual LUTX loading
    QList<LAULookUpTable> lookupTables = LAULookUpTable::LAULookUpTableX(filename);

    if (lookupTables.isEmpty()) {
        throw std::runtime_error("LUTX file contains no lookup tables");
    }

    clearTabs();
    
    // Lists cleared in clearTabs()

    for (int i = 0; i < lookupTables.size(); ++i) {
        const LAULookUpTable &table = lookupTables[i];

        QVector<double> jetrVector = table.jetr();
        if (jetrVector.size() != 37) {
            throw std::runtime_error(QString("Invalid JETR vector size in table %1: %2 (expected 37)")
                                         .arg(i + 1).arg(jetrVector.size()).toStdString());
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

        qDebug() << QString("LUT %1: Using transform matrix (user-defined alignment transforms)").arg(i + 1);

        // Make/model tracking now handled in addJETRTab

        // Create the widget first
        LAUJETRWidget *widget = new LAUJETRWidget(jetrVector);

        // Set the make and model information from the lookup table
        widget->setCameraMake(table.makeString());
        widget->setCameraModel(table.modelString());

        // Set read-only mode for LUTX inspection (no memory object available)
        widget->setReadOnly(true);

        // Server-only feature removed for standalone build
        // if (!dateFolder.isEmpty()) {
        //     QDate folderDate = LAULookUpTable::parseFolderDate(dateFolder);
        //     widget->setCurrentDate(folderDate);
        // }

        // Connect to receive updates
        connect(widget, &LAUJETRWidget::jetrVectorChanged,
                this, &LAUJETRDialog::onJETRVectorChanged);
        connect(widget, &LAUJETRWidget::requestBoundingBoxEdit,
                this, &LAUJETRDialog::onEditBoundingBox);

        QString tabTitle = QString("LUT %1 - %2 %3").arg(i + 1)
                               .arg(table.makeString()).arg(table.modelString());
        
        tabWidget->addTab(widget, tabTitle);
        jetrWidgets.append(widget);
    }

    infoLabel->setText(QString("Loaded %1 lookup table(s) from LUTX").arg(lookupTables.size()));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRDialog::preflight(const QString &filename, QWidget *parent)
{
    if (filename.isEmpty()) {
        return true; // Empty filename is valid for default constructor
    }
    
    // Check file type
    QFileInfo fileInfo(filename);
    QString suffix = fileInfo.suffix().toLower();
    
    // For TIFF files (memory objects), we need to check if camera selection is needed
    if (suffix == "tif" || suffix == "tiff") {
        LAUMemoryObject memoryObject(filename);
        
        if (!memoryObject.isValid()) {
            return false; // Invalid file
        }
        
        // Determine number of cameras
        int totalHeight = memoryObject.height();
        int cameraHeight = 480;
        int numCameras = qMax(1, totalHeight / cameraHeight);
        
        // Check if TIFF has valid JETR data
        if (memoryObject.hasValidJETRVector()) {
            QVector<double> jetrVector = memoryObject.jetr();
            int jetrVectorCount = jetrVector.size() / 37;
            
            // Check if JETR vector count matches number of cameras
            if (jetrVectorCount != numCameras) {
                QMessageBox::warning(parent, "JETR Vector Mismatch",
                                   QString("The TIFF file has %1 JETR vector(s) but contains %2 cameras.\n\n"
                                         "This needs to be fixed in the source data.")
                                   .arg(jetrVectorCount)
                                   .arg(numCameras));
                
                // After warning, check if there are cameras in inventory
                QList<QPair<QString, QString>> availablePairs = LAUJETRWidget::getAllMakeModelPairs();
                if (availablePairs.isEmpty()) {
                    return false; // No inventory, don't create dialog
                }
                // Has inventory, need camera selection - continue with preflight
            }
            // Valid JETR data with correct count - no need for camera selection
            return true;
        }
        
        // No valid JETR vectors in TIFF - warn user first
        QMessageBox::warning(parent, "No JETR Vectors",
                           QString("The TIFF file contains no valid JETR vectors for %1 camera(s).")
                           .arg(numCameras));
        
        // Check if there are cameras in inventory
        QList<QPair<QString, QString>> availablePairs = LAUJETRWidget::getAllMakeModelPairs();
        if (availablePairs.isEmpty()) {
            return false; // No inventory, don't create dialog
        }
        
        // Need camera selection - preflight should NOT call the dialog
        // The dialog constructor will handle the actual camera selection
        return true;
    }
    
    // For LUT and LUTX files, no preflight checks needed
    if (suffix == "lut" || suffix == "lutx") {
        return true;
    }
    
    // Unknown file type
    return false;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::applyBoundingBoxToAllTabs(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
    // Apply the bounding box values to all JETR widgets
    for (LAUJETRWidget *widget : jetrWidgets) {
        if (widget) {
            // Get the current JETR vector from the widget
            QVector<double> jetrVector = widget->getJETRVector();
            
            // Ensure the vector has the correct size (37 elements)
            if (jetrVector.size() >= 37) {
                // Update bounding box values (elements 28-33)
                jetrVector[28] = xMin;  // xMin
                jetrVector[29] = xMax;  // xMax
                jetrVector[30] = yMin;  // yMin
                jetrVector[31] = yMax;  // yMax
                jetrVector[32] = zMin;  // zMin
                jetrVector[33] = zMax;  // zMax
                
                // Set the updated vector back to the widget
                widget->setJETRVector(jetrVector, true); // updateUI = true to refresh display
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRDialog::reject()
{
    // Check if there are unsaved changes
    QList<QVector<double>> currentVectors = getJETRVectors();
    bool hasChanges = (currentVectors != originalJETRVectors) || hasUnsavedChanges;

    if (hasChanges) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Unsaved Changes");
        msgBox.setText("You have unsaved changes to the calibration data.");
        msgBox.setInformativeText("Do you want to discard these changes and close?");
        msgBox.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Warning);

        int result = msgBox.exec();

        if (result == QMessageBox::Cancel) {
            return; // Don't close, user wants to keep editing
        }
        // User clicked Discard, continue with reject
    }

    QDialog::reject();
}
