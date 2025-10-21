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

#ifndef LAUJETRWIDGET_H
#define LAUJETRWIDGET_H

#include <QtGui>
#include <QtCore>
#include <QWidget>
#include <QMatrix4x4>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSizePolicy>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QPushButton>
#include <QDate>

#include "laumemoryobject.h"
#include "laulookuptable.h"
#include "lauscan.h"

// Forward declaration
class LAUJETRDialog;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
struct LAUCameraInfo {
    QString make;
    QString model;
    QString position;
    bool rotated;
    
    LAUCameraInfo() : rotated(false) {}
    LAUCameraInfo(const QString &m, const QString &mod, const QString &pos = "unknown", bool rot = false)
        : make(m), model(mod), position(pos), rotated(rot) {}
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUJETRWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LAUJETRWidget(const QVector<double> &jetrVector, QWidget *parent = nullptr);
    explicit LAUJETRWidget(QWidget *parent = nullptr);
    ~LAUJETRWidget();

    void setJETRVector(const QVector<double> &jetrVector, bool updateUI = false);
    void setJETRVector(const LAULookUpTable &table, bool updateUI = false);
    void setJETRVector(const LAUMemoryObject &memoryObject, int cameraIndex = 0,
                       const QString &make = QString(), const QString &model = QString(), bool updateUI = false);
    
    // Set memory object for operations without changing JETR vector
    void setMemoryObjectOnly(const LAUMemoryObject &memoryObject, int cameraIndex = 0);

    QVector<double> getJETRVector() const;
    static QVector<double> createDefaultJETR();

    void setReadOnly(bool readOnly);
    bool isReadOnly() const;
    
    // Make and model metadata
    void setCameraMake(const QString &make);
    QString getCameraMake() const;
    void setCameraModel(const QString &model);
    QString getCameraModel() const;
    
    // Position and rotation metadata
    void setCameraPosition(const QString &position);
    QString getCameraPosition() const;
    void setCameraPositionReadOnly(bool readOnly);
    void setCameraRotation(bool rotate180);
    bool getCameraRotation() const;
    
    // Save/load camera metadata to/from settings
    void saveCameraMetadataToSettings(const QString &make, const QString &model);
    void loadCameraMetadataFromSettings(const QString &make, const QString &model);

    // Static method to select make/model from existing QSettings
    static QPair<QString, QString> getMakeAndModel(QWidget *parent = nullptr);
    
    // Enhanced version that displays the memory object image for visual identification
    static QPair<QString, QString> getMakeAndModel(const LAUMemoryObject &memoryObject, QWidget *parent = nullptr);
    
    // Multi-camera version that returns complete camera info for each camera in a stack
    static QList<LAUCameraInfo> getMultiCameraMakeAndModel(const LAUMemoryObject &memoryObject, QWidget *parent = nullptr);
    
    // Helper function to convert memory object to displayable QImage
    static QImage memoryObjectToImage(const LAUMemoryObject &memoryObject);
    
    // Extract individual camera image from stacked memory object
    static QImage extractCameraImage(const LAUMemoryObject &memoryObject, int cameraIndex);
    
    // Extract individual camera memory object from stacked memory object
    static LAUMemoryObject extractCameraMemoryObject(const LAUMemoryObject &memoryObject, int cameraIndex);
    
    // JETR validation methods
    QString makeMakeModelKey(const QString &make, const QString &model);
    static bool validateJETRVector(const QVector<double> &jetr);
    
    // Master scan management for scan merging
    void updateMasterScanIfTop();
    
    // Camera inventory access
    static QList<QPair<QString, QString>> getAllMakeModelPairs();
    
    // Camera identification from JETR vector
    static QPair<QString, QString> guessCameraFromJETR(const QVector<double> &jetrVector);
    static QPair<QString, QString> guessCameraFromMemoryObject(const LAUMemoryObject &memoryObject, int cameraIndex = 0);
    static double compareJETRVectorsOriginal(const QVector<double> &vector1, const QVector<double> &vector2);
    
    // Matrix helper methods for LAUMatrixTable
    QString matrixToMatlabString(QTableWidget *table) const;
    bool pasteFromMatlabString(QTableWidget *table, const QString &matlabString);
    
    // Cached LUT management
    LAULookUpTable getCachedLUT();
    void setCachedLUT(const LAULookUpTable &lut);

    // Date management for date-aware LUT generation
    void setCurrentDate(const QDate &date);
    QDate getCurrentDate() const;
    

public slots:
    void onParameterChanged();
    void onMakeChanged();
    void onPositionChanged();
    void onEditTransformMatrix();
    void onEditBoundingBox();
    void onPreviewRawImage();
    void onPreview3DScan();

signals:
    void jetrVectorChanged(const QVector<double> &jetrVector);
    void requestBoundingBoxEdit();

private:
    void setupUI();
    void createCameraInfoGroup();
    void createIntrinsicsGroup();
    void createExtrinsicsGroup();
    void createBoundingBoxGroup();
    void createDepthProcessingGroup();
    void createPreviewButtonsGroup();
    void updateAllDisplaysFromVector();  // Updates all read-only displays from vector
    void updateTransformMatrixDisplay();    // Updates only transform matrix display
    void updateIntrinsicDisplays();         // Updates only intrinsic parameter displays  
    void updateBoundingBoxDisplays();       // Updates only bounding box displays
    void updateDepthProcessingDisplays();   // Updates only depth processing displays
    void updateVectorFromDisplay();         // Simplified method for combo box changes
    
    // Transform matrix editor helper methods
    bool runStandardTransformEditor(QMatrix4x4 &transform); // Returns true if accepted
    bool performXYPlaneFitting(QMatrix4x4 &transform);    // XY plane fitting logic

    // Transform backup/restore methods for identity-based editing
    void saveCurrentTransform();                          // Backup current transform to backupTransform
    void setIdentityTransform();                          // Set JETR elements 12-27 to identity matrix
    void restoreBackupTransform();                        // Restore transform from backup

    // Static helper methods for make/model selection
    static QStringList getAvailableMakes();
    static QStringList getAvailableModels(const QString &make);

    // UI Layout
    QVBoxLayout *mainLayout;
    QVBoxLayout *boundingBoxLayout;  // Layout for bounding box within combined group
    QWidget *boundingBoxWidget;      // Widget containing bounding box

    // Group boxes
    QGroupBox *cameraInfoGroupBox;
    QGroupBox *intrinsicsGroupBox;
    QGroupBox *extrinsicsGroupBox;
    QGroupBox *boundingBoxGroupBox;
    QGroupBox *depthProcessingGroupBox;
    QGroupBox *previewGroupBox;

    // Camera information
    QComboBox *makeComboBox;
    QComboBox *modelComboBox;
    QComboBox *positionComboBox;
    QCheckBox *rotateCheckBox;

    // Intrinsic parameters (0-11)
    QLineEdit *fxLineEdit;
    QLineEdit *cxLineEdit;
    QLineEdit *fyLineEdit;
    QLineEdit *cyLineEdit;
    QLineEdit *k1LineEdit;
    QLineEdit *k2LineEdit;
    QLineEdit *k3LineEdit;
    QLineEdit *k4LineEdit;
    QLineEdit *k5LineEdit;
    QLineEdit *k6LineEdit;
    QLineEdit *p1LineEdit;
    QLineEdit *p2LineEdit;

    // Extrinsic parameters (12-27) - 4x4 matrix
    QLineEdit *matrixLineEdits[16];

    // Bounding box parameters (28-33)
    QLineEdit *xMinLineEdit;
    QLineEdit *xMaxLineEdit;
    QLineEdit *yMinLineEdit;
    QLineEdit *yMaxLineEdit;
    QLineEdit *zMinLineEdit;
    QLineEdit *zMaxLineEdit;

    // Depth processing parameters (34-36)
    QLineEdit *scaleFactorLineEdit;
    QLineEdit *zMinDistanceLineEdit;
    QLineEdit *zMaxDistanceLineEdit;

    // Edit buttons
    QPushButton *editTransformButton;
    QPushButton *editBoundingBoxButton;

    // Data storage
    QVector<double> jetrVector;
    bool readOnlyMode;
    
    // Memory object and camera index for 3D operations
    LAUMemoryObject currentMemoryObject;
    int currentCameraIndex;
    
    // Cached LUT to avoid regeneration
    LAULookUpTable cachedLUT;
    QString cachedLUTMake;
    QString cachedLUTModel;
    QVector<double> cachedJETRVector;
    QDate cachedLUTDate;
    
    // Current folder date for date-aware LUT generation
    QDate currentDate;

    // Transform matrix backup for identity-based editing
    QMatrix4x4 backupTransform;
    bool suppressChangeSignals;
    bool deferMasterScanGeneration;  // Flag to defer master scan generation during initial load

    // Matrix constants for LAUMatrixTable
    static const int MATRIX_SIZE = 4;

    // Static master scan for scan merging (created from "top" camera)
    static LAUScan masterScan;
};

#endif // LAUJETRWIDGET_H
