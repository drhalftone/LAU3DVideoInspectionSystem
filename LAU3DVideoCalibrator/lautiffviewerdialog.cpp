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

#include "lautiffviewerdialog.h"
#include <QMessageBox>
#include <QStandardPaths>
#include <QSettings>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUTiffViewerDialog::LAUTiffViewerDialog(QWidget *parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_tiffViewer(nullptr)
    , m_buttonBox(nullptr)
{
    setWindowTitle("3D Bounding Box Definition");
    setModal(true);
    resize(1200, 800);

    setupUi();
    
    // Restore dialog geometry from QSettings
    QSettings settings;
    settings.beginGroup("DialogGeometry");
    QByteArray geometry = settings.value("LAUTiffViewerDialog/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    settings.endGroup();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUTiffViewerDialog::~LAUTiffViewerDialog()
{
    // Save dialog geometry to QSettings
    QSettings settings;
    settings.beginGroup("DialogGeometry");
    settings.setValue("LAUTiffViewerDialog/geometry", saveGeometry());
    settings.endGroup();
    
    // Qt will handle cleanup of child widgets
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewerDialog::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(6, 6, 6, 6);
    m_mainLayout->setSpacing(6);

    // Create the TIFF viewer widget
    m_tiffViewer = new LAUTiffViewer(this);
    m_mainLayout->addWidget(m_tiffViewer);

    // Create button box with OK and Cancel
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_mainLayout->addWidget(m_buttonBox);

    // Connect button box signals
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &LAUTiffViewerDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &LAUTiffViewerDialog::reject);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewerDialog::setLookupTables(const QList<LAULookUpTable> &tables)
{
    m_lookupTables = tables;
    applyLookupTablesToViewer();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewerDialog::setTiffFilename(const QString &filename)
{
    m_tiffFilename = filename;
    
    if (m_tiffViewer && !filename.isEmpty()) {
        // Load the TIFF file into the viewer
        m_tiffViewer->loadTiffFile(filename);
        
        // Apply lookup tables if we have them
        applyLookupTablesToViewer();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewerDialog::applyLookupTablesToViewer()
{
    if (!m_tiffViewer || m_lookupTables.isEmpty()) {
        return;
    }

    // Create a temporary LUTX file to pass to the viewer
    QString tempLutxFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/temp_jetr_bounding_box.lutx";

    if (LAULookUpTable::saveLookUpTables(m_lookupTables, tempLutxFile)) {
        // Load the lookup tables into the viewer
        m_tiffViewer->loadLookupTables(tempLutxFile);

        // Apply bounding box values from the lookup tables to the viewer
        // Check if any lookup table has valid bounding box data
        bool foundValidBoundingBox = false;
        LookUpTableBoundingBox bbox;

        for (const LAULookUpTable &table : m_lookupTables) {
            LookUpTableBoundingBox tableBbox = table.boundingBox();

            // Check if this bounding box has non-zero/non-default values
            if (tableBbox.xMin != 0.0 || tableBbox.xMax != 0.0 ||
                tableBbox.yMin != 0.0 || tableBbox.yMax != 0.0 ||
                tableBbox.zMin != 0.0 || tableBbox.zMax != 0.0) {
                bbox = tableBbox;
                foundValidBoundingBox = true;
                break; // Use the first valid bounding box found
            }
        }

        // Apply bounding box to viewer
        if (foundValidBoundingBox) {
            m_tiffViewer->setBoundingBox(bbox);
        }
        // If no valid bounding box found, the viewer will use its default values
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUTiffViewer* LAUTiffViewerDialog::tiffViewer() const
{
    return m_tiffViewer;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LookUpTableBoundingBox LAUTiffViewerDialog::getBoundingBox() const
{
    LookUpTableBoundingBox bbox;

    if (m_tiffViewer) {
        // Extract bounding box values from the TIFF viewer using the getter methods
        bbox.xMin = m_tiffViewer->getBoundingBoxXMin();
        bbox.xMax = m_tiffViewer->getBoundingBoxXMax();
        bbox.yMin = m_tiffViewer->getBoundingBoxYMin();
        bbox.yMax = m_tiffViewer->getBoundingBoxYMax();
        bbox.zMin = m_tiffViewer->getBoundingBoxZMin();
        bbox.zMax = m_tiffViewer->getBoundingBoxZMax();
    } else {
        // Default values if no viewer available
        bbox.xMin = -1000.0;
        bbox.xMax = 1000.0;
        bbox.yMin = -1000.0;
        bbox.yMax = 1000.0;
        bbox.zMin = 500.0;
        bbox.zMax = 3000.0;
    }

    return bbox;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTiffViewerDialog::accept()
{
    // Validate that we have valid bounding box settings
    LookUpTableBoundingBox bbox = getBoundingBox();

    if (bbox.xMin >= bbox.xMax || bbox.yMin >= bbox.yMax || bbox.zMin >= bbox.zMax) {
        QMessageBox::warning(this, "Invalid Bounding Box",
                             "Invalid bounding box values. Please ensure min values are less than max values.");
        return;
    }

    // If validation passes, accept the dialog
    QDialog::accept();
}