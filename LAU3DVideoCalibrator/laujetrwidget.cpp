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

#include "laujetrwidget.h"
#include "laumatrixtable.h"
#include "../Calibration/lausetxyplanewidget.h"
#include "../Support/lauscan.h"
#include "../Support/lautransformeditorwidget.h"
#include "../Support/lauconstants.h"
#include "laucamerainventorydialog.h"
#include "lautiffviewer.h"
#include <QProgressDialog>
#include <QScrollArea>
#include <QLabel>
#include <QPixmap>
#include <QRegularExpression>
#include <limits>
#ifdef ENABLEPOINTMATCHER
#include "../Merge/laumergescanwidget.h"
#endif

// Define static member
LAUScan LAUJETRWidget::masterScan;
#include <QDoubleValidator>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QSettings>
#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QTabWidget>
#include <QFormLayout>
#include <QCheckBox>
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include "laucameraselectiondialog.h"
#include <limits>
#include <cmath>
#include <algorithm>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUJETRWidget::LAUJETRWidget(const QVector<double> &jetrVector, QWidget *parent)
    : QWidget(parent), jetrVector(jetrVector), readOnlyMode(false), suppressChangeSignals(false), deferMasterScanGeneration(true)
{
    setupUI();
    updateAllDisplaysFromVector();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUJETRWidget::LAUJETRWidget(QWidget *parent)
    : QWidget(parent), jetrVector(LAU_JETR_VECTOR_SIZE, NAN), readOnlyMode(false), suppressChangeSignals(false), deferMasterScanGeneration(true)
{
    setupUI();
    updateAllDisplaysFromVector();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUJETRWidget::~LAUJETRWidget()
{
    // Qt handles cleanup automatically
    qDebug() << this->geometry();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setupUI()
{
    setWindowTitle("JETR Vector Parameters");
    setMinimumWidth(700);

    // Main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);  // Add spacing between group boxes

    // Create parameter groups directly in main layout
    createCameraInfoGroup();
    createIntrinsicsGroup();
    createExtrinsicsGroup();
    createBoundingBoxGroup();
    createDepthProcessingGroup();
    createPreviewButtonsGroup();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::createIntrinsicsGroup()
{
    intrinsicsGroupBox = new QGroupBox("Intrinsic Parameters (Camera Internal Calibration)");
    QGridLayout *layout = new QGridLayout(intrinsicsGroupBox);
    layout->setContentsMargins(6, 6, 6, 6);

    // Set column stretch factors to match Camera Info group box
    layout->setColumnStretch(0, 0);  // Label column (no stretch)
    layout->setColumnStretch(1, 1);  // Left widget column (stretch factor 1)
    layout->setColumnStretch(2, 0);  // Label column (no stretch)
    layout->setColumnStretch(3, 1);  // Right widget column (stretch factor 1)

    // Create validator for double values
    QDoubleValidator *validator = new QDoubleValidator(this);
    validator->setDecimals(10);
    validator->setNotation(QDoubleValidator::ScientificNotation);

    // Focal lengths and principal point
    layout->addWidget(new QLabel("Focal Length X (fx):"), 0, 0);
    fxLineEdit = new QLineEdit();
    fxLineEdit->setReadOnly(true);
    fxLineEdit->setToolTip("Horizontal focal length in pixels");
    layout->addWidget(fxLineEdit, 0, 1);

    layout->addWidget(new QLabel("Principal Point X (cx):"), 0, 2);
    cxLineEdit = new QLineEdit();
    cxLineEdit->setReadOnly(true);
    cxLineEdit->setToolTip("Horizontal optical center in pixels");
    layout->addWidget(cxLineEdit, 0, 3);

    layout->addWidget(new QLabel("Focal Length Y (fy):"), 1, 0);
    fyLineEdit = new QLineEdit();
    fyLineEdit->setReadOnly(true);
    fyLineEdit->setToolTip("Vertical focal length in pixels");
    layout->addWidget(fyLineEdit, 1, 1);

    layout->addWidget(new QLabel("Principal Point Y (cy):"), 1, 2);
    cyLineEdit = new QLineEdit();
    cyLineEdit->setReadOnly(true);
    cyLineEdit->setToolTip("Vertical optical center in pixels");
    layout->addWidget(cyLineEdit, 1, 3);

    // Radial distortion coefficients
    layout->addWidget(new QLabel("Radial Distortion Coefficients:"), 2, 0, 1, 4);

    QLabel *k1Label = new QLabel("k1:");
    k1Label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(k1Label, 3, 0);
    k1LineEdit = new QLineEdit();
    k1LineEdit->setReadOnly(true);
    k1LineEdit->setToolTip("First radial distortion coefficient");
    layout->addWidget(k1LineEdit, 3, 1);

    QLabel *k2Label = new QLabel("k2:");
    k2Label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(k2Label, 3, 2);
    k2LineEdit = new QLineEdit();
    k2LineEdit->setReadOnly(true);
    k2LineEdit->setToolTip("Second radial distortion coefficient");
    layout->addWidget(k2LineEdit, 3, 3);

    QLabel *k3Label = new QLabel("k3:");
    k3Label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(k3Label, 4, 0);
    k3LineEdit = new QLineEdit();
    k3LineEdit->setReadOnly(true);
    k3LineEdit->setToolTip("Third radial distortion coefficient");
    layout->addWidget(k3LineEdit, 4, 1);

    QLabel *k4Label = new QLabel("k4:");
    k4Label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(k4Label, 4, 2);
    k4LineEdit = new QLineEdit();
    k4LineEdit->setReadOnly(true);
    k4LineEdit->setToolTip("Fourth radial distortion coefficient");
    layout->addWidget(k4LineEdit, 4, 3);

    QLabel *k5Label = new QLabel("k5:");
    k5Label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(k5Label, 5, 0);
    k5LineEdit = new QLineEdit();
    k5LineEdit->setReadOnly(true);
    k5LineEdit->setToolTip("Fifth radial distortion coefficient");
    layout->addWidget(k5LineEdit, 5, 1);

    QLabel *k6Label = new QLabel("k6:");
    k6Label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(k6Label, 5, 2);
    k6LineEdit = new QLineEdit();
    k6LineEdit->setReadOnly(true);
    k6LineEdit->setToolTip("Sixth radial distortion coefficient");
    layout->addWidget(k6LineEdit, 5, 3);

    // Tangential distortion coefficients
    layout->addWidget(new QLabel("Tangential Distortion Coefficients:"), 6, 0, 1, 4);

    QLabel *p1Label = new QLabel("p1:");
    p1Label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(p1Label, 7, 0);
    p1LineEdit = new QLineEdit();
    p1LineEdit->setReadOnly(true);
    p1LineEdit->setToolTip("First tangential distortion coefficient");
    layout->addWidget(p1LineEdit, 7, 1);

    QLabel *p2Label = new QLabel("p2:");
    p2Label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(p2Label, 7, 2);
    p2LineEdit = new QLineEdit();
    p2LineEdit->setReadOnly(true);
    p2LineEdit->setToolTip("Second tangential distortion coefficient");
    layout->addWidget(p2LineEdit, 7, 3);

    // No need to connect signals since intrinsic parameters are read-only

    mainLayout->addWidget(intrinsicsGroupBox);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::createExtrinsicsGroup()
{
    // Create the group box and assign it a VBox layout
    extrinsicsGroupBox = new QGroupBox("Extrinsic Parameters and 3D Bounding Box");
    QVBoxLayout *mainVLayout = new QVBoxLayout(extrinsicsGroupBox);
    mainVLayout->setContentsMargins(6, 6, 6, 6);

    // Top widget with HBox layout for transform matrix and bounding box
    QWidget *topWidget = new QWidget();
    QHBoxLayout *topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);  // No margins for internal widgets

    // First widget: Transform matrix (VBox layout) - NO BUTTON
    QWidget *transformWidget = new QWidget();
    QVBoxLayout *transformLayout = new QVBoxLayout(transformWidget);
    transformLayout->setContentsMargins(0, 0, 0, 0);  // No margins for internal widgets

    // Add 4x4 transform label (centered)
    QLabel *transformLabel = new QLabel("4x4 Transform Matrix:");
    transformLayout->addWidget(transformLabel, 0, Qt::AlignHCenter);

    // Create 4x4 matrix table
    LAUMatrixTable *matrixTable = new LAUMatrixTable(this);
    matrixTable->setRowCount(4);
    matrixTable->setColumnCount(4);
    matrixTable->setFixedSize(302, 122);
    matrixTable->horizontalHeader()->hide();
    matrixTable->verticalHeader()->hide();
    matrixTable->setShowGrid(true);
    matrixTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    matrixTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Set uniform column widths
    for (int col = 0; col < 4; ++col) {
        matrixTable->setColumnWidth(col, 75);
    }

    // Set uniform row heights
    for (int row = 0; row < 4; ++row) {
        matrixTable->setRowHeight(row, 30);
    }

    // Create validator for matrix elements
    QDoubleValidator *validator = new QDoubleValidator(this);
    validator->setDecimals(10);
    validator->setNotation(QDoubleValidator::ScientificNotation);

    // Add line edits to matrix table
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int index = row * 4 + col;
            matrixLineEdits[index] = new QLineEdit();
            matrixLineEdits[index]->setReadOnly(true);
            matrixLineEdits[index]->setAlignment(Qt::AlignCenter);
            matrixTable->setCellWidget(row, col, matrixLineEdits[index]);
        }
    }

    // Center the matrix table
    transformLayout->addWidget(matrixTable, 0, Qt::AlignHCenter);

    // Add transform widget to top layout
    topLayout->addWidget(transformWidget, 1, Qt::AlignTop);

    // Second widget: Bounding box (VBox layout) - NO BUTTON
    boundingBoxWidget = new QWidget();
    boundingBoxLayout = new QVBoxLayout(boundingBoxWidget);
    boundingBoxLayout->setContentsMargins(0, 0, 0, 0);  // No margins for internal widgets

    // Bounding box content will be added in createBoundingBoxGroup()

    // Add bounding box widget to top layout
    topLayout->addWidget(boundingBoxWidget, 1, Qt::AlignTop);

    // Add top widget to main layout
    mainVLayout->addWidget(topWidget);

    // Bottom widget with HBox layout for buttons
    QWidget *buttonWidget = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(0, 0, 0, 0);  // No margins for internal widgets

    // Add edit transform button
    editTransformButton = new QPushButton("Edit Transform Matrix...");
    editTransformButton->setToolTip("Open advanced transform matrix editor");
    connect(editTransformButton, &QPushButton::clicked, this, &LAUJETRWidget::onEditTransformMatrix);
    buttonLayout->addWidget(editTransformButton, 1);

    // Add edit bounding box button
    editBoundingBoxButton = new QPushButton("Edit Bounding Box...");
    editBoundingBoxButton->setToolTip("Open visual bounding box editor with 3D preview");
    connect(editBoundingBoxButton, &QPushButton::clicked, this, &LAUJETRWidget::onEditBoundingBox);
    buttonLayout->addWidget(editBoundingBoxButton, 1);

    // Add button widget to main layout
    mainVLayout->addWidget(buttonWidget);

    // Add group box to main layout
    mainLayout->addWidget(extrinsicsGroupBox);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::createBoundingBoxGroup()
{
    // Add bounding box content to boundingBoxLayout created in createExtrinsicsGroup

    // Add 3D Bounding Box label
    QLabel *bboxLabel = new QLabel("3D Bounding Box:");
    boundingBoxLayout->addWidget(bboxLabel, 0, Qt::AlignTop);

    // Create table for bounding box (3 rows x 2 columns)
    QTableWidget *bboxTable = new QTableWidget(3, 2, this);

    // Disable scrollbars and adjust size to fit content
    bboxTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bboxTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bboxTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    // Set fixed height to fit exactly 3 rows plus headers
    bboxTable->setMaximumHeight(130);  // Slightly taller to accommodate content
    bboxTable->setMinimumHeight(130);

    // Set headers
    bboxTable->setHorizontalHeaderLabels(QStringList() << "Min" << "Max");
    bboxTable->setVerticalHeaderLabels(QStringList() << "X" << "Y" << "Z");

    // Create line edits and add to table
    xMinLineEdit = new QLineEdit();
    xMinLineEdit->setReadOnly(true);
    xMinLineEdit->setAlignment(Qt::AlignCenter);
    xMinLineEdit->setToolTip("Minimum X coordinate in millimeters");
    bboxTable->setCellWidget(0, 0, xMinLineEdit);

    xMaxLineEdit = new QLineEdit();
    xMaxLineEdit->setReadOnly(true);
    xMaxLineEdit->setAlignment(Qt::AlignCenter);
    xMaxLineEdit->setToolTip("Maximum X coordinate in millimeters");
    bboxTable->setCellWidget(0, 1, xMaxLineEdit);

    yMinLineEdit = new QLineEdit();
    yMinLineEdit->setReadOnly(true);
    yMinLineEdit->setAlignment(Qt::AlignCenter);
    yMinLineEdit->setToolTip("Minimum Y coordinate in millimeters");
    bboxTable->setCellWidget(1, 0, yMinLineEdit);

    yMaxLineEdit = new QLineEdit();
    yMaxLineEdit->setReadOnly(true);
    yMaxLineEdit->setAlignment(Qt::AlignCenter);
    yMaxLineEdit->setToolTip("Maximum Y coordinate in millimeters");
    bboxTable->setCellWidget(1, 1, yMaxLineEdit);

    zMinLineEdit = new QLineEdit();
    zMinLineEdit->setReadOnly(true);
    zMinLineEdit->setAlignment(Qt::AlignCenter);
    zMinLineEdit->setToolTip("Minimum Z coordinate in millimeters");
    bboxTable->setCellWidget(2, 0, zMinLineEdit);

    zMaxLineEdit = new QLineEdit();
    zMaxLineEdit->setReadOnly(true);
    zMaxLineEdit->setAlignment(Qt::AlignCenter);
    zMaxLineEdit->setToolTip("Maximum Z coordinate in millimeters");
    bboxTable->setCellWidget(2, 1, zMaxLineEdit);

    // Set equal column widths
    bboxTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    boundingBoxLayout->addWidget(bboxTable);

    // No separate group box needed - integrated into extrinsics combined group
    boundingBoxGroupBox = nullptr;

    // Buttons are now in the bottom widget created in createExtrinsicsGroup
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::createDepthProcessingGroup()
{
    // Depth processing parameters now integrated into Camera Info group
    // This method is kept for compatibility but does nothing
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::createPreviewButtonsGroup()
{
    previewGroupBox = new QGroupBox("Preview Options");
    previewGroupBox->setLayout(new QHBoxLayout());
    previewGroupBox->layout()->setContentsMargins(6, 6, 6, 6);

    // Raw Image Preview Button
    QPushButton *rawImageButton = new QPushButton("Preview Raw Image");
    rawImageButton->setToolTip("View the raw depth/RGB image from the camera");
    rawImageButton->setMinimumHeight(40);
    connect(rawImageButton, &QPushButton::clicked, this, &LAUJETRWidget::onPreviewRawImage);
    previewGroupBox->layout()->addWidget(rawImageButton);
    
    // 3D Scan Preview Button  
    QPushButton *scanPreviewButton = new QPushButton("Preview 3D Scan");
    scanPreviewButton->setToolTip("Generate and view 3D point cloud using current JETR calibration");
    scanPreviewButton->setMinimumHeight(40);
    connect(scanPreviewButton, &QPushButton::clicked, this, &LAUJETRWidget::onPreview3DScan);
    previewGroupBox->layout()->addWidget(scanPreviewButton);
    
    mainLayout->addWidget(previewGroupBox);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::createCameraInfoGroup()
{
    cameraInfoGroupBox = new QGroupBox("Camera Information and Depth Processing");
    QHBoxLayout *hboxLayout = new QHBoxLayout(cameraInfoGroupBox);
    hboxLayout->setContentsMargins(6, 6, 6, 6);

    // Left widget: Camera info (Make, Model, Position, Rotate)
    QWidget *leftWidget = new QWidget();
    QFormLayout *leftLayout = new QFormLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    makeComboBox = new QComboBox();
    makeComboBox->setEditable(true);
    makeComboBox->addItem("Unknown");
    QStringList makes = getAvailableMakes();
    makeComboBox->addItems(makes);
    makeComboBox->setCurrentText("Unknown");
    makeComboBox->setToolTip("Select the camera manufacturer (e.g., Intel, Microsoft, Azure)");
    leftLayout->addRow("Make:", makeComboBox);

    modelComboBox = new QComboBox();
    modelComboBox->setEditable(true);
    modelComboBox->addItem("Unknown");
    modelComboBox->setCurrentText("Unknown");
    modelComboBox->setToolTip("Select the camera model (e.g., D415, Kinect v2)");
    leftLayout->addRow("Model:", modelComboBox);

    positionComboBox = new QComboBox();
    positionComboBox->addItem("Top", "A TOP");
    positionComboBox->addItem("Side", "B SIDE");
    positionComboBox->addItem("Bottom", "C BOTTOM");
    positionComboBox->addItem("Front", "D FRONT");
    positionComboBox->addItem("Back", "E BACK");
    positionComboBox->addItem("Quarter", "F QUARTER");
    positionComboBox->addItem("Rump", "G RUMP");
    positionComboBox->addItem("Unknown", "H UNKNOWN");
    positionComboBox->setCurrentText("Unknown");
    positionComboBox->setToolTip("Physical position of the camera relative to the subject");
    leftLayout->addRow("Position:", positionComboBox);

    rotateCheckBox = new QCheckBox("Rotate image by 180 degrees");
    rotateCheckBox->setToolTip("Check if the camera was mounted upside-down to rotate the image");
    leftLayout->addRow("", rotateCheckBox);

    // Right widget: Depth processing (Scale Factor, Min/Max Depth)
    QWidget *rightWidget = new QWidget();
    QFormLayout *rightLayout = new QFormLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    scaleFactorLineEdit = new QLineEdit();
    scaleFactorLineEdit->setReadOnly(true);
    scaleFactorLineEdit->setToolTip("Multiplier to convert raw depth values to millimeters");
    rightLayout->addRow("Scale Factor:", scaleFactorLineEdit);

    zMinDistanceLineEdit = new QLineEdit();
    zMinDistanceLineEdit->setReadOnly(true);
    zMinDistanceLineEdit->setToolTip("Minimum depth distance in millimeters (closer values are discarded)");
    rightLayout->addRow("Minimum Depth:", zMinDistanceLineEdit);

    zMaxDistanceLineEdit = new QLineEdit();
    zMaxDistanceLineEdit->setReadOnly(true);
    zMaxDistanceLineEdit->setToolTip("Maximum depth distance in millimeters (farther values are discarded)");
    rightLayout->addRow("Maximum Depth:", zMaxDistanceLineEdit);

    // Add stretch to right widget to push content to top
    rightLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    // Add widgets to HBox layout with equal stretch
    hboxLayout->addWidget(leftWidget, 1);
    hboxLayout->addWidget(rightWidget, 1);

    // Connect signals
    connect(makeComboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(onMakeChanged()));
    connect(modelComboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(onParameterChanged()));
    connect(positionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onPositionChanged()));
    connect(positionComboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(onParameterChanged()));
    connect(rotateCheckBox, SIGNAL(toggled(bool)), this, SLOT(onParameterChanged()));

    // Set size policy to prevent vertical expansion
    cameraInfoGroupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    mainLayout->addWidget(cameraInfoGroupBox);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setJETRVector(const QVector<double> &jetrVector, bool updateUI)
{
    if (validateJETRVector(jetrVector)) {
        this->jetrVector = jetrVector;
        if (updateUI) {
            updateAllDisplaysFromVector();
        }
        emit jetrVectorChanged(this->jetrVector);
    } else {
        // Fill with NaNs for invalid vector
        this->jetrVector = QVector<double>(37, NAN);
        if (updateUI) {
            updateAllDisplaysFromVector();
        }
    }
    
    // Update master scan if this is a "top" camera
    updateMasterScanIfTop();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setJETRVector(const LAULookUpTable &table, bool updateUI)
{
    if (!table.isValid()) {
        // Handle null table gracefully
        jetrVector = QVector<double>(37, NAN);
        if (updateUI) {
            updateAllDisplaysFromVector();
        }
        return;
    }

    QVector<double> tableJetr = table.jetr();
    if (!tableJetr.isEmpty() && validateJETRVector(tableJetr)) {
        // Table has valid JETR - use it (saving handled by camera inventory dialog)
        setJETRVector(tableJetr, updateUI);
    } else {
        // No JETR in table - try to load from Camera Inventory
        LAUCameraCalibration calibration = LAUCameraInventoryDialog::getCameraCalibration(table.makeString(), table.modelString());
        QVector<double> cachedJetr = calibration.jetrVector;

        if (!cachedJetr.isEmpty()) {
            // Found cached JETR - use it with default transform and bounding box
            QVector<double> fallbackJetr = cachedJetr;

            // Override transformation matrix with identity (elements 12-27)
            fallbackJetr[12] = 1.0; fallbackJetr[13] = 0.0; fallbackJetr[14] = 0.0; fallbackJetr[15] = 0.0;
            fallbackJetr[16] = 0.0; fallbackJetr[17] = 1.0; fallbackJetr[18] = 0.0; fallbackJetr[19] = 0.0;
            fallbackJetr[20] = 0.0; fallbackJetr[21] = 0.0; fallbackJetr[22] = 1.0; fallbackJetr[23] = 0.0;
            fallbackJetr[24] = 0.0; fallbackJetr[25] = 0.0; fallbackJetr[26] = 0.0; fallbackJetr[27] = 1.0;

            // Override bounding box with infinite bounds (elements 28-33)
            fallbackJetr[28] = -std::numeric_limits<double>::infinity(); // xMin
            fallbackJetr[29] = +std::numeric_limits<double>::infinity(); // xMax
            fallbackJetr[30] = -std::numeric_limits<double>::infinity(); // yMin
            fallbackJetr[31] = +std::numeric_limits<double>::infinity(); // yMax
            fallbackJetr[32] = -std::numeric_limits<double>::infinity(); // zMin
            fallbackJetr[33] = +std::numeric_limits<double>::infinity(); // zMax

            setJETRVector(fallbackJetr, updateUI);
        } else {
            // No cached data - fill with NaNs
            jetrVector = QVector<double>(37, NAN);
            if (updateUI) {
                updateAllDisplaysFromVector();
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setJETRVector(const LAUMemoryObject &memoryObject, int cameraIndex, const QString &make, const QString &model, bool updateUI)
{
    // Store memory object and camera index for later use (e.g., XY plane fitting)
    currentMemoryObject = memoryObject;
    currentCameraIndex = cameraIndex;
    
    if (!memoryObject.isValid()) {
        // Handle null object gracefully
        jetrVector = QVector<double>(37, NAN);
        if (updateUI) {
            updateAllDisplaysFromVector();
        }
        updateMasterScanIfTop();
        return;
    }

    QVector<double> objectJetr = memoryObject.jetr();

    if (!objectJetr.isEmpty()) {
        // Memory object has JETR data
        int numCameras = objectJetr.size() / 37;

        if (objectJetr.size() % 37 != 0 || numCameras == 0) {
            // Invalid JETR size
            jetrVector = QVector<double>(37, NAN);
            if (updateUI) {
                updateAllDisplaysFromVector();
            }
            updateMasterScanIfTop();
            return;
        }

        // Bounds check camera index
        if (cameraIndex < 0 || cameraIndex >= numCameras) {
            // Invalid camera index - fail gracefully
            updateMasterScanIfTop();
            return;
        }

        // Extract JETR for specific camera
        QVector<double> cameraJetr(37);
        int startIndex = cameraIndex * 37;
        for (int i = 0; i < 37; ++i) {
            cameraJetr[i] = objectJetr[startIndex + i];
        }

        if (validateJETRVector(cameraJetr)) {
            // Use the JETR data (saving handled by camera inventory dialog)
            setJETRVector(cameraJetr, updateUI);
        } else {
            jetrVector = QVector<double>(37, NAN);
            if (updateUI) {
                updateAllDisplaysFromVector();
            }
            updateMasterScanIfTop();
        }
    } else {
        // No JETR in memory object - try fallback with provided make/model
        if (!make.isEmpty() || !model.isEmpty()) {
            LAUCameraCalibration calibration = LAUCameraInventoryDialog::getCameraCalibration(make, model);
            QVector<double> cachedJetr = calibration.jetrVector;

            if (!cachedJetr.isEmpty()) {
                // Use cached JETR with defaults
                QVector<double> fallbackJetr = cachedJetr;

                // Override transformation matrix with identity
                fallbackJetr[12] = 1.0; fallbackJetr[13] = 0.0; fallbackJetr[14] = 0.0; fallbackJetr[15] = 0.0;
                fallbackJetr[16] = 0.0; fallbackJetr[17] = 1.0; fallbackJetr[18] = 0.0; fallbackJetr[19] = 0.0;
                fallbackJetr[20] = 0.0; fallbackJetr[21] = 0.0; fallbackJetr[22] = 1.0; fallbackJetr[23] = 0.0;
                fallbackJetr[24] = 0.0; fallbackJetr[25] = 0.0; fallbackJetr[26] = 0.0; fallbackJetr[27] = 1.0;

                // Override bounding box with infinite bounds
                fallbackJetr[28] = -std::numeric_limits<double>::infinity();
                fallbackJetr[29] = +std::numeric_limits<double>::infinity();
                fallbackJetr[30] = -std::numeric_limits<double>::infinity();
                fallbackJetr[31] = +std::numeric_limits<double>::infinity();
                fallbackJetr[32] = -std::numeric_limits<double>::infinity();
                fallbackJetr[33] = +std::numeric_limits<double>::infinity();

                setJETRVector(fallbackJetr, updateUI);
                return;
            }
        }

        // No cached data - fill with NaNs
        jetrVector = QVector<double>(37, NAN);
        if (updateUI) {
            updateAllDisplaysFromVector();
        }
    }
    
    // Update master scan if this is a "top" camera
    updateMasterScanIfTop();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector<double> LAUJETRWidget::getJETRVector() const
{
    return jetrVector;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setReadOnly(bool readOnly)
{
    readOnlyMode = readOnly;

    // Set all line edits to read-only
    fxLineEdit->setReadOnly(readOnly);
    cxLineEdit->setReadOnly(readOnly);
    fyLineEdit->setReadOnly(readOnly);
    cyLineEdit->setReadOnly(readOnly);
    k1LineEdit->setReadOnly(readOnly);
    k2LineEdit->setReadOnly(readOnly);
    k3LineEdit->setReadOnly(readOnly);
    k4LineEdit->setReadOnly(readOnly);
    k5LineEdit->setReadOnly(readOnly);
    k6LineEdit->setReadOnly(readOnly);
    p1LineEdit->setReadOnly(readOnly);
    p2LineEdit->setReadOnly(readOnly);

    for (int i = 0; i < 16; ++i) {
        matrixLineEdits[i]->setReadOnly(readOnly);
    }

    xMinLineEdit->setReadOnly(readOnly);
    xMaxLineEdit->setReadOnly(readOnly);
    yMinLineEdit->setReadOnly(readOnly);
    yMaxLineEdit->setReadOnly(readOnly);
    zMinLineEdit->setReadOnly(readOnly);
    zMaxLineEdit->setReadOnly(readOnly);

    scaleFactorLineEdit->setReadOnly(readOnly);
    zMinDistanceLineEdit->setReadOnly(readOnly);
    zMaxDistanceLineEdit->setReadOnly(readOnly);

    // Disable combo boxes and checkbox in read-only mode
    makeComboBox->setEnabled(!readOnly);
    modelComboBox->setEnabled(!readOnly);
    positionComboBox->setEnabled(!readOnly);
    rotateCheckBox->setEnabled(!readOnly);

    // Disable edit buttons in read-only mode
    if (editTransformButton) {
        editTransformButton->setEnabled(!readOnly);
    }
    if (editBoundingBoxButton) {
        editBoundingBoxButton->setEnabled(!readOnly);
    }

    // Hide preview group box in read-only mode (no memory object available)
    if (previewGroupBox) {
        previewGroupBox->setVisible(!readOnly);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRWidget::isReadOnly() const
{
    return readOnlyMode;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::onParameterChanged()
{
    if (!readOnlyMode) {
        updateVectorFromDisplay();

        // Only emit signal if not suppressed (e.g., during identity transform editing)
        if (!suppressChangeSignals) {
            emit jetrVectorChanged(jetrVector);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::onMakeChanged()
{
    // Update model combo box based on selected make
    QString selectedMake = makeComboBox->currentText();

    // Clear and repopulate model combo box
    modelComboBox->clear();
    modelComboBox->addItem("Unknown");

    if (!selectedMake.isEmpty() && selectedMake != "Unknown") {
        QStringList models = getAvailableModels(selectedMake);
        modelComboBox->addItems(models);
    }

    modelComboBox->setCurrentText("Unknown");

    // Call the regular parameter changed handler
    onParameterChanged();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::onPositionChanged()
{
    if (!positionComboBox) {
        return;
    }

    QString newPosition = positionComboBox->currentData().toString();

    // Check if user is trying to set this camera to "top"
    if (newPosition.toLower().endsWith("top")) {
        // Find if there's already a "top" camera among siblings
        QWidget *parentWidget = this->parentWidget();
        while (parentWidget && !qobject_cast<QDialog*>(parentWidget)) {
            parentWidget = parentWidget->parentWidget();
        }

        if (parentWidget) {
            QList<LAUJETRWidget*> siblings = parentWidget->findChildren<LAUJETRWidget*>();
            for (LAUJETRWidget *sibling : siblings) {
                if (sibling != this && sibling->getCameraPosition().toLower().endsWith("top")) {
                    // Another camera is already set to "top"
                    QMessageBox::warning(this, "Invalid Position",
                        "Only one camera (Camera 1) can be set to 'top' position.\n\n"
                        "Camera 1 is the reference camera and is already set to 'top'.\n\n"
                        "For this camera, please select a different position:\n"
                        "• 'side' - side view camera\n"
                        "• 'quarter' - three-quarter view\n"
                        "• 'rump' - rear view\n"
                        "• 'front' - front view\n"
                        "• 'back' - back view\n"
                        "• 'bottom' - bottom view");

                    // Revert to previous valid position (or "unknown" if it was just created)
                    positionComboBox->blockSignals(true);
                    int unknownIndex = positionComboBox->findData("H UNKNOWN");
                    if (unknownIndex >= 0) {
                        positionComboBox->setCurrentIndex(unknownIndex);
                    }
                    positionComboBox->blockSignals(false);
                    return;
                }
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::updateAllDisplaysFromVector()
{
    if (jetrVector.size() < 37) {
        return;
    }

    updateIntrinsicDisplays();
    updateTransformMatrixDisplay();
    updateBoundingBoxDisplays();
    updateDepthProcessingDisplays();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::updateIntrinsicDisplays()
{
    if (jetrVector.size() < 37) {
        return;
    }

    // Helper function to display NaN or value  
    auto displayValue = [](double value, int precision = 10) -> QString {
        if (std::isnan(value)) {
            return "NaN";
        } else if (std::isinf(value)) {
            if (value < 0) {
                return "-inf";
            } else {
                return "+inf";
            }
        } else {
            return QString::number(value, 'g', precision);
        }
    };

    // Update intrinsic parameters (elements 0-11) - no signal blocking needed since read-only
    fxLineEdit->setText(displayValue(jetrVector[0]));
    cxLineEdit->setText(displayValue(jetrVector[1]));
    fyLineEdit->setText(displayValue(jetrVector[2]));
    cyLineEdit->setText(displayValue(jetrVector[3]));
    k1LineEdit->setText(displayValue(jetrVector[4]));
    k2LineEdit->setText(displayValue(jetrVector[5]));
    k3LineEdit->setText(displayValue(jetrVector[6]));
    k4LineEdit->setText(displayValue(jetrVector[7]));
    k5LineEdit->setText(displayValue(jetrVector[8]));
    k6LineEdit->setText(displayValue(jetrVector[9]));
    p1LineEdit->setText(displayValue(jetrVector[10]));
    p2LineEdit->setText(displayValue(jetrVector[11]));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::updateBoundingBoxDisplays()
{
    if (jetrVector.size() < 37) {
        return;
    }

    // Helper function to display NaN or value
    auto displayValue = [](double value, int precision = 6) -> QString {
        if (std::isnan(value)) {
            return "NaN";
        } else if (std::isinf(value)) {
            if (value < 0) {
                return "-inf";
            } else {
                return "+inf";
            }
        } else {
            return QString::number(value, 'g', precision);
        }
    };

    // Update bounding box parameters (elements 28-33) - no signal blocking needed since read-only
    xMinLineEdit->setText(displayValue(jetrVector[28]));
    xMaxLineEdit->setText(displayValue(jetrVector[29]));
    yMinLineEdit->setText(displayValue(jetrVector[30]));
    yMaxLineEdit->setText(displayValue(jetrVector[31]));
    zMinLineEdit->setText(displayValue(jetrVector[32]));
    zMaxLineEdit->setText(displayValue(jetrVector[33]));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::updateDepthProcessingDisplays()
{
    if (jetrVector.size() < 37) {
        return;
    }

    // Helper function to display NaN or value
    auto displayValue = [](double value, int precision = 6) -> QString {
        if (std::isnan(value)) {
            return "NaN";
        } else if (std::isinf(value)) {
            if (value < 0) {
                return "-inf";
            } else {
                return "+inf";
            }
        } else {
            return QString::number(value, 'g', precision);
        }
    };

    // Update depth processing parameters (elements 34-36) - no signal blocking needed since read-only
    scaleFactorLineEdit->setText(displayValue(jetrVector[34]));
    zMinDistanceLineEdit->setText(displayValue(jetrVector[35]));
    zMaxDistanceLineEdit->setText(displayValue(jetrVector[36]));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::updateTransformMatrixDisplay()
{
    if (jetrVector.size() < 37) {
        return;
    }

    // Helper function to display NaN or value
    auto displayValue = [](double value, int precision = 5) -> QString {
        if (std::isnan(value)) {
            return "NaN";
        } else if (std::isinf(value)) {
            if (value < 0) {
                return "-inf";
            } else {
                return "+inf";
            }
        } else {
            return QString::number(value, 'g', precision);
        }
    };

    // Update ONLY extrinsic parameters (4x4 matrix, elements 12-27) - no signal blocking needed since read-only
    // Note: QMatrix4x4::data() returns column-major, but we want to display row-major
    // So we need to transpose the indices when displaying
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int displayIndex = row * 4 + col;  // Row-major display index
            // JETR is stored in row-major format, so use displayIndex directly
            matrixLineEdits[displayIndex]->setText(displayValue(jetrVector[12 + displayIndex], 5));
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::updateVectorFromDisplay()
{
    // Since most UI elements are now read-only, this method only needs to handle 
    // the non-read-only elements (make/model/position/rotation combo boxes)
    // The JETR vector numerical data is now managed programmatically and doesn't 
    // need to be read back from UI elements
    
    // Note: This method is kept minimal since the new architecture keeps
    // JETR vector as the single source of truth, updated programmatically
    // The combo boxes handle their own data and don't affect the JETR vector directly
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector<double> LAUJETRWidget::createDefaultJETR()
{
    // Add an initial empty tab with sensible defaults
    QVector<double> defaultJetr(37, NAN);

    // Set transformation matrix to identity (elements 12-27)
    defaultJetr[12] = 1.0; defaultJetr[13] = 0.0; defaultJetr[14] = 0.0; defaultJetr[15] = 0.0;
    defaultJetr[16] = 0.0; defaultJetr[17] = 1.0; defaultJetr[18] = 0.0; defaultJetr[19] = 0.0;
    defaultJetr[20] = 0.0; defaultJetr[21] = 0.0; defaultJetr[22] = 1.0; defaultJetr[23] = 0.0;
    defaultJetr[24] = 0.0; defaultJetr[25] = 0.0; defaultJetr[26] = 0.0; defaultJetr[27] = 1.0;

    // Set bounding box to infinite bounds (elements 28-33)
    defaultJetr[28] = -std::numeric_limits<double>::infinity(); // xMin
    defaultJetr[29] = +std::numeric_limits<double>::infinity(); // xMax
    defaultJetr[30] = -std::numeric_limits<double>::infinity(); // yMin
    defaultJetr[31] = +std::numeric_limits<double>::infinity(); // yMax
    defaultJetr[32] = -std::numeric_limits<double>::infinity(); // zMin
    defaultJetr[33] = +std::numeric_limits<double>::infinity(); // zMax

    return defaultJetr;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUJETRWidget::makeMakeModelKey(const QString &make, const QString &model)
{
    if (make.isEmpty() && model.isEmpty()) {
        return QString();
    }

    QString cleanMake = make.simplified().replace(" ", "_");
    QString cleanModel = model.simplified().replace(" ", "_");

    if (cleanMake.isEmpty()) {
        return cleanModel;
    } else if (cleanModel.isEmpty()) {
        return cleanMake;
    } else {
        return QString("%1_%2").arg(cleanMake, cleanModel);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRWidget::validateJETRVector(const QVector<double> &jetr)
{
    if (jetr.size() != 37) {
        return false;
    }

    // Check that most values are valid numbers (not NaN or infinite)
    // Exception: bounding box values (28-33) can be infinite
    for (int i = 0; i < 37; ++i) {
        double value = jetr[i];

        if (i >= 28 && i <= 33) {
            // Bounding box values - infinite is OK, but NaN is not
            if (std::isnan(value)) {
                return false;
            }
        } else {
            // Other values should be finite
            if (!std::isfinite(value)) {
                return false;
            }
        }
    }

    return true;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QPair<QString, QString> LAUJETRWidget::getMakeAndModel(QWidget *parent)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Select Camera Make and Model");
    dialog.setMinimumSize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // Info label
    QLabel *infoLabel = new QLabel("Select a camera make and model from cached configurations:");
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    // Get all available make/model pairs
    QList<QPair<QString, QString>> pairs = getAllMakeModelPairs();

    if (pairs.isEmpty()) {
        QMessageBox::information(parent, "No Camera Data",
                                 "No cached camera configurations found.\n\n"
                                 "Please import LUTX files first to populate the camera database.");
        return QPair<QString, QString>();
    }

    // Create list widget with make/model combinations
    QListWidget *listWidget = new QListWidget();
    for (const auto &pair : pairs) {
        QString displayText = QString("%1 - %2").arg(pair.first, pair.second);
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, QVariant::fromValue(pair));
        listWidget->addItem(item);
    }

    // Select first item by default
    if (listWidget->count() > 0) {
        listWidget->setCurrentRow(0);
    }

    layout->addWidget(listWidget);

    // Button box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        QListWidgetItem *currentItem = listWidget->currentItem();
        if (currentItem) {
            return currentItem->data(Qt::UserRole).value<QPair<QString, QString>>();
        }
    }

    return QPair<QString, QString>(); // Return empty pair if cancelled
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QPair<QString, QString> LAUJETRWidget::getMakeAndModel(const LAUMemoryObject &memoryObject, QWidget *parent)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Select Camera Make and Model");
    dialog.setMinimumSize(800, 600);

    QHBoxLayout *mainLayout = new QHBoxLayout(&dialog);

    // Left side - Image display
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    QLabel *imageLabel = new QLabel("Depth Image Preview:");
    leftLayout->addWidget(imageLabel);
    
    // Create image display widget
    QLabel *imageWidget = new QLabel();
    imageWidget->setMinimumSize(400, 300);
    imageWidget->setMaximumSize(400, 300);
    imageWidget->setScaledContents(true);
    imageWidget->setAlignment(Qt::AlignCenter);
    imageWidget->setStyleSheet("border: 1px solid gray; background-color: #f0f0f0;");
    
    // Convert memory object to displayable image
    QImage depthImage = memoryObjectToImage(memoryObject);
    if (!depthImage.isNull()) {
        imageWidget->setPixmap(QPixmap::fromImage(depthImage));
    } else {
        imageWidget->setText("Failed to load\ndepth image");
        imageWidget->setAlignment(Qt::AlignCenter);
    }
    
    leftLayout->addWidget(imageWidget);
    leftLayout->addStretch();
    
    mainLayout->addLayout(leftLayout);

    // Right side - Make/Model selection
    QVBoxLayout *rightLayout = new QVBoxLayout();
    
    // Info label
    QLabel *infoLabel = new QLabel("Based on the depth image characteristics, select the camera make and model:");
    infoLabel->setWordWrap(true);
    rightLayout->addWidget(infoLabel);

    // Get all available make/model pairs
    QList<QPair<QString, QString>> pairs = getAllMakeModelPairs();

    if (pairs.isEmpty()) {
        QMessageBox::information(parent, "No Camera Data",
                                 "No cached camera configurations found.\n\n"
                                 "Please import LUTX files first to populate the camera database.");
        return QPair<QString, QString>();
    }

    // Create list widget with make/model combinations
    QListWidget *listWidget = new QListWidget();
    for (const auto &pair : pairs) {
        QString displayText = QString("%1 - %2").arg(pair.first, pair.second);
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, QVariant::fromValue(pair));
        listWidget->addItem(item);
    }

    // Select first item by default
    if (listWidget->count() > 0) {
        listWidget->setCurrentRow(0);
    }

    rightLayout->addWidget(listWidget);

    // Button box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    rightLayout->addWidget(buttonBox);
    
    mainLayout->addLayout(rightLayout);

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        QListWidgetItem *currentItem = listWidget->currentItem();
        if (currentItem) {
            return currentItem->data(Qt::UserRole).value<QPair<QString, QString>>();
        }
    }

    return QPair<QString, QString>(); // Return empty pair if cancelled
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QImage LAUJETRWidget::memoryObjectToImage(const LAUMemoryObject &memoryObject)
{
    if (!memoryObject.isValid()) {
        return QImage();
    }
    
    int width = memoryObject.width();
    int height = memoryObject.height();
    int channels = memoryObject.colors();
    int depth = memoryObject.depth();
    
    // Handle different data types and channel configurations
    QImage image;
    
    if (channels == 1 && depth == sizeof(unsigned short)) {
        // Single channel 16-bit depth data (most common for depth cameras)
        const unsigned short *data = reinterpret_cast<const unsigned short*>(memoryObject.constPointer());
        
        // Find min/max for scaling
        unsigned short minVal = 65535, maxVal = 0;
        for (int i = 0; i < width * height; ++i) {
            if (data[i] > 0) { // Ignore zero values (invalid depth)
                minVal = qMin(minVal, data[i]);
                maxVal = qMax(maxVal, data[i]);
            }
        }
        
        // Create 8-bit grayscale image
        image = QImage(width, height, QImage::Format_Grayscale8);
        
        if (maxVal > minVal) {
            double scale = 255.0 / (maxVal - minVal);
            for (int y = 0; y < height; ++y) {
                unsigned char *scanLine = image.scanLine(y);
                for (int x = 0; x < width; ++x) {
                    unsigned short depthVal = data[y * width + x];
                    if (depthVal == 0) {
                        scanLine[x] = 0; // Black for invalid depth
                    } else {
                        scanLine[x] = static_cast<unsigned char>((depthVal - minVal) * scale);
                    }
                }
            }
        } else {
            image.fill(0);
        }
    } 
    else if (channels == 3 && depth == sizeof(unsigned char)) {
        // RGB 8-bit color data
        const unsigned char *data = reinterpret_cast<const unsigned char*>(memoryObject.constPointer());
        image = QImage(width, height, QImage::Format_RGB888);
        
        for (int y = 0; y < height; ++y) {
            unsigned char *scanLine = image.scanLine(y);
            for (int x = 0; x < width; ++x) {
                int srcIndex = (y * width + x) * 3;
                int dstIndex = x * 3;
                scanLine[dstIndex] = data[srcIndex];     // R
                scanLine[dstIndex + 1] = data[srcIndex + 1]; // G
                scanLine[dstIndex + 2] = data[srcIndex + 2]; // B
            }
        }
    }
    else if (channels == 1 && depth == sizeof(float)) {
        // Single channel float data
        const float *data = reinterpret_cast<const float*>(memoryObject.constPointer());
        
        // Find min/max for scaling
        float minVal = std::numeric_limits<float>::max();
        float maxVal = std::numeric_limits<float>::lowest();
        for (int i = 0; i < width * height; ++i) {
            if (std::isfinite(data[i])) {
                minVal = qMin(minVal, data[i]);
                maxVal = qMax(maxVal, data[i]);
            }
        }
        
        // Create 8-bit grayscale image
        image = QImage(width, height, QImage::Format_Grayscale8);
        
        if (maxVal > minVal) {
            double scale = 255.0 / (maxVal - minVal);
            for (int y = 0; y < height; ++y) {
                unsigned char *scanLine = image.scanLine(y);
                for (int x = 0; x < width; ++x) {
                    float val = data[y * width + x];
                    if (!std::isfinite(val)) {
                        scanLine[x] = 0; // Black for invalid values
                    } else {
                        scanLine[x] = static_cast<unsigned char>((val - minVal) * scale);
                    }
                }
            }
        } else {
            image.fill(0);
        }
    }
    else {
        // Unsupported format - create a placeholder
        image = QImage(width, height, QImage::Format_RGB888);
        image.fill(QColor(64, 64, 64)); // Dark gray
        
        // Draw some text indicating the format
        QPainter painter(&image);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 12));
        QString formatText = QString("Unsupported format\n%1x%2, %3 channels\n%4 bytes per pixel")
                             .arg(width).arg(height).arg(channels).arg(depth);
        painter.drawText(image.rect(), Qt::AlignCenter, formatText);
    }
    
    return image;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<LAUCameraInfo> LAUJETRWidget::getMultiCameraMakeAndModel(const LAUMemoryObject &memoryObject, QWidget *parent)
{
    if (!memoryObject.isValid()) {
        return QList<LAUCameraInfo>();
    }
    
    // Always use LAUCameraSelectionDialog for consistent behavior
    // The dialog will automatically handle single or multiple cameras
    LAUCameraSelectionDialog dialog(memoryObject, parent);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Get all selection results
        QList<QPair<QString, QString>> makeModelPairs = dialog.getMakeModelPairs();
        QStringList positions = dialog.getPositions();
        QList<bool> rotations = dialog.getRotations();
        
        // Create LAUCameraInfo objects with complete data
        QList<LAUCameraInfo> cameraInfos;
        for (int i = 0; i < makeModelPairs.size(); ++i) {
            QString make = makeModelPairs[i].first;
            QString model = makeModelPairs[i].second;
            QString position = (i < positions.size()) ? positions[i] : "unknown";
            bool rotation = (i < rotations.size()) ? rotations[i] : false;
            
            LAUCameraInfo info(make, model, position, rotation);
            cameraInfos.append(info);
        }
        
        return cameraInfos;
    }
    
    // User cancelled
    return QList<LAUCameraInfo>();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QImage LAUJETRWidget::extractCameraImage(const LAUMemoryObject &memoryObject, int cameraIndex)
{
    if (!memoryObject.isValid()) {
        return QImage();
    }
    
    // For 3D video monitoring systems, cameras are typically stacked vertically
    // Each camera is 640x480, so total height = numCameras * 480
    int totalWidth = memoryObject.width();
    int totalHeight = memoryObject.height();
    int cameraHeight = 480; // Standard camera height
    int cameraWidth = 640;  // Standard camera width
    
    // Calculate number of cameras based on height
    int numCameras = totalHeight / cameraHeight;
    
    if (cameraIndex < 0 || cameraIndex >= numCameras) {
        return QImage(); // Invalid camera index
    }
    
    if (totalWidth != cameraWidth) {
        // Width doesn't match expected camera width
        qDebug() << "Warning: Unexpected width" << totalWidth << "expected" << cameraWidth;
    }
    
    // Calculate the region for this camera
    int startY = cameraIndex * cameraHeight;
    int endY = startY + cameraHeight;
    
    if (endY > totalHeight) {
        return QImage(); // Camera region extends beyond image
    }
    
    // Create a temporary memory object for just this camera's region
    LAUMemoryObject cameraObject(cameraWidth, cameraHeight, memoryObject.colors(), memoryObject.depth(), 1);
    
    // Copy the camera's data
    int bytesPerPixel = memoryObject.colors() * memoryObject.depth();
    const unsigned char *sourceData = reinterpret_cast<const unsigned char*>(memoryObject.constPointer());
    unsigned char *destData = reinterpret_cast<unsigned char*>(cameraObject.pointer());
    
    for (int y = 0; y < cameraHeight; ++y) {
        int sourceRowOffset = ((startY + y) * totalWidth) * bytesPerPixel;
        int destRowOffset = (y * cameraWidth) * bytesPerPixel;
        
        memcpy(destData + destRowOffset, 
               sourceData + sourceRowOffset, 
               cameraWidth * bytesPerPixel);
    }
    
    // Convert the camera's data to an image
    return memoryObjectToImage(cameraObject);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUJETRWidget::getAvailableMakes()
{
    QStringList makes;
    QList<QPair<QString, QString>> pairs = getAllMakeModelPairs();

    for (const auto &pair : pairs) {
        if (!makes.contains(pair.first)) {
            makes.append(pair.first);
        }
    }

    makes.sort();
    return makes;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUJETRWidget::getAvailableModels(const QString &make)
{
    QStringList models;
    QList<QPair<QString, QString>> pairs = getAllMakeModelPairs();

    for (const auto &pair : pairs) {
        if (pair.first == make && !models.contains(pair.second)) {
            models.append(pair.second);
        }
    }

    models.sort();
    return models;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QPair<QString, QString>> LAUJETRWidget::getAllMakeModelPairs()
{
    QList<QPair<QString, QString>> pairs;
    
    // Get all camera calibrations from the Camera Inventory
    QList<LAUCameraCalibration> calibrations = LAUCameraInventoryDialog::getAllCameraCalibrations();
    
    for (const LAUCameraCalibration &calibration : calibrations) {
        if (calibration.isValid()) {
            QPair<QString, QString> pair(calibration.make, calibration.model);
            if (!pairs.contains(pair)) {
                pairs.append(pair);
            }
        }
    }
    
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
void LAUJETRWidget::setCameraMake(const QString &make)
{
    if (makeComboBox) {
        makeComboBox->setCurrentText(make);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUJETRWidget::getCameraMake() const
{
    if (makeComboBox) {
        return makeComboBox->currentText();
    }
    return "Unknown";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setCameraModel(const QString &model)
{
    if (modelComboBox) {
        modelComboBox->setCurrentText(model);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUJETRWidget::getCameraModel() const
{
    if (modelComboBox) {
        return modelComboBox->currentText();
    }
    return "Unknown";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setCameraPosition(const QString &position)
{
    if (positionComboBox) {
        // Find and set the position in the combo box
        // Try exact match first
        int index = positionComboBox->findData(position);

        // If not found, try case-insensitive match
        if (index < 0) {
            QString upperPosition = position.toUpper();
            for (int i = 0; i < positionComboBox->count(); ++i) {
                QString itemData = positionComboBox->itemData(i).toString();
                if (itemData.toUpper() == upperPosition) {
                    index = i;
                    break;
                }
            }
        }

        if (index >= 0) {
            positionComboBox->setCurrentIndex(index);
            qDebug() << "setCameraPosition: Set combo box to" << positionComboBox->currentText()
                     << "for position:" << position;
        } else {
            // If not found by data, try to find the "unknown" item as fallback
            int unknownIndex = positionComboBox->findData("H UNKNOWN");
            if (unknownIndex >= 0) {
                positionComboBox->setCurrentIndex(unknownIndex);
                qDebug() << "setCameraPosition: Position" << position << "not found, defaulting to Unknown";
            }
        }
    }

    // Update master scan if this camera is now set to "top"
    updateMasterScanIfTop();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUJETRWidget::getCameraPosition() const
{
    if (positionComboBox) {
        return positionComboBox->currentData().toString();
    }
    return "unknown";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setCameraPositionReadOnly(bool readOnly)
{
    if (positionComboBox) {
        positionComboBox->setEnabled(!readOnly);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setCameraRotation(bool rotate180)
{
    if (rotateCheckBox) {
        rotateCheckBox->setChecked(rotate180);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRWidget::getCameraRotation() const
{
    if (rotateCheckBox) {
        return rotateCheckBox->isChecked();
    }
    return false;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::saveCameraMetadataToSettings(const QString &make, const QString &model)
{
    if (make.isEmpty() || model.isEmpty()) {
        return;
    }
    
    QString makeModelKey = makeMakeModelKey(make, model);
    
    QSettings settings;
    settings.beginGroup("CameraMetadata");
    
    // Save position
    QString position = getCameraPosition();
    if (!position.isEmpty()) {
        settings.setValue(makeModelKey + "_position", position);
    }
    
    // Save rotation
    bool rotation = getCameraRotation();
    settings.setValue(makeModelKey + "_rotation", rotation);
    
    settings.endGroup();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::loadCameraMetadataFromSettings(const QString &make, const QString &model)
{
    if (make.isEmpty() || model.isEmpty()) {
        return;
    }
    
    QString makeModelKey = makeMakeModelKey(make, model);
    
    QSettings settings;
    settings.beginGroup("CameraMetadata");
    
    // Load position
    QString position = settings.value(makeModelKey + "_position", "unknown").toString();
    setCameraPosition(position);
    
    // Load rotation
    bool rotation = settings.value(makeModelKey + "_rotation", false).toBool();
    setCameraRotation(rotation);
    
    settings.endGroup();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUJETRWidget::matrixToMatlabString(QTableWidget *table) const
{
    if (!table) return QString();

    QString matlabString = "[";

    for (int row = 0; row < MATRIX_SIZE; ++row) {
        if (row > 0) {
            matlabString += "; ";  // MATLAB row separator
        }

        for (int col = 0; col < MATRIX_SIZE; ++col) {
            if (col > 0) {
                matlabString += ", ";  // MATLAB column separator
            }

            QTableWidgetItem *item = table->item(row, col);
            if (item) {
                matlabString += item->text();
            } else {
                matlabString += "0";
            }
        }
    }

    matlabString += "]";
    return matlabString;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRWidget::pasteFromMatlabString(QTableWidget *table, const QString &matlabString)
{
    if (!table || matlabString.isEmpty()) return false;

    QString cleanString = matlabString.trimmed();

    // Remove outer brackets if present
    if (cleanString.startsWith('[') && cleanString.endsWith(']')) {
        cleanString = cleanString.mid(1, cleanString.length() - 2);
    }

    // Split by semicolons to get rows
    QStringList rows = cleanString.split(';', Qt::SkipEmptyParts);

    // If no semicolons found, try splitting by newlines (alternative format)
    if (rows.size() == 1) {
        rows = cleanString.split('\n', Qt::SkipEmptyParts);
    }

    // If still only one row, might be space-separated values, try to detect 4x4
    if (rows.size() == 1) {
        QStringList allValues = cleanString.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);
        if (allValues.size() == 16) {
            // Reshape into 4 rows of 4 values each
            rows.clear();
            for (int i = 0; i < 4; ++i) {
                QStringList rowValues;
                for (int j = 0; j < 4; ++j) {
                    rowValues << allValues[i * 4 + j];
                }
                rows << rowValues.join(", ");
            }
        }
    }

    // Check if we have the right number of rows
    if (rows.size() != MATRIX_SIZE) {
        return false; // Invalid format
    }

    // Parse each row
    for (int row = 0; row < MATRIX_SIZE && row < rows.size(); ++row) {
        QString rowString = rows[row].trimmed();

        // Split by commas, spaces, or tabs
        QStringList values = rowString.split(QRegularExpression("[\\s,\\t]+"), Qt::SkipEmptyParts);

        if (values.size() != MATRIX_SIZE) {
            return false; // Invalid format
        }

        // Set values in table
        for (int col = 0; col < MATRIX_SIZE && col < values.size(); ++col) {
            bool ok;
            double value = values[col].toDouble(&ok);
            if (!ok) {
                return false; // Invalid number format
            }

            QTableWidgetItem *item = table->item(row, col);
            if (!item) {
                item = new QTableWidgetItem();
                table->setItem(row, col, item);
            }
            item->setText(QString::number(value, 'f', 6));
        }
    }

    return true;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::onEditTransformMatrix()
{
    QString position = getCameraPosition();
    QMatrix4x4 transform;
    bool accepted = false;

    // Step 1: Backup current transform and set to identity
    saveCurrentTransform();
    setIdentityTransform();

    try {
        if (position.toLower().endsWith("top")) {
            // For top view cameras, use XY plane fitting
            accepted = performXYPlaneFitting(transform);

            // Show transform review dialog for top camera
            if (accepted) {
                LAUTransformEditorDialog reviewDialog(transform, this);
                reviewDialog.setWindowTitle("Review XY Plane Transform - Top Camera");
                if (reviewDialog.exec() == QDialog::Accepted) {
                    transform = reviewDialog.transform();
                    accepted = true;
                } else {
                    accepted = false;
                }
            }
        } else {
            // For other cameras, use standard transform editor
            accepted = runStandardTransformEditor(transform);
        }

        if (accepted) {
            // Step 2: Apply the new transform from alignment dialog
            suppressChangeSignals = true;

            const float *data = transform.constData();

            // Helper function to display values
            auto displayValue = [](double value, int precision = 5) -> QString {
                if (std::isnan(value)) {
                    return "NaN";
                } else if (std::isinf(value)) {
                    return value < 0 ? "-inf" : "+inf";
                } else {
                    return QString::number(value, 'g', precision);
                }
            };

            // Update JETR vector (elements 12-27) and UI simultaneously
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    int jetrIndex = row * 4 + col;  // JETR is row-major
                    int dataIndex = col * 4 + row;  // QMatrix4x4 is column-major

                    // Update JETR vector with correct row-major ordering
                    jetrVector[12 + jetrIndex] = static_cast<double>(data[dataIndex]);

                    // Update UI element directly (UI is also row-major)
                    matrixLineEdits[jetrIndex]->setText(displayValue(static_cast<double>(data[dataIndex])));
                }
            }

            suppressChangeSignals = false;

            // Notify listeners of final result
            emit jetrVectorChanged(jetrVector);

            qDebug() << "Transform matrix successfully updated from alignment dialog";
        } else {
            // Step 3: User cancelled - restore backup transform
            restoreBackupTransform();
        }
    } catch (...) {
        // Emergency cleanup: restore backup transform on any exception
        restoreBackupTransform();
        throw; // Re-throw the exception
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::saveCurrentTransform()
{
    // Save current transform matrix (elements 12-27) to backup
    backupTransform.setToIdentity();
    const float* backupData = backupTransform.constData();
    float* writableData = const_cast<float*>(backupData);

    // Copy JETR elements 12-27 to backup matrix (converting row-major to column-major)
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int jetrIndex = 12 + (row * 4 + col);        // JETR uses row-major storage
            int matrixIndex = col * 4 + row;             // QMatrix4x4 uses column-major storage
            writableData[matrixIndex] = static_cast<float>(jetrVector[jetrIndex]);
        }
    }

    qDebug() << "Transform matrix backed up for identity-based editing";
    qDebug() << "Backup matrix:" << backupTransform;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setIdentityTransform()
{
    suppressChangeSignals = true;

    // Set JETR elements 12-27 to identity matrix
    for (int i = 0; i < 16; ++i) {
        int row = i / 4;
        int col = i % 4;
        jetrVector[12 + i] = (row == col) ? 1.0 : 0.0;  // Diagonal = 1, others = 0
    }

    // Update UI to show identity matrix
    updateTransformMatrixDisplay();

    suppressChangeSignals = false;

    qDebug() << "Transform matrix set to identity for alignment editing";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::restoreBackupTransform()
{
    suppressChangeSignals = true;

    // Restore JETR elements 12-27 from backup matrix
    const float* data = backupTransform.constData();

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int jetrIndex = 12 + (row * 4 + col);        // JETR uses row-major storage
            int matrixIndex = col * 4 + row;             // QMatrix4x4 uses column-major storage
            jetrVector[jetrIndex] = static_cast<double>(data[matrixIndex]);
        }
    }

    // Update UI to show restored matrix
    updateTransformMatrixDisplay();

    suppressChangeSignals = false;

    qDebug() << "Transform matrix restored from backup";
    qDebug() << "Restored matrix:" << backupTransform;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRWidget::runStandardTransformEditor(QMatrix4x4 &transform)
{
#ifdef ENABLEPOINTMATCHER
    // Check if camera make, model, and position are set
    QString make = getCameraMake();
    QString model = getCameraModel();
    QString position = getCameraPosition();
    
    if (make.isEmpty() || model.isEmpty() || make == "Unknown" || model == "Unknown") {
        QMessageBox::warning(this, "Configuration Required", 
            "Please set the camera make and model before editing the transform matrix.\n\n"
            "Use the dropdowns at the top of this tab to select:\n"
            "• Camera Make (e.g., Orbbec, Intel, FLIR)\n"
            "• Camera Model (e.g., Femto Mega, RealSense D435)");
        return false;
    }
    
    if (position.isEmpty() || position == "unknown") {
        QMessageBox::warning(this, "Configuration Required",
            "Please set the camera position before editing the transform matrix.\n\n"
            "Use the position dropdown to select:\n"
            "• 'top' for the reference camera (first camera only)\n"
            "• 'side', 'quarter', 'rump', 'front', 'back', 'bottom' for other cameras\n\n"
            "The position identifies where the camera is mounted relative to the animal.");
        return false;
    }

    // For non-top cameras, check if top camera transform is set to floor
    if (!position.toLower().endsWith("top")) {
        // Need to verify the top camera has its transform set to the floor
        // We check the JETR vector transform matrix (elements 12-27)
        // The top camera should have Z-axis pointing down (negative Z direction)
        // This indicates the XY plane has been fitted to the floor

        // Find the top camera widget in the parent dialog to check its transform
        QWidget *parentWidget = this->parentWidget();
        while (parentWidget && !qobject_cast<QDialog*>(parentWidget)) {
            parentWidget = parentWidget->parentWidget();
        }

        bool topCameraTransformValid = false;
        if (parentWidget) {
            // Look for sibling JETR widgets
            QList<LAUJETRWidget*> siblings = parentWidget->findChildren<LAUJETRWidget*>();
            for (LAUJETRWidget *sibling : siblings) {
                if (sibling != this && sibling->getCameraPosition().toLower().endsWith("top")) {
                    // Check if the top camera's transform has been set (not identity)
                    QVector<double> topJetr = sibling->getJETRVector();
                    if (topJetr.size() == 37) {
                        // Check if transform is identity matrix (meaning it hasn't been set)
                        bool isIdentity = true;
                        for (int i = 0; i < 16; ++i) {
                            int row = i / 4;
                            int col = i % 4;
                            double expected = (row == col) ? 1.0 : 0.0;
                            double actual = topJetr[12 + i];
                            if (std::abs(actual - expected) > 0.001) {
                                isIdentity = false;
                                break;
                            }
                        }

                        // If transform is NOT identity, then XY plane has been set
                        if (!isIdentity) {
                            topCameraTransformValid = true;
                            qDebug() << "Top camera transform is set (not identity) - allowing secondary camera alignment";
                        } else {
                            qDebug() << "Top camera transform is still identity - XY plane not set yet";
                        }
                    }
                    break;
                }
            }
        }

        if (!topCameraTransformValid) {
            QMessageBox::warning(this, "Top Camera Floor Transform Required",
                "The top camera transform must be set to the floor before aligning other cameras.\n\n"
                "Please complete the top camera setup first:\n"
                "1. Switch to the 'top' camera tab\n"
                "2. Click 'Edit Transform Matrix...'\n"
                "3. Use 'Set XY Plane' to fit the floor plane\n"
                "4. Accept the transform\n"
                "\nThis creates the master scan needed for camera alignment.\n"
                "Then return to this camera for alignment.");
            return false;
        }

        // Also check if master scan exists (it should be generated when we try to use it)
        if (!masterScan.isValid()) {
            QMessageBox::warning(this, "Master Scan Required",
                "No master scan available from the 'top' camera.\n\n"
                "Please complete the top camera setup first:\n"
                "• Set one camera to 'top' position\n"
                "• Set its make and model\n"
                "• Click 'Edit Transform Matrix' and set the XY plane\n"
                "\nThen return to this camera for alignment.");
            return false;
        }
    }
    
    // Check if we have valid memory object and camera data
    if (!currentMemoryObject.isValid()) {
        QMessageBox::warning(this, "Scan Merging Error", 
            "No depth data available for scan merging.\n\n"
            "The main window must provide depth data before this operation can be performed.");
        return false;
    }
    
    // Get current JETR vector for creating lookup table
    QVector<double> currentJetr = getJETRVector();
    if (currentJetr.size() != 37) {
        QMessageBox::warning(this, "Scan Merging Error", 
            "Invalid JETR vector. Cannot create lookup table for scan generation.");
        return false;
    }
    
    // Get cached lookup table
    LAULookUpTable lookupTable = getCachedLUT();
    if (!lookupTable.isValid()) {
        // LUT generation failed or was cancelled by user
        // Don't show error message - just return false to abort the operation
        return false;
    }
    
    // Extract camera memory object from stacked memory object
    LAUMemoryObject cameraMemoryObject = extractCameraMemoryObject(currentMemoryObject, currentCameraIndex);
    if (!cameraMemoryObject.isValid()) {
        QMessageBox::warning(this, "Scan Merging Error", 
            "Failed to extract camera data from memory object.");
        return false;
    }
    
    // Create LAUScan from memory object and lookup table using static method
    LAUScan slaveScan = LAUTiffViewer::convertMemoryObjectToScan(cameraMemoryObject, lookupTable);
    if (!slaveScan.isValid()) {
        QMessageBox::warning(this, "Scan Merging Error", 
            "Failed to generate 3D scan from depth data.\n\n"
            "The lookup table or depth data may be invalid.");
        return false;
    }
    
    // Set parent name for identification in packet list (since we're not loading from file)
    slaveScan.setParentName(QString("JETR Calibration Widget Tab %1").arg(currentCameraIndex));
    
    // Check if we have a valid master scan from the "top" camera
    if (!masterScan.isValid()) {
        QMessageBox::warning(this, "Scan Merging Error", 
            "No master scan available from the 'top' camera.\n\n"
            "Please ensure:\n"
            "• One camera is set to 'top' position\n"
            "• The top camera has valid depth data and calibration\n"
            "• The top camera's JETR vector has been properly configured");
        return false;
    }
    
    // Launch merge scan dialog without parent to avoid event loop conflicts
    LAUMergeScanDialog dialog(masterScan, slaveScan, nullptr);
    dialog.setWindowTitle("Scan Alignment - " + getCameraMake() + " " + getCameraModel());
    dialog.setWindowModality(Qt::ApplicationModal);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Get the transformation matrix from ICP alignment
        transform = dialog.transform();
        qDebug() << "Scan merging accepted. Transform matrix:" << transform;
        return true;
    }
    
    // User cancelled the dialog
    return false;
#else
    Q_UNUSED(transform); // Mark parameter as intentionally unused
    
    // PCL support not available
    QMessageBox::information(this, "Scan Merging Unavailable", 
        "Scan merging functionality requires PCL (Point Cloud Library) support.\n\n"
        "This build was compiled without PCL support. To enable scan merging:\n"
        "• Install PCL library\n"
        "• Rebuild with CONFIG += merging flag");
    
    return false;
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUJETRWidget::performXYPlaneFitting(QMatrix4x4 &transform)
{
    // Check if camera make and model are set
    QString make = getCameraMake();
    QString model = getCameraModel();
    
    if (make.isEmpty() || model.isEmpty() || make == "Unknown" || model == "Unknown") {
        QMessageBox::warning(this, "Configuration Required", 
            "Please set the camera make and model before setting the XY plane.\n\n"
            "Use the dropdowns at the top of this tab to select:\n"
            "• Camera Make (e.g., Orbbec, Intel, FLIR)\n"
            "• Camera Model (e.g., Femto Mega, RealSense D435)");
        return false;
    }
    
    // Check if we have valid memory object and camera data
    if (!currentMemoryObject.isValid()) {
        QMessageBox::warning(this, "XY Plane Fitting Error", 
            "No depth data available for XY plane fitting.\n\n"
            "The main window must provide depth data before this operation can be performed.");
        return false;
    }
    
    // Get current JETR vector for creating lookup table
    QVector<double> currentJetr = getJETRVector();
    if (currentJetr.size() != 37) {
        QMessageBox::warning(this, "XY Plane Fitting Error", 
            "Invalid JETR vector. Cannot create lookup table for plane fitting.");
        return false;
    }
    
    // Get cached lookup table
    LAULookUpTable lookupTable = getCachedLUT();
    if (!lookupTable.isValid()) {
        // LUT generation failed or was cancelled by user
        // Don't show error message - just return false to abort the operation
        return false;
    }
    
    // Extract camera's depth data from memory object
    LAUMemoryObject cameraMemoryObject;
    
    // Calculate camera stack info
    int totalHeight = currentMemoryObject.height();
    int cameraHeight = 480; // Standard camera height  
    int numCameras = totalHeight / cameraHeight;
    
    if (numCameras > 1 && currentCameraIndex < numCameras) {
        // Extract specific camera from stack
        cameraMemoryObject = extractCameraMemoryObject(currentMemoryObject, currentCameraIndex);
    } else {
        // Single camera or use full image
        cameraMemoryObject = currentMemoryObject;
    }
    
    if (!cameraMemoryObject.isValid()) {
        QMessageBox::warning(this, "XY Plane Fitting Error", 
            "Failed to extract camera depth data.\n\n"
            "The camera region may be outside the image bounds.");
        return false;
    }
    
    // Create LAUScan from depth data and lookup table
    LAUScan scan = LAUTiffViewer::convertMemoryObjectToScan(cameraMemoryObject, lookupTable);
    if (!scan.isValid()) {
        QMessageBox::warning(this, "XY Plane Fitting Error", 
            "Failed to create 3D scan from depth data.\n\n"
            "The depth data may be invalid or incompatible with the current calibration.");
        return false;
    }
    
    // Set scan metadata
    scan.setMake(getCameraMake());
    scan.setModel(getCameraModel());
    
    // Launch XY plane fitting dialog without parent to avoid event loop conflicts
    LAUSetXYPlaneDialog dialog(scan, nullptr);
    dialog.setWindowTitle("Fit XY Plane - Top View Camera");
    dialog.setWindowModality(Qt::ApplicationModal);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Get the transform matrix from the plane fitting
        transform = dialog.transform();
        
        // Update master scan since XY plane was successfully set
        // This ensures other cameras can reference the updated top camera scan
        updateMasterScanIfTop();
        
        return true; // Dialog was accepted
    }
    
    return false; // Dialog was cancelled or failed
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setMemoryObjectOnly(const LAUMemoryObject &memoryObject, int cameraIndex)
{
    // Store memory object and camera index for later use (e.g., XY plane fitting)
    // without modifying the current JETR vector
    currentMemoryObject = memoryObject;
    currentCameraIndex = cameraIndex;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::onEditBoundingBox()
{
    // Emit signal to request bounding box editing at the dialog level
    // The parent dialog will handle the actual bounding box editor
    emit requestBoundingBoxEdit();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::onPreviewRawImage()
{
    if (!currentMemoryObject.isValid()) {
        QMessageBox::information(this, "Preview", "No memory object available for preview.");
        return;
    }
    
    try {
        // Extract the specific camera's memory object from the stack
        LAUMemoryObject cameraMemObj = extractCameraMemoryObject(currentMemoryObject, currentCameraIndex);
        
        if (!cameraMemObj.isValid()) {
            QMessageBox::warning(this, "Preview Error", "Unable to extract camera memory object for preview.");
            return;
        }
        
        // Convert memory object to image
        QImage image = extractCameraImage(currentMemoryObject, currentCameraIndex);
        
        if (image.isNull()) {
            QMessageBox::warning(this, "Preview Error", "Unable to convert memory object to image.");
            return;
        }
        
        // Create simple image preview dialog with fixed size
        QDialog *viewer = new QDialog(this);
        viewer->setWindowTitle(QString("Raw Image Preview - Camera %1").arg(currentCameraIndex + 1));
        viewer->setAttribute(Qt::WA_DeleteOnClose);
        viewer->setWindowModality(Qt::ApplicationModal);  // Make dialog modal
        viewer->setFixedSize(660, 540); // Fixed window size to accommodate 640x480 image plus margins
        
        QVBoxLayout *layout = new QVBoxLayout(viewer);
        layout->setContentsMargins(10, 10, 10, 10);
        
        // Create label to display the image with fixed size
        QLabel *imageLabel = new QLabel();
        imageLabel->setFixedSize(640, 480);
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setStyleSheet("QLabel { background-color: #f0f0f0; border: 1px solid #ccc; }");
        
        // Scale the image to fit the label while maintaining aspect ratio
        QPixmap pixmap = QPixmap::fromImage(image);
        imageLabel->setPixmap(pixmap.scaled(640, 480, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        
        layout->addWidget(imageLabel);
        
        // Add dialog button box with OK button
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, viewer);
        connect(buttonBox, &QDialogButtonBox::accepted, viewer, &QDialog::accept);
        layout->addWidget(buttonBox);
        
        viewer->exec();  // Use exec() instead of show() for modal dialog
        
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Preview Error", 
                             QString("Failed to preview raw image: %1").arg(e.what()));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::onPreview3DScan()
{
    if (!currentMemoryObject.isValid()) {
        QMessageBox::information(this, "Preview", "No memory object available for 3D preview.");
        return;
    }
    
    // Check if camera make and model are set before starting
    QString make = getCameraMake();
    QString model = getCameraModel();
    
    if (make.isEmpty() || model.isEmpty() || make == "Unknown" || model == "Unknown") {
        QMessageBox::warning(this, "Configuration Required", 
            "Please set the camera make and model before previewing the 3D scan.\n\n"
            "Use the dropdowns at the top of this tab to select:\n"
            "• Camera Make (e.g., Orbbec, Intel, FLIR)\n"
            "• Camera Model (e.g., Femto Mega, RealSense D435)");
        return;
    }
    
    try {
        // Extract the specific camera's memory object from the stack
        LAUMemoryObject cameraMemObj = extractCameraMemoryObject(currentMemoryObject, currentCameraIndex);

        if (!cameraMemObj.isValid()) {
            QMessageBox::warning(this, "Preview Error", "Unable to extract camera memory object for 3D preview.");
            return;
        }

        // Get cached LUT (generates if needed - has its own progress dialog)
        LAULookUpTable lut = getCachedLUT();

        if (lut.isNull()) {
            return;
        }

        // Convert memory object to LAUScan using the LUT without applying transform
        // (preview shows scan in camera coordinates)
        LAUScan scan = LAUTiffViewer::convertMemoryObjectToScan(cameraMemObj, lut, false);

        if (scan.isNull()) {
            return;
        }

        // Use LAUScan's built-in inspection method
        scan.inspectImage();

    } catch (const std::exception &e) {
        QMessageBox::critical(this, "3D Preview Error",
                             QString("Failed to generate 3D scan preview: %1").arg(e.what()));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMemoryObject LAUJETRWidget::extractCameraMemoryObject(const LAUMemoryObject &memoryObject, int cameraIndex)
{
    // Validate input parameters
    if (!memoryObject.isValid()) {
        return LAUMemoryObject();
    }
    
    // For multi-camera stacks, assume 4 cameras at 640x480 each
    int numCameras = 4;
    if (cameraIndex < 0 || cameraIndex >= numCameras) {
        return LAUMemoryObject();
    }
    
    // Calculate camera region parameters
    int cameraWidth = 640;
    int cameraHeight = 480;
    int startRow = cameraIndex * cameraHeight;
    int endRow = startRow + cameraHeight;
    
    if (endRow > (int)memoryObject.height()) {
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
void LAUJETRWidget::updateMasterScanIfTop()
{
    // Defer master scan generation during initial file load (it will be generated when actually needed)
    if (deferMasterScanGeneration) {
        qDebug() << "updateMasterScanIfTop: Deferred (will generate when needed for scan operations)";
        return;
    }

    // Only create master scan if this widget is configured as "top" position
    if (!getCameraPosition().toLower().endsWith("top")) {
        return;
    }

    // Check if camera make and model are set
    QString make = getCameraMake();
    QString model = getCameraModel();
    if (make.isEmpty() || model.isEmpty() || make == "Unknown" || model == "Unknown") {
        qDebug() << "updateMasterScanIfTop: Camera make/model not set. Make:" << make << "Model:" << model;
        return;
    }
    
    // Check if we have valid memory object and JETR vector
    if (!currentMemoryObject.isValid()) {
        qDebug() << "updateMasterScanIfTop: No valid memory object available";
        return;
    }
    
    QVector<double> currentJetr = getJETRVector();
    if (currentJetr.size() != 37) {
        qDebug() << "updateMasterScanIfTop: Invalid JETR vector size:" << currentJetr.size();
        return;
    }
    
    // Extract camera memory object from stacked memory object
    LAUMemoryObject cameraMemoryObject = extractCameraMemoryObject(currentMemoryObject, currentCameraIndex);
    if (!cameraMemoryObject.isValid()) {
        qDebug() << "updateMasterScanIfTop: Failed to extract camera memory object";
        return;
    }
    
    // Get cached lookup table
    LAULookUpTable lookupTable = getCachedLUT();
        
    if (!lookupTable.isValid()) {
        qDebug() << "updateMasterScanIfTop: Failed to create lookup table";
        return;
    }
    
    // Generate master scan from the top camera data
    masterScan = LAUTiffViewer::convertMemoryObjectToScan(cameraMemoryObject, lookupTable);
    
    if (masterScan.isValid()) {
        // Set parent name for identification in packet list (using "top" as identifier)
        masterScan.setParentName(QString("JETR Calibration Widget Tab %1 (Top)").arg(currentCameraIndex));
        qDebug() << "Master scan updated successfully from top camera:" << getCameraMake() << getCameraModel();
    } else {
        qDebug() << "updateMasterScanIfTop: Failed to generate valid master scan";
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAULookUpTable LAUJETRWidget::getCachedLUT()
{
    // Clear defer flag when LUT is actually needed
    deferMasterScanGeneration = false;

    QString currentMake = getCameraMake();
    QString currentModel = getCameraModel();
    QVector<double> currentJetr = getJETRVector();

    // Check if we can use cached LUT with smart partial updates
    bool lutValid = cachedLUT.isValid();

    // Full cache match - nothing changed
    if (lutValid &&
        cachedLUTMake == currentMake &&
        cachedLUTModel == currentModel &&
        cachedJETRVector == currentJetr &&
        cachedLUTDate == currentDate) {
        qDebug() << "✓ Using cached LUT for" << currentMake << currentModel;
        return cachedLUT;
    }

    // Smart caching: Check if we can update cached LUT instead of regenerating
    if (lutValid && cachedJETRVector.size() == 37 && currentJetr.size() == 37) {
        // Check which parts of JETR changed
        bool intrinsicsChanged = false;
        bool transformChanged = false;
        bool boundingBoxChanged = false;

        // Elements 0-11: Intrinsics and distortion (requires full regeneration)
        for (int i = 0; i < 12; i++) {
            if (cachedJETRVector[i] != currentJetr[i]) {
                intrinsicsChanged = true;
                break;
            }
        }

        // Elements 12-27: Transform matrix (can update without regeneration)
        for (int i = 12; i < 28; i++) {
            if (cachedJETRVector[i] != currentJetr[i]) {
                transformChanged = true;
                break;
            }
        }

        // Elements 28-33: Bounding box (can update without regeneration)
        for (int i = 28; i < 34 && i < currentJetr.size(); i++) {
            if (cachedJETRVector[i] != currentJetr[i]) {
                boundingBoxChanged = true;
                break;
            }
        }

        // Check if date change affects rotation (Orbbec Femto only)
        bool rotationLogicChanged = false;
        if (cachedLUTDate != currentDate) {
            bool isOrbbecFemto = (currentMake.toLower().contains("orbbec") &&
                                 currentModel.toLower().contains("femto"));
            if (isOrbbecFemto) {
                QDate mountingChangeDate(2025, 9, 6);
                bool cachedShouldRotate = !cachedLUTDate.isValid() || cachedLUTDate < mountingChangeDate;
                bool currentShouldRotate = !currentDate.isValid() || currentDate < mountingChangeDate;
                rotationLogicChanged = (cachedShouldRotate != currentShouldRotate);
            }
        }

        // If only transform/bounding box changed, update cached LUT
        if (!intrinsicsChanged && !rotationLogicChanged &&
            cachedLUTMake == currentMake && cachedLUTModel == currentModel) {

            if (transformChanged || boundingBoxChanged) {
                qDebug() << "⚡ Smart cache update (avoiding full regeneration):";

                if (transformChanged) {
                    // Extract transform from JETR and update cached LUT
                    QMatrix4x4 newTransform;
                    // Load transform matrix from JETR vector (elements 12-27)
                    // JETR stores in row-major order, but QMatrix4x4 needs (row,col) indexing
                    for (int row = 0; row < 4; row++) {
                        for (int col = 0; col < 4; col++) {
                            int jetrIndex = 12 + (row * 4 + col); // Row-major in JETR
                            newTransform(row, col) = static_cast<float>(currentJetr[jetrIndex]);
                        }
                    }
                    cachedLUT.setTransform(newTransform);
                    qDebug() << "  → Updated transform matrix";
                }

                if (boundingBoxChanged) {
                    LookUpTableBoundingBox bbox;
                    bbox.xMin = currentJetr[28];
                    bbox.xMax = currentJetr[29];
                    bbox.yMin = currentJetr[30];
                    bbox.yMax = currentJetr[31];
                    bbox.zMin = currentJetr[32];
                    bbox.zMax = currentJetr[33];
                    cachedLUT.setBoundingBox(bbox);
                    qDebug() << "  → Updated bounding box";
                }

                // Update cache metadata
                cachedJETRVector = currentJetr;
                cachedLUTDate = currentDate;

                return cachedLUT;
            }
        }
    }

    // Explain why full regeneration is needed
    qDebug() << "✗ Full LUT regeneration required:";
    if (!lutValid) {
        qDebug() << "  → No cached LUT available (first generation)";
    } else {
        // Provide detailed reason for regeneration
        if (cachedLUTMake != currentMake) {
            qDebug() << "  → Camera make changed: cached=" << cachedLUTMake << "→ current=" << currentMake;
        }
        if (cachedLUTModel != currentModel) {
            qDebug() << "  → Camera model changed: cached=" << cachedLUTModel << "→ current=" << currentModel;
        }
        if (cachedJETRVector.isEmpty()) {
            qDebug() << "  → No cached JETR vector";
        } else if (cachedJETRVector.size() != currentJetr.size()) {
            qDebug() << "  → JETR vector size changed: cached=" << cachedJETRVector.size() << "→ current=" << currentJetr.size();
        } else if (cachedJETRVector.size() >= 12 && currentJetr.size() >= 12) {
            // Check for intrinsics/distortion changes (elements 0-11)
            QStringList intrinsicChanges;
            if (cachedJETRVector[0] != currentJetr[0]) intrinsicChanges << "fx";
            if (cachedJETRVector[1] != currentJetr[1]) intrinsicChanges << "cx";
            if (cachedJETRVector[2] != currentJetr[2]) intrinsicChanges << "fy";
            if (cachedJETRVector[3] != currentJetr[3]) intrinsicChanges << "cy";
            for (int i = 4; i < 12; i++) {
                if (cachedJETRVector[i] != currentJetr[i]) {
                    intrinsicChanges << QString("distortion[%1]").arg(i-4);
                    break;
                }
            }
            if (!intrinsicChanges.isEmpty()) {
                qDebug() << "  → Intrinsics/distortion changed:" << intrinsicChanges.join(", ");
            }

            // Check if Orbbec Femto date changed rotation logic
            if (cachedLUTDate != currentDate) {
                bool isOrbbecFemto = (currentMake.toLower().contains("orbbec") &&
                                     currentModel.toLower().contains("femto"));
                if (isOrbbecFemto) {
                    QDate mountingChangeDate(2025, 9, 6);
                    bool cachedShouldRotate = !cachedLUTDate.isValid() || cachedLUTDate < mountingChangeDate;
                    bool currentShouldRotate = !currentDate.isValid() || currentDate < mountingChangeDate;
                    if (cachedShouldRotate != currentShouldRotate) {
                        qDebug() << "  → Orbbec Femto rotation logic changed due to date: cached="
                                 << cachedLUTDate.toString("yyyy-MM-dd") << "→ current="
                                 << currentDate.toString("yyyy-MM-dd");
                    }
                }
            }
        }
    }
    
    // Need to generate new LUT
    if (!currentMemoryObject.isValid()) {
        qDebug() << "getCachedLUT: No memory object available";
        return LAULookUpTable();
    }
    
    if (currentJetr.size() != 37) {
        qDebug() << "getCachedLUT: Invalid JETR vector size:" << currentJetr.size();
        return LAULookUpTable();
    }
    
    if (currentMake.isEmpty() || currentModel.isEmpty() || 
        currentMake == "Unknown" || currentModel == "Unknown") {
        qDebug() << "getCachedLUT: Invalid make/model:" << currentMake << currentModel;
        return LAULookUpTable();
    }
    
    // Calculate dimensions
    int totalHeight = currentMemoryObject.height();
    int cameraHeight = 480; // Standard camera height  
    int numCameras = totalHeight / cameraHeight;
    int perCameraHeight = (numCameras > 1) ? cameraHeight : totalHeight;
    
    qDebug() << "Generating new LUT for" << currentMake << currentModel;

    // Generate fresh LUT using current date for date-aware rotation logic
    qDebug() << "Generating LUT with date:" << (currentDate.isValid() ? currentDate.toString("yyyy-MM-dd") : "INVALID (will use old behavior)");

    bool lutCompleted = false;
    LAULookUpTable newLUT = LAULookUpTable::generateTableFromJETR(
        currentMemoryObject.width(),
        perCameraHeight,
        currentJetr,
        currentMake,
        currentModel,
        currentDate,
        this,
        &lutCompleted);

    // Check if LUT generation was cancelled by user
    if (!lutCompleted) {
        qDebug() << "LUT generation cancelled by user";
        return LAULookUpTable();  // Return invalid LUT
    }

    if (newLUT.isValid()) {
        // Cache the new LUT
        cachedLUT = newLUT;
        cachedLUTMake = currentMake;
        cachedLUTModel = currentModel;
        cachedJETRVector = currentJetr;
        cachedLUTDate = currentDate;
        qDebug() << "Cached new LUT for" << currentMake << currentModel << "with date" << (currentDate.isValid() ? currentDate.toString("yyyy-MM-dd") : "INVALID");
    } else {
        qDebug() << "Failed to generate LUT for" << currentMake << currentModel;
    }

    return newLUT;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setCachedLUT(const LAULookUpTable &lut)
{
    if (!lut.isValid()) {
        qDebug() << "setCachedLUT: Ignoring invalid LUT";
        return;
    }

    // Update the cached LUT
    cachedLUT = lut;

    // Update cache metadata
    cachedLUTMake = getCameraMake();
    cachedLUTModel = getCameraModel();
    cachedJETRVector = getJETRVector();
    cachedLUTDate = currentDate;

    qDebug() << "setCachedLUT: Cached LUT set for" << cachedLUTMake << cachedLUTModel;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUJETRWidget::setCurrentDate(const QDate &date)
{
    if (currentDate != date) {
        currentDate = date;
        qDebug() << "LAUJETRWidget: Set current date to" << (date.isValid() ? date.toString("yyyy-MM-dd") : "INVALID");
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QDate LAUJETRWidget::getCurrentDate() const
{
    return currentDate;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QPair<QString, QString> LAUJETRWidget::guessCameraFromJETR(const QVector<double> &jetrVector)
{
    if (jetrVector.size() < 6) {
        qWarning() << "JETR vector too short for camera identification (need at least 6 core parameters)";
        return QPair<QString, QString>();
    }
    
    // Get all known camera calibrations
    QList<LAUCameraCalibration> allCameras = LAUCameraInventoryDialog::getAllCameraCalibrations();
    
    if (allCameras.isEmpty()) {
        qWarning() << "No camera calibrations available for comparison";
        return QPair<QString, QString>();
    }
    
    double bestScore = std::numeric_limits<double>::max();
    QPair<QString, QString> bestMatch;
    int validComparisons = 0;
    
    qDebug() << QString("Comparing JETR vector (%1 values) with inventory:").arg(jetrVector.size());
    
    // Compare against each known camera using the original algorithm
    for (const LAUCameraCalibration &camera : allCameras) {
        if (!camera.isValid() || camera.jetrVector.size() < 6) {
            qDebug() << QString("  %1 - %2: NO VALID CALIBRATION").arg(camera.make).arg(camera.model);
            continue;
        }
        
        qDebug() << QString("  %1 - %2: inventory has %3 values")
                    .arg(camera.make).arg(camera.model).arg(camera.jetrVector.size());
        
        // Use the original compareJETRVectors algorithm
        double score = compareJETRVectorsOriginal(jetrVector, camera.jetrVector);
        qDebug() << QString("    Score: %1").arg(score);
        
        if (score < bestScore) {
            bestScore = score;
            bestMatch = QPair<QString, QString>(camera.make, camera.model);
        }
        validComparisons++;
    }
    
    qDebug() << QString("%1 valid comparisons, best score: %2").arg(validComparisons).arg(bestScore);
    
    // If no valid match found but have cameras available, return first one
    if (bestMatch.first.isEmpty() && !allCameras.isEmpty()) {
        const LAUCameraCalibration &firstCamera = allCameras.first();
        bestMatch = QPair<QString, QString>(firstCamera.make, firstCamera.model);
    }
    
    qDebug() << QString("Best guess %1 - %2 (score: %3)")
                .arg(bestMatch.first).arg(bestMatch.second).arg(bestScore);
    
    return bestMatch;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
double LAUJETRWidget::compareJETRVectorsOriginal(const QVector<double> &vector1, const QVector<double> &vector2)
{
    if (vector1.size() < 6 || vector2.size() < 6) {
        return std::numeric_limits<double>::max(); // Need at least 6 core intrinsic parameters
    }
    
    // Only compare core intrinsic parameters (indices 0-5) - exactly like original
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

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QPair<QString, QString> LAUJETRWidget::guessCameraFromMemoryObject(const LAUMemoryObject &memoryObject, int cameraIndex)
{
    QVector<double> jetrVector = memoryObject.jetr();
    
    if (jetrVector.isEmpty()) {
        qWarning() << "Memory object has no JETR data for camera identification";
        return QPair<QString, QString>();
    }
    
    // Handle multi-camera case
    if (jetrVector.size() % 37 == 0 && jetrVector.size() > 37) {
        int numCameras = jetrVector.size() / 37;
        
        if (cameraIndex >= numCameras) {
            qWarning() << QString("Camera index %1 out of range (0-%2)").arg(cameraIndex).arg(numCameras - 1);
            return QPair<QString, QString>();
        }
        
        // Extract JETR vector for specific camera
        QVector<double> singleCameraJetr(37);
        int startIndex = cameraIndex * 37;
        for (int i = 0; i < 37; ++i) {
            singleCameraJetr[i] = jetrVector[startIndex + i];
        }
        
        qDebug() << QString("Camera %1:").arg(cameraIndex + 1);
        return guessCameraFromJETR(singleCameraJetr);
    } else if (jetrVector.size() == 37) {
        // Single camera case
        if (cameraIndex != 0) {
            qWarning() << QString("Requested camera index %1 but only single camera data available").arg(cameraIndex);
            return QPair<QString, QString>();
        }
        
        qDebug() << QString("Camera %1:").arg(cameraIndex + 1);
        return guessCameraFromJETR(jetrVector);
    } else {
        qWarning() << QString("Invalid JETR vector size: %1 (expected 37 or multiple of 37)").arg(jetrVector.size());
        return QPair<QString, QString>();
    }
}

