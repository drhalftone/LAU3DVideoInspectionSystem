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

#include "lau3dvideoplayerwidget.h"

#include <QDir>
#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>
#include <QProgressDialog>

using namespace libtiff;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DVideoPlayerWidget::LAU3DVideoPlayerWidget(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent) : QDialog(parent), numRows(rows), numCols(cols), playbackColor(color), replayVideoLabel(NULL), replayGLWidget(NULL)
{
    saveFlag = true;
    if (playbackColor == ColorGray) {
        numChns = 1;
    } else if (playbackColor == ColorRGB) {
        numChns = 3;
    } else if (playbackColor == ColorXYZG) {
        numChns = 4;
    } else if (playbackColor == ColorXYZRGB) {
        numChns = 6;
    } else if (playbackColor == ColorXYZWRGBA) {
        numChns = 8;
    }
    initializeInterface();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DVideoPlayerWidget::LAU3DVideoPlayerWidget(QString filenameString, QWidget *parent) : QDialog(parent), numRows(0), numCols(0), replayVideoLabel(NULL), replayGLWidget(NULL)
{
    saveFlag = false;
    if (filenameString.isEmpty()) {
        // LOAD IMAGE FROM DISK AND INITIALIZE ALL SIZE, COLOR, AND DEVICE PARAMETERS
        // ASK THE USER FOR A FILE TO SAVE
        QSettings settings;
        QString dirString = settings.value(QString("LastUsedDirectory"), QDir::homePath()).toString();
        filenameString = QFileDialog::getOpenFileName(NULL, QString("Load video from disk (*.tif)"), dirString.append("/Untitled.tif"), QString("*.tif"));

        // SEE IF USER SPECIFIED A FILE, AND IF SO, SAVE THE DIRECTORY FOR FUTURE ITERATIONS OF FILE DIALOG
        if (!filenameString.isNull()) {
            settings.setValue(QString("LastUsedDirectory"), QFileInfo(filenameString).absolutePath());
        }
    }

    // CREATE A VECTOR TO HOLD THE RANGE LIMITS
    float rangeValues[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // SEE IF USER SPECIFIED A FILE, AND IF SO, SAVE THE DIRECTORY FOR FUTURE ITERATIONS OF FILE DIALOG
    if (!filenameString.isNull()) {
        if (QFile::exists(filenameString)) {
            // OPEN TIFF FILE FOR SAVING THE IMAGE
            TIFF *inputTiff = TIFFOpen(filenameString.toLocal8Bit(), "r");
            if (inputTiff) {
                int numFrames = TIFFNumberOfDirectories(inputTiff);
                QProgressDialog dialog(filenameString, QString(), 0, numFrames);
                dialog.show();
                for (int n = 0; n < numFrames; n++) {
                    dialog.setValue(n);
                    qApp->processEvents();

                    // SET THE CURRENT DIRECTORY
                    TIFFSetDirectory(inputTiff, (unsigned short)n);

                    // CREATE A LAUSCAN OBJECT FROM THE CURRENT TIFF DIRECTORY
                    LAUScan scan(inputTiff);

                    // MAKE SURE WE HAVE A VALID SCAN
                    if (scan.isValid()) {
                        if (recordedVideoFramesBufferList.isEmpty()) {
                            // FIRST FRAME IS THE KEY FRAME THAT DEFINES THE SIZE OF ALL
                            // SUBSEQUENT FRAMES IN THE STACK
                            numRows = scan.height();
                            numCols = scan.width();
                            numChns = scan.colors();
                            playbackColor = scan.color();

                            // READ THE RANGE OF VALUES FOR EACH DIMENSION AND USE
                            // THEM TO SET THE LIMITS FOR THE GLWIDGET
                            rangeValues[0] = scan.minX();
                            rangeValues[1] = scan.maxX();
                            rangeValues[2] = scan.minY();
                            rangeValues[3] = scan.maxY();
                            rangeValues[4] = scan.minZ();
                            rangeValues[5] = scan.maxZ();
                            rangeValues[6] = scan.centroid().x();
                            rangeValues[7] = scan.centroid().y();
                            rangeValues[8] = scan.centroid().z();
                        } else {
                            // MAKE SURE IMAGE MATCHES KEY FRAME, OTHERWISE DISCARD
                            if (scan.height() != numRows || scan.width() != numCols || scan.colors() != numChns) {
                                continue;
                            }
                        }
                        // ADD THE SCAN TO OUR LIST OF SCANS
                        recordedVideoFramesBufferList.append(scan);
                    } else {
                        // SKIP THIS INVALID DIRECTORY
                        continue;
                    }
                }
                dialog.setValue(recordedVideoFramesBufferList.count());

                // CLOSE TIFF FILE
                TIFFClose(inputTiff);
            } else {
                QMessageBox::warning(this, QString("Video Player"), QString("Error opening tiff file: %1").arg(LAUMemoryObject::lastTiffErrorString));
            }
        } else {
            QMessageBox::warning(this, QString("Video Player"), QString("Specified file does not exist."));
        }
    }

    // NOW THAT WE HAVE THE PARAMETERS THAT DEFINE THE VIDEO
    // WE CAN BUILD THE USER INTERFACE TO DISPLAYING VIDEO
    initializeInterface();

    // SET THE LIMITS FOR THE GLWIDGET
    replayGLWidget->setLimits(rangeValues[0], rangeValues[1], rangeValues[2], rangeValues[3], rangeValues[4], rangeValues[5], rangeValues[6], rangeValues[7], rangeValues[8]);

    // SEND ALL VIDEO FRAMES TO THE VIDEO WIDGET FOR REPLAY
    for (int n = 0; n < recordedVideoFramesBufferList.count(); n++) {
        replayVideoLabel->onInsertPacket(recordedVideoFramesBufferList.at(n));
    }

    return;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DVideoPlayerWidget::~LAU3DVideoPlayerWidget()
{
    // DELETE ALL VIDEO FRAMES FROM MEMORY
    while (!recordedVideoFramesBufferList.isEmpty()) {
        // WE MUST REMOVE THE COPY OF THE CURRENT BUFFER FROM THE REPLAY WIDGET
        replayVideoLabel->onRemovePacket(recordedVideoFramesBufferList.takeFirst());
    }
    qDebug() << QString("LAU3DVideoPlayerWidget::~LAU3DVideoPlayerWidget()");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoPlayerWidget::initializeInterface()
{
    this->setWindowTitle(QString("Video Player"));
    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->layout()->setSpacing(0);

    // CREATE A GLWIDGET TO PROCESS THE DFT COEFFICIENTS AND DISPLAY THE POINT CLOUD
    replayGLWidget = new LAU3DScanGLWidget(numCols, numRows, playbackColor, NULL);
    replayGLWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->layout()->addWidget(replayGLWidget);

    replayVideoLabel = new LAUVideoPlayerLabel(LAUVideoPlayerLabel::StateVideoPlayer);
    connect(replayVideoLabel, SIGNAL(emitPacket(LAUMemoryObject)), replayGLWidget, SLOT(onUpdateBuffer(LAUMemoryObject)));
    this->layout()->addWidget(replayVideoLabel);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoPlayerWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_B) {
        qDebug() << "LAU3DVideoPlayerWidget::Key_B";
    } else if (event->key() == Qt::Key_PageDown) {
        qDebug() << "LAU3DVideoPlayerWidget::Key_PageDown";
    } else if (event->key() == Qt::Key_PageUp) {
        qDebug() << "LAU3DVideoPlayerWidget::Key_PageUp";
    } else if (event->key() == Qt::Key_Shift) {
        qDebug() << "LAU3DVideoPlayerWidget::Key_Shift";
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoPlayerWidget::closeEvent(QCloseEvent *event)
{
    if (saveFlag && recordedVideoFramesBufferList.count() > 0) {
        // STOP THE VIDEO PLAYER IN CASE ITS RUNNING
        replayVideoLabel->onPlayButtonClicked(false);

        // ASK THE USER IF THEY WANT TO SAVE OR DISCARD THE STORED VIDEO
        int ret = QMessageBox::warning(0, QString("Kinect Video Recorder"),
                                       QString("Save video to disk before closing?"),
                                       QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel) {
            event->ignore();
            return;
        } else if (ret == QMessageBox::No) {
            ;
        } else {
            if (!saveRecordedVideoToDisk()) {
                event->ignore();
                return;
            }
        }
    }
    QDialog::closeEvent(event);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DVideoPlayerWidget::onInsertPacket(LAUMemoryObject packet)
{
    recordedVideoFramesBufferList << packet;
    replayVideoLabel->onInsertPacket(packet);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAU3DVideoPlayerWidget::saveRecordedVideoToDisk()
{
    long long expectedFileSize = (long long)numCols * (long long)numRows * (long long)numChns * (long long)recordedVideoFramesBufferList.count() * (long long)sizeof(float);
    int numberOfFiles = (int)(expectedFileSize >> 30) + 1;
    int framesPerFile = recordedVideoFramesBufferList.count() / numberOfFiles;
    int fileCounter = 0;
    int frameCounter = 0;

    // ASK THE USER FOR A FILE TO SAVE
    QSettings settings;
    QString dirString = settings.value(QString("LastUsedDirectory"), QDir::homePath()).toString();
    QString filenameString = QFileDialog::getSaveFileName(NULL, QString("Save video to disk (*.tif)"), dirString.append("/Untitled.tif"), QString("*.tif"));

    if (!filenameString.isNull()) {
        if (!filenameString.toLower().endsWith(QString(".tif"))) {
            filenameString = QString("%1.tif").arg(filenameString);
        }
        settings.setValue(QString("LastUsedDirectory"), QFileInfo(filenameString).absolutePath());

        QProgressDialog dialog(filenameString, QString(), 0, recordedVideoFramesBufferList.count(), this, Qt::Sheet);
        while (frameCounter < recordedVideoFramesBufferList.count()) {
            // GENERATE NEW FILE STRING INCLUDING SEQUENCING LETTER, IF NEEDED
            if (numberOfFiles > 1) {
                if (fileCounter == 0) {
                    // REMOVE THE .TIFF POSTFIX
                    filenameString.chop(4);
                    filenameString.append(QString("a.tif"));
                } else {
                    // REMOVE THE X.TIFF POSTFIX
                    filenameString.chop(5);
                    filenameString.append(QChar(97 + fileCounter));
                    filenameString.append(QString(".tif"));
                }
                // INCREMENT THE FILE COUNTER FOR THE NEXT ITERATION, IF NEEDED
                fileCounter++;
            }

            // OPEN TIFF FILE FOR SAVING THE IMAGE
            TIFF *outputTiff = TIFFOpen(filenameString.toLocal8Bit(), "w8");
            if (outputTiff) {
                // CALCULATE THE NUMBER OF FRAMES THAT WE CAN RECORD IN THIS PARTICULAR FILE
                int framesLeftToRecord = recordedVideoFramesBufferList.count() - frameCounter;
                if (framesLeftToRecord > framesPerFile) {
                    framesLeftToRecord = framesPerFile;
                }
                for (int n = 0; n < framesLeftToRecord; n++) {
                    dialog.setValue(frameCounter);
                    qApp->processEvents();

                    // CREATE A FILESTRING TO NAME THE FRAME
                    QString string = QString("%1").arg(n);
                    while (string.length() < 3) {
                        string.prepend("0");
                    }
                    string.prepend("frame");

                    // CREATE A UNIQUE DIRECTORY FOR EACH SCAN IMAGE IN THE STACK
                    TIFFSetField(outputTiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
                    TIFFSetField(outputTiff, TIFFTAG_PAGENUMBER, (unsigned short)n, (unsigned short)framesLeftToRecord);

                    // GRAB A LOCAL COPY OF THE SUBJECT IMAGE
                    LAUMemoryObject packet = recordedVideoFramesBufferList.at(frameCounter++);

                    // CREATE A LAUSCAN OBJECT TO WRAP AROUND THE PACKET
                    LAUScan scan(packet, playbackColor);
                    scan.setFilename(string);
                    scan.save(outputTiff, n);
                }
                // CLOSE TIFF FILE
                TIFFClose(outputTiff);
            } else {
                QMessageBox::warning(this, QString("Video Player"), QString("Error opening tiff file: %1").arg(LAUMemoryObject::lastTiffErrorString));
                return (false);
            }
        }
        dialog.setValue(recordedVideoFramesBufferList.count());
        return (true);
    }
    return (false);
}
