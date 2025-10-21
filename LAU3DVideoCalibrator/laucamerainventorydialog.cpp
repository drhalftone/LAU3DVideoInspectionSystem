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

#include "laucamerainventorydialog.h"
#include "laujetrdialog.h"
#include "laujetrwidget.h"
#include <algorithm>
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QFileInfo>

// Forward declaration for CSV helper function
static QString quoteCSVField(const QString &field);

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCameraInventoryDialog::LAUCameraInventoryDialog(QWidget *parent)
    : QDialog(parent), hasChanges(false)
{
    // Load default calibrations on first run
    loadDefaultCalibrations();

    setupUI();
    initializeStaging();
    refreshInventory();
    
    // Restore dialog geometry from QSettings
    QSettings settings;
    settings.beginGroup("DialogGeometry");
    QByteArray geometry = settings.value("LAUCameraInventoryDialog/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    settings.endGroup();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCameraInventoryDialog::~LAUCameraInventoryDialog()
{
    // Save dialog geometry to QSettings
    QSettings settings;
    settings.beginGroup("DialogGeometry");
    settings.setValue("LAUCameraInventoryDialog/geometry", saveGeometry());
    settings.endGroup();
    
    // Qt handles cleanup automatically
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::setupUI()
{
    setWindowTitle("Camera Calibration Inventory");
    setFixedSize(800, 550);

    mainLayout = new QVBoxLayout(this);

    // Header label
    headerLabel = new QLabel();
    headerLabel->setStyleSheet("font-weight: bold; margin: 10px;");
    mainLayout->addWidget(headerLabel);

    // Instructional text
    QLabel *instructionLabel = new QLabel(
        "<b>About Camera Calibrations:</b><br>"
        "This inventory stores camera calibration data (JETR vectors and lookup tables) that enable "
        "3D reconstruction from depth camera images. Each entry represents a specific camera make/model "
        "configuration.<br><br>"
        "<b>How to use:</b><br>"
        "• <b>Import:</b> Add new camera calibrations from LUT or LUTX files<br>"
        "• <b>Display:</b> View detailed JETR parameters for a selected camera<br>"
        "• <b>Delete:</b> Remove unwanted calibrations from inventory<br>"
        "• <b>Double-click:</b> Quick way to display a camera's configuration<br><br>"
        "<i>Note: Changes are only saved when you click OK. Click Cancel to discard all changes.</i>"
    );
    instructionLabel->setWordWrap(true);
    instructionLabel->setTextFormat(Qt::RichText);
    instructionLabel->setStyleSheet(
        "background-color: #f0f0f0; "
        "border: 1px solid #ccc; "
        "border-radius: 4px; "
        "padding: 10px; "
        "margin: 5px;"
    );
    mainLayout->addWidget(instructionLabel);

    // Create table widget
    table = new QTableWidget();
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels(QStringList() << "Make" << "Model" << "Cached Date" << "JETR Status");
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);  // Make table read-only
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainLayout->addWidget(table);

    // Info label
    infoLabel = new QLabel();
    infoLabel->setStyleSheet("color: #666; font-style: italic; margin: 5px;");
    mainLayout->addWidget(infoLabel);

    // Button box - Accept/Cancel buttons on right side
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // Create action buttons - these will appear on the left side with ActionRole
    importButton = new QPushButton("Import");
    importButton->setToolTip("Import camera calibrations from LUT or LUTX files to add new camera configurations");
    buttonBox->addButton(importButton, QDialogButtonBox::ActionRole);

    exportButton = new QPushButton("Export");
    exportButton->setToolTip("Export all camera calibrations to a CSV file with make, model, and JETR vector elements");
    buttonBox->addButton(exportButton, QDialogButtonBox::ActionRole);

    displayButton = new QPushButton("Display");
    displayButton->setToolTip("View detailed JETR parameters and settings for the selected camera configuration");
    displayButton->setEnabled(false);
    buttonBox->addButton(displayButton, QDialogButtonBox::ActionRole);

    deleteButton = new QPushButton("Delete");
    deleteButton->setToolTip("Permanently remove the selected camera calibration from the inventory");
    deleteButton->setEnabled(false);
    buttonBox->addButton(deleteButton, QDialogButtonBox::ActionRole);

    mainLayout->addWidget(buttonBox);

    // Connect signals
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(displayButton, &QPushButton::clicked, this, &LAUCameraInventoryDialog::onDisplayClicked);
    connect(deleteButton, &QPushButton::clicked, this, &LAUCameraInventoryDialog::onDeleteClicked);
    connect(importButton, &QPushButton::clicked, this, &LAUCameraInventoryDialog::onImportClicked);
    connect(exportButton, &QPushButton::clicked, this, &LAUCameraInventoryDialog::onExportClicked);

    // Connect selection changes
    connect(table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &LAUCameraInventoryDialog::onSelectionChanged);
    connect(table, &QTableWidget::currentItemChanged,
            this, &LAUCameraInventoryDialog::onSelectionChanged);

    // Double-click to display
    connect(table, &QTableWidget::itemDoubleClicked, [this]() {
        displayButton->click();
    });

    // Right-click to deselect
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table, &QWidget::customContextMenuRequested, this, &LAUCameraInventoryDialog::onTableRightClicked);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::refreshInventory()
{
    populateTable();
    updateHeaderLabel();
    updateButtonStates();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::populateTable()
{
    // Clear existing data
    table->setRowCount(0);
    
    // Get all cached JETR vectors from QSettings
    QSettings settings;
    settings.beginGroup("CameraParams");
    QStringList keys = settings.childKeys();
    
    // Structure to hold camera data for sorting
    struct CameraData {
        QString make;
        QString model;
        QString jetrStatus;
        QColor statusColor;
    };
    
    QList<CameraData> cameraList;
    
    // Collect all camera data
    for (const QString &key : keys) {
        CameraData camera;
        
        // Parse make and model from key (format: Make_Model)
        int underscorePos = key.indexOf('_');
        if (underscorePos > 0) {
            camera.make = key.left(underscorePos);
            camera.model = key.mid(underscorePos + 1);
        } else {
            camera.make = key;
            camera.model = "";
        }
        
        // Get JETR vector to validate
        QVariantList variantList = settings.value(key).toList();
        if (variantList.size() != 37) {
            camera.jetrStatus = "Invalid (wrong size)";
            camera.statusColor = QColor(255, 0, 0); // Red
        } else {
            // Check if vector contains valid data
            bool hasValidData = false;
            for (const QVariant &variant : variantList) {
                double value = variant.toDouble();
                if (!std::isnan(value) && value != 0.0) {
                    hasValidData = true;
                    break;
                }
            }
            camera.jetrStatus = hasValidData ? "Valid" : "Empty/Zero";
            camera.statusColor = hasValidData ? QColor(0, 128, 0) : QColor(255, 0, 0); // Green or Red
        }
        
        cameraList.append(camera);
    }
    
    settings.endGroup();
    
    // Sort alphabetically by make, then by model
    std::sort(cameraList.begin(), cameraList.end(), [](const CameraData &a, const CameraData &b) {
        if (a.make != b.make) {
            return a.make < b.make;
        }
        return a.model < b.model;
    });
    
    // Populate table with sorted data
    if (!cameraList.isEmpty()) {
        table->setRowCount(cameraList.size());
        
        for (int row = 0; row < cameraList.size(); ++row) {
            const CameraData &camera = cameraList[row];
            
            // Create table items
            table->setItem(row, 0, new QTableWidgetItem(camera.make));
            table->setItem(row, 1, new QTableWidgetItem(camera.model));
            table->setItem(row, 2, new QTableWidgetItem("Cached"));
            
            QTableWidgetItem *statusItem = new QTableWidgetItem(camera.jetrStatus);
            statusItem->setForeground(QBrush(camera.statusColor));
            table->setItem(row, 3, statusItem);
        }
    }
    
    // Clear selection to ensure buttons remain disabled until user explicitly selects
    table->clearSelection();
    table->setCurrentItem(nullptr);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::initializeStaging()
{
    deletedKeys.clear();
    importedKeys.clear();
    hasChanges = false;
    
    // Capture the original keys that existed when dialog opened
    originalKeys.clear();
    QSettings settings;
    settings.beginGroup("CameraParams");
    originalKeys = settings.childKeys();
    settings.endGroup();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::accept()
{
    if (hasChanges) {
        commitChangesToSettings();
    }
    QDialog::accept();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::reject()
{
    if (hasChanges) {
        rollbackChanges();
    }
    QDialog::reject();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::commitChangesToSettings()
{
    if (!hasChanges) {
        return;
    }
    
    // Apply deletions
    if (!deletedKeys.isEmpty()) {
        QSettings settings;
        settings.beginGroup("CameraParams");
        for (const QString &key : deletedKeys) {
            settings.remove(key);
        }
        settings.endGroup();
    }
    
    // Reset staging state
    initializeStaging();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::rollbackChanges()
{
    // Remove any imports that were made during this session
    if (!importedKeys.isEmpty()) {
        QSettings settings;
        settings.beginGroup("CameraParams");
        for (const QString &key : importedKeys) {
            settings.remove(key);
        }
        settings.endGroup();
    }
    
    // Reset staging state
    initializeStaging();
    
    // Refresh the table to show original state
    refreshInventory();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::updateHeaderLabel()
{
    int count = table->rowCount();
    if (count > 0) {
        headerLabel->setText(QString("Found %1 cached camera calibration(s):").arg(count));
    } else {
        headerLabel->setText("No cached camera calibrations found. Use 'Import' to add calibrations.");
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::updateButtonStates()
{
    int currentRow = table->currentRow();
    bool hasSelection = currentRow >= 0 && table->rowCount() > 0;
    
    // Delete button: enabled when any row is selected
    deleteButton->setEnabled(hasSelection);
    
    // Display button: enabled only when a valid LUT is selected
    bool hasValidLUT = false;
    if (hasSelection) {
        QTableWidgetItem *statusItem = table->item(currentRow, 3); // JETR Status column
        if (statusItem && statusItem->text() == "Valid") {
            hasValidLUT = true;
        }
    }
    displayButton->setEnabled(hasValidLUT);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::onSelectionChanged()
{
    updateButtonStates();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::onTableRightClicked(const QPoint &pos)
{
    Q_UNUSED(pos)
    
    // Clear selection and current item on right-click
    table->clearSelection();
    table->setCurrentItem(nullptr);
    
    // This will trigger onSelectionChanged() which updates button states
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::onDisplayClicked()
{
    int currentRow = table->currentRow();
    if (currentRow >= 0) {
        QString make = table->item(currentRow, 0)->text();
        QString model = table->item(currentRow, 1)->text();
        
        // Load JETR vector from cache
        LAUCameraCalibration calibration = getCameraCalibration(make, model);
        QVector<double> jetrVector = calibration.jetrVector;
        
        if (!jetrVector.isEmpty()) {
            // Create new JETR dialog as modal child
            LAUJETRDialog *dialog = new LAUJETRDialog(this);
            dialog->setWindowTitle(QString("Display Calibration - %1 %2").arg(make, model));
            dialog->setModal(true);
            dialog->clearTabs();
            dialog->addJETRTab(jetrVector, make, model);
            dialog->setDisplayMode(true); // Set to read-only display mode
            dialog->exec(); // Modal execution - blocks until closed
            dialog->deleteLater(); // Explicit cleanup
        } else {
            QMessageBox::warning(this, "Load Failed",
                               QString("Could not load calibration for %1 %2").arg(make, model));
        }
    } else {
        QMessageBox::information(this, "No Selection",
                               "Please select a camera configuration to load.");
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::onDeleteClicked()
{
    int currentRow = table->currentRow();
    if (currentRow >= 0) {
        QString make = table->item(currentRow, 0)->text();
        QString model = table->item(currentRow, 1)->text();
        
        int result = QMessageBox::question(this, "Delete Configuration",
                                         QString("Are you sure you want to delete the cached calibration for:\n\n%1 %2\n\nThis action cannot be undone.")
                                         .arg(make, model),
                                         QMessageBox::Yes | QMessageBox::No);
        
        if (result == QMessageBox::Yes) {
            // Stage deletion (don't actually delete from QSettings until accepted)
            LAUJETRWidget tempWidget;
            QString makeModelKey = tempWidget.makeMakeModelKey(make, model);
            deletedKeys.append(makeModelKey);
            hasChanges = true;
            
            // Remove row from table to show staging effect
            table->removeRow(currentRow);
            updateHeaderLabel();
            updateButtonStates();
            
            QMessageBox::information(this, "Staged for Deletion",
                                   QString("Calibration for %1 %2 will be deleted when you click OK.\n\n"
                                          "Click Cancel to undo this deletion.")
                                   .arg(make, model));
        }
    } else {
        QMessageBox::information(this, "No Selection",
                               "Please select a camera configuration to delete.");
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::onImportClicked()
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
        return;
    }

    // Save the directory for next time
    QFileInfo fileInfo(filename);
    settings.setValue("LastImportDirectory", fileInfo.absolutePath());

    // Create new JETR dialog with the selected file
    LAUJETRDialog dialog(filename, this);
    dialog.setWindowTitle("Import Camera Calibrations");
    
    // Show the dialog for editing/review of the imported data
    int result = dialog.exec(); // Modal execution
    
    if (result == QDialog::Accepted) {
            // Get the JETR vectors from the dialog
            QList<QVector<double>> vectors = dialog.getJETRVectors();
            QStringList makes = dialog.getMakes();
            QStringList models = dialog.getModels();
            
            if (!vectors.isEmpty()) {
                // Save each camera's calibration to the make/model cache
                QSettings settings;
                settings.beginGroup("CameraParams");
                
                int savedCount = 0;
                for (int i = 0; i < vectors.size() && i < makes.size() && i < models.size(); ++i) {
                    if (!makes[i].isEmpty() && !models[i].isEmpty()) {
                        // Create the make_model key using the standard method
                        LAUJETRWidget tempWidget;
                        QString key = tempWidget.makeMakeModelKey(makes[i], models[i]);
                        
                        // Convert vector to variant list
                        QVariantList variantList;
                        for (double value : vectors[i]) {
                            variantList.append(value);
                        }
                        
                        // Save to settings
                        settings.setValue(key, variantList);
                        
                        // Only track truly new keys for potential rollback (not pre-existing ones)
                        if (!originalKeys.contains(key)) {
                            importedKeys.append(key);
                        }
                        savedCount++;
                    }
                }
                
                settings.endGroup();
                
                if (savedCount > 0) {
                    // Mark that changes have been made (imports are immediately saved)
                    hasChanges = true;
                    
                    // Refresh the inventory to show newly imported calibrations
                    refreshInventory();
                    
                    QMessageBox::information(this, "Import Complete",
                                           QString("Successfully imported %1 camera calibration(s).\n\n"
                                                   "The inventory has been updated to show the new calibrations.\n"
                                                   "Click OK to confirm, or Cancel to remove these imports.")
                                           .arg(savedCount));
                }
            }
        }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::onExportClicked()
{
    // Get all camera calibrations
    QList<LAUCameraCalibration> calibrations = getAllCameraCalibrations();

    if (calibrations.isEmpty()) {
        QMessageBox::information(this, "No Data",
                               "No camera calibrations available to export.");
        return;
    }

    // Get last used directory from QSettings
    QSettings settings;
    QString lastDir = settings.value("LastExportDirectory", QDir::homePath()).toString();

    // Prompt user for save location
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Export Camera Calibrations to CSV",
        lastDir + "/camera_calibrations.csv",
        "CSV Files (*.csv);;All Files (*)"
    );

    if (filename.isEmpty()) {
        return; // User cancelled
    }

    // Save the directory for next time
    QFileInfo fileInfo(filename);
    settings.setValue("LastExportDirectory", fileInfo.absolutePath());

    // Open file for writing
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Export Failed",
                            QString("Failed to open file for writing:\n%1").arg(filename));
        return;
    }

    QTextStream out(&file);

    // Write CSV header
    out << "Make,Model";

    // Add column headers for all 37 JETR elements
    // JETR vector structure (37 elements):
    // 0-11: Intrinsics (focal length, principal point, distortion)
    // 12-27: Transform matrix (rotation + translation, 16 elements)
    // 28-33: Bounding box (6 elements)
    // 34-36: Depth parameters (3 elements)
    for (int i = 0; i < 37; ++i) {
        out << ",JETR_" << QString::number(i);
    }
    out << "\n";

    // Sort calibrations by make, then model
    std::sort(calibrations.begin(), calibrations.end(),
              [](const LAUCameraCalibration &a, const LAUCameraCalibration &b) {
        if (a.make != b.make) {
            return a.make < b.make;
        }
        return a.model < b.model;
    });

    // Write data rows
    int exportedCount = 0;
    for (const LAUCameraCalibration &calibration : calibrations) {
        if (!calibration.isValid()) {
            continue; // Skip invalid calibrations
        }

        // Write make and model (quote if they contain commas or quotes)
        out << quoteCSVField(calibration.make) << ","
            << quoteCSVField(calibration.model);

        // Write all 37 JETR vector elements
        for (int i = 0; i < 37; ++i) {
            out << "," << QString::number(calibration.jetrVector[i], 'g', 15); // 15 digits precision
        }
        out << "\n";
        exportedCount++;
    }

    file.close();

    QMessageBox::information(this, "Export Complete",
                           QString("Successfully exported %1 camera calibration(s) to:\n\n%2")
                           .arg(exportedCount).arg(filename));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString quoteCSVField(const QString &field)
{
    // Quote the field if it contains comma, quote, or newline
    if (field.contains(',') || field.contains('"') || field.contains('\n')) {
        QString quoted = field;
        quoted.replace("\"", "\"\""); // Escape quotes by doubling them
        return QString("\"%1\"").arg(quoted);
    }
    return field;
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<LAUCameraCalibration> LAUCameraInventoryDialog::getAllCameraCalibrations()
{
    QList<LAUCameraCalibration> calibrations;
    
    QSettings settings;
    settings.beginGroup("CameraParams");
    
    QStringList keys = settings.allKeys();
    for (const QString &key : keys) {
        QVariantList data = settings.value(key).toList();
        
        if (data.size() >= 37) {
            // Parse the make/model from the key (format: "make_model")
            QStringList parts = key.split('_');
            if (parts.size() >= 2) {
                QString make = parts[0];
                QString model = parts.mid(1).join('_'); // Rejoin in case model has underscores
                
                QVector<double> jetrVector;
                for (int i = 0; i < 37; ++i) {
                    jetrVector.append(data[i].toDouble());
                }
                
                calibrations.append(LAUCameraCalibration(make, model, jetrVector));
            }
        }
    }
    
    settings.endGroup();
    return calibrations;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCameraCalibration LAUCameraInventoryDialog::getCameraCalibration(const QString &make, const QString &model)
{
    QString key = QString("%1_%2").arg(make, model);
    
    QSettings settings;
    settings.beginGroup("CameraParams");
    
    if (settings.contains(key)) {
        QVariantList data = settings.value(key).toList();
        
        if (data.size() >= 37) {
            QVector<double> jetrVector;
            for (int i = 0; i < 37; ++i) {
                jetrVector.append(data[i].toDouble());
            }
            
            settings.endGroup();
            return LAUCameraCalibration(make, model, jetrVector);
        }
    }
    
    settings.endGroup();
    return LAUCameraCalibration(); // Invalid calibration
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::saveCameraCalibration(const LAUCameraCalibration &calibration)
{
    if (!calibration.isValid()) {
        return;
    }
    
    QSettings settings;
    settings.beginGroup("CameraParams");
    
    QVariantList variantList;
    for (double value : calibration.jetrVector) {
        variantList.append(value);
    }
    
    settings.setValue(calibration.makeModelKey(), variantList);
    settings.endGroup();
    
    // Invalidate LUT cache for this camera since JETR vector may have changed
    invalidateLUTCache(calibration.make, calibration.model);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUCameraInventoryDialog::hasCameraCalibration(const QString &make, const QString &model)
{
    QString key = QString("%1_%2").arg(make, model);
    
    QSettings settings;
    settings.beginGroup("CameraParams");
    bool exists = settings.contains(key);
    settings.endGroup();
    
    return exists;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUCameraInventoryDialog::getAvailableMakes()
{
    QStringList makes;
    
    QSettings settings;
    settings.beginGroup("CameraParams");
    
    QStringList keys = settings.allKeys();
    for (const QString &key : keys) {
        QStringList parts = key.split('_');
        if (parts.size() >= 2) {
            QString make = parts[0];
            if (!makes.contains(make)) {
                makes.append(make);
            }
        }
    }
    
    settings.endGroup();
    makes.sort();
    return makes;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUCameraInventoryDialog::getAvailableModels(const QString &make)
{
    QStringList models;

    QSettings settings;
    settings.beginGroup("CameraParams");

    QStringList keys = settings.allKeys();
    for (const QString &key : keys) {
        QStringList parts = key.split('_');
        if (parts.size() >= 2 && parts[0] == make) {
            QString model = parts.mid(1).join('_');
            if (!models.contains(model)) {
                models.append(model);
            }
        }
    }

    settings.endGroup();
    models.sort();
    return models;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::loadDefaultCalibrations()
{
    // Check if defaults have already been loaded AND verify actual data exists
    QSettings settings;
    bool flagSet = settings.value("DefaultCalibrationsLoaded", false).toBool();

    // Also check if we actually have camera data in QSettings
    settings.beginGroup("CameraParams");
    QStringList existingKeys = settings.childKeys();
    settings.endGroup();

    // Only skip if flag is set AND we have actual camera data
    if (flagSet && !existingKeys.isEmpty()) {
        qDebug() << "Default calibrations already loaded, found" << existingKeys.count() << "cameras in inventory";
        return; // Already loaded with actual data, don't overwrite
    }

    // Flag was set but no data exists, or flag not set - reload defaults
    if (flagSet && existingKeys.isEmpty()) {
        qWarning() << "DefaultCalibrationsLoaded flag was set but no camera data found - reloading defaults";
    }

    // Hard-coded default camera calibrations
    struct DefaultCalibration {
        QString make;
        QString model;
        QVector<double> jetr;
    };

    QList<DefaultCalibration> defaults;

    // Lucid Helios_2
    defaults.append({
        "Lucid",
        "Helios_2",
        {523.925, 316.193, 523.925, 228.353, -0.23032, 0.11712, -0.04607, -0.04607, -0.04607, -0.04607, -3e-05, -0.00042,
         0.99949, -0.0090389, -0.030751, 0, 0.012154, 0.99464, 0.10266, 0, 0.029658, -0.10299, 0.99424, 0,
         923.51, 357.43, 2642.1, 1, 150, 3500, -50, 800, 500, 5000, 0.25, -8400, -33}
    });

    // Orbbec Astra_2
    defaults.append({
        "Orbbec",
        "Astra_2",
        {712.677, 400.289, 712.536, 300.999, 0, 0, 0, 0, 0, 0, 0, 0,
         0.0818, -0.0025535, -0.99664, 0, 0.46839, -0.88259, 0.040704, 0, -0.87973, -0.47014, -0.070999, 0,
         946.1, 183.97, 3200.5, 1, 150, 3500, -50, 800, 500, 5000, 0.25, -8400, -33}
    });

    // Orbbec Femto_Mega_i
    defaults.append({
        "Orbbec",
        "Femto_Mega_i",
        {504.251, 321.675, 504.321, 326.112, 19.3159, 10.0552, 0.37672, 19.6183, 16.6113, 2.35048, 0.00011, 0,
         0.9989, 0.036661, 0.02922, 0, -0.023933, -0.13715, 0.99026, 0, 0.040312, -0.98987, -0.13612, 0,
         2236.8, -1214.5, 2485.3, 1, 150, 3500, -50, 800, 500, 5000, 0.25, -8400, -33}
    });

    // Save all defaults to QSettings
    settings.beginGroup("CameraParams");
    for (const DefaultCalibration &defCal : defaults) {
        QString key = QString("%1_%2").arg(defCal.make, defCal.model);

        // Only save if doesn't already exist (don't overwrite user data)
        if (!settings.contains(key)) {
            QVariantList variantList;
            for (double value : defCal.jetr) {
                variantList.append(value);
            }
            settings.setValue(key, variantList);
            qDebug() << "Loaded default calibration:" << defCal.make << defCal.model;
        }
    }
    settings.endGroup();

    // Mark that defaults have been loaded
    settings.setValue("DefaultCalibrationsLoaded", true);

    qDebug() << "Default camera calibrations loaded successfully";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
// Static cache and generator variable definitions
QHash<QString, LAULookUpTable> LAUCameraInventoryDialog::lutCache;
QMutex LAUCameraInventoryDialog::lutCacheMutex;
LAULookUpTableGenerator* LAUCameraInventoryDialog::backgroundGenerator = nullptr;

// Standard dimensions for background LUT generation
const QList<QSize> LAULookUpTableGenerator::STANDARD_DIMENSIONS = {
    QSize(640, 480)   // Only generate 640x480 lookup tables
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUCameraInventoryDialog::makeLUTCacheKey(const QString &make, const QString &model, 
                                                   int width, int height)
{
    return QString("%1_%2_%3x%4").arg(make, model).arg(width).arg(height);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable LAUCameraInventoryDialog::getCachedLUT(const QString &make, const QString &model, 
                                                       int width, int height, QWidget *parent)
{
    // TEMPORARY ASSERT: Ensure we only request 640x480 LUTs (remove this later)
    Q_ASSERT_X(width == 640 && height == 480, "getCachedLUT", 
               QString("LUT request for incorrect dimensions: %1x%2. Expected 640x480 only!")
               .arg(width).arg(height).toLocal8Bit().constData());
    
    // Check if we have the camera calibration
    if (!hasCameraCalibration(make, model)) {
        return LAULookUpTable(); // Return invalid LUT if no calibration
    }
    
    // Generate cache key
    QString cacheKey = makeLUTCacheKey(make, model, width, height);
    
    // Check if LUT is already cached
    {
        QMutexLocker locker(&lutCacheMutex);
        if (lutCache.contains(cacheKey)) {
            qDebug() << "LUT cache hit for:" << make << model << QString("%1x%2").arg(width).arg(height);
            return lutCache[cacheKey];
        }
    }
    
    // Generate new LUT
    qDebug() << "Generating LUT for:" << make << model << QString("%1x%2").arg(width).arg(height);
    LAUCameraCalibration calibration = getCameraCalibration(make, model);
    if (!calibration.isValid()) {
        return LAULookUpTable(); // Return invalid LUT if calibration is invalid
    }
    
    // Generate LUT from JETR vector
    // Use invalid date for inventory/cache mode - this will preserve old behavior (always rotate Femto Mega)
    LAULookUpTable lut = LAULookUpTable::generateTableFromJETR(width, height, calibration.jetrVector, make, model, QDate(), nullptr);
    
    // Cache the LUT if valid
    if (lut.isValid()) {
        lutCache[cacheKey] = lut;
        qDebug() << "Cached LUT for:" << make << model << QString("%1x%2").arg(width).arg(height);
    }
    
    return lut;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::cacheLUT(const QString &make, const QString &model,
                                        int width, int height, const LAULookUpTable &lut)
{
    if (lut.isValid()) {
        QString cacheKey = makeLUTCacheKey(make, model, width, height);
        QMutexLocker locker(&lutCacheMutex);
        lutCache[cacheKey] = lut;
        qDebug() << "Manually cached LUT for:" << make << model << QString("%1x%2").arg(width).arg(height);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::invalidateLUTCache(const QString &make, const QString &model)
{
    // Remove all cached LUTs for this make/model combination (all dimensions)
    QStringList keysToRemove;
    QString prefix = QString("%1_%2_").arg(make, model);
    
    for (auto it = lutCache.begin(); it != lutCache.end(); ++it) {
        if (it.key().startsWith(prefix)) {
            keysToRemove << it.key();
        }
    }
    
    for (const QString &key : keysToRemove) {
        lutCache.remove(key);
        qDebug() << "Invalidated LUT cache entry:" << key;
    }
    
    if (!keysToRemove.isEmpty()) {
        qDebug() << "Invalidated" << keysToRemove.size() << "LUT cache entries for:" << make << model;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::clearLUTCache()
{
    QMutexLocker locker(&lutCacheMutex);
    int cacheSize = lutCache.size();
    lutCache.clear();
    qDebug() << "Cleared entire LUT cache (" << cacheSize << "entries )";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::startBackgroundLUTGeneration()
{
    if (!backgroundGenerator) {
        backgroundGenerator = new LAULookUpTableGenerator();
        // Connect signals for monitoring (optional)
        QObject::connect(backgroundGenerator, &LAULookUpTableGenerator::lutGenerated,
                        [](const QString &make, const QString &model, int width, int height) {
                            qDebug() << "Background generated LUT for:" << make << model 
                                    << QString("%1x%2").arg(width).arg(height);
                        });
    }
    backgroundGenerator->startBackgroundGeneration();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::stopBackgroundLUTGeneration()
{
    // Use a static mutex to prevent concurrent execution of this function
    static QMutex stopMutex;
    QMutexLocker stopLocker(&stopMutex);
    
    // Check if generator exists and hasn't been already cleaned up
    if (backgroundGenerator) {
        // Store the pointer and immediately set the global to nullptr to prevent double-deletion
        LAULookUpTableGenerator* generator = backgroundGenerator;
        backgroundGenerator = nullptr;
        
        qDebug() << "Stopping background LUT generation...";
        generator->stopGeneration();
        
        // Clear the LUT cache to prevent access to shared data during shutdown
        {
            QMutexLocker locker(&lutCacheMutex);
            lutCache.clear();
        }
        
        // Wait for thread to stop gracefully while processing events
        int gracefulAttempts = 0;
        while (generator->isRunning() && gracefulAttempts < 100) { // ~1 second at 10ms intervals
            QApplication::processEvents();
            QThread::msleep(10);
            gracefulAttempts++;
        }
        
        if (!generator->isRunning()) {
            qDebug() << "Background LUT generator stopped gracefully";
        } else {
            qWarning() << "Background LUT generator did not stop gracefully, requesting interruption...";
            generator->requestInterruption();
            
            // Wait after interruption request while processing events
            int interruptAttempts = 0;
            while (generator->isRunning() && interruptAttempts < 100) { // ~1 second
                QApplication::processEvents();
                QThread::msleep(10);
                interruptAttempts++;
            }
            
            if (!generator->isRunning()) {
                qDebug() << "Background LUT generator stopped after interruption";
            } else {
                qWarning() << "Background LUT generator still running, terminating forcefully...";
                generator->terminate();
                
                // Wait for termination while processing events
                int terminateAttempts = 0;
                while (generator->isRunning() && terminateAttempts < 200) { // ~2 seconds
                    QApplication::processEvents();
                    QThread::msleep(10);
                    terminateAttempts++;
                }
                
                if (!generator->isRunning()) {
                    qDebug() << "Background LUT generator terminated successfully";
                } else {
                    qCritical() << "CRITICAL: Background LUT generator failed to terminate!";
                }
            }
        }
        
        if (!generator->isRunning()) {
            delete generator;
            qDebug() << "Background LUT generation cleanup completed";
        } else {
            qCritical() << "CRITICAL: Cannot safely delete background generator - potential memory leak";
            // generator is already detached from global pointer, so just leak it
        }
    } else {
        qDebug() << "Background LUT generator already stopped or never started";
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::pauseBackgroundLUTGeneration()
{
    if (backgroundGenerator) {
        qDebug() << "Pausing background LUT generation...";
        backgroundGenerator->pauseGeneration();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::resumeBackgroundLUTGeneration()
{
    if (backgroundGenerator) {
        qDebug() << "Resuming background LUT generation...";
        backgroundGenerator->resumeGeneration();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUCameraInventoryDialog::hasLUTInCache(const QString &make, const QString &model, int width, int height)
{
    QString cacheKey = makeLUTCacheKey(make, model, width, height);
    QMutexLocker locker(&lutCacheMutex);
    return lutCache.contains(cacheKey);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraInventoryDialog::addLUTToCache(const QString &make, const QString &model, int width, int height, 
                                             const LAULookUpTable &lut)
{
    QString cacheKey = makeLUTCacheKey(make, model, width, height);
    QMutexLocker locker(&lutCacheMutex);
    lutCache[cacheKey] = lut;
    qDebug() << "Added LUT to cache:" << cacheKey;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable LAUCameraInventoryDialog::getCachedLUTWithPriority(const QString &make, const QString &model, 
                                                                   int width, int height, QWidget *parent)
{
    // TEMPORARY ASSERT: Ensure we only request 640x480 LUTs (remove this later)
    Q_ASSERT_X(width == 640 && height == 480, "getCachedLUTWithPriority", 
               QString("LUT request for incorrect dimensions: %1x%2. Expected 640x480 only!")
               .arg(width).arg(height).toLocal8Bit().constData());
    
    // Check if already cached
    QString cacheKey = makeLUTCacheKey(make, model, width, height);
    {
        QMutexLocker locker(&lutCacheMutex);
        if (lutCache.contains(cacheKey)) {
            qDebug() << "Priority LUT cache hit for:" << make << model << QString("%1x%2").arg(width).arg(height);
            return lutCache[cacheKey];
        }
    }
    
    // Not cached - request priority generation
    if (backgroundGenerator) {
        qDebug() << "Requesting priority LUT generation for:" << make << model << QString("%1x%2").arg(width).arg(height);
        backgroundGenerator->requestPriorityLUT(make, model, width, height);
        
        // Wait briefly for priority generation (non-blocking UI approach)
        // If still not ready, fall back to immediate synchronous generation
        QThread::msleep(100); // Brief wait
        
        if (lutCache.contains(cacheKey)) {
            return lutCache[cacheKey];
        }
    }
    
    // Fallback to immediate generation if background system not ready
    qDebug() << "Fallback to immediate LUT generation for:" << make << model << QString("%1x%2").arg(width).arg(height);
    return getCachedLUT(make, model, width, height, parent);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
// LAULookUpTableGenerator Implementation
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTableGenerator::LAULookUpTableGenerator(QObject *parent)
    : QThread(parent), shouldStop(false), isPaused(false), backgroundComplete(false)
{
}

LAULookUpTableGenerator::~LAULookUpTableGenerator()
{
    qDebug() << "Destroying LAULookUpTableGenerator...";
    stopGeneration();
    
    // Ensure thread is completely stopped before destructor completes
    if (isRunning()) {
        qDebug() << "Thread still running during destruction, forcing shutdown...";
        quit();
        
        // Wait for graceful quit while processing events
        int quitAttempts = 0;
        while (isRunning() && quitAttempts < 200) { // ~2 seconds at 10ms intervals
            QApplication::processEvents();
            QThread::msleep(10);
            quitAttempts++;
        }
        
        if (!isRunning()) {
            qDebug() << "Thread quit gracefully";
        } else {
            qWarning() << "Thread did not quit gracefully, terminating...";
            terminate();
            
            // Wait for termination while processing events
            int terminateAttempts = 0;
            while (isRunning() && terminateAttempts < 100) { // ~1 second
                QApplication::processEvents();
                QThread::msleep(10);
                terminateAttempts++;
            }
            
            if (!isRunning()) {
                qDebug() << "Thread terminated successfully";
            } else {
                qCritical() << "CRITICAL: Thread failed to terminate in destructor!";
            }
        }
    }
    qDebug() << "LAULookUpTableGenerator destruction completed";
}

void LAULookUpTableGenerator::startBackgroundGeneration()
{
    QMutexLocker locker(&queueMutex);
    shouldStop = false;
    isPaused = false;
    backgroundComplete = false;
    
    if (!isRunning()) {
        start();
    }
}

void LAULookUpTableGenerator::requestPriorityLUT(const QString &make, const QString &model, int width, int height)
{
    QMutexLocker locker(&queueMutex);
    LAULUTGenerationTask priorityTask(make, model, width, height, true);
    priorityQueue.enqueue(priorityTask);
    taskAvailable.wakeOne();
}

void LAULookUpTableGenerator::pauseGeneration()
{
    QMutexLocker locker(&queueMutex);
    isPaused = true;
}

void LAULookUpTableGenerator::resumeGeneration()
{
    QMutexLocker locker(&queueMutex);
    isPaused = false;
    pauseCondition.wakeAll();
}

void LAULookUpTableGenerator::stopGeneration()
{
    {
        QMutexLocker locker(&queueMutex);
        shouldStop = true;
        isPaused = false;
        // Clear the queues to prevent further processing
        backgroundQueue.clear();
        priorityQueue.clear();
        taskAvailable.wakeAll();
        pauseCondition.wakeAll();
    }
    // Request thread interruption
    requestInterruption();
}

void LAULookUpTableGenerator::run()
{
    generateBackgroundTasks();
    
    while (!shouldStop && !isInterruptionRequested()) {
        LAULUTGenerationTask task = getNextTask();
        
        if (task.make.isEmpty()) {
            // No tasks available, wait
            QMutexLocker locker(&queueMutex);
            if (!shouldStop && backgroundQueue.isEmpty() && priorityQueue.isEmpty()) {
                taskAvailable.wait(&queueMutex, 1000); // Wait up to 1 second
            }
            continue;
        }
        
        // Check for interruption before processing
        if (isInterruptionRequested()) {
            break;
        }
        
        // Handle pause for priority tasks
        {
            QMutexLocker locker(&queueMutex);
            while (isPaused && !shouldStop && !isInterruptionRequested()) {
                pauseCondition.wait(&queueMutex);
            }
        }
        
        if (!shouldStop && !isInterruptionRequested()) {
            processTask(task);
        }
    }
}

void LAULookUpTableGenerator::generateBackgroundTasks()
{
    QMutexLocker locker(&queueMutex);
    
    // Get all cameras from inventory
    QList<LAUCameraCalibration> cameras = LAUCameraInventoryDialog::getAllCameraCalibrations();
    
    // Generate tasks for each camera at standard dimensions
    for (const LAUCameraCalibration &camera : cameras) {
        for (const QSize &dimensions : STANDARD_DIMENSIONS) {
            // Check if already cached
            QString cacheKey = LAUCameraInventoryDialog::makeLUTCacheKey(
                camera.make, camera.model, dimensions.width(), dimensions.height());
            
            {
                QMutexLocker cacheLock(&LAUCameraInventoryDialog::lutCacheMutex);
                if (!LAUCameraInventoryDialog::lutCache.contains(cacheKey)) {
                    LAULUTGenerationTask task(camera.make, camera.model, 
                                            dimensions.width(), dimensions.height(), false);
                    backgroundQueue.enqueue(task);
                }
            }
        }
    }
    
    qDebug() << "Generated" << backgroundQueue.size() << "background LUT generation tasks";
}

LAULUTGenerationTask LAULookUpTableGenerator::getNextTask()
{
    QMutexLocker locker(&queueMutex);
    
    // Priority queue first
    if (!priorityQueue.isEmpty()) {
        return priorityQueue.dequeue();
    }
    
    // Then background queue
    if (!backgroundQueue.isEmpty()) {
        return backgroundQueue.dequeue();
    }
    
    // No tasks
    if (!backgroundComplete && backgroundQueue.isEmpty() && priorityQueue.isEmpty()) {
        backgroundComplete = true;
        emit backgroundGenerationComplete();
    }
    
    return LAULUTGenerationTask(); // Empty task
}

void LAULookUpTableGenerator::processTask(const LAULUTGenerationTask &task)
{
    // Check if we should stop before doing any work
    if (shouldStop || isInterruptionRequested()) {
        return;
    }
    
    // Check if already cached (might have been generated by another request)
    QString cacheKey = LAUCameraInventoryDialog::makeLUTCacheKey(task.make, task.model, task.width, task.height);
    {
        QMutexLocker locker(&LAUCameraInventoryDialog::lutCacheMutex);
        if (LAUCameraInventoryDialog::lutCache.contains(cacheKey)) {
            return; // Already done
        }
    }
    
    // Check again if we should stop before starting expensive operation
    if (shouldStop || isInterruptionRequested()) {
        return;
    }
    
    // Generate the LUT (now safe to call from background thread with nullptr parent)
    LAUCameraCalibration calibration = LAUCameraInventoryDialog::getCameraCalibration(task.make, task.model);
    if (calibration.isValid()) {
        // Final check before creating LUT - this is where the crash occurs
        if (shouldStop || isInterruptionRequested()) {
            return;
        }

        // Use invalid date for background generation - this will preserve old behavior (always rotate Femto Mega)
        LAULookUpTable lut = LAULookUpTable::generateTableFromJETR(
            task.width, task.height, calibration.jetrVector, task.make, task.model, QDate(), nullptr);

        // Check if we should stop before caching (in case generation took a while)
        if (shouldStop || isInterruptionRequested()) {
            return;
        }

        if (lut.isValid()) {
            // Cache the generated LUT
            LAUCameraInventoryDialog::cacheLUT(task.make, task.model, task.width, task.height, lut);
            emit lutGenerated(task.make, task.model, task.width, task.height);
        }
    }
}
