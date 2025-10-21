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

#include "laudocumentwidget.h"
#include "lauconstants.h"
#include <QInputDialog>
#include <QStyle>

#ifndef STANDALONE_EOS
#include "lau3dvideorecordingwidget.h"
#include "laucameraclassifierdialog.h"
#endif

#ifdef EOS
#include "laueoswidget.h"
#endif

#ifdef CASSI
#include "cassi.h"
#endif

#ifdef SANDBOX
#include "lau3dsandboxcalibrationwidget.h"
#include "lau3dsandboxvideorecorderwidget.h"
#endif

#ifdef MOTIVE
#include "lau3dmotivevideorecorderwidget.h"
#endif

#ifdef ENABLECLASSIFIER
#include "laudeepnetworkobject.h"
#endif

#ifdef ENABLECASCADE
#include "laucascadeclassifierglfilter.h"
#endif

#ifdef HYPERSPECTRAL
#include "lau3dhyperspectralwidget.h"
#endif

#ifdef USETCP
#include "lau3dvideotcpwidget.h"
#include "lau3dvideotcpserver.h"
#include "lau3dvideotcpmultichannelwidget.h"
#endif

#include "laubackgroundglfilter.h"
#include "laugreenscreenglfilter.h"

#ifdef ENABLEPOINTMATCHER
#include "laumergescanwidget.h"
#include "lau3dtrackingwidget.h"
#include "lau3dbcstrackingwidget.h"
#ifdef ENABLECALIBRATION
#include "lausetxyplanewidget.h"
#endif
#endif

#ifdef ENABLECALIBRATION
#include "lausetxyplanewidget.h"
#include "laucaltagglfilter.h"
#include "laubinarizeglfilter.h"
#include "lau3droundgridwidget.h"
#include "laucaltagscanglfilter.h"
#include "lau3dcalibrationwidget.h"
#include "laubinarizescanglfilter.h"
#include "laualphatrimmedmeanglfilter.h"
#include "laugeneratelookuptablewidget.h"
#ifdef EOS
#include "laurasterizeglfilter.h"
#endif
#endif


using namespace libtiff;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUDocumentWidget::LAUDocumentWidget(QString filenameString, QWidget *parent, LAUVideoPlaybackColor color) : QWidget(parent), defaultColorSpace(color), documentString(filenameString), document(nullptr), saveOnNewScanFlag(false), imageStackWidget(nullptr)
{
    QPalette pal(palette());
    pal.setColor(QPalette::Window, QColor(64, 64, 64));
    pal.setColor(QPalette::WindowText, QColor(164, 164, 164));
    //pal.setColor(QPalette::Button, QColor(96, 96, 96));
    pal.setColor(QPalette::NoRole, QColor(164, 164, 164));

    this->setLayout(new QHBoxLayout());
#ifdef Q_OS_WIN
    this->layout()->setContentsMargins(6, 6, 6, 6);
#else
    this->layout()->setContentsMargins(6, 6, 6, 6);
#endif
    this->setAttribute(Qt::WA_DeleteOnClose, true);
    this->setAutoFillBackground(true);
    this->setPalette(pal);

    this->setWindowFlag(Qt::CustomizeWindowHint, true);
    this->setWindowFlag(Qt::WindowCloseButtonHint, true);
    this->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    this->setWindowFlag(Qt::WindowMaximizeButtonHint, true);

    imageListWidget = new LAUImageListWidget(QStringList());
    imageListWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    connect(imageListWidget, SIGNAL(duplicateImageAction(QString)), this, SLOT(onDuplicateImage(QString)));
    connect(imageListWidget, SIGNAL(removeImageAction(QString)), this, SLOT(onRemoveImage(QString)));
    connect(imageListWidget, SIGNAL(currentItemDoubleClicked(QString)), this, SLOT(onInspectImage(QString)));
    connect(imageListWidget, SIGNAL(insertImageAction()), this, SLOT(onInsertImage()));
    connect(imageListWidget, SIGNAL(swapImageAction(QString, QString)), this, SLOT(onSwapImage(QString, QString)));
    connect(imageListWidget, SIGNAL(contextualMenuTriggered(QPoint)), this, SLOT(onContextualMenuTriggered(QPoint)));

    imageStackGroupBox = new QGroupBox(QString("Image List"));
    imageStackGroupBox->setPalette(pal);
    imageStackGroupBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    imageStackGroupBox->setFixedWidth(315);
    imageStackGroupBox->setLayout(new QVBoxLayout());
    imageStackGroupBox->layout()->setContentsMargins(6, 6, 6, 6);
    imageStackGroupBox->layout()->addWidget(imageListWidget);
    this->layout()->addWidget(imageStackGroupBox);

    if (documentString.isEmpty()) {
        documentString = QString("Untitled%1").arg(LAUDocument::untitledDocumentCounter);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUDocumentWidget::~LAUDocumentWidget()
{
    if (document) {
        delete document;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::showEvent(QShowEvent *event)
{
    // SEE IF WE SHOULD BE FULL SCREEN FOR SMALL DISPLAYS
#ifdef ENABLETOUCHPANEL
    this->window()->showMaximized();
#else
    this->setFixedWidth(340);
    this->setMinimumHeight(LAU_MIN_WIDGET_HEIGHT);

    QList<QScreen*> screens = QGuiApplication::screens();
    for (int n = 0; n < screens.count(); n++){
        QRect rect = screens.at(n)->geometry();
        if (rect.contains(this->mapToGlobal(QPoint(this->width()/2,this->height()/2)))){
            if (rect.width() < MINIMUMSCREENWIDTHFORFULLSCREEN || rect.height() < MINIMUMSCREENHEIGTFORFULLSCREEN) {
                this->window()->showMaximized();
            }
        }
    }
#endif

    if (document == nullptr) {
        // CALL THE UNDERLYING CLASSES METHOD
        QWidget::showEvent(event);

        document = new LAUDocument(documentString, this);
        documentString = QFileInfo(document->filename()).baseName();

        imageListWidget->insertImages(document->parentStringList());

        if (document->count() > 0) {
            LAUScan scan = document->image(document->parentStringList().takeFirst());
            imageStackWidget = new LAU3DMultiScanGLWidget(qMin((unsigned int)1024, scan.width()), qMin((unsigned int)768, scan.height()), scan.color());
        } else {
            if (defaultColorSpace == ColorUndefined) {
#if defined(XIMEA)
                imageStackWidget = new LAU3DMultiScanGLWidget(LAU_CAMERA_DEFAULT_WIDTH, LAU_CAMERA_DEFAULT_HEIGHT, ColorGray);
#elif defined(KINECT)
                imageStackWidget = new LAU3DMultiScanGLWidget(LAU_PRIMESENSE_WIDTH, LAU_PRIMESENSE_HEIGHT, ColorXYZWRGBA);
#elif defined(VIMBA)
                imageStackWidget = new LAU3DMultiScanGLWidget(800, 600, ColorXYZWRGBA);
#elif defined(IDS)
                imageStackWidget = new LAU3DMultiScanGLWidget(800, 600, ColorRGB);
#elif defined(EOS)
                imageStackWidget = new LAU3DMultiScanGLWidget(800, 600, ColorRGB);
#else
                imageStackWidget = new LAU3DMultiScanGLWidget(LAU_CAMERA_DEFAULT_WIDTH, LAU_CAMERA_DEFAULT_HEIGHT, ColorXYZWRGBA);
#endif
            } else {
                imageStackWidget = new LAU3DMultiScanGLWidget(800, 600, defaultColorSpace);
            }
        }

        if (imageStackWidget) {
            imageStackGroupBox = new QGroupBox(QString("Image Preview"));
            imageStackGroupBox->setPalette(this->palette());
            imageStackGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            imageStackGroupBox->setLayout(new QVBoxLayout());
            imageStackGroupBox->layout()->setContentsMargins(6, 6, 6, 6);

            this->layout()->addWidget(imageStackGroupBox);

            imageStackWidget->setMutuallyExclusive(true);
            imageStackWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            imageStackWidget->setMinimumWidth(320);
            imageStackGroupBox->layout()->addWidget(imageStackWidget);

            // MAKE CONNECTIONS BETWEEN IMAGE LIST WIDGET AND STACK PREVIEW WIDGET
            connect(imageListWidget, SIGNAL(currentItemChanged(QString)), imageStackWidget, SLOT(onEnableScan(QString)));
        }

        // RESYNCHRONIZE IMAGE LIST AND DOCUMENT IMAGES
        onUpdateNumberOfImages();

        QStringList stringList = document->parentStringList();
        if (stringList.count() > 0) {
            QProgressDialog progressDialog(QString("Generating previews..."), QString(), 0, stringList.count(), this, Qt::Sheet);
            for (int n = stringList.count() - 1; n >= 0; n--) {
                imageStackWidget->onInsertScan(document->image(stringList.at(n)));
                progressDialog.setValue(stringList.count() - n);
                qApp->processEvents();
            }
            progressDialog.setValue(stringList.count());
        }

        // UPDATE THE WINDOW TITLE
        this->window()->setWindowTitle(documentString);
    } else {
        // CALL THE UNDERLYING CLASSES METHOD
        QWidget::showEvent(event);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::closeEvent(QCloseEvent *event)
{
    if (mutex.tryLock()) {
        while (document->isDirty()) {
            int ret = QMessageBox::warning(this, QString("LAU3DVideoRecorder Document"),
                                           QString("Save changes to the LAU3DVideoRecorder Document \"%1\" before closing?").arg(document->filename()),
                                           QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);
            if (ret == QMessageBox::Cancel) {
                event->ignore();
                mutex.unlock();
                return;
            } else if (ret == QMessageBox::No) {
                document->makeClean();
            } else {
                // SYNCHRONIZE DOCUMENT TO ORDER IN IMAGE LIST
                document->orderChannels(imageListWidget->imageList());
                document->save();
            }
        }
        QWidget::closeEvent(event);
        mutex.unlock();
    } else {
        event->ignore();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUDocumentWidget::targetScanner()
{
    QStringList strings;
#ifdef EOS
    strings << QString("Canon EOS");
#endif
#ifdef IDS
    strings << QString("IDS");
#endif
#ifdef KINECT
    strings << QString("Kinect NIR");
    strings << QString("Kinect RGB");
    strings << QString("Kinect NIR-D");
    strings << QString("Kinect RGB-D");
#endif
#ifdef LUCID
    strings << QString("Lucid NIR");
    strings << QString("Lucid RGB");
    strings << QString("Lucid NIR-D");
    strings << QString("Lucid RGB-D");
#endif
#ifdef VIDU
    strings << QString("Vidu NIR");
    strings << QString("Vidu RGB");
    strings << QString("Vidu NIR-D");
    strings << QString("Vidu RGB-D");
#endif
#ifdef ORBBEC
    strings << QString("Orbbec NIR");
    strings << QString("Orbbec RGB");
    strings << QString("Orbbec NIR-D");
    strings << QString("Orbbec RGB-D");
#endif
#ifdef VZENSE
    strings << QString("VZense NIR");
    strings << QString("VZense NIR-D");
#endif
#if defined(PRIMESENSE)
    strings << QString("Prime Sense NIR-D");
    strings << QString("Prime Sense RGB-D");
#endif
#if defined(PROSILICA)
    strings << QString("Prosilica ARG"); // AUGMENTED REALITY SCANNER
    strings << QString("Prosilica GRY"); // MONOCHROME GRAYSCALE VIDEO
    strings << QString("Prosilica RGB"); // COLOR VIDEO
    strings << QString("Prosilica IOS"); // DUAL-FREQUENCY SCANNER
    strings << QString("Prosilica LCG"); // THREE FREQUENCY SCANNER
    strings << QString("Prosilica AST"); // ACTIVE STEREOVISION
    strings << QString("Prosilica PST"); // PASSIVE STEREOVISION
    strings << QString("Prosilica DPR"); // DUAL PROJECTOR
#elif defined(VIMBA)
    strings << QString("Allied Vision ARG"); // AUGMENTED REALITY SCANNER
    strings << QString("Allied Vision GRY"); // MONOCHROME GRAYSCALE VIDEO
    strings << QString("Allied Vision RGB"); // COLOR VIDEO
    strings << QString("Allied Vision IOS"); // DUAL-FREQUENCY SCANNER
    strings << QString("Allied Vision LCG"); // THREE FREQUENCY SCANNER
    strings << QString("Allied Vision AST"); // ACTIVE STEREOVISION
    strings << QString("Allied Vision PST"); // PASSIVE STEREOVISION
    strings << QString("Allied Vision DPR"); // DUAL PROJECTOR
#elif defined(BASLERUSB)
    strings << QString("Basler ARG"); // AUGMENTED REALITY SCANNER
    strings << QString("Basler GRY"); // MONOCHROME GRAYSCALE VIDEO
    strings << QString("Basler RGB"); // COLOR VIDEO
    strings << QString("Basler IOS"); // DUAL-FREQUENCY SCANNER
    strings << QString("Basler LCG"); // THREE FREQUENCY SCANNER
    strings << QString("Basler AST"); // ACTIVE STEREOVISION
    strings << QString("Basler PST"); // PASSIVE STEREOVISION
    strings << QString("Basler DPR"); // DUAL PROJECTOR
#ifdef KINECT
    strings << QString("Basler TOF"); // BASLER PLUS KINECT TOF SENSOR
#endif
#endif
#if defined(REALSENSE)
#ifdef USETCP
    strings << QString("Real Sense TCP");
#endif
    strings << QString("Real Sense GRY");
    strings << QString("Real Sense RGB");
    strings << QString("Real Sense NIR-D");
    strings << QString("Real Sense RGB-D");
#endif
#if defined(SEEK)
    strings << QString("Seek Thermal");
#endif
#ifdef XIMEA
    strings << QString("Ximea");
#endif
    if (strings.isEmpty()) {
        QMessageBox::information(this, QString("Target Scanner"), QString("No cameras detected."));
    } else if (strings.count() == 1) {
        return (strings.first());
    } else {
        bool okay;
        QSettings settings;
        int index = settings.value(QString("LAUDocumentWidget::targetScanner()"), 0).toInt();
        QString string = QInputDialog::getItem(this, QString("Select Scanner"), QString("Select input device"), strings, index, false, &okay);
        if (okay) {
            index = strings.indexOf(string);
            settings.setValue(QString("LAUDocumentWidget::targetScanner()"), index);
            string.replace(QString("Allied Vision"), QString("Prosilica"));
            string.replace(QString("Basler"), QString("Prosilica"));
            return (string);
        }
    }
    return (QString());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUDocumentWidget::filters()
{
    QStringList list;
    
    // Always add Snap-shot and Video at the top
    list << "Snap-shot";
    list << "Video";
    
#ifdef STANDALONE_EOS
    list << "Launch EOS";
#else
    // list << "Scan User Path";  // Commented out as requested
#endif
#ifdef HYPERSPECTRAL
    list << "Hyperspectral";
    list << "Hyperspectral Merge";
#endif
#ifdef ENABLECALIBRATION
    list << "Set XY Plane";
    list << "Calibration";
    list << "CalTag";
    list << "Generate LUT";
#ifdef EOS
    list << "Rasterize";
#endif
#endif // ENABLECALIBRATION
#ifdef ENABLECASCADE
    list << "Cascade Classifier";
#endif
    list << "Background";
    list << "Green Screen";
#ifdef ENABLECLASSIFIER
    list << "YOLO Classifier";
#endif

#if defined(SANDBOX)
    list << "Sandbox Calibration";
    list << "Sandbox";
#endif

#ifdef ENABLEPOINTMATCHER
    list << "Auto Merge";
    list << "Merge";
    list << "Symmetry";
    list << "BCS Tracking";
    list << "Tracking";
#endif // ENABLEPOINTMATCHER

    // Don't sort the entire list - keep Snap-shot and Video at the top
    // Only sort the items after the first two
    if (list.size() > 2) {
        QStringList topItems = list.mid(0, 2);  // Extract Snap-shot and Video
        QStringList otherItems = list.mid(2);   // Extract all other items
        otherItems.sort(Qt::CaseInsensitive);   // Sort only the other items
        list = topItems + otherItems;           // Combine them back
    }
    return (list);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onContextualMenuTriggered(QPoint pos)
{
    QMenu contextMenu(QString("Actions"), nullptr);

    QMenu *fileMenu = contextMenu.addMenu(QString("File"));
    fileMenu->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    QAction *newAction = fileMenu->addAction(QString("New File or Project..."), this, SLOT(onFileCreateNewDocument()));
    newAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    QAction *openAction = fileMenu->addAction(QString("Open File or Project..."), this, SLOT(onFileLoadDocumentFromDisk()));
    openAction->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    fileMenu->addSeparator();
    QAction *closeAction = fileMenu->addAction(QString("Close Project..."), this, SLOT(onFileCloseCurrentDocument()));
    closeAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    QAction *closeAllAction = fileMenu->addAction(QString("Close All Projects..."), this, SLOT(onFileCloseAllDocuments()));
    closeAllAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    fileMenu->addSeparator();
    QAction *saveAction = fileMenu->addAction(QString("Save Current Project..."), this, SLOT(onFileSaveDocumentToDisk()));
    saveAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    QAction *saveAsAction = fileMenu->addAction(QString("Save Current Project As..."), this, SLOT(onFileSaveDocumentToDiskAs()));
    saveAsAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    fileMenu->addSeparator();
    QAction *aboutAction = fileMenu->addAction(QString("About"), this, SLOT(onFileActionAboutBox()));
    aboutAction->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));

    QStringList filterStrings = this->filters();
    if (filterStrings.count() > 0) {
        QMenu *filterMenu = contextMenu.addMenu(QString("Tools"));
        filterMenu->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
        
        // Add Snap-shot and Video (always first two items)
        if (filterStrings.count() >= 1) {
            QAction *snapAction = filterMenu->addAction(filterStrings.at(0), this, SLOT(onFilter()));  // Snap-shot
            snapAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        }
        if (filterStrings.count() >= 2) {
            QAction *videoAction = filterMenu->addAction(filterStrings.at(1), this, SLOT(onFilter()));  // Video
            videoAction->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
        }
        
        // Add separator if there are more filters beyond Snap-shot and Video
        if (filterStrings.count() > 2) {
            filterMenu->addSeparator();
            
            // Add the remaining filters
            for (int n = 2; n < filterStrings.count(); n++) {
                QAction *filterAction = filterMenu->addAction(filterStrings.at(n), this, SLOT(onFilter()));
                filterAction->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
            }
        }
    }
    contextMenu.setGeometry(QRect(pos.x(), pos.y(), 300, 600));
    contextMenu.exec();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onInspectImage(QString string)
{
    document->image(string).inspectImage();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onInsertImage()
{
    QStringList oldStringList = document->parentStringList();

    QSettings settings;
    QString directory = settings.value("LAUScan::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QStringList parentStringList = QFileDialog::getOpenFileNames(this, QString("Load image from disk (*.lau)"), directory, QString("*.lau"));
    if (parentStringList.isEmpty()) {
        return;
    }

    for (int n = 0; n < parentStringList.count(); n++) {
        // GRAB THE FIRST INPUT IMAGE STRING ON THE STACK
        QString string = parentStringList.at(n);
        settings.setValue("LAUScan::lastUsedDirectory", QFileInfo(string).absolutePath());
        if (document->exists(string)) {
            int ret = QMessageBox::information(this, string, QString("Selected image is already in list and cannot be added. Would you like to reload image from disk?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                document->removeImage(string);
                imageStackWidget->onRemoveScan(string);
                document->insertImage(string);
                imageStackWidget->onInsertScan(document->image(string));
            }
        } else {
            document->insertImage(string);
        }
    }

    QStringList newStringList = document->parentStringList();
    for (int n = 0; n < newStringList.count(); n++) {
        if (oldStringList.indexOf(newStringList.at(n)) == -1) {
            imageStackWidget->onInsertScan(document->image(newStringList.at(n)));
            imageListWidget->insertImage(newStringList.at(n));
        }
    }
    document->orderChannels(imageListWidget->imageList());
    onUpdateNumberOfImages();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onInsertImage(LAUScan scan)
{
    // CREATE A FILENAME FOR THE INCOMING IMAGE, IF IT DOESN'T ALREADY HAVE ONE
    if (scan.parentName().isEmpty()) {
        int index = document->count();
        QString imageString;
        if (index < 10) {
            imageString = QString("image000%1").arg(index);
        } else if (index < 100) {
            imageString = QString("image00%1").arg(index);
        } else if (index < 1000) {
            imageString = QString("image0%1").arg(index);
        } else {
            imageString = QString("image%1").arg(index);
        }
        scan.updateLimits();
        if (saveOnNewScanFlag) {
            scan.save(QString("%1/%2.tif").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).arg(imageString));
        }
        scan.setParentName(imageString);
    }

    // ADD THE INCOMING IMAGE TO THE DOCUMENT, IMAGE LIST, AND IMAGE PREVIEW STACK
    QString string = scan.parentName();
    document->insertImage(scan, document->count());
    imageListWidget->insertImage(string, document->count());
    imageStackWidget->onInsertScan(scan);

    // SYNCHRONIZE DOCUMENT TO WIDGETS
    document->orderChannels(imageListWidget->imageList());
    onUpdateNumberOfImages();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onInsertImage(LAUMemoryObject image)
{
#ifdef EOS
    // MAKE SURE IMAGES ALWAYS COME IN LANDSCAPE MODE
    if (scannerMake == QString("Canon EOS")) {
        if (image.height() > image.width()) {
            image = image.rotate();
        }
    }
#endif
    // VALIDATE THE IMAGE BEFORE CREATING LAUSCAN
    if (!image.isValid() || image.constPointer() == nullptr) {
        qDebug() << "LAUDocumentWidget::onInsertImage() - image is invalid or has null pointer, ignoring";
        return;
    }

    // DEBUG: Check image and scanner color mismatch
    qDebug() << "LAUDocumentWidget::onInsertImage() - image colors:" << image.colors()
             << "scannerColor:" << scannerColor
             << "expected colors:" << LAU3DVideoParameters::colors(scannerColor);

    // CREATE A LAUSCAN TO HOLD THE INCOMING SNAP SHOT
    LAUScan scan(image, scannerColor);
    if (scan.isValid()) {
        // SET THE CAMERA SPECIFIC STRINGS
        scan.setSoftware(QString("Lau 3D Video Recorder"));
        scan.setMake(scannerMake);
        scan.setModel(scannerModel);

        // NOW INSERT THE SCAN INTO THE DOCUMENT
        onInsertImage(scan);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onInsertImage(QList<LAUMemoryObject> imageList)
{
    int counter = 0;
    QStringList stringList;

    if (mutex.tryLock()){
        QProgressDialog dialog(QString("Importing video..."), QString("Abort"), 0, imageList.count(), this);
        dialog.setModal(true);
        dialog.show();
        while (imageList.count() > 0) {
            if (dialog.wasCanceled()) {
                break;
            }
            dialog.setValue(++counter);
            qApp->processEvents();

            // CREATE A LAUSCAN TO HOLD THE INCOMING SNAP SHOT
            LAUScan scan(imageList.takeFirst(), scannerColor);
            if (scan.isValid()) {
                // SET THE CAMERA SPECIFIC STRINGS
                scan.setSoftware(QString("Lau 3D Video Recorder"));
                scan.setMake(scannerMake);
                scan.setModel(scannerModel);

                int index = document->count();
                QString string;
                if (index < 10) {
                    string = QString("image000%1").arg(index);
                } else if (index < 100) {
                    string = QString("image00%1").arg(index);
                } else if (index < 1000) {
                    string = QString("image0%1").arg(index);
                } else {
                    string = QString("image%1").arg(index);
                }
                scan.setFilename(string);
                stringList << string;

                // ADD THE INCOMING IMAGE TO THE DOCUMENT, IMAGE LIST, AND IMAGE PREVIEW STACK
                if (imageStackWidget) {
                    imageStackWidget->onInsertScan(scan);
                }
                document->insertImage(scan);
            }
        }
        mutex.unlock();
    } else {
        while (imageList.count() > 0) {
            // CREATE A LAUSCAN TO HOLD THE INCOMING SNAP SHOT
            LAUScan scan(imageList.takeFirst(), scannerColor);
            if (scan.isValid()) {
                // SET THE CAMERA SPECIFIC STRINGS
                scan.setSoftware(QString("Lau 3D Video Recorder"));
                scan.setMake(scannerMake);
                scan.setModel(scannerModel);

                int index = document->count();
                QString string;
                if (index < 10) {
                    string = QString("image000%1").arg(index);
                } else if (index < 100) {
                    string = QString("image00%1").arg(index);
                } else if (index < 1000) {
                    string = QString("image0%1").arg(index);
                } else {
                    string = QString("image%1").arg(index);
                }
                scan.setFilename(string);
                stringList << string;

                // ADD THE INCOMING IMAGE TO THE DOCUMENT, IMAGE LIST, AND IMAGE PREVIEW STACK
                if (imageStackWidget) {
                    imageStackWidget->onInsertScan(scan);
                }
                document->insertImage(scan);
            }
        }
    }

    // SYNCHRONIZE DOCUMENT TO WIDGETS
    imageListWidget->insertImages(stringList);
    document->orderChannels(imageListWidget->imageList());
    onUpdateNumberOfImages();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onInsertImage(QList<LAUScan> scanList)
{
    int counter = 0;
    QStringList stringList;
    QProgressDialog dialog(QString("Importing video..."), QString("Abort"), 0, scanList.count(), this);
    dialog.setModal(true);
    dialog.show();
    while (scanList.count() > 0) {
        if (dialog.wasCanceled()) {
            break;
        }
        dialog.setValue(++counter);
        qApp->processEvents();

        // CREATE A LAUSCAN TO HOLD THE INCOMING SNAP SHOT
        LAUScan scan = scanList.takeFirst();
        if (scan.isValid()) {
            // SET THE CAMERA SPECIFIC STRINGS
            if (scan.software().isEmpty()) {
                scan.setSoftware(QString("Lau 3D Video Recorder"));
            }
            if (scan.make().isEmpty()) {
                scan.setMake(scannerMake);
            }
            if (scan.model().isEmpty()) {
                scan.setModel(scannerModel);
            }
            scan.updateLimits();

            // CREATE A NEW FILENAME IF SCAN DOES NOT ALREADY HAVE ONE
            if (scan.parentName().isEmpty()) {
                int index = document->count();
                QString string;
                if (index < 10) {
                    string = QString("image000%1").arg(index);
                } else if (index < 100) {
                    string = QString("image00%1").arg(index);
                } else if (index < 1000) {
                    string = QString("image0%1").arg(index);
                } else {
                    string = QString("image%1").arg(index);
                }
                scan.setParentName(string);
            }

            // MAKE SURE THE SCAN FILENAME IS NOT ALREADY IN LIST AND IF SO MODIFY IT
            if (stringList.contains(scan.parentName())) {
                int index = 1;
                while (1) {
                    QString string = QString("%1_%2").arg(scan.parentName()).arg(index++);
                    if (stringList.contains(string) == false) {
                        scan.setParentName(string);
                        break;
                    }
                }
            }

            // ADD CURRENT SCAN FILENAME TO OUR SEPARATE STRING LIST
            stringList << scan.parentName();

            // ADD THE INCOMING IMAGE TO THE DOCUMENT, IMAGE LIST, AND IMAGE PREVIEW STACK
            if (imageStackWidget) {
                imageStackWidget->onInsertScan(scan);
            }
            document->insertImage(scan);
        }
    }

    // SYNCHRONIZE DOCUMENT TO WIDGETS
    imageListWidget->insertImages(stringList);
    document->orderChannels(imageListWidget->imageList());
    onUpdateNumberOfImages();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onImportDocument()
{
    QSettings settings;
    QString directory = settings.value("LAUDocumentWidget::importDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QStringList parentStringList = QFileDialog::getOpenFileNames(this, QString("Load image from disk (*.skw, *.csv, *.tif)"), directory, QString("*.skw;*.csv;*.tif;*.tiff"));
    if (parentStringList.isEmpty()) {
        return;
    }

    for (int n = 0; n < parentStringList.count(); n++) {
        // GRAB THE FIRST INPUT IMAGE STRING ON THE STACK
        QString string = parentStringList.at(n);
        if (string.endsWith(".skw")) {
            LAUScan scan = LAUScan::loadFromSKW(string);
            if (scan.isValid()) {
                settings.setValue("LAUDocumentWidget::importDirectory", QFileInfo(string).absolutePath());
                if (document->exists(scan.parentName())) {
                    int ret = QMessageBox::information(this, scan.parentName(), QString("Selected image is already in list and cannot be added. Would you like to reload image from disk?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                    if (ret == QMessageBox::Yes) {
                        if (imageStackWidget) {
                            imageStackWidget->onRemoveScan(scan.parentName());
                            imageStackWidget->onInsertScan(scan);
                        }
                        document->removeImage(scan.parentName());
                        document->insertImage(scan);
                    }
                } else {
                    imageListWidget->insertImage(scan.parentName());
                    if (imageStackWidget) {
                        imageStackWidget->onInsertScan(scan);
                    }
                    document->insertImage(scan);
                }
            }
        } else if (string.endsWith(".csv")) {
            LAUScan scan = LAUScan::loadFromCSV(string);
            if (scan.isValid()) {
                settings.setValue("LAUDocumentWidget::importDirectory", QFileInfo(string).absolutePath());
                if (document->exists(scan.parentName())) {
                    int ret = QMessageBox::information(this, scan.parentName(), QString("Selected image is already in list and cannot be added. Would you like to reload image from disk?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                    if (ret == QMessageBox::Yes) {
                        if (imageStackWidget) {
                            imageStackWidget->onRemoveScan(scan.parentName());
                            imageStackWidget->onInsertScan(scan);
                        }
                        document->removeImage(scan.parentName());
                        document->insertImage(scan);
                    }
                } else {
                    imageListWidget->insertImage(scan.parentName());
                    if (imageStackWidget) {
                        imageStackWidget->onInsertScan(scan);
                    }
                    document->insertImage(scan);
                }
            }
        } else if (string.endsWith(".tif") || string.endsWith(".tiff")) {
            // OPEN TIFF FILE FOR LOADING THE IMAGE FROM DISK
            TIFF *tiff = TIFFOpen(string.toLocal8Bit(), "r");
            if (tiff) {
                // SAVE THE DIRECTORY STRING FOR NEXT TIME
                settings.setValue("LAUDocumentWidget::importDirectory", QFileInfo(string).absolutePath());

                // CHOP OFF FILE EXTENSION FROM FILENAME
                string = string.left(string.indexOf("."));

                int numDirectories = TIFFNumberOfDirectories(tiff);
                QProgressDialog progressDialog(QString("Importing image..."), QString(), 0, numDirectories, this, Qt::Sheet);
                for (int n = 0; n < numDirectories; n++) {
                    progressDialog.setValue(n);
                    qApp->processEvents();

                    // SET THE CURRENT DIRECTORY
                    TIFFSetDirectory(tiff, (unsigned short)n);

                    // LOAD THE CURRENT DIRECTORY INTO A NEW IMAGE AND ADD TO LIST
                    LAUMemoryObject object(tiff);

                    // MAKE SURE OBJECT IS FLOATING POINT
                    if (object.depth() != sizeof(float)) {
                        object = object.toFloat();
                    }

                    // SEE IF THIS IS A VALID SCAN FORMAT
                    LAUScan scan;
                    if (object.colors() == 1) {
                        scan = LAUScan(object, ColorGray);
                    } else if (object.colors() == 3) {
                        scan = LAUScan(object, ColorRGB);
                    } else if (object.colors() == 4) {
                        scan = LAUScan(object, ColorXYZG);
                    } else if (object.colors() == 6) {
                        scan = LAUScan(object, ColorXYZRGB);
                    } else if (object.colors() == 8) {
                        scan = LAUScan(object, ColorXYZWRGBA);
                    }

                    if (scan.isValid()) {
                        scan.updateLimits();
                        if (scan.zLimits().x() > 0.0f && scan.zLimits().y() > 0.0f){
                            float xMean = (scan.minX() + scan.maxX())/2.0f;
                            float yMean = (scan.minY() + scan.maxY())/2.0f;

                            scan.transformScanInPlace(QMatrix4x4(1.0, 0.0, 0.0, -xMean, 0.0, -1.0, 0.0, yMean, 0.0, 0.0, -1.0, -6000.0-scan.maxZ(), 0.0, 0.0, 0.0, 1.0));
                        }
                        QString filename = string;
                        if (n < 10) {
                            filename.append(QString("_000%2").arg(n));
                        } else if (n < 100) {
                            filename.append(QString("_00%2").arg(n));
                        } else if (n < 1000) {
                            filename.append(QString("_0%2").arg(n));
                        } else {
                            filename.append(QString("_%2").arg(n));
                        }
                        scan.setFilename(filename);
                        imageListWidget->insertImage(scan.parentName());
                        if (imageStackWidget) {
                            imageStackWidget->onInsertScan(scan);
                        }
                        document->insertImage(scan);
                    }
                }
                progressDialog.setValue(numDirectories);

                // CLOSE TIFF FILE
                TIFFClose(tiff);
            } else {
                QMessageBox::warning(this, QString("Import Document"), QString("Error opening tiff file: %1").arg(LAUMemoryObject::lastTiffErrorString));
            }
        }
    }
    document->orderChannels(imageListWidget->imageList());
    onUpdateNumberOfImages();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onExecuteAsDialogComplete()
{
    // NOW THAT QDIALOG HAS BEEN DELETED, THEN ALL CAMERAS HAVE BEEN DISCONNECTED
    // SO UNLOCK THE MUTEX SO THE USER CAN'T CLOSE THIS WIDGET BEFORE CAMERAS ARE DELETED
    mutex.unlock();
    this->setEnabled(true);

    // DELETE THE SCAN TO DISK WIDGET IF IT EXISTS
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::launchAsDialog(QWidget *widget, QString string)
{
    // LOCK THE MUTEX SO THE USER CAN'T CLOSE THIS WIDGET BEFORE CAMERAS ARE DELETED
    mutex.lock();
    this->setEnabled(false);

    // CONNECT THE DESTROYED SIGNAL TO RE-ENABLE THIS DOCUMENT WIDGET
    connect(widget, SIGNAL(destroyed()), this, SLOT(onExecuteAsDialogComplete()));

    // CREATE A DIALOG AND CAN RUN A NEW EXECUTE LOOP FOR THE WIDGET
    QDialog dialog;
    if (string.isEmpty() == false) {
        dialog.setWindowTitle(string);
    }
    dialog.setLayout(new QVBoxLayout());
    dialog.layout()->setContentsMargins(0, 0, 0, 0);
    dialog.layout()->addWidget(widget);

    dialog.exec();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onImportCalTagObjects()
{
    QSettings settings;
    QString directory = settings.value("LAUScan::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QStringList parentStringList = QFileDialog::getOpenFileNames(this, QString("Load image from disk (*.skw, *.cs*.tif)"), directory, QString("*.skw;*.csv;*.tif;*.tiff"));
    if (parentStringList.isEmpty()) {
        return;
    }

#ifdef ENABLECALIBRATION
    LAUCalTagScanDialog dialog(parentStringList);
    if (dialog.exec() == QDialog::Accepted) {
        onInsertImage(dialog.results());
    }
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onSwapImage(QString stringA, QString stringB)
{
    Q_UNUSED(stringA);
    Q_UNUSED(stringB);
    document->orderChannels(imageListWidget->imageList());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onDuplicateImage(QString string)
{
    QString duplicateImageString = document->duplicateImage(string);
    if (!duplicateImageString.isNull()) {
        // ADD THE DUPLICATED IMAGE TO THE IMAGE LIST AND THE IMAGE PREVIEW
        if (imageStackWidget) {
            imageStackWidget->onInsertScan(document->image(duplicateImageString));
        }
        imageListWidget->insertImage(duplicateImageString);

        // SYNCHRONIZE DOCUMENT IMAGES TO SAVE ORDER AS IN IMAGE LIST
        document->orderChannels(imageListWidget->imageList());
        onUpdateNumberOfImages();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onRemoveImage(QString string)
{
    document->removeImage(string);
    imageListWidget->removeImage(string);
    if (imageStackWidget) {
        imageStackWidget->onRemoveScan(string);
    }
    document->orderChannels(imageListWidget->imageList());
    onUpdateNumberOfImages();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onUpdateNumberOfImages()
{
    if (imageStackWidget) {
        if (imageListWidget->count() == 0) {
            if (this->window()->isMaximized() == false) {
                imageStackGroupBox->setVisible(false);
                this->setFixedWidth(340);
            }
        } else {
            imageStackGroupBox->setVisible(true);

            if (this->window()->isMaximized() == false) {
                QSize minSize = imageStackWidget->size();
                int wdth = qMin(400 + minSize.width(), 760);
                int hght = qMin(100 + minSize.height(), 520);

                // SET THE MAXIMUM WINDOW SIZE
                QRect rect = this->windowHandle()->screen()->geometry();
#ifndef Q_OS_MACOS
                QList<QScreen*> screens = QGuiApplication::screens();
                for (int n = 0; n < screens.count(); n++){
                    if (screens.at(n)->geometry().contains(this->mapToGlobal(QPoint(this->width()/2,this->height()/2)))){
                        rect = screens.at(n)->geometry();
                    }
                }
                // QRect rect = QGuiApplication::screenAt(this->mapToGlobal({this->width()/2,this->height()/2}))->availableGeometry();
#endif
                setMaximumSize(rect.size());

                // RESIZE THE WINDOW THE APPROPRIATE SIZE
                rect = this->geometry();
                rect.setHeight(qMax(hght, this->height()));
                rect.setWidth(qMax(wdth, this->width()));
                this->setGeometry(rect);

                // NOW SET THE MINIMUM WINDOW SIZE SINCE THE WINDOW IS LARGER THAN THIS NOW
                setMinimumSize(QSize(wdth, hght));
            }

            //if (hght <= this->height()) {
            //    if (wdth <= this->width()) {
            //        setMinimumSize(QSize(wdth, hght) / 2);
            //        setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
            //    } else {
            //        hide();
            //        setMinimumSize(QSize(wdth, hght));
            //        setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
            //        show();
            //    }
            //} else {
            //    hide();
            //    setMinimumSize(QSize(wdth, hght));
            //    setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));

            //    show();
            //}
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilter(QString operation)
{
    // IF WE GET AN EMPTY SIGNAL, IT MUST HAVE COME FROM A QACTION
    // SO ASK THE QACTION FOR ITS TEXT
    if (operation.isEmpty()) {
        operation = (static_cast<QAction *>(QObject::sender()))->text();
    }

    if (operation == QString("Snap-shot") || operation == QString("Launch EOS")) {
        onFilterScannerTool();
    } else if (operation == QString("Video")) {
        onFilterVideoTool();
    } else if (operation == QString("Scan User Path")){
        onFilterScanVelmexRailOnUserPath();
    }

#ifdef HYPERSPECTRAL
    if (operation == QString("Hyperspectral")) {
        onFilterHyperspectral();
    } else if (operation == QString("Hyperspectral Merge")) {
        onFilterHyperspectralMerge();
    }
#endif

#ifdef ENABLECALIBRATION
    if (operation == QString("Calibration")) {
        onFilterCalibration();
    } else if (operation == QString("Set XY Plane")){
        onFilterSetXYPlane();
#ifdef EOS
    } else if (operation == QString("Rasterize")) {
        onFilterRasterize();
#endif
    } else if (operation == QString("Generate LUT")){
        onFilterGenerateLUT();
    } else if (operation == QString("CalTag")) {
        onFilterCalTagTool();
    } else if (operation == QString("Binarize")) {
        onFilterBinaryTool();
    } else if (operation == QString("Alpha Trimmed")) {
        onFilterAlphaTrimmedMean();
    }
#endif

#ifdef ENABLECLASSIFIER
    if (operation == QString("YOLO Classifier")) {
        onFilterYolo();
    }
#endif
#ifdef ENABLECASCADE
    if (operation == QString("Cascade Classifier")) {
        onFilterCascade();
    }
#endif
    if (operation == QString("Background")) {
        onFilterBackgroundTool();
    } else if (operation == QString("Green Screen")) {
        onFilterGreenScreenTool();
    }

#ifdef ENABLEPOINTMATCHER
    if (operation == QString("Auto Merge")) {
        onFilterAutoMergeTool();
    } else if (operation == QString("Merge")) {
        onFilterMergeTool();
    } else if (operation == QString("Symmetry")) {
        onFilterSymmetryTool();
    } else if (operation == QString("BCS Tracking")) {
        onFilterBCSTracking();
    } else if (operation == QString("Tracking")) {
        onFilterTracking();
    }
#endif // ENABLEPOINTMATCHER

#ifdef SANDBOX
    if (operation == QString("Sandbox Calibration")) {
        onFilterSandboxCalibrationTool();
    } else if (operation == QString("Sandbox")) {
        onFilterSandboxTool();
    }
#endif // SANDBOX
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterScannerTool()
{
    QString string = targetScanner();
#ifndef STANDALONE_EOS
    LAU3DVideoRecordingWidget *recorder = nullptr;
    if (string == QString("Prime Sense NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DevicePrimeSense);
    } else if (string == QString("Seek Thermal")){
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceSeek);
    } else if (string == QString("Prime Sense RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZRGB, DevicePrimeSense);
    } else if (string == QString("Real Sense RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZRGB, DeviceRealSense);
    } else if (string == QString("Real Sense NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceRealSense);
    } else if (string == QString("Real Sense GRY")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceRealSense);
    } else if (string == QString("Real Sense RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceRealSense);
    } else if (string == QString("Prosilica GRY")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceProsilicaGRY);
    } else if (string == QString("Prosilica RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceProsilicaRGB);
    } else if (string == QString("Prosilica PST")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceProsilicaPST);
    } else if (string == QString("Prosilica AST")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceProsilicaAST);
    } else if (string == QString("Prosilica LCG")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceProsilicaLCG);
    } else if (string == QString("Prosilica TOF")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceProsilicaTOF);
    } else if (string == QString("Prosilica DPR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceProsilicaDPR);
    } else if (string == QString("Prosilica IOS")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZWRGBA, DeviceProsilicaIOS);
    } else if (string == QString("Kinect NIR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceKinect);
    } else if (string == QString("Kinect RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceKinect);
    } else if (string == QString("Kinect NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceKinect);
    } else if (string == QString("Kinect RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZWRGBA, DeviceKinect);
    } else if (string == QString("VZense NIR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceVZense);
    } else if (string == QString("VZense NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceVZense);
    } else if (string == QString("Lucid NIR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceLucid);
    } else if (string == QString("Lucid RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceLucid);
    } else if (string == QString("Lucid NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceLucid);
    } else if (string == QString("Lucid RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZRGB, DeviceLucid);
    } else if (string == QString("Vidu NIR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceVidu);
    } else if (string == QString("Vidu RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceVidu);
    } else if (string == QString("Vidu NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceVidu);
    } else if (string == QString("Vidu RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZRGB, DeviceVidu);
    } else if (string == QString("Orbbec NIR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceOrbbec);
    } else if (string == QString("Orbbec RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceOrbbec);
    } else if (string == QString("Orbbec NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceOrbbec);
    } else if (string == QString("Orbbec RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZRGB, DeviceOrbbec);
    } else if (string == QString("Ximea")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceXimea);
    } else if (string == QString("IDS")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceIDS);
#ifdef USETCP
    } else if (string == QString("Real Sense TCP")) {
        LAU3DVideoTCPMultiChannelWidget *widget = new LAU3DVideoTCPMultiChannelWidget(ColorXYZRGB, DeviceRealSense);
        connect(widget, SIGNAL(emitVideoFrames(LAUMemoryObject)), this, SLOT(onInsertImage(LAUMemoryObject)));
        connect(widget, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), this, SLOT(onInsertImage(QList<LAUMemoryObject>)));
        widget->enableSnapShotMode(true);

        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake  = QString("TCP Client");
        scannerColor = ColorXYZRGB;
        scannerModel = QString("Intel Real Sense");

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        launchAsDialog(widget, QString("TCP Multichannel Recorder"));
#endif
#ifndef EOS
    } else {
        return;
    }
#else
    } else if (string == QString("Canon EOS")) {
        LAUEOSControllerWidget *widget = new LAUEOSControllerWidget();
        connect(widget, SIGNAL(emitVideoFrames(LAUMemoryObject)), this, SLOT(onInsertImage(LAUMemoryObject)));

        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake  = QString("Canon EOS");
        scannerColor = ColorRGBA;

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        connect(this, SIGNAL(destroyed()), widget, SLOT(deleteLater()));
        widget->setDeleteOnClose(true);
        widget->show();
        return;
    } else {
        return;
    }
#endif

    // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
    if (recorder && recorder->isValid()) {
        connect(recorder, SIGNAL(emitVideoFrames(LAUMemoryObject)), this, SLOT(onInsertImage(LAUMemoryObject)));
        recorder->enableSnapShotMode(true);
#ifndef EXCLUDE_LAUVELMEXWIDGET
        recorder->enableVelmexScanMode(true);
#endif
        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake  = recorder->make();
        scannerColor = recorder->color();
        scannerModel = recorder->model();

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        launchAsDialog(recorder);
    } else {
        delete recorder;
    }
#else //STANDALONE_EOS
    LAUEOSControllerWidget *widget = new LAUEOSControllerWidget();
    connect(widget, SIGNAL(emitVideoFrames(LAUMemoryObject)), this, SLOT(onInsertImage(LAUMemoryObject)));

    // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
    scannerMake  = QString("Canon EOS");
    scannerColor = ColorRGBA;

    // CREATE A DIALOG TO WRAP AROUND THE SCANNER
    connect(this, SIGNAL(destroyed()), widget, SLOT(deleteLater()));
    widget->setDeleteOnClose(true);
    widget->show();
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterVideoTool()
{
    QString string = targetScanner();
#ifndef STANDALONE_EOS
    LAU3DVideoRecordingWidget *recorder = nullptr;
    if (string == QString("Prime Sense NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DevicePrimeSense);
    } else if (string == QString("Seek Thermal")){
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceSeek);
    } else if (string == QString("Prime Sense RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZRGB, DevicePrimeSense);
    } else if (string == QString("Real Sense GRY")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceRealSense);
    } else if (string == QString("Real Sense RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceRealSense);
    } else if (string == QString("Real Sense RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZRGB, DeviceRealSense);
    } else if (string == QString("Real Sense NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceRealSense);
    } else if (string == QString("Prosilica GRY")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceProsilicaGRY);
    } else if (string == QString("Prosilica RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceProsilicaRGB);
    } else if (string == QString("Prosilica PST")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceProsilicaPST);
    } else if (string == QString("Prosilica AST")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceProsilicaAST);
    } else if (string == QString("Prosilica LCG")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceProsilicaLCG);
    } else if (string == QString("Prosilica TOF")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceProsilicaTOF);
    } else if (string == QString("Prosilica DPR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceProsilicaDPR);
    } else if (string == QString("Prosilica IOS")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZWRGBA, DeviceProsilicaIOS);
    } else if (string == QString("Kinect NIR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceKinect);
    } else if (string == QString("Kinect RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceKinect);
    } else if (string == QString("Kinect NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceKinect);
    } else if (string == QString("Kinect RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZWRGBA, DeviceKinect);
    } else if (string == QString("VZense NIR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceVZense);
    } else if (string == QString("VZense NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceVZense);
    } else if (string == QString("Lucid NIR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceLucid);
    } else if (string == QString("Lucid RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceLucid);
    } else if (string == QString("Lucid NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceLucid);
    } else if (string == QString("Lucid RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZRGB, DeviceLucid);
    } else if (string == QString("Vidu NIR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceVidu);
    } else if (string == QString("Vidu RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceVidu);
    } else if (string == QString("Vidu NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceVidu);
    } else if (string == QString("Vidu RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZRGB, DeviceVidu);
    } else if (string == QString("Orbbec NIR")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceOrbbec);
    } else if (string == QString("Orbbec RGB")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceOrbbec);
    } else if (string == QString("Orbbec NIR-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZG, DeviceOrbbec);
    } else if (string == QString("Orbbec RGB-D")) {
        recorder = new LAU3DVideoRecordingWidget(ColorXYZRGB, DeviceOrbbec);
    } else if (string == QString("Ximea")) {
        recorder = new LAU3DVideoRecordingWidget(ColorGray, DeviceXimea);
    } else if (string == QString("IDS")) {
        recorder = new LAU3DVideoRecordingWidget(ColorRGB, DeviceIDS);
    } else {
        return;
    }

    // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
    if (recorder && recorder->isValid()) {
        connect(recorder, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), this, SLOT(onInsertImage(QList<LAUMemoryObject>)));
        recorder->enableSnapShotMode(false);
#ifndef EXCLUDE_LAUVELMEXWIDGET
        recorder->enableVelmexScanMode(false);
#endif
        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake  = recorder->make();
        scannerColor = recorder->color();
        scannerModel = recorder->model();

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        launchAsDialog(recorder);
    } else {
        delete recorder;
    }
#endif // STANDALONE_EOS
}

#ifdef ENABLECALIBRATION
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterGenerateLUT()
{
    if (document->images().count() > 4){
        LAUGenerateLookUpTableDialog dialog(this->document);
        if (dialog.exec() == QDialog::Accepted){
            ;
        }
    } else {
        QMessageBox::information(this, QString("Generate LUT"), QString("Need at least five scans to generate a LUT."));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterCalibration()
{
    QString string = targetScanner();

    if (string.isEmpty()) {
        return;
    } else if (string.contains("Prosilica") || string.contains("Real Sense") || string.contains("Kinect") || string.contains("Lucid") || string.contains("Vidu") || string.contains("Orbbec")) {
        LAU3DCalibrationWidget *recorder = nullptr;
        if (string == QString("Prosilica LCG")) {
            recorder = new LAU3DCalibrationWidget(DeviceProsilicaLCG, LAU3DCalibrationGLFilter::ChannelColor);
        } else if (string == QString("Prosilica GRY")) {
            recorder = new LAU3DCalibrationWidget(DeviceProsilicaGRY, LAU3DCalibrationGLFilter::ChannelGray);
        } else if (string == QString("Prosilica IOS")) {
            recorder = new LAU3DCalibrationWidget(DeviceProsilicaIOS, LAU3DCalibrationGLFilter::ChannelColor);
        } else if (string == QString("Prosilica DPR")) {
            recorder = new LAU3DCalibrationWidget(DeviceProsilicaDPR, LAU3DCalibrationGLFilter::ChannelColor);
        } else if (string == QString("Real Sense RGB-D")) {
            recorder = new LAU3DCalibrationWidget(DeviceRealSense, LAU3DCalibrationGLFilter::ChannelColor);
        } else if (string == QString("Real Sense NIR-D")) {
            recorder = new LAU3DCalibrationWidget(DeviceRealSense, LAU3DCalibrationGLFilter::ChannelDepth);
        } else if (string == QString("Kinect NIR-D")) {
            recorder = new LAU3DCalibrationWidget(DeviceKinect, LAU3DCalibrationGLFilter::ChannelDepth);
        } else if (string == QString("Kinect NIR")) {
            recorder = new LAU3DCalibrationWidget(DeviceKinect, LAU3DCalibrationGLFilter::ChannelGray);
        } else if (string == QString("Kinect RGB")) {
            recorder = new LAU3DCalibrationWidget(DeviceKinect, LAU3DCalibrationGLFilter::ChannelColor);
        } else if (string == QString("Kinect NIR")) {
            recorder = new LAU3DCalibrationWidget(DeviceKinect, LAU3DCalibrationGLFilter::ChannelColor);
        } else if (string == QString("Kinect RGB")) {
            recorder = new LAU3DCalibrationWidget(DeviceKinect, LAU3DCalibrationGLFilter::ChannelColor);
        } else if (string == QString("VZense NIR-D")) {
            recorder = new LAU3DCalibrationWidget(DeviceVZense, LAU3DCalibrationGLFilter::ChannelDepth);
        } else if (string == QString("Lucid NIR")) {
            recorder = new LAU3DCalibrationWidget(DeviceLucid, LAU3DCalibrationGLFilter::ChannelGray);
        } else if (string == QString("Lucid NIR-D")) {
            recorder = new LAU3DCalibrationWidget(DeviceLucid, LAU3DCalibrationGLFilter::ChannelDepth);
        } else if (string == QString("Vidu NIR")) {
            recorder = new LAU3DCalibrationWidget(DeviceVidu, LAU3DCalibrationGLFilter::ChannelGray);
        } else if (string == QString("Vidu NIR-D")) {
            recorder = new LAU3DCalibrationWidget(DeviceVidu, LAU3DCalibrationGLFilter::ChannelDepth);
        } else if (string == QString("Orbbec NIR")) {
            recorder = new LAU3DCalibrationWidget(DeviceOrbbec, LAU3DCalibrationGLFilter::ChannelGray);
        } else if (string == QString("Orbbec NIR-D")) {
            recorder = new LAU3DCalibrationWidget(DeviceOrbbec, LAU3DCalibrationGLFilter::ChannelDepth);
        } else {
            return;
        }

        // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
        if (recorder && recorder->isValid()) {
            // ENABLE VIDEO RECORDING TO MERGE 30 SCANS TOGETHER INTO A SINGLE SCAN
            recorder->enableSnapShotMode(false);

            // CONNECT THE EMITTED VIDEO FRAME SIGNALS TO THEIR SLOTS
            connect(recorder, SIGNAL(emitVideoFrames(LAUScan)), this, SLOT(onInsertImage(LAUScan)));
            connect(recorder, SIGNAL(emitVideoFrames(QList<LAUScan>)), recorder, SLOT(onReceiveVideoFrames(QList<LAUScan>)));

            // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
            scannerMake      = recorder->make();
            scannerColor     = recorder->color();
            scannerModel     = recorder->model();
            scannerTransform = recorder->transform();
            saveOnNewScanFlag = true;

            // CREATE A DIALOG TO WRAP AROUND THE SCANNER
            launchAsDialog(recorder);
        } else {
            delete recorder;
        }
    } else {
        QStringList strings;
        strings << "Depth";
        strings << "Color";

        bool okay;
        QSettings settings;
        int index = settings.value(QString("LAUDocumentWidget::onFilterCalTag"), 0).toInt();
        QString channel = QInputDialog::getItem(this, QString("Select Color"), QString("Select color space"), strings, index, false, &okay);
        if (okay) {
            index = strings.indexOf(channel);
            settings.setValue(QString("LAUDocumentWidget::onFilterCalTag"), index);

            LAU3DRoundGridWidget *recorder = nullptr;
            if (string == QString("Prime Sense")) {
                if (channel == "Depth") {
                    recorder = new LAU3DRoundGridWidget(ColorGray, DevicePrimeSense);
                } else {
                    recorder = new LAU3DRoundGridWidget(ColorRGB, DevicePrimeSense);
                }
            } else if (string == QString("Kinect NIR-D") || string == QString("Kinect RGB-D")) {
                if (channel == "Depth") {
                    recorder = new LAU3DRoundGridWidget(ColorGray, DeviceKinect);
                } else {
                    recorder = new LAU3DRoundGridWidget(ColorRGB, DeviceKinect);
                }
            } else if (string == QString("Lucid NIR-D")) {
                recorder = new LAU3DRoundGridWidget(ColorGray, DeviceLucid);
            } else if (string == QString("Vidu NIR-D")) {
                recorder = new LAU3DRoundGridWidget(ColorGray, DeviceVidu);
            } else if (string == QString("Orbbec NIR-D")) {
                recorder = new LAU3DRoundGridWidget(ColorGray, DeviceOrbbec);
            } else if (string == QString("VZense NIR-D")) {
                recorder = new LAU3DRoundGridWidget(ColorGray, DeviceVZense);
            } else {
                return;
            }

            // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
            if (recorder && recorder->isValid()) {
                // ENABLE VIDEO RECORDING TO MERGE 30 SCANS TOGETHER INTO A SINGLE SCAN
                recorder->enableSnapShotMode(true);

                // CONNECT THE EMITTED VIDEO FRAME SIGNALS TO THEIR SLOTS
                connect(recorder, SIGNAL(emitVideoFrames(LAUMemoryObject)), this, SLOT(onInsertImage(LAUMemoryObject)));
                connect(recorder, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), recorder, SLOT(onReceiveVideoFrames(QList<LAUMemoryObject>)));

                // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
                scannerMake      = recorder->make();
                scannerColor     = recorder->color();
                scannerModel     = recorder->model();
                scannerTransform = recorder->transform();
                saveOnNewScanFlag = true;

                // CREATE A DIALOG TO WRAP AROUND THE SCANNER
                launchAsDialog(recorder);
            } else {
                delete recorder;
            }
            saveOnNewScanFlag = false;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterCalTagTool()
{
    int index = document->indexOf(imageListWidget->currentItem());
    if (index > -1) {
        LAUCalTagScanDialog dialog(document->images(), index);
        if (dialog.exec() == QDialog::Accepted) {
            emit fileCreateNewDocument(dialog.results(),QString());
        }
    } else {
        QString string = targetScanner();
#ifndef STANDALONE_EOS
        LAU3DVideoRecordingWidget *recorder = nullptr;
        if (string == QString("Prime Sense")) {
            recorder = new LAUCalTagWidget(ColorXYZWRGBA, DevicePrimeSense);
        } else if (string == QString("Real Sense GRY")) {
            recorder = new LAUCalTagWidget(ColorGray, DeviceRealSense);
        } else if (string == QString("Real Sense RGB")) {
            recorder = new LAUCalTagWidget(ColorRGB, DeviceRealSense);
        } else if (string == QString("Real Sense RGB-D")) {
            recorder = new LAUCalTagWidget(ColorXYZWRGBA, DeviceRealSense);
        } else if (string == QString("Real Sense NIR-D")) {
            recorder = new LAUCalTagWidget(ColorXYZG, DeviceRealSense);
        } else if (string == QString("Prosilica GRY")) {
            recorder = new LAUCalTagWidget(ColorGray, DeviceProsilicaGRY);
#ifdef BASLERUSB
        } else if (string == QString("Prosilica PST")) {
            recorder = new LAUStereoCalTagWidget(ColorGray, DeviceProsilicaPST);
        } else if (string == QString("Prosilica AST")) {
            recorder = new LAUStereoCalTagWidget(ColorRGB, DeviceProsilicaAST);
#endif
        } else if (string == QString("Prosilica LCG")) {
            recorder = new LAUCalTagWidget(ColorXYZG, DeviceProsilicaLCG);
        } else if (string == QString("Prosilica IOS")) {
            recorder = new LAUCalTagWidget(ColorXYZG, DeviceProsilicaIOS);
        } else if (string == QString("Kinect NIR")) {
            recorder = new LAUCalTagWidget(ColorGray, DeviceKinect);
        } else if (string == QString("Kinect RGB")) {
            recorder = new LAUCalTagWidget(ColorRGB, DeviceKinect);
        } else if (string == QString("Kinect NIR-D")) {
            recorder = new LAUCalTagWidget(ColorXYZG, DeviceKinect);
        } else if (string == QString("Kinect RGB-D")) {
            recorder = new LAUCalTagWidget(ColorXYZWRGBA, DeviceKinect);
        } else {
            return;
        }

        // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
        if (recorder && recorder->isValid()) {
            connect(recorder, SIGNAL(emitVideoFrames(LAUMemoryObject)), this, SLOT(onInsertImage(LAUMemoryObject)));
            recorder->enableSnapShotMode(true);

            // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
            scannerMake      = recorder->make();
            scannerColor     = recorder->color();
            scannerModel     = recorder->model();
            scannerTransform = recorder->transform();

            // CREATE A DIALOG TO WRAP AROUND THE SCANNER
            launchAsDialog(recorder);
        } else {
            delete recorder;
        }
    }
}

#ifdef EOS
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterRasterize()
{
    // MAKE SURE WE HAVE SCANS TO RASTERIZE
    if (document->images().isEmpty()) {
        return;
    }

    // SEE IF CURRENT DOCUMENT HAS XYZ CHANNELS, IF SO THEN WE CAN PROCEED
    // OTHERWISE, WE SHOULD GIVE THE USER A CHANCE TO LOAD THE MASTER SCAN
    // WITH XYZ COORDINATES FROM DISK
    if (document->color() == ColorUndefined || document->color() == ColorGray || document->color() == ColorRGB || document->color() == ColorRGBA) {
        this->setEnabled(false);

        // OPEN A DIALOG TO MERGE THIS DOCUMENT WITH THE XYZ DOCUMENT
        LAURasterizeScansDialog dialogA(LAUDocument(), *document);
        if (dialogA.exec() == QDialog::Accepted) {
            // GET THE MERGED SCAN FROM THE DIALOG
            LAUDocument mergedDocument = dialogA.mergeResult();

            // MAKE THE DOCUMENT CLEAN SINCE USER ALREADY HAD CHANCE TO SAVE
            mergedDocument.makeClean();

            // NOW OPEN THE DIALOG TO DO THE RASTERIZINE
            LAURasterizeDialog dialogB(mergedDocument.images());
            if (dialogB.exec() == QDialog::Accepted) {
                // PULL OUT THE GRAYSCALE CHANNEL
                LAUScan result = dialogB.smooth().extractChannel(0);

                // SAVE THE IMAGE TO DISK AS SOMETHING WE CAN OPEN WITH PHOTOSHOP
                while (1) {
                    if (result.saveAsUint8(QString())) {
                        break;
                    } else if (QMessageBox::warning(this, QString("Rasterize Filter"), QString("You are about to lose the rasterized scan.  Abort?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No) {
                        break;
                    }
                }
            }
            mergedDocument.makeClean();
        }
        this->setEnabled(true);
    } else {
        int index = document->indexOf(imageListWidget->currentItem());
        if (index > -1) {
            this->setEnabled(false);
            LAURasterizeDialog dialog(document->images());
            if (dialog.exec() == QDialog::Accepted) {
                // PULL OUT THE GRAYSCALE CHANNEL
                LAUScan result = dialog.smooth().extractChannel(0);

                // SAVE THE IMAGE TO DISK AS SOMETHING WE CAN OPEN WITH PHOTOSHOP
                while (1) {
                    if (result.saveAsUint8(QString())) {
                        break;
                    } else if (QMessageBox::warning(this, QString("Rasterize Filter"), QString("You are about to loss the rasterized scan.  Abort?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No) {
                        break;
                    }
                }
            }
            this->setEnabled(true);
        }
    }
#endif // STANDALONE_EOS
}
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterBinaryTool()
{
    int index = document->indexOf(imageListWidget->currentItem());
    if (index > -1) {
        LAUBinarizeScanDialog dialog(document->image(imageListWidget->currentItem()));
        if (dialog.exec() == QDialog::Accepted) {
            ;
        }
    } else {
        QString string = targetScanner();
        LAUBinarizeWidget *recorder = nullptr;
        if (string == QString("Prime Sense")) {
            recorder = new LAUBinarizeWidget(ColorXYZWRGBA, DevicePrimeSense);
        } else if (string == QString("Real Sense GRY")) {
            recorder = new LAUBinarizeWidget(ColorGray, DeviceRealSense);
        } else if (string == QString("Real Sense RGB")) {
            recorder = new LAUBinarizeWidget(ColorRGB, DeviceRealSense);
        } else if (string == QString("Real Sense RGB-D")) {
            recorder = new LAUBinarizeWidget(ColorXYZWRGBA, DeviceRealSense);
        } else if (string == QString("Real Sense NIR-D")) {
            recorder = new LAUBinarizeWidget(ColorXYZG, DeviceRealSense);
        } else if (string == QString("Prosilica LCG")) {
            recorder = new LAUBinarizeWidget(ColorXYZG, DeviceProsilicaLCG);
        } else if (string == QString("Prosilica IOS")) {
            recorder = new LAUBinarizeWidget(ColorXYZG, DeviceProsilicaIOS);
        } else if (string == QString("Kinect NIR")) {
            recorder = new LAUBinarizeWidget(ColorGray, DeviceKinect);
        } else if (string == QString("Kinect RGB")) {
            recorder = new LAUBinarizeWidget(ColorRGB, DeviceKinect);
        } else if (string == QString("Kinect NIR-D")) {
            recorder = new LAUBinarizeWidget(ColorXYZG, DeviceKinect);
        } else if (string == QString("Kinect RGB-D")) {
            recorder = new LAUBinarizeWidget(ColorXYZWRGBA, DeviceKinect);
        } else {
            return;
        }

        // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
        if (recorder && recorder->isValid()) {
            connect(recorder, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), this, SLOT(onInsertImage(QList<LAUMemoryObject>)));
            recorder->enableSnapShotMode(false);

            // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
            scannerMake      = recorder->make();
            scannerColor     = recorder->color();
            scannerModel     = recorder->model();
            scannerTransform = recorder->transform();

            // CREATE A DIALOG TO WRAP AROUND THE SCANNER
            launchAsDialog(recorder);
        } else {
            delete recorder;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterAlphaTrimmedMean()
{
    QString string = targetScanner();

    LAUAlphaTrimmedMeanWidget *recorder = nullptr;
    if (string == QString("Prime Sense NIR-D")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZG, DevicePrimeSense);
    } else if (string == QString("Prime Sense RGB-D")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZRGB, DevicePrimeSense);
    } else if (string == QString("Real Sense RGB-D")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZRGB, DeviceRealSense);
    } else if (string == QString("Real Sense NIR-D")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZG, DeviceRealSense);
    } else if (string == QString("Prosilica AST")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZG, DeviceProsilicaAST);
    } else if (string == QString("Prosilica LCG")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZG, DeviceProsilicaLCG);
    } else if (string == QString("Prosilica DPR")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZG, DeviceProsilicaDPR);
    } else if (string == QString("Prosilica IOS")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZWRGBA, DeviceProsilicaIOS);
    } else if (string == QString("Kinect NIR-D")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZG, DeviceKinect);
    } else if (string == QString("Kinect RGB-D")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZWRGBA, DeviceKinect);
    } else if (string == QString("VZense NIR-D")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZG, DeviceVZense);
    } else if (string == QString("Lucid NIR-D")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZG, DeviceLucid);
    } else if (string == QString("Vidu NIR-D")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZG, DeviceVidu);
    } else if (string == QString("Orbbec NIR-D")) {
        recorder = new LAUAlphaTrimmedMeanWidget(ColorXYZG, DeviceOrbbec);
    } else {
        return;
    }

    // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
    if (recorder && recorder->isValid()) {
        connect(recorder, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), this, SLOT(onInsertImage(QList<LAUMemoryObject>)));
        recorder->enableSnapShotMode(false);

        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake      = recorder->make();
        scannerColor     = recorder->color();
        scannerModel     = recorder->model();
        scannerTransform = recorder->transform();

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        launchAsDialog(recorder);
    } else {
        delete recorder;
    }

    //QList<LAUScan> scans;
    //QStringList stringList = document->parentStringList();
    //for (int n = 0; n < stringList.count(); n++) {
    //    scans << document->image(stringList.at(n));
    //}
    //if (scans.count() > 1) {
    //    LAUAlphaTrimmedMeanDialog dialog(scans);
    //    if (dialog.exec() == QDialog::Accepted) {
    //        LAUScan scan = dialog.smooth();
    //        scan.save();
    //    }
    //}
}
#endif

#ifdef ENABLECLASSIFIER
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterYolo()
{
    // LOAD A TRAINED MDOEL FROM DISK
    LAUYoloPoseObject poseNetwork((QString()));
    if (poseNetwork.isValid() == false){
        return;
    }

    QList<LAUScan> maleList;
    QList<LAUScan> fmleList;

    QStringList strings = document->parentStringList();
    QProgressDialog dialog(QString("Classifying images..."), QString("Abort"), 0, strings.count(), this, Qt::Sheet);
    dialog.show();
    for (int n = 0; n < strings.count(); n++) {
        if (dialog.wasCanceled()) {
            break;
        } else {
            dialog.setValue(n);
            qApp->processEvents();
        }

        LAUScan scan = document->image(strings.at(n));
        if (scan.isValid()) {
            // MAKE SURE SCAN IMAGE IS SQUARE IN SIZE
            if (scan.width() > scan.height()){
                int lft = scan.width()/2 - scan.height()/2;
                scan = scan.crop(lft, 0, scan.height(), scan.height());
            } else if (scan.width() < scan.height()){
                int top = scan.height()/2 - scan.width()/2;
                scan = scan.crop(0, top, scan.width(), scan.width());
            }

            // PROCESS THE SCAN WITH THE DEEP NETWORK OBJECT
            QList<LAUMemoryObject> objects = poseNetwork.process(scan.channelsToFrames());
            if (objects.isEmpty() == false){
                // GET POINTS FOR MALE MOSQUITOS
                float confMale = 0.70f;
                float confFmle = 0.70f;

                QList<QVector3D> pointsMale = poseNetwork.points(0, &confMale);
                QList<QVector3D> pointsFmle = poseNetwork.points(1, &confFmle);

                if (confMale > 0.9 && confMale > confFmle){
                    maleList << document->image(strings.at(n));
                } else {
                    fmleList << document->image(strings.at(n));
                }
            } else {
                fmleList << document->image(strings.at(n));
            }
        }
    }
    dialog.setValue(strings.count());

    // EMIT SCAN LISTS TO CREATE NEW DOCUMENTS
    if (maleList.isEmpty() == false){
        emit fileCreateNewDocument(maleList, QString("MALES (%1)").arg(maleList.count()));
    }
    if (fmleList.isEmpty() == false){
        emit fileCreateNewDocument(fmleList, QString("NOT MALES (%1)").arg(fmleList.count()));
    }
}
#endif
#ifdef ENABLECASCADE
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterCascade()
{
    QString string = targetScanner();
    LAUCascadeClassifierWidget *recorder = nullptr;
    if (string == QString("Prime Sense")) {
        recorder = new LAUCascadeClassifierWidget(ColorXYZWRGBA, DevicePrimeSense);
    } else if (string == QString("Real Sense RGB-D")) {
        recorder = new LAUCascadeClassifierWidget(ColorXYZWRGBA, DeviceRealSense);
    } else if (string == QString("Real Sense NIR-D")) {
        recorder = new LAUCascadeClassifierWidget(ColorXYZG, DeviceRealSense);
    } else if (string == QString("Prosilica LCG")) {
        recorder = new LAUCascadeClassifierWidget(ColorXYZG, DeviceProsilicaLCG);
    } else if (string == QString("Prosilica IOS")) {
        recorder = new LAUCascadeClassifierWidget(ColorXYZG, DeviceProsilicaIOS);
    } else if (string == QString("Kinect NIR-D")) {
        recorder = new LAUCascadeClassifierWidget(ColorXYZG, DeviceKinect);
    } else if (string == QString("Kinect RGB-D")) {
        recorder = new LAUCascadeClassifierWidget(ColorXYZWRGBA, DeviceKinect);
    } else if (string ==  QString("Lucid NIR-D")) {
        recorder = new LAUCascadeClassifierWidget(ColorXYZRGB, DeviceLucid);
    } else if (string ==  QString("Vidu NIR-D")) {
        recorder = new LAUCascadeClassifierWidget(ColorXYZRGB, DeviceVidu);
    } else if (string ==  QString("Orbbec NIR-D")) {
        recorder = new LAUCascadeClassifierWidget(ColorXYZRGB, DeviceOrbbec);
    } else if (string ==  QString("VZense NIR-D")) {
        recorder = new LAUCascadeClassifierWidget(ColorXYZRGB, DeviceVZense);
    } else {
        return;
    }

    // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
    if (recorder && recorder->isValid()) {
        connect(recorder, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), this, SLOT(onInsertImage(QList<LAUMemoryObject>)));
        recorder->enableSnapShotMode(false);

        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake      = recorder->make();
        scannerColor     = recorder->color();
        scannerModel     = recorder->model();
        scannerTransform = recorder->transform();

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        launchAsDialog(recorder);
    } else {
        delete recorder;
    }
}
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterBackgroundTool()
{
#ifdef ENABLECASCADE
    // RESET ANY CAMERA CLASSIFICATIONS
    LAUCameraClassifierDialog::resetCameraAssignments();
#endif

    QString string = targetScanner();
    LAUBackgroundWidget *recorder = nullptr;
    if (string == QString("Prime Sense")) {
        recorder = new LAUBackgroundWidget(ColorXYZWRGBA, DevicePrimeSense);
    } else if (string == QString("Real Sense RGB-D")) {
        recorder = new LAUBackgroundWidget(ColorXYZRGB, DeviceRealSense);
    } else if (string == QString("Real Sense NIR-D")) {
        recorder = new LAUBackgroundWidget(ColorXYZG, DeviceRealSense);
    } else if (string == QString("Prime Sense")) {
        recorder = new LAUBackgroundWidget(ColorXYZWRGBA, DeviceRealSense);
    } else if (string == QString("Prosilica LCG")) {
        recorder = new LAUBackgroundWidget(ColorXYZG, DeviceProsilicaLCG);
    } else if (string == QString("Prosilica IOS")) {
        recorder = new LAUBackgroundWidget(ColorXYZG, DeviceProsilicaIOS);
    } else if (string == QString("Kinect NIR-D")) {
        recorder = new LAUBackgroundWidget(ColorXYZG, DeviceKinect);
    } else if (string == QString("Kinect RGB-D")) {
        recorder = new LAUBackgroundWidget(ColorXYZWRGBA, DeviceKinect);
    } else if (string == QString("VZense NIR-D")) {
        recorder = new LAUBackgroundWidget(ColorXYZG, DeviceVZense);
    } else if (string == QString("Lucid NIR-D")) {
        recorder = new LAUBackgroundWidget(ColorXYZG, DeviceLucid);
    } else if (string == QString("Vidu NIR-D")) {
        recorder = new LAUBackgroundWidget(ColorXYZG, DeviceVidu);
    } else if (string == QString("Orbbec NIR-D")) {
        recorder = new LAUBackgroundWidget(ColorXYZG, DeviceOrbbec);
    } else {
        return;
    }

    // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
    if (recorder && recorder->isValid()) {
        connect(recorder, SIGNAL(emitVideoFrames(LAUMemoryObject)), this, SLOT(onInsertImage(LAUMemoryObject)));

        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake      = recorder->make();
        scannerColor     = recorder->color();
        scannerModel     = recorder->model();
        scannerTransform = recorder->transform();

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        recorder->setContentsMargins(6,6,6,6);
        launchAsDialog(recorder);
    } else {
        delete recorder;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterGreenScreenTool()
{
    QString string = targetScanner();
    //#define USEGREENSCREENWITHACCUMULATE
#ifdef USEGREENSCREENWITHACCUMULATE
    LAUGreenScreenWithAccumulateWidget *recorder = nullptr;
    if (string == QString("Prime Sense")) {
        recorder = new LAUGreenScreenWithAccumulateWidget(ColorXYZWRGBA, DevicePrimeSense);
    } else if (string == QString("Real Sense RGB-D")) {
        recorder = new LAUGreenScreenWithAccumulateWidget(ColorXYZWRGBA, DeviceRealSense);
    } else if (string == QString("Real Sense NIR-D")) {
        recorder = new LAUGreenScreenWithAccumulateWidget(ColorXYZG, DeviceRealSense);
    } else if (string ==  QString("VZense NIR-D")) {
        recorder = new LAUGreenScreenWithAccumulateWidget(ColorXYZRGB, DeviceVZense);
    } else if (string ==  QString("Lucid NIR-D")) {
        recorder = new LAUGreenScreenWithAccumulateWidget(ColorXYZRGB, DeviceLucid);
    } else if (string ==  QString("Vidu NIR-D")) {
        recorder = new LAUGreenScreenWithAccumulateWidget(ColorXYZRGB, DeviceVidu);
    } else if (string ==  QString("Orbbec NIR-D")) {
        recorder = new LAUGreenScreenWithAccumulateWidget(ColorXYZRGB, DeviceOrbbec);
    } else if (string == QString("Prosilica LCG")) {
        recorder = new LAUGreenScreenWithAccumulateWidget(ColorXYZG, DeviceProsilicaLCG);
    } else if (string == QString("Prosilica IOS")) {
        recorder = new LAUGreenScreenWithAccumulateWidget(ColorXYZG, DeviceProsilicaIOS);
    } else if (string == QString("Kinect NIR-D")) {
        recorder = new LAUGreenScreenWithAccumulateWidget(ColorXYZG, DeviceKinect);
    } else if (string == QString("Kinect RGB-D")) {
        recorder = new LAUGreenScreenWithAccumulateWidget(ColorXYZWRGBA, DeviceKinect);
    } else {
        return;
    }

    // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
    if (recorder && recorder->isValid()) {
        // GET THE DIRECTORY TO SAVE IMAGES
        QSettings settings;
        QString directory = settings.value("LAUMSColorHistogramGLFilter::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        directory = QFileDialog::getExistingDirectory(this, QString("Select location to save video frames"), directory);
        if (directory.isEmpty() == false) {
            recorder->setDirectory(directory);
            settings.setValue("LAUMSColorHistogramGLFilter::lastUsedDirectory", directory);
        } else {
            QMessageBox::warning(this, QString("Green Screen With Accumulate"), QString("No sink directory specified."));
        }

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        launchAsDialog(recorder);
    } else {
        delete recorder;
    }
#else
    LAUGreenScreenWidget *recorder = nullptr;
    if (string == QString("Prime Sense")) {
        recorder = new LAUGreenScreenWidget(ColorXYZWRGBA, DevicePrimeSense);
    } else if (string == QString("Real Sense RGB-D")) {
        recorder = new LAUGreenScreenWidget(ColorXYZWRGBA, DeviceRealSense);
    } else if (string ==  QString("VZense NIR-D")) {
        recorder = new LAUGreenScreenWidget(ColorXYZG, DeviceVZense);
    } else if (string ==  QString("Lucid NIR-D")) {
        recorder = new LAUGreenScreenWidget(ColorXYZG, DeviceLucid);
    } else if (string ==  QString("Vidu NIR-D")) {
        recorder = new LAUGreenScreenWidget(ColorXYZG, DeviceVidu);
    } else if (string ==  QString("Orbbec NIR-D")) {
        recorder = new LAUGreenScreenWidget(ColorXYZG, DeviceOrbbec);
    } else if (string == QString("Real Sense NIR-D")) {
        recorder = new LAUGreenScreenWidget(ColorXYZG, DeviceRealSense);
    } else if (string == QString("Prosilica LCG")) {
        recorder = new LAUGreenScreenWidget(ColorXYZG, DeviceProsilicaLCG);
    } else if (string == QString("Prosilica IOS")) {
        recorder = new LAUGreenScreenWidget(ColorXYZG, DeviceProsilicaIOS);
    } else if (string == QString("Kinect NIR-D")) {
        recorder = new LAUGreenScreenWidget(ColorXYZG, DeviceKinect);
    } else if (string == QString("Kinect RGB-D")) {
        recorder = new LAUGreenScreenWidget(ColorXYZWRGBA, DeviceKinect);
    } else {
        return;
    }

    // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
    if (recorder && recorder->isValid()) {
        connect(recorder, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), this, SLOT(onInsertImage(QList<LAUMemoryObject>)));
        recorder->enableSnapShotMode(false);

        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake      = recorder->make();
        scannerColor     = recorder->color();
        scannerModel     = recorder->model();
        scannerTransform = recorder->transform();

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        launchAsDialog(recorder);
    } else {
        delete recorder;
    }
#endif
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterSetXYPlane()
{
    int index = document->indexOf(imageListWidget->currentItem());
    if (index > -1) {
#ifdef ENABLECALIBRATION
        LAUSetXYPlaneDialog dialog(document->image(index));
        if (dialog.exec() == QDialog::Accepted) {
            ;
        }
#endif
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterScanVelmexRailOnUserPath()
{
#ifndef EXCLUDE_LAUVELMEXWIDGET
    QList<QVector4D> points;
    for (int n = 0; n < document->count(); n++){
        float *buffer = (float*)(document->image(n).constPointer());
        points << QVector4D(buffer[0], buffer[1], buffer[2], buffer[3]);
    }

    // FILTER OUT POINTS THAT HAVE SAME X,Y BUT DIFFERENT Z
    for (int n = 1; n < points.count(); n++){
        QVector4D delta = points.at(n-1) - points.at(n);
        if (qAbs(delta.x()) < 0.001){
            if (qAbs(delta.y()) < 0.001){
                points.removeAt(n);
            }
        }
    }

    if (points.count() > 0){
        LAUVelmexUserPathOffsetDialog offsetDialog(4);
        if (offsetDialog.exec() == QDialog::Accepted){
            // GET THE OFFSET FROM THE SCAN COORDINATES
            QVector4D offsetA = offsetDialog.offset();
            QVector4D offsetB = QVector4D(offsetA.x(), offsetA.y(), offsetA.z() - 1.0, offsetA.w());

            // CREATE USER PATH FROM EXTRACTED CAMERA POINTS
            QList<QVector4D> upDownPoints;
            for (int n = 0; n < points.count(); n++){
                // FIRST POINT USES THE OFFSET X AND Y COORDINATES TO GET THE VACUUM ABOVE THE WELL
                upDownPoints << (points.at(n) + offsetB);

                // SECOND POINT MOVES THE VACUUM IN THE Z DIRECTION TO LOWER AND PICK UP MOSQUITO
                upDownPoints << (points.at(n) + offsetA);

                // THIRD POINT MOVES THE VACUUM IN THE Z DIRECTION TO RAISE THE VACUUM ABOVE THE WELL
                upDownPoints << (points.at(n) + offsetB);
            }

            // CREATE VELMEX RAIL WIDGET TO CONTROL RAIL
            LAUMultiVelmexWidget *velmexWidget = new LAUMultiVelmexWidget(-1, this);
            if (velmexWidget->isValid()) {
                // ENABLE THE WIDGET SO THAT THE USER CAN INTERACT WITH IT
                velmexWidget->scanUserPath(upDownPoints);

                // CREATE A DIALOG TO WRAP AROUND THE SCANNER
                launchAsDialog(velmexWidget);
            } else {
                // DISABLE THE WIDGET SO THAT THE USER CANNOT INTERACT WITH IT
                delete velmexWidget;
            }
        }
    }
#endif //EXCLUDE_LAUVELMEXWIDGET
}

#ifdef ENABLEPOINTMATCHER
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterMergeTool()
{
    int index = document->indexOf(imageListWidget->currentItem());
    if (index > -1 && index < document->count() - 1) {
        QStringList stringList = document->parentStringList();
        LAUScan master = document->image(stringList.at(index));   //master = master.transformScan(master.transform());
        LAUScan slave = document->image(stringList.at(index + 1)); //slave = slave.transformScan(slave.transform());

        LAUMergeScanDialog dialog(master, slave);
        if (dialog.exec() == QDialog::Accepted) {
            QMatrix4x4 transform = master.transform() * dialog.transform();
            slave.setTransform(transform);
            document->replaceImage(slave);
            imageStackWidget->onUpdateScan(slave);

            // SAVE THE SCAN TO DISK WITH THE FRAME TO FRAME TRANSFORM
            //slave.setTransform(dialog.transform());
            //slave.save(QString());
            //return;
        }
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterAutoMergeTool()
{
    QStringList stringList = document->parentStringList();
    if (stringList.count() > 1) {
        int index = qMax(0, document->indexOf(imageListWidget->currentItem()));

        LAUScan master = document->image(stringList.at(index));
        LAU3DTrackingFilter filter(master.width(), master.height());
        QProgressDialog progressDialog(QString("Aligning scans..."), QString(), 1, stringList.count(), this, Qt::Sheet);
        for (int n = 1; n < stringList.count(); n++) {
            if (progressDialog.wasCanceled()) {
                break;
            }
            progressDialog.setValue(n);
            qApp->processEvents();

            LAUScan master = document->image(stringList.at(n - 1));
            LAUScan slave = document->image(stringList.at(n));

            QMatrix4x4 transform = filter.findTransform(master, slave);
            slave = slave.transformScan(master.transform() * transform);
            document->replaceImage(slave);
            imageStackWidget->onUpdateScan(slave);
        }
        progressDialog.setValue(stringList.count());
        imageStackWidget->onEnableScan(imageListWidget->currentItem());
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterBCSTracking()
{
    QString string = targetScanner();
    LAU3DBCSTrackingWidget *recorder = nullptr;
    if (string == QString("Prime Sense")) {
        recorder = new LAU3DBCSTrackingWidget(ColorXYZWRGBA, DevicePrimeSense);
    } else if (string == QString("Real Sense RGB-D")) {
        recorder = new LAU3DBCSTrackingWidget(ColorXYZWRGBA, DeviceRealSense);
    } else if (string == QString("Real Sense NIR-D")) {
        recorder = new LAU3DBCSTrackingWidget(ColorXYZG, DeviceRealSense);
    } else if (string == QString("Prosilica LCG")) {
        recorder = new LAU3DBCSTrackingWidget(ColorXYZG, DeviceProsilicaLCG);
    } else if (string == QString("Prosilica IOS")) {
        recorder = new LAU3DBCSTrackingWidget(ColorXYZG, DeviceProsilicaIOS);
    } else if (string == QString("Kinect NIR-D")) {
        recorder = new LAU3DBCSTrackingWidget(ColorXYZG, DeviceKinect);
    } else if (string == QString("Kinect RGB-D")) {
        recorder = new LAU3DBCSTrackingWidget(ColorXYZWRGBA, DeviceKinect);
    } else {
        return;
    }

    // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
    if (recorder && recorder->isValid()) {
        connect(recorder, SIGNAL(emitVideoFrames(LAUScan)), this, SLOT(onInsertImage(LAUScan)));
        connect(recorder, SIGNAL(emitVideoFrames(QList<LAUScan>)), this, SLOT(onInsertImage(QList<LAUScan>)));
        recorder->enableSnapShotMode(false);

        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake      = recorder->make();
        scannerColor     = recorder->color();
        scannerModel     = recorder->model();

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        launchAsDialog(recorder);
    } else {
        delete recorder;
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterTracking()
{
    QString string = targetScanner();
    LAU3DTrackingWidget *recorder = nullptr;
    if (string == QString("Prime Sense")) {
        recorder = new LAU3DTrackingWidget(ColorXYZWRGBA, DevicePrimeSense);
    } else if (string == QString("Real Sense RGB-D")) {
        recorder = new LAU3DTrackingWidget(ColorXYZWRGBA, DeviceRealSense);
    } else if (string == QString("Real Sense NIR-D")) {
        recorder = new LAU3DTrackingWidget(ColorXYZG, DeviceRealSense);
    } else if (string == QString("Prosilica LCG")) {
        recorder = new LAU3DTrackingWidget(ColorXYZG, DeviceProsilicaLCG);
    } else if (string == QString("Prosilica IOS")) {
        recorder = new LAU3DTrackingWidget(ColorXYZG, DeviceProsilicaIOS);
    } else if (string == QString("Kinect NIR-D")) {
        recorder = new LAU3DTrackingWidget(ColorXYZG, DeviceKinect);
    } else if (string == QString("Kinect RGB-D")) {
        recorder = new LAU3DTrackingWidget(ColorXYZWRGBA, DeviceKinect);
    } else {
        return;
    }

    // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
    if (recorder && recorder->isValid()) {
        connect(recorder, SIGNAL(emitVideoFrames(LAUScan)), this, SLOT(onInsertImage(LAUScan)));
        connect(recorder, SIGNAL(emitVideoFrames(QList<LAUScan>)), this, SLOT(onInsertImage(QList<LAUScan>)));
        recorder->enableSnapShotMode(false);

        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake      = recorder->make();
        scannerColor     = recorder->color();
        scannerModel     = recorder->model();

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        launchAsDialog(recorder);
    } else {
        delete recorder;
    }
    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterSymmetryTool()
{
#ifdef DONTCOMPILE
    LAU3DSymmetryFilter smFilter(LAU_CAMERA_DEFAULT_WIDTH, LAU_CAMERA_DEFAULT_HEIGHT, LAU3DSymmetryFilter::AxisY);

    QStringList strings = document->parentStringList();
    QProgressDialog dialog(QString("Resampling images..."), QString("Abort"), 0, strings.count(), this, Qt::Sheet);
    dialog.show();
    for (int n = 0; n < strings.count(); n++) {
        if (dialog.wasCanceled()) {
            break;
        } else {
            dialog.setValue(n);
            qApp->processEvents();
        }
        LAUScan scan = document->image(strings.at(n));
        if (scan.isValid()) {
            scan.setTransform(smFilter.findTransform(scan));
            LAUResampleGLFilter rsFilter(scan, -100.0f, -2000.0f, 1.0035643199f, 0.78539816339f);
            rsFilter.grabScan((float *)scan.constPointer());
            scan.setTransform(QMatrix4x4());
            document->replaceImage(scan);
            //scan.setTransform(QMatrix4x4());
            //scan.save(QString("%1/%2").arg(directory).arg(scan.filename().split("/").last()));
        }
    }
    dialog.setValue(strings.count());
#else
    int index = document->indexOf(imageListWidget->currentItem());
    if (index > -1) {
        QStringList stringList = document->parentStringList();
        LAUScan master = document->image(stringList.at(index));   //master = master.transformScan(master.transform());
        LAUScan slave = document->image(stringList.at(index));   //master = master.transformScan(master.transform());

        // CHANGE THE FILENAME STRINGS SO WE DON'T CONFUSE THEM LATER
        master.setFilename(QString("master"));
        slave.setFilename(QString("slave"));

        QMatrix4x4 flipTransform;
        flipTransform.scale(-1.0, 1.0, 1.0);
        slave = slave.transformScan(flipTransform);
        slave = slave.flipLeftRight();

        LAUMergeScanDialog dialog(master, slave);
        if (dialog.exec() == QDialog::Accepted) {
            QMatrix4x4 transform = master.transform() * dialog.transform();
            slave.setTransform(transform);
            document->replaceImage(slave);
            imageStackWidget->onUpdateScan(slave);

            // SAVE THE SCAN TO DISK WITH THE FRAME TO FRAME TRANSFORM
            //slave.setTransform(dialog.transform());
            //slave.save(QString());
            //return;
        }
    }
    return;
    if (index > -1) {
        LAUScan scan = document->image(imageListWidget->currentItem());
        LAU3DSymmetryDialog dialog(scan);
        if (dialog.exec() == QDialog::Accepted) {
            ;
        } else {
            return;
        }
    }
    return;
#endif
    return;
}
#endif

#ifdef SANDBOX
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterSandboxCalibrationTool()
{
    QString string = targetScanner();
    LAU3DSandboxCalibrationWidget *recorder = nullptr;
    if (string == QString("Prime Sense")) {
        recorder = new LAU3DSandboxCalibrationWidget(ColorXYZWRGBA, DevicePrimeSense);
    } else if (string == QString("Real Sense RGB-D")) {
        recorder = new LAU3DSandboxCalibrationWidget(ColorXYZWRGBA, DeviceRealSense);
    } else if (string == QString("Real Sense NIR-D")) {
        recorder = new LAU3DSandboxCalibrationWidget(ColorXYZG, DeviceRealSense);
    } else if (string == QString("Prosilica LCG")) {
        recorder = new LAU3DSandboxCalibrationWidget(ColorXYZWRGBA, DeviceProsilicaLCG);
    } else if (string == QString("Prosilica IOS")) {
        recorder = new LAU3DSandboxCalibrationWidget(ColorXYZWRGBA, DeviceProsilicaIOS);
    } else if (string == QString("Kinect NIR-D")) {
        recorder = new LAU3DSandboxCalibrationWidget(ColorXYZG, DeviceKinect);
    } else if (string == QString("Kinect RGB-D")) {
        recorder = new LAU3DSandboxCalibrationWidget(ColorXYZWRGBA, DeviceKinect);
    } else if (string == QString("Ximea")) {
        recorder = new LAU3DSandboxCalibrationWidget(ColorGray, DeviceXimea);
    } else if (string == QString("IDS")) {
        recorder = new LAU3DSandboxCalibrationWidget(ColorRGB, DeviceIDS);
    } else {
        return;
    }

    // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
    if (recorder && recorder->isValid()) {
        connect(recorder, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), this, SLOT(onInsertImage(QList<LAUMemoryObject>)));
        recorder->enableSnapShotMode(false);

        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake      = recorder->make();
        scannerColor     = recorder->color();
        scannerModel     = recorder->model();
        //scannerTransform = recorder->transform();

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        launchAsDialog(recorder);
    } else {
        delete recorder;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterSandboxTool()
{
#ifdef ENABLECALIBRATION
    int index = document->indexOf(imageListWidget->currentItem());
    if (index > -1) {
        LAUBinarizeScanDialog dialog(document->image(imageListWidget->currentItem()));
        if (dialog.exec() == QDialog::Accepted) {
            ;
        }
    } else {
#endif
        QString string = targetScanner();
        LAU3DSandboxVideoRecorderWidget *recorder = nullptr;
        if (string == QString("Prime Sense")) {
            recorder = new LAU3DSandboxVideoRecorderWidget(ColorXYZWRGBA, DevicePrimeSense);
        } else if (string == QString("Real Sense RGB-D")) {
            recorder = new LAU3DSandboxVideoRecorderWidget(ColorXYZWRGBA, DeviceRealSense);
        } else if (string == QString("Real Sense NIR-D")) {
            recorder = new LAU3DSandboxVideoRecorderWidget(ColorXYZG, DeviceRealSense);
        } else if (string == QString("Prosilica LCG")) {
            recorder = new LAU3DSandboxVideoRecorderWidget(ColorXYZWRGBA, DeviceProsilicaLCG);
        } else if (string == QString("Prosilica IOS")) {
            recorder = new LAU3DSandboxVideoRecorderWidget(ColorXYZWRGBA, DeviceProsilicaIOS);
        } else if (string == QString("Kinect NIR-D")) {
            recorder = new LAU3DSandboxVideoRecorderWidget(ColorXYZG, DeviceKinect);
        } else if (string == QString("Kinect RGB-D")) {
            recorder = new LAU3DSandboxVideoRecorderWidget(ColorXYZWRGBA, DeviceKinect);
        } else if (string == QString("Ximea")) {
            recorder = new LAU3DSandboxVideoRecorderWidget(ColorGray, DeviceXimea);
        } else if (string == QString("IDS")) {
            recorder = new LAU3DSandboxVideoRecorderWidget(ColorRGB, DeviceIDS);
        } else {
            return;
        }

        // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
        if (recorder && recorder->isValid()) {
            connect(recorder, SIGNAL(destroyed()), this, SLOT(onExecuteAsDialogComplete()));
            connect(recorder, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), this, SLOT(onInsertImage(QList<LAUMemoryObject>)));

            // SET THE RECORDING WIDGET TO SNAP SHOT MODE
            recorder->enableSnapShotMode(true);

            // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
            scannerMake      = recorder->make();
            scannerColor     = recorder->color();
            scannerModel     = recorder->model();
            //scannerTransform = recorder->transform();

            // LOCK THE QMUTEX TO PREVENT THE USER FROM DELETING THIS WIDGET WHEN THERE IS A CAMERA OBJECT
            mutex.lock();

            // CREATE A DIALOG TO WRAP AROUND THE SCANNER
            QDialog dialog;
            dialog.setLayout(new QVBoxLayout());
            dialog.layout()->setContentsMargins(0, 0, 0, 0);
            dialog.layout()->addWidget(recorder);
            dialog.exec();
        } else {
            delete recorder;
        }
#ifdef ENABLECALIBRATION
    }
#endif
}
#endif

#ifdef HYPERSPECTRAL
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterHyperspectral()
{
    // CREATE A VIDEO RECORDED SPECIFIC TO PASSIVE STEREOVISION
    LAU3DHyperspectralRecordingWidget *recorder = new LAU3DHyperspectralRecordingWidget();

    // MAKE SURE WE HAVE A VALID SCANNER AND DISPLAY IT IF WE DO
    if (recorder && recorder->isValid()) {
        connect(recorder, SIGNAL(emitVideoFrames(QList<LAUMemoryObject>)), this, SLOT(onInsertImage(QList<LAUMemoryObject>)));
        recorder->enableSnapShotMode(false);
        recorder->enableVelmexScanMode(true);

        // SAVE THESE FEATURES OF THE CAMERA FOR ANY INCOMING VIDEO FRAMES
        scannerMake  = recorder->make();
        scannerColor = recorder->color();
        scannerModel = recorder->model();

        // CREATE A DIALOG TO WRAP AROUND THE SCANNER
        launchAsDialog(recorder);
    } else {
        delete recorder;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocumentWidget::onFilterHyperspectralMerge()
{
    QList<LAUScan> scans = document->images();
    if (scans.count() > 0) {
        LAUScan scan = LAU3DHyperspectralRecordingWidget::processFrames(scans);
        if (scan.approveImage()) {
            ;
        }
    }
}
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUImageListWidget::LAUImageListWidget(QStringList stringList, QWidget *parent) : QWidget(parent)
{
    this->setFixedWidth(300);
    this->setLayout(new QVBoxLayout());

    imageListWidget = new LAUListWidget();
    imageListWidget->setAlternatingRowColors(true);
    imageListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    imageListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    connect(imageListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(onItemDoubleClicked(QListWidgetItem *)), Qt::QueuedConnection);
    connect(imageListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(onCurrentItemChanged()), Qt::QueuedConnection);
    connect(imageListWidget, SIGNAL(contextualMenuTriggered(QPoint)), this, SLOT(onContextualMenuTriggered(QPoint)), Qt::QueuedConnection);

    this->layout()->addWidget(imageListWidget);
    this->layout()->setContentsMargins(0, 0, 0, 0);

    QWidget *buttonBox = new QWidget();
    buttonBox->setLayout(new QHBoxLayout());
    buttonBox->layout()->setContentsMargins(0, 0, 0, 0);
    buttonBox->layout()->setSpacing(0);

    // Style for buttons - ensure light background and dark text for readability
    // Use default size, only override colors and borders
    QString buttonStyle =
        "QToolButton {"
        "  background-color: #f0f0f0;"
        "  color: #000000;"
        "  border: 1px solid #c0c0c0;"
        "}"
        "QToolButton:hover {"
        "  background-color: #e0e0e0;"
        "  border: 1px solid #a0a0a0;"
        "}"
        "QToolButton:pressed {"
        "  background-color: #d0d0d0;"
        "  border: 1px solid #808080;"
        "}";

    QToolButton *button = new QToolButton();
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setStyleSheet(buttonStyle);
#ifdef REALTIMESLI
    button->setText(QString("Insert"));
    button->setFixedWidth(75);
#else
    button->setText(QString("Add"));
#endif
    connect(button, SIGNAL(clicked()), this, SLOT(onInsertButtonClicked()));
    buttonBox->layout()->addWidget(button);

    button = new QToolButton();
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setStyleSheet(buttonStyle);
#ifdef REALTIMESLI
    button->setText(QString("Remove"));
    button->setFixedWidth(75);
#else
    button->setText(QString("Sub"));
#endif
    connect(button, SIGNAL(clicked()), this, SLOT(onRemoveButtonClicked()));
    buttonBox->layout()->addWidget(button);

#ifndef REALTIMESLI
    button = new QToolButton();
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setStyleSheet(buttonStyle);
    button->setText(QString("x2"));
    connect(button, SIGNAL(clicked()), this, SLOT(onDuplicateButtonClicked()));
    buttonBox->layout()->addWidget(button);
#endif

    button = new QToolButton();
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setStyleSheet(buttonStyle);
#ifdef REALTIMESLI
    button->setText(QString("Move Up"));
    button->setFixedWidth(75);
#else
    button->setText(QString("Up"));
#endif
    connect(button, SIGNAL(clicked()), this, SLOT(onMoveUpButtonClicked()));
    buttonBox->layout()->addWidget(button);

    button = new QToolButton();
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setStyleSheet(buttonStyle);
#ifdef REALTIMESLI
    button->setText(QString("Move Down"));
    button->setFixedWidth(75);
#else
    button->setText(QString("Down"));
#endif
    connect(button, SIGNAL(clicked()), this, SLOT(onMoveDownButtonClicked()));
    buttonBox->layout()->addWidget(button);

    this->layout()->addWidget(buttonBox);
    this->insertImages(stringList);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUImageListWidget::~LAUImageListWidget()
{
    ;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUImageListWidget::currentItem()
{
    QListWidgetItem *item = imageListWidget->currentItem();
    if (item) {
        return (item->data(Qt::ToolTipRole).toString());
    }
    return (QString());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUImageListWidget::imageList()
{
    QStringList stringList;
    for (int n = 0; n < imageListWidget->count(); n++) {
        stringList.append(imageListWidget->item(n)->data(Qt::ToolTipRole).toString());
    }
    return (stringList);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUImageListWidget::stringAlreadyInList(QString string)
{
    for (int n = 0; n < imageListWidget->count(); n++) {
        if (imageListWidget->item(n)->data(Qt::ToolTipRole).toString() == string) {
            return (true);
        }
    }
    return (false);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUImageListWidget::insertImages(QStringList imageList)
{
    while (!imageList.isEmpty()) {
        // INSERT LIST BACK TO FRONT BECAUSE IMAGES GET INSERTED
        // IN FRONT OF THE CURRENT ROW
        insertImage(imageList.takeLast());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUImageListWidget::insertImage(QString string, int index)
{
    QListWidgetItem *item = new QListWidgetItem(string.split(QDir::separator()).last());
    if (index == -1){
        int currentRow = qMax(imageListWidget->currentRow(), 0);
        item->setData(Qt::ToolTipRole, string);
        imageListWidget->insertItem(currentRow, item);
        imageListWidget->setCurrentRow(currentRow);
    } else {
        index = qMax(0, qMin(index, imageListWidget->count()));
        item->setData(Qt::ToolTipRole, string);
        imageListWidget->insertItem(index, item);
        imageListWidget->setCurrentRow(index);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUImageListWidget::onRemoveButtonClicked()
{
    if (imageListWidget->currentRow() >= 0) {
        emit removeImageAction(imageListWidget->currentItem()->data(Qt::ToolTipRole).toString());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUImageListWidget::onDuplicateButtonClicked()
{
    if (imageListWidget->currentRow() >= 0) {
        emit duplicateImageAction(imageListWidget->currentItem()->data(Qt::ToolTipRole).toString());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUImageListWidget::onItemDoubleClicked(QListWidgetItem *item)
{
    if (imageListWidget->currentRow() >= 0) {
        emit currentItemDoubleClicked(item->data(Qt::ToolTipRole).toString());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUImageListWidget::onMoveUpButtonClicked()
{
    int currentRow = imageListWidget->currentRow();
    if (currentRow > 0) {
        QString stringA = imageListWidget->item(currentRow)->data(Qt::ToolTipRole).toString();
        QString stringB = imageListWidget->item(currentRow - 1)->data(Qt::ToolTipRole).toString();

        QListWidgetItem *item = imageListWidget->takeItem(currentRow);
        imageListWidget->insertItem(currentRow - 1, item);
        imageListWidget->setCurrentRow(currentRow - 1);

        emit swapImageAction(stringA, stringB);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUImageListWidget::onMoveDownButtonClicked()
{
    int currentRow = imageListWidget->currentRow();
    if (currentRow < (imageListWidget->count() - 1)) {
        QString stringA = imageListWidget->item(currentRow + 1)->data(Qt::ToolTipRole).toString();
        QString stringB = imageListWidget->item(currentRow)->data(Qt::ToolTipRole).toString();

        QListWidgetItem *item = imageListWidget->takeItem(currentRow);
        imageListWidget->insertItem(currentRow + 1, item);
        imageListWidget->setCurrentRow(currentRow + 1);

        emit swapImageAction(stringA, stringB);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUImageListWidget::onCurrentItemChanged(QListWidgetItem *item)
{
    Q_UNUSED(item);
    if (imageListWidget->currentItem()) {
        emit currentItemChanged(imageListWidget->currentItem()->data(Qt::ToolTipRole).toString());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUImageListWidget::removeImage(QString string)
{
    for (int n = 0; n < imageListWidget->count(); n++) {
        if (imageListWidget->item(n)->data(Qt::ToolTipRole).toString() == string) {
            QListWidgetItem *item = imageListWidget->takeItem(n);
            delete item;
            return;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUImageListWidget::removeImages(QStringList imageList)
{
    for (int n = 0; n < imageList.count(); n++) {
        removeImage(imageList.at(n));
    }
}
