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

#ifndef LAUJETRSTANDALONEDIALOG_H
#define LAUJETRSTANDALONEDIALOG_H

#include <QtGui>
#include <QtCore>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QAbstractButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <QProgressBar>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QApplication>
#include <QSettings>
#include <QDir>
#include <QTabWidget>
#include <QCloseEvent>

#include "laujetrwidget.h"
#include "laumemoryobject.h"
#include "laulookuptable.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUJETRStandaloneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUJETRStandaloneDialog(QWidget *parent = nullptr);
    explicit LAUJETRStandaloneDialog(const QString &filePath, QWidget *parent = nullptr);
    ~LAUJETRStandaloneDialog();

    // Get the current JETR vector(s)
    QVector<double> getJETRVector() const;
    QList<QVector<double>> getJETRVectors() const;
    
    // Get camera information
    QStringList getMakes() const;
    QStringList getModels() const;
    
    // Get the current file path
    QString getCurrentFilePath() const { return currentTiffPath; }

public slots:
    void onOpenFileClicked();
    void onSaveLUTXClicked();
    void onImportLUTXClicked();
    void onJETRVectorChanged(const QVector<double> &vector);
    void onEditBoundingBox();

protected:
    void setupUI();
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void loadTiffFile(const QString &filePath);
    void updateUI();
    void enableControls(bool enabled);
    void reject() override;
    
    // Multi-camera support
    void clearTabs();
    void addJETRTab(const QVector<double> &jetrVector, const QString &tabTitle = QString());
    void addJETRTab(const QVector<double> &jetrVector, const QString &make, const QString &model, const QString &tabTitle = QString(), bool readOnly = true);
    void setJETRVectors(const QList<QVector<double>> &vectors);
    
    // File operations
    bool saveLUTXFile(const QString &filePath, const QList<QVector<double>> &jetrVectors);
    bool importLUTXFile(const QString &filePath);
    bool importFromTiffFile(const QString &filePath);
    QString getDefaultLUTXPath(const QString &tiffPath) const;

    // Background file save and LUTX copy operations
    bool saveBackgroundToInstallFolder(const QList<QVector<double>> &jetrVectors);
    bool copyLUTXToPublicPictures(const QString &lutxFilePath);
    
    // LUT caching
    void cacheLUTs(const QList<LAULookUpTable> &luts);
    void populateCacheFromWidgets();
    
    // Utility methods
    QString extractDateFromFilename(const QString &filePath) const;
    bool compareJETRVectors(const QVector<double> &vec1, const QVector<double> &vec2, double tolerance = 5e-5) const;
    QString generateJETRComparisonSummary(const QList<QVector<double>> &tiffVectors, const QList<QVector<double>> &lutxVectors) const;
    QStringList readCameraPositionsFromConfig() const;

private:
    // Main layout
    QVBoxLayout *mainLayout;
    
    // File info group
    QGroupBox *fileInfoGroupBox;
    QGridLayout *fileInfoLayout;
    QLabel *filePathLabel;
    QLineEdit *filePathLineEdit;
    QPushButton *openFileButton;
    
    // Status group
    QGroupBox *statusGroupBox;
    QVBoxLayout *statusLayout;
    QLabel *statusLabel;
    QProgressBar *progressBar;
    
    // JETR tabs container
    QTabWidget *tabWidget;
    QList<LAUJETRWidget *> jetrWidgets;
    
    // Buttons
    QPushButton *saveLUTXButton;
    QPushButton *importLUTXButton;
    QPushButton *closeButton;
    
    // Data members
    QString currentTiffPath;
    LAUMemoryObject memoryObject;
    bool fileLoaded;
    bool widgetsModified;

    // Store original JETR vectors from TIFF file for comparison
    QList<QVector<double>> originalTiffJETRVectors;

    // Import control flags
    bool skipIntrinsicsDuringImport;

    // Cached LUTs to avoid regeneration
    QList<LAULookUpTable> cachedLUTs;
    
    // Settings
    QSettings *settings;
};

#endif // LAUJETRSTANDALONEDIALOG_H