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

#include "laumenuwidget.h"
#include "laulookuptable.h"
#include "laumergedocumentswidget.h"
#include "lautransformeditorwidget.h"

#include <QGuiApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QSurfaceFormat>
#include <QStyle>

using namespace libtiff;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMenuWidget::LAUMenuWidget(QWidget *parent) : QMenuBar(parent)
{
    this->setNativeMenuBar(true);
    connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)), this, SLOT(onApplicationFocusChanged(QWidget *, QWidget *)));

    splashScreen = nullptr;

    QMenu *menu = new QMenu(QString("File"));
    createNewDocumentAction           = menu->addAction(QString("New File or Project..."), QKeySequence(Qt::CTRL | Qt::Key_N), this, SLOT(onCreateNewDocument()));
    createNewDocumentAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    loadDocumentFromDiskAction        = menu->addAction(QString("Open File or Project..."), QKeySequence(Qt::CTRL | Qt::Key_O), this, SLOT(onLoadDocumentFromDisk()));
    loadDocumentFromDiskAction->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    menu->addSeparator();
    closeCurrentDocumentAction        = menu->addAction(QString("Close Project..."), QKeySequence(Qt::CTRL | Qt::Key_W), this, SLOT(onCloseCurrentDocument()));
    closeCurrentDocumentAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    closeAllDocumentsAction           = menu->addAction(QString("Close All Projects..."), QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_W), this, SLOT(onCloseAllDocuments()));
    closeAllDocumentsAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    menu->addSeparator();
    saveDocumentToDiskAction          = menu->addAction(QString("Save Current Project..."), QKeySequence(Qt::CTRL | Qt::Key_S), this, SLOT(onSaveDocumentToDisk()));
    saveDocumentToDiskAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    saveDocumentToDiskActionAs        = menu->addAction(QString("Save Current Project As..."), QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_S), this, SLOT(onSaveDocumentToDiskAs()));
    saveDocumentToDiskActionAs->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    this->addMenu(menu);

    menu = new QMenu(QString("Tools"));
    menu->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));

    filterStringList = LAUDocumentWidget::filters();
    if (filterStringList.count() > 0) {
        filtersMenu = new QMenu(QString("Filters..."));
        menu->addMenu(filtersMenu);
        for (int n = 0; n < filterStringList.count(); n++) {
            QString string = filterStringList.at(n);
            if (n < 10) {
                filterActionList.append(filtersMenu->addAction(string, QKeySequence(Qt::CTRL | (Qt::Key_0 + n)), this, SLOT(onFilterCurrentDocument())));
            } else {
                filterActionList.append(filtersMenu->addAction(string, this, SLOT(onFilterCurrentDocument())));
            }
        }
    } else {
        filtersMenu = nullptr;
    }
    menu->addSeparator();
    showAboutBoxAction = menu->addAction(QString("About"), QKeySequence(Qt::CTRL | Qt::Key_A), this, SLOT(onActionAboutBox()));
    showAboutBoxAction->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    settingsAction = menu->addAction(QString("Settings"), QKeySequence(Qt::CTRL | Qt::Key_Question), this, SLOT(onActionSettings()));
    settingsAction->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    this->addMenu(menu);

    this->onUpdateMenuTexts();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMenuWidget::~LAUMenuWidget()
{
    while (documentList.count() > 0) {
        LAUDocumentWidget *document = documentList.takeFirst();
        document->onSaveDocument();
        delete document;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onActionAboutBox()
{
    QString aboutText;
    aboutText += "<h2>LAU 3D Video Recorder</h2>";
    aboutText += "<p><b>Advanced 3D Imaging and Video Recording Platform</b></p>";
    aboutText += QString("<p>Version: Built %1 at %2</p>")
        .arg(QString::fromLocal8Bit(QByteArray(__DATE__)))
        .arg(QString::fromLocal8Bit(QByteArray(__TIME__)));

    aboutText += "<hr>";
    aboutText += "<h3>Description</h3>";
    aboutText += "<p>LAU3DVideoRecorder is a comprehensive platform for capturing, processing, ";
    aboutText += "and analyzing 3D video data from multiple camera systems. It supports ";
    aboutText += "real-time depth sensing, structured light scanning, and advanced ";
    aboutText += "computer vision applications.</p>";

    aboutText += "<h3>Key Features</h3>";
    aboutText += "<ul>";
    aboutText += "<li>Multi-camera synchronization and recording</li>";
    aboutText += "<li>Real-time 3D visualization</li>";
    aboutText += "<li>Depth map processing and filtering</li>";
    aboutText += "<li>Point cloud generation and analysis</li>";
    aboutText += "<li>Video playback and frame extraction</li>";
    aboutText += "<li>Calibration and alignment tools</li>";
#ifdef ENABLEFILTERS
    aboutText += "<li>Advanced filtering pipeline (enabled)</li>";
#endif
#ifdef ENABLECASCADE
    aboutText += "<li>Machine learning object detection (enabled)</li>";
#endif
#ifdef MERGING
    aboutText += "<li>3D scan merging capabilities (enabled)</li>";
#endif
    aboutText += "</ul>";

    aboutText += "<h3>Supported Camera Systems (this build)</h3>";
    aboutText += "<ul>";
#if defined(LUCID)
    aboutText += "<li>Lucid Vision Labs cameras</li>";
#endif
#if defined(ORBBEC)
    aboutText += "<li>Orbbec depth cameras</li>";
#endif
#if defined(KINECT) || defined(AZUREKINECT)
    aboutText += "<li>Azure Kinect</li>";
#endif
#if defined(REALSENSE)
    aboutText += "<li>Intel RealSense cameras</li>";
#endif
#if defined(VIDU)
    aboutText += "<li>Vidu 3D cameras</li>";
#endif
#if defined(VZENSE)
    aboutText += "<li>VZense depth cameras</li>";
#endif
#if defined(PRIMESENSE)
    aboutText += "<li>PrimeSense cameras</li>";
#endif
#if defined(STRUCTURECORE)
    aboutText += "<li>Structure Core cameras</li>";
#endif
#if defined(PROSILICA)
    aboutText += "<li>Prosilica GigE cameras</li>";
#endif
#if defined(BASLER) || defined(BASLERUSB)
    aboutText += "<li>Basler cameras</li>";
#endif
#if defined(VIMBA) || defined(VIMBAX)
    aboutText += "<li>Allied Vision cameras (Vimba API)</li>";
#endif
#if defined(SEEK)
    aboutText += "<li>Seek Thermal cameras</li>";
#endif
#if defined(IDS)
    aboutText += "<li>IDS cameras</li>";
#endif
#if defined(EOS)
    aboutText += "<li>Canon EOS cameras</li>";
#endif
    aboutText += "</ul>";

    aboutText += "<h3>Additional Capabilities (this build)</h3>";
    aboutText += "<ul>";
#ifdef USETCP
    aboutText += "<li>TCP/IP network streaming</li>";
#endif
#ifdef HYPERSPECTRAL
    aboutText += "<li>Hyperspectral imaging</li>";
#endif
#ifdef MOTIVE
    aboutText += "<li>Motion capture integration (Motive)</li>";
#endif
#ifdef POINTCLOUDLIBRARY
    aboutText += "<li>Point Cloud Library (PCL) support</li>";
#endif
#ifdef CALIBRATION
    aboutText += "<li>Advanced calibration tools</li>";
#endif
#ifdef AUTOSCANTODISKS
    aboutText += "<li>Automatic scan-to-disk functionality</li>";
#endif
#ifdef IOT
    aboutText += "<li>Azure IoT Hub integration</li>";
#endif
#ifdef IMU
    aboutText += "<li>IMU sensor integration</li>";
#endif
    aboutText += "</ul>";

    aboutText += "<hr>";
    aboutText += "<h3>Platform Information</h3>";
    aboutText += "<p>";
#if defined(Q_OS_WIN)
    aboutText += "Operating System: Windows<br>";
#endif
#if defined(Q_OS_MACOS)
    aboutText += "Operating System: macOS<br>";
#endif
#if defined(Q_OS_LINUX)
    aboutText += "Operating System: Linux<br>";
#endif
    aboutText += QString("Qt Version: %1<br>").arg(QT_VERSION_STR);
    aboutText += QString("OpenGL: %1.%2")
        .arg(QSurfaceFormat::defaultFormat().majorVersion())
        .arg(QSurfaceFormat::defaultFormat().minorVersion());
    aboutText += "</p>";

    aboutText += "<hr>";
    aboutText += "<p><b>Lau Consulting Inc.</b><br>";
    aboutText += "Copyright © 2017-2025 Dr. Daniel L. Lau<br>";
    aboutText += "All rights reserved.<br>";
    aboutText += "Website: <a href=\"http://drhalftone.com\">drhalftone.com</a></p>";

    aboutText += "<p><small>This software is provided \"as is\" without warranty of any kind. ";
    aboutText += "See license agreement for full terms and conditions.</small></p>";

    QMessageBox aboutBox;
    aboutBox.setWindowTitle("About LAU 3D Video Recorder");
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(aboutText);
    aboutBox.setIcon(QMessageBox::Information);
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onApplicationFocusChanged(QWidget *previous, QWidget *current)
{
    Q_UNUSED(previous);
    Q_UNUSED(current);
    qDebug() << previous << current;
    this->onUpdateMenuTexts();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onActionSettings()
{
    ;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onCreateNewDocument(LAUScan scan, QString string)
{
    LAUDocumentWidget *document = new LAUDocumentWidget(QString(), nullptr, scan.color());
    connect(document, SIGNAL(destroyed()), this, SLOT(onDocumentWidgetClosed()));
    connect(document, SIGNAL(fileCreateNewDocument(QString)), this, SLOT(onCreateNewDocument(QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCreateNewDocument(LAUScan,QString)), this, SLOT(onCreateNewDocument(LAUScan,QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCreateNewDocument(QList<LAUScan>,QString)), this, SLOT(onCreateNewDocument(QList<LAUScan>,QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileLoadDocumentFromDisk()), this, SLOT(onLoadDocumentFromDisk()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileSaveDocumentToDisk()), this, SLOT(onSaveDocumentToDisk()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileSaveDocumentToDiskAs()), this, SLOT(onSaveDocumentToDiskAs()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileSaveAllDocumentsToDisk()), this, SLOT(onSaveAllDocumentsToDisk()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCloseCurrentDocument()), this, SLOT(onCloseCurrentDocument()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCloseAllDocuments()), this, SLOT(onCloseAllDocuments()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileActionAboutBox()), this, SLOT(onActionAboutBox()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileExportImages()), this, SLOT(onExprtImages()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileMergeDocuments()), this, SLOT(onMergeDocuments()), Qt::QueuedConnection);
    connect(document, SIGNAL(editTransforms()), this, SLOT(onEditTransforms()), Qt::QueuedConnection);
    connect(document, SIGNAL(mergeLookUpTables()), this, SLOT(onMergeLookUpTablesFromDisk()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileSplitDocuments()), this, SLOT(onSplitDocuments()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileLandscapeDocuments()), this, SLOT(onRotateLandscapeDocuments()), Qt::QueuedConnection);

    documentList.append(document);

#ifndef ENABLETOUCHPANEL
    // MAKE SURE TO POSITION NEW WINDOW AT OFFSET TO PREVIOUS
    QList<QScreen*> screens = QGuiApplication::screens();
    for (int n = 0; n < screens.count(); n++){
        QRect rect = screens.at(n)->geometry();
        if (rect.contains(this->mapToGlobal(QPoint(this->width()/2,this->height()/2)))){
            if (rect.width() > MINIMUMSCREENWIDTHFORFULLSCREEN || rect.height() > MINIMUMSCREENHEIGTFORFULLSCREEN) {
                QRect geometry = document->geometry();
                geometry.moveTo(100 + 25 * documentList.count(), 100 + 25 * documentList.count());
                document->setGeometry(geometry);
            }
        }
    }
    //QRect rect = QGuiApplication::screenAt(this->mapToGlobal({this->width()/2,this->height()/2}))->availableGeometry();
#endif
    document->show();

    // INSERT THE NEW SCANS
    document->onInsertImage(scan);
    document->setTitle(string);

    // MAKE THE NEW WINDOW THE ACTIVE WINDOW
    document->activateWindow();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onCreateNewDocument(QList<LAUScan> scans, QString string)
{
    LAUVideoPlaybackColor color = (scans.isEmpty()) ? ColorUndefined : scans.first().color();

    LAUDocumentWidget *document = new LAUDocumentWidget(QString(), nullptr, color);
    connect(document, SIGNAL(destroyed()), this, SLOT(onDocumentWidgetClosed()));
    connect(document, SIGNAL(fileCreateNewDocument(QString)), this, SLOT(onCreateNewDocument(QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCreateNewDocument(LAUScan,QString)), this, SLOT(onCreateNewDocument(LAUScan,QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCreateNewDocument(QList<LAUScan>,QString)), this, SLOT(onCreateNewDocument(QList<LAUScan>,QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileLoadDocumentFromDisk()), this, SLOT(onLoadDocumentFromDisk()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileSaveDocumentToDisk()), this, SLOT(onSaveDocumentToDisk()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileSaveDocumentToDiskAs()), this, SLOT(onSaveDocumentToDiskAs()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCloseCurrentDocument()), this, SLOT(onCloseCurrentDocument()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCloseAllDocuments()), this, SLOT(onCloseAllDocuments()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileActionAboutBox()), this, SLOT(onActionAboutBox()), Qt::QueuedConnection);

    documentList.append(document);

#ifndef ENABLETOUCHPANEL
    // MAKE SURE TO POSITION NEW WINDOW AT OFFSET TO PREVIOUS
    QList<QScreen*> screens = QGuiApplication::screens();
    for (int n = 0; n < screens.count(); n++){
        QRect rect = screens.at(n)->geometry();
        if (rect.contains(this->mapToGlobal(QPoint(this->width()/2,this->height()/2)))){
            if (rect.width() > MINIMUMSCREENWIDTHFORFULLSCREEN || rect.height() > MINIMUMSCREENHEIGTFORFULLSCREEN) {
                QRect geometry = document->geometry();
                geometry.moveTo(100 + 25 * documentList.count(), 100 + 25 * documentList.count());
                document->setGeometry(geometry);
            }
        }
    }
    // QRect rect = QGuiApplication::screenAt(this->mapToGlobal({this->width()/2,this->height()/2}))->availableGeometry();
#endif
    document->show();

    // INSERT THE NEW SCANS
    document->onInsertImage(scans);
    document->setTitle(string);

    // MAKE THE NEW WINDOW THE ACTIVE WINDOW
    document->activateWindow();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onCreateNewDocument(QString string)
{
    LAUDocumentWidget *document = new LAUDocumentWidget();
    connect(document, SIGNAL(destroyed()), this, SLOT(onDocumentWidgetClosed()));
    connect(document, SIGNAL(fileCreateNewDocument(QString)), this, SLOT(onCreateNewDocument(QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCreateNewDocument(LAUScan,QString)), this, SLOT(onCreateNewDocument(LAUScan,QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCreateNewDocument(QList<LAUScan>,QString)), this, SLOT(onCreateNewDocument(QList<LAUScan>,QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileLoadDocumentFromDisk()), this, SLOT(onLoadDocumentFromDisk()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileSaveDocumentToDisk()), this, SLOT(onSaveDocumentToDisk()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileSaveDocumentToDiskAs()), this, SLOT(onSaveDocumentToDiskAs()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCloseCurrentDocument()), this, SLOT(onCloseCurrentDocument()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCloseAllDocuments()), this, SLOT(onCloseAllDocuments()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileActionAboutBox()), this, SLOT(onActionAboutBox()), Qt::QueuedConnection);

    documentList.append(document);

#ifndef ENABLETOUCHPANEL
    // MAKE SURE TO POSITION NEW WINDOW AT OFFSET TO PREVIOUS
    QList<QScreen*> screens = QGuiApplication::screens();
    for (int n = 0; n < screens.count(); n++){
        QRect rect = screens.at(n)->geometry();
        if (rect.contains(this->mapToGlobal(QPoint(this->width()/2,this->height()/2)))){
            if (rect.width() > MINIMUMSCREENWIDTHFORFULLSCREEN || rect.height() > MINIMUMSCREENHEIGTFORFULLSCREEN) {
                QRect geometry = document->geometry();
                geometry.moveTo(100 + 25 * documentList.count(), 100 + 25 * documentList.count());
                document->setGeometry(geometry);
            }
        }
    }
    //QRect rect = QGuiApplication::screenAt(this->mapToGlobal({this->width()/2,this->height()/2}))->availableGeometry();
#endif
    document->show();
    document->setTitle(string);

    // MAKE THE NEW WINDOW THE ACTIVE WINDOW
    document->activateWindow();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onLoadDocumentFromDisk()
{
    QSettings settings;
    QString directory = settings.value("LAUScan::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QString filestring = QFileDialog::getOpenFileName(nullptr, QString("Open image from disk (*.lau)"), directory, QString("*.lau"));
    if (filestring.isEmpty()) {
        if (documentList.count() == 0) {
            emit showSplashScreen();
        }
        return;
    } else {
        // LET'S TEST FILENAME TO MAKE SURE WE CAN OPEN IT
        // OPEN TIFF FILE FOR LOADING THE IMAGE FROM DISK
        TIFF *inTiff = TIFFOpen(filestring.toLocal8Bit(), "r");
        if (inTiff) {
            // CLOSE TIFF FILE
            TIFFClose(inTiff);
        } else {
            QMessageBox::warning(this, QString("Load Document"), QString("Error opening tiff file: %1").arg(LAUMemoryObject::lastTiffErrorString));
            if (documentList.count() == 0) {
                emit showSplashScreen();
            }
            return;
        }

        // MAKE SURE THERE ISNT ALREADY A DOCUMENT OPEN WITH THIS SAME NAME
        QString string = QFileInfo(filestring).baseName().toLower();
        for (int n = 0; n < documentList.count(); n++) {
            if (documentList.at(n)->baseName().toLower() == string) {
                QMessageBox::warning(nullptr, QString("File already open?"), QString("A document is already open with this filename."));
                return;
            }
        }
    }
    settings.setValue("LAUScan::lastUsedDirectory", QFileInfo(filestring).absolutePath());

    LAUDocumentWidget *document = new LAUDocumentWidget(filestring);
    connect(document, SIGNAL(destroyed()), this, SLOT(onDocumentWidgetClosed()));
    connect(document, SIGNAL(fileCreateNewDocument(QString)), this, SLOT(onCreateNewDocument(QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCreateNewDocument(LAUScan,QString)), this, SLOT(onCreateNewDocument(LAUScan,QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCreateNewDocument(QList<LAUScan>,QString)), this, SLOT(onCreateNewDocument(QList<LAUScan>,QString)), Qt::QueuedConnection);
    connect(document, SIGNAL(fileLoadDocumentFromDisk()), this, SLOT(onLoadDocumentFromDisk()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileSaveDocumentToDisk()), this, SLOT(onSaveDocumentToDisk()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileSaveDocumentToDiskAs()), this, SLOT(onSaveDocumentToDiskAs()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCloseCurrentDocument()), this, SLOT(onCloseCurrentDocument()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileCloseAllDocuments()), this, SLOT(onCloseAllDocuments()), Qt::QueuedConnection);
    connect(document, SIGNAL(fileActionAboutBox()), this, SLOT(onActionAboutBox()), Qt::QueuedConnection);

    // ADD TO OUR LIST OF OPEN DOCUMENTS
    documentList.append(document);

#ifndef ENABLETOUCHPANEL
    // MAKE SURE TO POSITION NEW WINDOW AT OFFSET TO PREVIOUS
    QList<QScreen*> screens = QGuiApplication::screens();
    for (int n = 0; n < screens.count(); n++){
        QRect rect = screens.at(n)->geometry();
        if (rect.contains(this->mapToGlobal(QPoint(this->width()/2,this->height()/2)))){
            if (rect.width() > MINIMUMSCREENWIDTHFORFULLSCREEN || rect.height() > MINIMUMSCREENHEIGTFORFULLSCREEN) {
                QRect geometry = document->geometry();
                geometry.moveTo(100 + 25 * documentList.count(), 100 + 25 * documentList.count());
                document->setGeometry(geometry);
            }
        }
    }
    //QRect rect = QGuiApplication::screenAt(this->mapToGlobal({this->width()/2,this->height()/2}))->availableGeometry();
#endif
    document->show();

    // MAKE THE NEW WINDOW THE ACTIVE WINDOW
    document->activateWindow();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onSaveDocumentToDisk()
{
    for (int n = 0; n < documentList.count(); n++) {
        if (documentList.at(n)->isActiveWindow()) {
            documentList.at(n)->onSaveDocument();
            return;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onSaveDocumentToDiskAs()
{
    for (int n = 0; n < documentList.count(); n++) {
        if (documentList.at(n)->isActiveWindow()) {
            documentList.at(n)->onSaveDocumentAs(QString());
            return;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onCloseCurrentDocument()
{
    for (int n = 0; n < documentList.count(); n++) {
        if (documentList.at(n)->isActiveWindow()) {
            LAUDocumentWidget *document = documentList.takeAt(n);
            delete document;
            return;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onCloseAllDocuments()
{
    while (documentList.count() > 0) {
        LAUDocumentWidget *document = documentList.takeFirst();
        delete document;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onDocumentWidgetClosed()
{
    // IF THE USER CLOSED THE WINDOW, WE NEED TO REMOVE IT
    // FROM THE OPEN DOCUMENTS LIST
    for (int n = 0; n < documentList.count(); n++) {
        if (QObject::sender() == documentList.at(n)) {
            documentList.removeAt(n);
            break;
        }
    }

    // IF THERE ARE NO DOCUMENTS OPEN, THEN SHOW SPLASH SCREEN AGAIN
    if (documentList.count() == 0) {
        emit showSplashScreen();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onDisableAllMenus()
{
    showAboutBoxAction->setDisabled(true);
    createNewDocumentAction->setDisabled(true);
    loadDocumentFromDiskAction->setDisabled(true);
    saveDocumentToDiskAction->setDisabled(true);
    saveDocumentToDiskActionAs->setDisabled(true);
    closeCurrentDocumentAction->setDisabled(true);
    closeAllDocumentsAction->setDisabled(true);

    if (filtersMenu) {
        filtersMenu->setHidden(true);
        filtersMenu->setDisabled(true);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onUpdateMenuTexts()
{
    if (splashScreen && splashScreen->isVisible()) {
        return;
    }

    // THESE MENU ACTIONS ARE ALWAYS AVAILABLE
    showAboutBoxAction->setEnabled(true);
    createNewDocumentAction->setEnabled(true);
    loadDocumentFromDiskAction->setEnabled(true);

    // THESE ACTIONS AREN'T AVAILABLE UNLESS AT LEAST ONE DOCUMENT IS OPEN
    if (documentList.count() == 0) {
        saveDocumentToDiskAction->setText(QString("Save Current Project..."));
        saveDocumentToDiskAction->setEnabled(false);
        saveDocumentToDiskActionAs->setText(QString("Save Current Project As..."));
        saveDocumentToDiskActionAs->setEnabled(false);
        closeCurrentDocumentAction->setText(QString("Close Current Project..."));
        closeCurrentDocumentAction->setEnabled(false);
        closeAllDocumentsAction->setEnabled(false);
        if (filtersMenu) {
            filtersMenu->setTitle(QString("Filter..."));
            filtersMenu->setEnabled(false);
        }
    } else {
        QString documentString;
        for (int n = 0; n < documentList.count(); n++) {
            if (documentList.at(n)->isActiveWindow()) {
                documentString = documentList.at(n)->baseName();
                break;
            }
        }

        // IF CURRENT WINDOW IS NOT A DOCUMENT WINDOW, DO NOTHING
        if (documentString.isEmpty()) {
            return;
        }

        saveDocumentToDiskAction->setText(QString("Save \"%1\"...").arg(documentString));
        saveDocumentToDiskAction->setEnabled(true);
        saveDocumentToDiskActionAs->setText(QString("Save \"%1\" As...").arg(documentString));
        saveDocumentToDiskActionAs->setEnabled(true);
        closeCurrentDocumentAction->setText(QString("Close \"%1\"...").arg(documentString));
        closeCurrentDocumentAction->setEnabled(true);
        if (filtersMenu) {
            filtersMenu->setTitle(QString("Filter \"%1\"...").arg(documentString));
            filtersMenu->setEnabled(true);
        }
        if (documentList.count() > 1) {
            // THESE ACTIONS ARE ONLY AVAILABLE WHEN AT LEAST TWO DOCUMENTS ARE OPEN
            closeAllDocumentsAction->setEnabled(true);
        } else {
            closeAllDocumentsAction->setEnabled(false);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMenuWidget::onFilterCurrentDocument()
{
    QString string;

    // FIRST WE NEED TO KNOW WHICH FILTER ITEM TRIGGER THE ARRIVING SIGNAL
    for (int n = 0; n < filterActionList.count(); n++) {
        if (filterActionList.at(n) == QObject::sender()) {
            string = filterStringList.at(n);
            break;
        }
    }

    // NOW SEND THE ARRIVING FILTER SIGNALS STRING TO THE ACTIVE DOCUMENT
    for (int n = 0; n < documentList.count(); n++) {
        if (documentList.at(n)->isActiveWindow()) {
            documentList.at(n)->onFilter(string);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUSplashScreen::LAUSplashScreen(QWidget *parent) : QWidget(parent)
{
    QPalette pal(palette());
    pal.setColor(QPalette::Window, QColor(64, 64, 64));
#ifdef Q_OS_WIN
    pal.setColor(QPalette::ButtonText, QColor(0, 0, 0));
#else
    //pal.setColor(QPalette::Button, QColor(96, 96, 96));
    //pal.setColor(QPalette::ButtonText, QColor(255, 255, 255));
    pal.setColor(QPalette::ButtonText, QColor(0, 0, 0));
#endif
    this->setAutoFillBackground(true);
    this->setPalette(pal);
    this->setLayout(new QVBoxLayout());
    this->setWindowFlag(Qt::WindowStaysOnTopHint);

    menuBar = new LAUMenuWidget();
    menuBar->setSplashScreenWidget(this);
    connect(menuBar, SIGNAL(hideSplashScreen()), this, SLOT(close()));
    connect(menuBar, SIGNAL(showSplashScreen()), this, SLOT(show()));

    QImage image(":/Images/3DVideoSplashScreen.jpg");

    QLabel *label = new QLabel();
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    label->setScaledContents(true);
    label->setPixmap(QPixmap::fromImage(QImage(":/Images/3DVideoSplashScreen.jpg")));
    label->setFrameStyle(QFrame::Box);
    this->layout()->addWidget(label);

    QWidget *widget = new QWidget();
    widget->setLayout(new QHBoxLayout());
    widget->layout()->setContentsMargins(0, 0, 0, 0);
    this->layout()->addWidget(widget);

    QPushButton *button = new QPushButton(QString("Quit"));
    button->setFixedWidth(150);
    connect(button, SIGNAL(clicked()), QCoreApplication::instance(), SLOT(quit()));
    widget->layout()->addWidget(button);

    ((QHBoxLayout *)(widget->layout()))->addStretch();

    button = new QPushButton(QString("New Project"));
    button->setFixedWidth(150);
    connect(button, SIGNAL(clicked()), this, SLOT(close()));
    connect(button, SIGNAL(clicked()), menuBar, SLOT(onCreateNewDocument()));
    widget->layout()->addWidget(button);

    button = new QPushButton(QString("Open Project"));
    button->setFixedWidth(150);
    connect(button, SIGNAL(clicked()), this, SLOT(close()));
    connect(button, SIGNAL(clicked()), menuBar, SLOT(onLoadDocumentFromDisk()));
    widget->layout()->addWidget(button);

    // SEE IF WE SHOULD BE FULL SCREEN FOR SMALL DISPLAYS
    QRect rect = qApp->screens().at(0)->availableGeometry();

#ifdef ENABLETOUCHPANEL
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setGeometry(rect);
    this->setFixedSize(rect.width(), rect.height());
#else
    if (rect.width() < MINIMUMSCREENWIDTHFORFULLSCREEN || rect.height() < MINIMUMSCREENHEIGTFORFULLSCREEN) {
        if (this->parent() && dynamic_cast<QMenuBar *>(this->parent()) == nullptr) {
            reinterpret_cast<QWidget *>(this->parent())->showFullScreen();
        } else {
            this->setWindowFlags(Qt::FramelessWindowHint);
            this->setFixedSize(rect.width(), rect.height());
            this->setWindowState(Qt::WindowFullScreen);
        }
    } else {
        this->setWindowFlag(Qt::SplashScreen);
        ((QVBoxLayout *)(this->layout()))->insertStretch(0, 0);
    }
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUSplashScreen::~LAUSplashScreen()
{
    delete menuBar;
}
