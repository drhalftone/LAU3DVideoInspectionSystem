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

#ifndef LAUDOCUMENTWIDGET_H
#define LAUDOCUMENTWIDGET_H

#include <QFont>
#include <QMutex>
#include <QDebug>
#include <QThread>
#include <QWidget>
#include <QMenuBar>
#include <QGroupBox>
#include <QToolButton>
#include <QModelIndex>
#include <QStringList>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidgetItem>

#include "laudocument.h"
#include "lau3dmultiscanglwidget.h"

#ifdef CASSI
#include "laucodedapertureglfilter.h"
#endif

#if defined(BASLERUSB) & defined(ENABLECALIBRATION)
#include "laustereocaltagglfilter.h"
#endif

#ifdef ENABLEPOINTCLOUDLIBRARY
#include "lauplyglwidget.h"
#endif

class LAUImageListWidget;
class LAUImageStackWidget;
class LAUImageStackTextLabel;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUDocumentWidget : public QWidget
{
    Q_OBJECT

public:
    LAUDocumentWidget(QString filenameString = QString(), QWidget *parent = nullptr, LAUVideoPlaybackColor color = LAUVideoPlaybackColor::ColorUndefined);
    ~LAUDocumentWidget();

    QString filename()
    {
        return (document->filename());
    }

    void setTitle(QString string)
    {
        if (string.isEmpty() == false){
            this->setWindowTitle(string);
        }
    }

    QString baseName()
    {
        return (documentString.split("/").last());
    }

    void filterImage(QString operation);
    QString targetScanner();

    QList<LAUScan> images() const
    {
        if (document) {
            return (document->images());
        }
        return (QList<LAUScan>());
    }

    static QStringList filters();
    static QStringList scanners();

protected:
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::RightButton) {
            onContextualMenuTriggered(event->pos());
        }
    }

public slots:
    void onSaveDocument()
    {
        mutex.lock();
        document->save();
        mutex.unlock();
    }

    void onSaveDocumentAs(QString filenameString = QString())
    {
        mutex.lock();
        document->saveToDisk(filenameString);
        this->setWindowTitle(QFileInfo(document->filename()).baseName());
        mutex.unlock();
    }

    void onSendDocumentToCloud()
    {
        mutex.lock();
        document->sendToCloud();
        mutex.unlock();
    }

    void onImportDocument();
    void onImportCalTagObjects();
    void onContextualMenuTriggered(QPoint pos);
    void onSwapImage(QString stringA, QString stringB);
    void onInspectImage(QString string);
    void onDuplicateImage(QString string);
    void onRemoveImage(QString string);
    void onUpdateNumberOfImages();

    void onInsertImage();
    void onInsertImage(LAUScan scan);
    void onInsertImage(LAUMemoryObject image);
    void onInsertImage(QList<LAUScan> scanList);
    void onInsertImage(QList<LAUMemoryObject> imageList);

    void onFilter(QString operation = QString());
    void onExecuteAsDialogComplete();
    void onFilterScannerTool();
    void onFilterVideoTool();

    void onFileCreateNewDocument(QString string = QString())
    {
        emit fileCreateNewDocument(string);
    }

    void onFileLoadDocumentFromDisk()
    {
        emit fileLoadDocumentFromDisk();
    }

    void onFileSaveDocumentToDisk()
    {
        emit fileSaveDocumentToDisk();
    }

    void onFileSaveDocumentToDiskAs()
    {
        emit fileSaveDocumentToDiskAs();
    }

    void onFileSaveDocumentToDiskAsRotated()
    {
        emit fileSaveDocumentToDiskAsRotated();
    }

    void onFileSaveAllDocumentsToDisk()
    {
        emit fileSaveAllDocumentsToDisk();
    }

    void onFileCloseCurrentDocument()
    {
        emit fileCloseCurrentDocument();
    }

    void onFileCloseAllDocuments()
    {
        emit fileCloseAllDocuments();
    }

    void onFileActionAboutBox()
    {
        emit fileActionAboutBox();
    }

    void onFileSplitDocuments()
    {
        emit fileSplitDocuments();
    }

    void onFileMergeDocuments()
    {
        emit fileMergeDocuments();
    }

    void onFileExportImages()
    {
        emit fileExportImages();
    }

    void onEditTransforms()
    {
        emit editTransforms();
    }

    void onMergeLookUpTables()
    {
        emit mergeLookUpTables();
    }

    void onFileLandscapeDocuments()
    {
        emit fileLandscapeDocuments();
    }

#ifdef HYPERSPECTRAL
    void onFilterHyperspectral();
    void onFilterHyperspectralMerge();
#endif

#ifdef SANDBOX
    void onFilterSandboxTool();
    void onFilterSandboxCalibrationTool();
#endif

#ifdef ENABLECALIBRATION
    void onFilterCalibration();
    void onFilterCalTagTool();
    void onFilterBinaryTool();
    void onFilterGenerateLUT();
    void onFilterAlphaTrimmedMean();
#ifdef EOS
    void onFilterRasterize();
#endif
#endif

#ifdef ENABLECLASSIFIER
    void onFilterYolo();
#endif
#ifdef ENABLECASCADE
    void onFilterCascade();
#endif
    void onFilterBackgroundTool();
    void onFilterGreenScreenTool();
    void onFilterSetXYPlane();
    void onFilterScanVelmexRailOnUserPath();

#ifdef ENABLEPOINTMATCHER
    void onFilterMergeTool();
    void onFilterBCSTracking();
    void onFilterAutoMergeTool();
    void onFilterSymmetryTool();
    void onFilterTracking();
#endif

private:
    QMutex mutex;
    LAUVideoPlaybackColor defaultColorSpace;
    LAUDocument *document;
    LAUImageListWidget *imageListWidget;
    LAU3DMultiScanGLWidget *imageStackWidget;
    QGroupBox *imageStackGroupBox;
    QString documentString;

    // VARIABLES FOR ANY ATTACHED SCANNERS
    LAUVideoPlaybackColor scannerColor;
    QMatrix4x4 scannerTransform;
    QString scannerSoftware;
    QString scannerMake;
    QString scannerModel;
    bool saveOnNewScanFlag;

    void launchAsDialog(QWidget *widget, QString string = QString());

signals:
    void fileCreateNewDocument(QList<LAUScan>, QString);
    void fileCreateNewDocument(LAUScan, QString);
    void fileCreateNewDocument(QString);
    void fileLoadDocumentFromDisk();
    void fileSaveDocumentToDisk();
    void fileSaveDocumentToDiskAs();
    void fileSaveDocumentToDiskAsRotated();
    void fileSaveAllDocumentsToDisk();
    void fileCloseCurrentDocument();
    void fileCloseAllDocuments();
    void fileSplitDocuments();
    void fileExportImages();
    void fileMergeDocuments();
    void fileLandscapeDocuments();
    void fileActionAboutBox();
    void editTransforms();
    void mergeLookUpTables();
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit LAUListWidget(QWidget *parent = NULL) : QListWidget(parent) { ; }

protected:
    void keyPressEvent(QKeyEvent *event)
    {
        if (event->key() == Qt::Key_PageUp) {
            if (this->count() > 0) {
                this->setCurrentRow((this->count() + this->currentRow() + 1) % this->count());
            }
        } else if (event->key() == Qt::Key_PageDown) {
            if (this->count() > 0) {
                this->setCurrentRow((this->count() + this->currentRow() - 1) % this->count());
            }
        } else if (event->key() == Qt::Key_Escape) {
            ;
        } else {
            QListWidget::keyPressEvent(event);
        }
    }

    void mousePressEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::RightButton) {
            emit contextualMenuTriggered(event->globalPosition().toPoint());
        } else {
            QListWidget::mousePressEvent(event);
        }
    }

signals:
    void contextualMenuTriggered(QPoint pos);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUImageListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LAUImageListWidget(QStringList stringList = QStringList(), QWidget *parent = NULL);
    ~LAUImageListWidget();

    QString currentItem();
    QStringList imageList();
    void clearImageList();
    void insertImage(QString string, int index = -1);
    void removeImage(QString string);
    void insertImages(QStringList imageList);
    void removeImages(QStringList imageList);

    bool stringAlreadyInList(QString string);

    int count() const
    {
        if (imageListWidget) {
            return (imageListWidget->count());
        } else {
            return (0);
        }
    }

protected:
    void mousePressEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::RightButton) {
            onContextualMenuTriggered(event->globalPosition().toPoint());
        } else {
            QWidget::mousePressEvent(event);
        }
    }

private:
    LAUListWidget *imageListWidget;

private slots:
    void onInsertButtonClicked()
    {
        emit insertImageAction();
    }

    void onContextualMenuTriggered(QPoint pos)
    {
        emit contextualMenuTriggered(pos);
    }

    void onRemoveButtonClicked();
    void onMoveUpButtonClicked();
    void onMoveDownButtonClicked();
    void onDuplicateButtonClicked();
    void onCurrentItemChanged(QListWidgetItem *item = nullptr);
    void onItemDoubleClicked(QListWidgetItem *item);

signals:
    void insertImageAction();
    void contextualMenuTriggered(QPoint pos);
    void duplicateImageAction(QString string);
    void removeImageAction(QString string);
    void swapImageAction(QString stringA, QString stringB);
    void currentItemDoubleClicked(QString filename);
    void currentItemChanged(QString filename);
};

#endif // LAUDOCUMENTWIDGET_H
