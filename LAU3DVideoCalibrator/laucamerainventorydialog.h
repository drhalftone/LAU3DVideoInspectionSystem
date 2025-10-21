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

#ifndef LAUCAMERAINVENTORYDIALOG_H
#define LAUCAMERAINVENTORYDIALOG_H

#include <QtGui>
#include <QtCore>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QTableWidgetItem>
#include <QBrush>
#include <QColor>
#include <QMessageBox>
#include <QSettings>
#include <QHash>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

#include "../Support/laulookuptable.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
struct LAULUTGenerationTask {
    QString make;
    QString model;
    int width;
    int height;
    bool isPriority;

    LAULUTGenerationTask() : width(0), height(0), isPriority(false) {}
    LAULUTGenerationTask(const QString &m, const QString &mod, int w, int h, bool priority = false)
        : make(m), model(mod), width(w), height(h), isPriority(priority) {}

    QString taskKey() const {
        return QString("%1_%2_%3x%4").arg(make, model).arg(width).arg(height);
    }
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAULookUpTableGenerator : public QThread {
    Q_OBJECT
    
public:
    explicit LAULookUpTableGenerator(QObject *parent = nullptr);
    ~LAULookUpTableGenerator();
    
    void startBackgroundGeneration();
    void requestPriorityLUT(const QString &make, const QString &model, int width, int height);
    void pauseGeneration();
    void resumeGeneration();
    void stopGeneration();
    
protected:
    void run() override;
    
signals:
    void lutGenerated(const QString &make, const QString &model, int width, int height);
    void backgroundGenerationComplete();
    
private:
    void generateBackgroundTasks();
    LAULUTGenerationTask getNextTask();
    void processTask(const LAULUTGenerationTask &task);
    
    QQueue<LAULUTGenerationTask> backgroundQueue;
    QQueue<LAULUTGenerationTask> priorityQueue;
    QMutex queueMutex;
    QWaitCondition taskAvailable;
    QWaitCondition pauseCondition;
    
    bool shouldStop;
    bool isPaused;
    bool backgroundComplete;
    
    static const QList<QSize> STANDARD_DIMENSIONS;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
struct LAUCameraCalibration {
    QString make;
    QString model;
    QVector<double> jetrVector;
    
    LAUCameraCalibration() {}
    LAUCameraCalibration(const QString &m, const QString &mod, const QVector<double> &jetr)
        : make(m), model(mod), jetrVector(jetr) {}
    
    bool isValid() const {
        return !make.isEmpty() && !model.isEmpty() && jetrVector.size() == 37;
    }
    
    QString makeModelKey() const {
        return QString("%1_%2").arg(make, model);
    }
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCameraInventoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUCameraInventoryDialog(QWidget *parent = nullptr);
    ~LAUCameraInventoryDialog();
    
    // Camera calibration management methods
    static QList<LAUCameraCalibration> getAllCameraCalibrations();
    static LAUCameraCalibration getCameraCalibration(const QString &make, const QString &model);
    static void saveCameraCalibration(const LAUCameraCalibration &calibration);
    static bool hasCameraCalibration(const QString &make, const QString &model);
    static QStringList getAvailableMakes();
    static QStringList getAvailableModels(const QString &make);
    static void loadDefaultCalibrations();
    
    // LUT caching methods
    static LAULookUpTable getCachedLUT(const QString &make, const QString &model, 
                                       int width, int height, QWidget *parent = nullptr);
    static void cacheLUT(const QString &make, const QString &model,
                        int width, int height, const LAULookUpTable &lut);
    static void invalidateLUTCache(const QString &make, const QString &model);
    static void clearLUTCache();
    
    // Background LUT generation
    static void startBackgroundLUTGeneration();
    static void stopBackgroundLUTGeneration();
    static void pauseBackgroundLUTGeneration();
    static void resumeBackgroundLUTGeneration();
    static LAULookUpTable getCachedLUTWithPriority(const QString &make, const QString &model,
                                                    int width, int height, QWidget *parent = nullptr);
    
    // LUT cache check and add methods
    static bool hasLUTInCache(const QString &make, const QString &model, int width, int height);
    static void addLUTToCache(const QString &make, const QString &model, int width, int height, 
                              const LAULookUpTable &lut);
    
    // Monitor access methods
    static int getCacheSize() { 
        QMutexLocker locker(&lutCacheMutex);
        return lutCache.size(); 
    }
    static LAULookUpTableGenerator* getBackgroundGenerator() { return backgroundGenerator; }

public slots:
    void refreshInventory();
    void accept() override;
    void reject() override;

private slots:
    void onDisplayClicked();
    void onDeleteClicked();
    void onImportClicked();
    void onExportClicked();
    void onSelectionChanged();
    void onTableRightClicked(const QPoint &pos);

private:
    void setupUI();
    void updateButtonStates();
    void populateTable();
    void updateHeaderLabel();
    
    // Staging methods for transactional behavior
    void initializeStaging();
    void commitChangesToSettings();
    void rollbackChanges();

    // UI Components
    QVBoxLayout *mainLayout;
    QLabel *headerLabel;
    QTableWidget *table;
    QLabel *infoLabel;
    QDialogButtonBox *buttonBox;
    
    // Action buttons
    QPushButton *displayButton;
    QPushButton *deleteButton;
    QPushButton *importButton;
    QPushButton *exportButton;
    
    // Staging data for transactional behavior
    QStringList deletedKeys;         // Keys marked for deletion
    QStringList importedKeys;        // Keys that were imported (for rollback)
    QStringList originalKeys;        // Keys that existed when dialog opened
    bool hasChanges;
    
    // Static LUT cache and background generation
    static LAULookUpTableGenerator *backgroundGenerator;
    
public:
    static QHash<QString, LAULookUpTable> lutCache;
    static QMutex lutCacheMutex;  // Mutex for thread-safe cache access
    static QString makeLUTCacheKey(const QString &make, const QString &model, 
                                   int width, int height);
};

#endif // LAUCAMERAINVENTORYDIALOG_H