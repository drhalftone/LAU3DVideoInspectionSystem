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

#ifndef LAUCAMERASELECTIONDIALOG_H
#define LAUCAMERASELECTIONDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QList>
#include <QPair>
#include <QPushButton>
#include <QMessageBox>
#include "../Support/laumemoryobject.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCameraSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUCameraSelectionDialog(const LAUMemoryObject &memoryObject, QWidget *parent = nullptr);
    ~LAUCameraSelectionDialog();

    // Get selection results
    QList<QPair<QString, QString>> getMakeModelPairs() const;
    QStringList getPositions() const;
    QList<bool> getRotations() const;
    
    // Indexed set methods for specific camera
    void setMakeModel(int cameraIndex, const QString &make, const QString &model);
    void setPosition(int cameraIndex, const QString &position);
    void setRotation(int cameraIndex, bool rotate180);
    
    // Indexed get methods for specific camera
    QPair<QString, QString> getMakeModel(int cameraIndex) const;
    QString getPosition(int cameraIndex) const;
    bool getRotation(int cameraIndex) const;
    
    // Get number of cameras
    int getCameraCount() const { return numCameras; }

public slots:
    void accept() override;

private slots:
    void onRotationChanged();
    void onPreview3DClicked(int cameraIndex);

private:
    void setupUI();
    void populateCameraTabs();
    QImage extractCameraImage(const LAUMemoryObject &memoryObject, int cameraIndex);
    QList<QPair<QString, QString>> getAllMakeModelPairs();
    void loadMetadataFromSettings(const QString &make, const QString &model, QComboBox *positionBox, QCheckBox *rotationBox);
    
    // JETR vector comparison for intelligent guessing
    QPair<QString, QString> guessBestMakeModel(int cameraIndex, const QList<QPair<QString, QString>> &availablePairs);
    double compareJETRVectors(const QVector<double> &vector1, const QVector<double> &vector2);
    
    // 3D Preview functionality
    LAUMemoryObject extractCameraMemoryObject(const LAUMemoryObject &memoryObject, int cameraIndex);
    
    // Image rotation helpers
    void updateImageRotation(int cameraIndex);
    
    // Position validation
    bool validatePositions();

    // UI components
    QVBoxLayout *mainLayout;
    QLabel *infoLabel;
    QTabWidget *tabWidget;
    QDialogButtonBox *buttonBox;
    
    // Storage for user selections
    QList<QComboBox*> makeModelBoxes;
    QList<QComboBox*> positionBoxes;
    QList<QCheckBox*> rotationBoxes;
    QList<QPushButton*> previewButtons;
    
    // Storage for image display
    QList<QLabel*> imageLabels;
    QList<QImage> originalImages;
    
    // Data
    LAUMemoryObject memoryObject;
    int numCameras;
};

#endif // LAUCAMERASELECTIONDIALOG_H