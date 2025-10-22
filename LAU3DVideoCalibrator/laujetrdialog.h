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

#ifndef LAUJETRDIALOG_H
#define LAUJETRDIALOG_H

#include <QtGui>
#include <QtCore>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>

#include "laujetrwidget.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUJETRDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUJETRDialog(QWidget *parent = nullptr);
    explicit LAUJETRDialog(const QString &filename, QWidget *parent = nullptr);
    ~LAUJETRDialog();

    // Get the JETR vectors from all tabs
    QList<QVector<double>> getJETRVectors() const;
    
    // Get the currently active JETR vector
    QVector<double> getCurrentJETRVector() const;
    
    // Set initial JETR data
    void setJETRVectors(const QList<QVector<double>> &vectors);
    void setJETRVector(const QVector<double> &vector);
    
    // Get make/model/position/rotation information for each camera
    QStringList getMakes() const;
    QStringList getModels() const;
    QStringList getPositions() const;
    QList<bool> getRotations() const;
    
    // Get the JETR vector from the camera marked as "top" position
    QVector<double> getTopCameraJETRVector() const;
    int getTopCameraIndex() const;

    // Auto-import functionality
    void checkAutoImport();
    
    // Direct import for LUT/LUTX files
    bool importLUTsDirectly();
    
    // Set dialog to display-only mode (read-only, hide Load button)
    void setDisplayMode(bool displayMode = true);
    
    // Set dialog to preloaded mode (editable, hide Load button)
    void setPreloadedMode(bool preloadedMode = true);
    
    // Set memory object for depth data operations - must be called before showing dialog
    void setMemoryObject(const LAUMemoryObject &memObject) { memoryObject = memObject; }
    
    // Set the TIFF filename for bounding box editing
    void setTiffFilename(const QString &filename) { tiffFilename = filename; }
    QString getTiffFilename() const { return tiffFilename; }
    
    // Set memory object on all JETR widgets for 3D operations
    void setMemoryObjectOnAllWidgets(const LAUMemoryObject &memObject);
    
    // Tab management
    void clearTabs();
    void addJETRTab(const QVector<double> &jetrVector, const QString &tabTitle = QString());
    void addJETRTab(const QVector<double> &jetrVector, const QString &make, const QString &model, const QString &tabTitle = QString());
    void setTabMetadata(const QStringList &makes, const QStringList &models, const QStringList &positions, const QList<bool> &rotations);
    
    // Check if import was cancelled
    bool wasImportCancelled() const { return importCancelled; }
    
    // Preflight check - returns true if dialog should be shown, false if user cancelled
    static bool preflight(const QString &filename, QWidget *parent = nullptr);

public slots:
    void onImportClicked();
    void onExportCSVClicked();
    void onInventoryClicked();
    void onJETRVectorChanged(const QVector<double> &vector);
    void onEditBoundingBox();
    void reject() override;
    // Server-only features - removed for standalone build
    // void onResetClicked();
    // void onGenerateHistoryClicked();
    // void onImportFromHistoryClicked();
    // void setDateContext(const QString &folderName);

protected:
    void setupUI();
    void showEvent(QShowEvent *event) override;
    void updateButtonVisibility();
    
    // Bounding box methods
    void applyBoundingBoxToAllTabs(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
    
    // Import handlers
    void importMemoryObject(const QString &filename);
    void importMemoryObjectDirect(const QString &filename); // Direct import without user dialogs
    void importLookUpTable(const QString &filename);
    void importLUTX(const QString &filename);

    // Server-only features - removed for standalone build
    // QStringList findExistingHistoryFiles(const QString &rootDirectory);
    // QString findPythonScript();
    // void runPythonScriptWithVenv(const QString &scriptPath, const QString &rootDirectory);
    // QString createVenvWrapperScript(const QString &scriptPath, const QString &rootDirectory, const QString &venvDir);
    // QStringList readHistoryCSV(const QString &filename);
    // QString findSampleTiffFile(const QString &baseDir, const QString &serial);

    // File type detection
    enum FileType {
        FileTypeUnknown,
        FileTypeMemoryObject,
        FileTypeLookUpTable,
        FileTypeLUTX
    };
    FileType detectFileType(const QString &filename) const;

private:
    QVBoxLayout *mainLayout;
    QTabWidget *tabWidget;
    QDialogButtonBox *buttonBox;
    QPushButton *importButton;
    QPushButton *acceptButton;   // Import button
    QPushButton *rejectButton;   // Discard button
    // QPushButton *resetButton;    // Reset button (server-only)
    QLabel *infoLabel;

    QList<LAUJETRWidget*> jetrWidgets;
    QStringList currentMakes;
    QStringList currentModels;
    QStringList currentPositions;
    QList<bool> currentRotations;
    QString tiffFilename; // TIFF filename for bounding box editing
    // QString dateFolder;   // Date folder context for calibration (server-only)
    bool showLoadButton;  // Whether to show the load button
    bool importCancelled; // Whether import was cancelled by user
    LAUMemoryObject memoryObject; // Memory object for depth data operations
    bool hasUnsavedChanges; // Track if user made changes since load/save
    QList<QVector<double>> originalJETRVectors; // Original state for change detection
};

#endif // LAUJETRDIALOG_H
