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


#include "laumachinelearningvideoframelabelerwidget.h"
#include "QtWidgets/qtablewidget.h"
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QStringList>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QHeaderView>
#include <QScrollBar>
#include <QMenuBar>
#include <QMenu>
#include <QFile>
#include <QTextStream>

using namespace libtiff;

typedef struct {
    QString frameString;
    unsigned int directory;
    unsigned int elapsed;
} FramePacket;

bool FramePacket_LessThan(const FramePacket &s1, const FramePacket &s2)
{
    return (s1.elapsed < s2.elapsed);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMachineLearningVideoFrameLabelerWidget::LAUMachineLearningVideoFrameLabelerWidget(unsigned int depth, QWidget *parent) : QWidget(parent), editFlag(false), playbackDepth(depth), paletteWidget(nullptr)
{
    this->setWindowFlags(Qt::Tool);
    this->setWindowTitle(QString("Labeler"));
    this->setLayout(new QVBoxLayout());
#ifdef Q_OS_WIN
    this->layout()->setContentsMargins(6, 6, 6, 6);
#else
    this->layout()->setContentsMargins(6, 6, 6, 6);
#endif
    this->layout()->setSpacing(1);
    this->setMinimumWidth(400);

    buttonWidget = new QWidget();
    buttonWidget->setLayout(new QHBoxLayout());
    buttonWidget->layout()->setContentsMargins(0, 0, 0, 0);
    buttonWidget->layout()->setSpacing(1);
    this->layout()->addWidget(buttonWidget);

    QPushButton *button = new QPushButton("LOAD");
    buttonWidget->layout()->addWidget(button);
    connect(button, SIGNAL(clicked()), this, SLOT(onLoadFromDisk()));

    button = new QPushButton("SAVE");
    buttonWidget->layout()->addWidget(button);
    connect(button, SIGNAL(clicked()), this, SLOT(onSaveToDisk()));

    button = new QPushButton("IMPORT");
    buttonWidget->layout()->addWidget(button);
    connect(button, SIGNAL(clicked()), this, SLOT(onImportImagesFromDisk()));

    button = new QPushButton("EXPORT");
    buttonWidget->layout()->addWidget(button);
    connect(button, SIGNAL(clicked()), this, SLOT(onExportFramesToDisk()));

    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(3);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->setStyleSheet("QTableView::item:selected { color:black; background:gray; font-weight:900; }"
                               "QTableCornerButton::section { background-color:gray; }"
                               "QHeaderView::section { color:white; background-color:#232326; }");
    tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    connect(tableWidget, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(onCellActivated(int, int, int, int)));

    QStringList headers;
    headers << "Filename" << "Frame" << "Label";
    tableWidget->setHorizontalHeaderLabels(headers);
    this->layout()->addWidget(tableWidget);

    QWidget *widget = new QWidget();
    widget->setLayout(new QHBoxLayout());
    widget->layout()->setContentsMargins(0, 0, 0, 0);
    widget->layout()->setSpacing(1);
    this->layout()->addWidget(widget);

    button = new QPushButton("YES");
    widget->layout()->addWidget(button);
    connect(button, SIGNAL(clicked()), this, SLOT(onYesButtonClicked()));

    button = new QPushButton("NO");
    widget->layout()->addWidget(button);
    connect(button, SIGNAL(clicked()), this, SLOT(onNoButtonClicked()));

    qApp->installEventFilter(this);

    paletteWidget = new LAUDepthLabelerPaletteWidget();
    paletteWidget->setWindowFlag(Qt::Tool, true);
    paletteWidget->setWindowFlag(Qt::WindowStaysOnTopHint, true);

    QSettings settings;
    QRect rect = settings.value("LAUMachineLearningVideoFrameLabelerWidget::geometry", QRect(0, 0, 0, 0)).toRect();
    if (rect.width() > 0) {
        QRect geom = this->geometry();
        geom.setX(rect.x());
        geom.setY(rect.y());
        geom.setHeight(rect.height());
        this->setGeometry(geom);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMachineLearningVideoFrameLabelerWidget::~LAUMachineLearningVideoFrameLabelerWidget()
{
    QSettings settings;
    settings.setValue("LAUMachineLearningVideoFrameLabelerWidget::geometry", this->geometry());

    while (editFlag) {
        int ret = QMessageBox::warning(nullptr, QString("LAUMachineLearningVideoFrameLabelerWidget Document"), QString("Save changes to labeler widget before closing?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (ret == QMessageBox::No) {
            break;
        }
        onSaveToDisk();
    }

    if (paletteWidget) {
        paletteWidget->deleteLater();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMachineLearningVideoFrameLabelerWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    int wdth = tableWidget->verticalHeader()->width();
    wdth += tableWidget->columnWidth(0);
    wdth += tableWidget->columnWidth(1);
    wdth += tableWidget->columnWidth(2);
    wdth += tableWidget->verticalScrollBar()->width();

    this->setFixedWidth(wdth + 15);

    if (paletteWidget && paletteWidget->isConnected()) {
        paletteWidget->show();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMachineLearningVideoFrameLabelerWidget::onKeyPress(QKeyEvent *keyEvent)
{
    static int key = Qt::Key_Space;

    if (keyEvent->key() == Qt::Key_Down) {
        int row = qMin(tableWidget->currentRow() + 1, tableWidget->rowCount() - 1);
        tableWidget->setCurrentCell(row, tableWidget->currentColumn());
        if (keyEvent->modifiers() & Qt::ShiftModifier) {
            editFlag = true;
            if (key == Qt::Key_Right) {
                tableWidget->item(row, 2)->setText("YES");
                tableWidget->item(row, 0)->setBackground(QBrush(QColor(128, 255, 128)));
                tableWidget->item(row, 1)->setBackground(QBrush(QColor(128, 255, 128)));
                tableWidget->item(row, 2)->setBackground(QBrush(QColor(128, 255, 128)));
            } else if (key == Qt::Key_Left) {
                tableWidget->item(row, 2)->setText("NO");
                tableWidget->item(row, 0)->setBackground(QBrush(QColor(255, 128, 128)));
                tableWidget->item(row, 1)->setBackground(QBrush(QColor(255, 128, 128)));
                tableWidget->item(row, 2)->setBackground(QBrush(QColor(255, 128, 128)));
            }
        }
    } else if (keyEvent->key() == Qt::Key_Up) {
        int row = qMax(tableWidget->currentRow() - 1, 0);
        tableWidget->setCurrentCell(row, tableWidget->currentColumn());
        if (keyEvent->modifiers() & Qt::ShiftModifier) {
            editFlag = true;
            if (key == Qt::Key_Right) {
                tableWidget->item(row, 2)->setText("YES");
                tableWidget->item(row, 0)->setBackground(QBrush(QColor(128, 255, 128)));
                tableWidget->item(row, 1)->setBackground(QBrush(QColor(128, 255, 128)));
                tableWidget->item(row, 2)->setBackground(QBrush(QColor(128, 255, 128)));
            } else if (key == Qt::Key_Left) {
                tableWidget->item(row, 2)->setText("NO");
                tableWidget->item(row, 0)->setBackground(QBrush(QColor(255, 128, 128)));
                tableWidget->item(row, 1)->setBackground(QBrush(QColor(255, 128, 128)));
                tableWidget->item(row, 2)->setBackground(QBrush(QColor(255, 128, 128)));
            }
        }
    } else if (keyEvent->key() == Qt::Key_Right) {
        key = Qt::Key_Right;
        int row = tableWidget->currentRow();
        if (row > -1) {
            editFlag = true;
            tableWidget->item(row, 2)->setText("YES");
            tableWidget->item(row, 0)->setBackground(QBrush(QColor(128, 255, 128)));
            tableWidget->item(row, 1)->setBackground(QBrush(QColor(128, 255, 128)));
            tableWidget->item(row, 2)->setBackground(QBrush(QColor(128, 255, 128)));
        }
    } else if (keyEvent->key() == Qt::Key_Left) {
        key = Qt::Key_Left;
        int row = tableWidget->currentRow();
        if (row > -1) {
            editFlag = true;
            tableWidget->item(row, 2)->setText("NO");
            tableWidget->item(row, 0)->setBackground(QBrush(QColor(255, 128, 128)));
            tableWidget->item(row, 1)->setBackground(QBrush(QColor(255, 128, 128)));
            tableWidget->item(row, 2)->setBackground(QBrush(QColor(255, 128, 128)));
        }
    } else if (keyEvent->key() == Qt::Key_Space) {
        key = Qt::Key_Left;
        int row = tableWidget->currentRow();
        if (row > -1) {
            editFlag = true;
            tableWidget->item(row, 2)->setText("???");
            tableWidget->item(row, 0)->setBackground(QBrush(QColor(255, 255, 255)));
            tableWidget->item(row, 1)->setBackground(QBrush(QColor(255, 255, 255)));
            tableWidget->item(row, 2)->setBackground(QBrush(QColor(255, 255, 255)));
        }
    } else if (keyEvent->modifiers() == Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_X){
            int row = tableWidget->currentRow();
            if (row > -1) {
                QString fileName = tableWidget->item(row, 0)->text();
                QFile file(fileName);
                if (file.exists()){
                    file.moveToTrash();
                }
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMachineLearningVideoFrameLabelerWidget::onYesButtonClicked()
{
    int row = tableWidget->currentRow();
    if (row > -1) {
        editFlag = true;
        tableWidget->item(row, 2)->setText("YES");
        tableWidget->item(row, 0)->setBackground(QBrush(QColor(128, 255, 128)));
        tableWidget->item(row, 1)->setBackground(QBrush(QColor(128, 255, 128)));
        tableWidget->item(row, 2)->setBackground(QBrush(QColor(128, 255, 128)));
        while (row < tableWidget->rowCount()) {
            if (tableWidget->item(row, 2)->text() == QString("???")) {
                tableWidget->setCurrentCell(row, 0);
                return;
            }
            row++;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMachineLearningVideoFrameLabelerWidget::onNoButtonClicked()
{
    int row = tableWidget->currentRow();
    if (row > -1) {
        editFlag = true;
        tableWidget->item(row, 2)->setText("NO");
        tableWidget->item(row, 0)->setBackground(QBrush(QColor(255, 128, 128)));
        tableWidget->item(row, 1)->setBackground(QBrush(QColor(255, 128, 128)));
        tableWidget->item(row, 2)->setBackground(QBrush(QColor(255, 128, 128)));
        while (row < tableWidget->rowCount()) {
            if (tableWidget->item(row, 2)->text() == QString("???")) {
                tableWidget->setCurrentCell(row, 0);
                return;
            }
            row++;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMachineLearningVideoFrameLabelerWidget::onCellActivated(int row, int col, int rowp, int colp)
{
    Q_UNUSED(col);
    Q_UNUSED(rowp);
    Q_UNUSED(colp);
    if (row > -1 && row < tableWidget->rowCount()) {
        emit emitBuffer(tableWidget->item(row, 0)->text(), tableWidget->item(row, 1)->text().toInt());
    } else {
        emit emitBuffer(QString(), -1);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMachineLearningVideoFrameLabelerWidget::onImportImagesFromDisk()
{
    QSettings settings;
    QString directory = settings.value("LAUMachineLearningVideoFrameLabelerWidget::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QStringList fileStrings = QFileDialog::getOpenFileNames(nullptr, QString("Load video files from disk (*.tif)"), directory, QString("*.tif"));
    if (fileStrings.isEmpty()) {
        return;
    }
    fileStrings.sort();
    settings.setValue("LAUMachineLearningVideoFrameLabelerWidget::lastUsedDirectory", QFileInfo(fileStrings.first()).absolutePath());

    buttonWidget->setEnabled(false);
    int numFrames = tableWidget->rowCount();
    int firstFrameToSelect = numFrames;
    for (int m = 0; m < fileStrings.count(); m++) {
        bool flag = false;
        for (int row = 0; row < tableWidget->rowCount(); row++) {
            if (fileStrings.at(m) == tableWidget->item(row, 0)->text()) {
                flag = true;
                break;
            }
        }

        if (flag == false) {
            // KEEP A LIST OF FRAMES
            QList<FramePacket> frameList;

            // OPEN INPUT TIFF FILE FROM DISK
            TIFF *inTiff = TIFFOpen(fileStrings.at(m).toLatin1(), "r");
            if (inTiff) {
                int frms = TIFFNumberOfDirectories(inTiff);
                for (int n = 1; n < frms; n++) {
                    qApp->processEvents();

                    // LOAD INPUT TIFF FILE PARAMETERS IMPORTANT TO RESAMPLING THE IMAGE
                    unsigned short uShortVariable;

                    // SET THE DIRECTORY TO THE USER SUPPLIED FRAME
                    TIFFSetDirectory(inTiff, (unsigned short)n);
                    TIFFGetField(inTiff, TIFFTAG_SAMPLESPERPIXEL, &uShortVariable);

                    // ONLY ADD FRAMES FROM FILE WITH EXACTLY ONE CHANNEL
                    if (uShortVariable == playbackDepth) {
                        FramePacket packet;

                        // SAVE THE STRING AND DIRECTORY INDEX
                        packet.frameString = fileStrings.at(m);
                        packet.directory = n;

                        // GET THE ELAPSED TIME VALUE FROM THE EXIF TAG FOR SUBSECOND TIME
                        uint64_t directoryOffset;
                        if (TIFFGetField(inTiff, TIFFTAG_EXIFIFD, &directoryOffset)) {
                            char *byteArray;
                            TIFFReadEXIFDirectory(inTiff, directoryOffset);
                            if (TIFFGetField(inTiff, EXIFTAG_SUBSECTIME, &byteArray)) {
                                packet.elapsed = QString(QByteArray(byteArray)).toUInt();
                            }
                        }

                        // SEE IF THIS IS VALID FRAME
                        if (packet.elapsed != 4294967295){
                            frameList << packet;
                            //break;
                        }
                    }
                }
                TIFFClose(inTiff);
            }

            // SORT CAMERAS IN CHRONOLOGICAL ORDER ACCORDING TO THE ELAPSED TIME
            std::sort(frameList.begin(), frameList.end(), FramePacket_LessThan);

            // INSERT EACH FRAME IN CHRONOLOGICAL ORDER
            while (frameList.isEmpty() == false){
                FramePacket packet = frameList.takeFirst();

                tableWidget->setRowCount(numFrames + 1);
                tableWidget->setItem(numFrames, 0, new QTableWidgetItem(packet.frameString));
                tableWidget->setItem(numFrames, 1, new QTableWidgetItem(QString("%1").arg(packet.directory)));
                tableWidget->setItem(numFrames, 2, new QTableWidgetItem(QString("???")));
                numFrames++;
            }
        }
    }

    if (tableWidget->rowCount() > 0) {
        tableWidget->setCurrentCell(firstFrameToSelect, 0);
        editFlag = true;
    }

    // RESIZE THE COLUMN WIDTH OF THE TABLE
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    int wdth = tableWidget->verticalHeader()->width();
    wdth += tableWidget->columnWidth(0);
    wdth += tableWidget->columnWidth(1);
    wdth += tableWidget->columnWidth(2);
    wdth += tableWidget->verticalScrollBar()->width();

    this->setFixedWidth(wdth + 15);
    buttonWidget->setEnabled(true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMachineLearningVideoFrameLabelerWidget::onLoadFromDisk()
{
    QSettings settings;
    QString directory = settings.value("LAUMachineLearningVideoFrameLabelerWidget::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QString fileString = QFileDialog::getOpenFileName(nullptr, QString("Load label file from disk (*.csv)"), directory, QString("*.csv"));
    if (fileString.isNull()) {
        return;
    }
    settings.setValue("LAUMachineLearningVideoFrameLabelerWidget::lastUsedDirectory", QFileInfo(fileString).absolutePath());

    // CLEAR THE TABLE
    tableWidget->clear();

    int numFrames = tableWidget->rowCount();
    int firstFrameToSelect = numFrames;

    QFile file(fileString);
    if (file.open(QIODevice::ReadOnly)) {
        while (file.atEnd() == false) {
            QStringList stringList = QString(file.readLine()).split(",");
            if (stringList.count() >= 3) {
                tableWidget->setRowCount(numFrames + 1);
                tableWidget->setItem(numFrames, 0, new QTableWidgetItem(stringList.at(0).simplified()));
                tableWidget->setItem(numFrames, 1, new QTableWidgetItem(stringList.at(1).simplified()));
                tableWidget->setItem(numFrames, 2, new QTableWidgetItem(stringList.at(2).simplified()));
                if (stringList.at(2).contains(QString("YES"))) {
                    tableWidget->item(numFrames, 0)->setBackground(QBrush(QColor(128, 255, 128)));
                    tableWidget->item(numFrames, 1)->setBackground(QBrush(QColor(128, 255, 128)));
                    tableWidget->item(numFrames, 2)->setBackground(QBrush(QColor(128, 255, 128)));
                } else if (stringList.at(2).contains(QString("NO"))) {
                    tableWidget->item(numFrames, 0)->setBackground(QBrush(QColor(255, 128, 128)));
                    tableWidget->item(numFrames, 1)->setBackground(QBrush(QColor(255, 128, 128)));
                    tableWidget->item(numFrames, 2)->setBackground(QBrush(QColor(255, 128, 128)));
                }
                numFrames++;
            }
        }
        file.close();
    }

    if (firstFrameToSelect > 0) {
        editFlag = true;
    }

    if (tableWidget->rowCount() > 0) {
        tableWidget->setCurrentCell(firstFrameToSelect, 0);
    }

    // RESIZE THE COLUMN WIDTH OF THE TABLE
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    int wdth = tableWidget->verticalHeader()->width();
    wdth += tableWidget->columnWidth(0);
    wdth += tableWidget->columnWidth(1);
    wdth += tableWidget->columnWidth(2);
    wdth += tableWidget->verticalScrollBar()->width();

    this->setFixedWidth(wdth + 15);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMachineLearningVideoFrameLabelerWidget::onSaveToDisk()
{
    QSettings settings;
    QString directory = settings.value("LAUMachineLearningVideoFrameLabelerWidget::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QString fileString = QFileDialog::getSaveFileName(nullptr, QString("Save label file to disk (*.csv)"), directory, QString("*.csv"));
    if (fileString.isNull()) {
        return;
    }
    settings.setValue("LAUMachineLearningVideoFrameLabelerWidget::lastUsedDirectory", QFileInfo(fileString).absolutePath());

    QFile file(fileString);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream textStream(&file);
        for (int row = 0; row < tableWidget->rowCount(); row++) {
            textStream << tableWidget->item(row, 0)->text() << ",";
            textStream << tableWidget->item(row, 1)->text() << ",";
            textStream << tableWidget->item(row, 2)->text() << "\n";
        }
        file.close();
        editFlag = false;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMachineLearningVideoFrameLabelerWidget::onExportFramesToDisk()
{
    if (tableWidget->rowCount() == 0) {
        return;
    }

    QSettings settings;
    QString directory = settings.value("LAUMachineLearningVideoFrameLabelerWidget::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QString dirString = QFileDialog::getExistingDirectory(nullptr, QString("Locate folder to save output..."), directory);
    if (dirString.isNull()) {
        return;
    }
    settings.setValue("LAUMachineLearningVideoFrameLabelerWidget::lastUsedDirectory", dirString);

    // CREATE YES AND NO FOLDERS TO EXPORT OUTPUT
    QString ysString = QString("%1/YES").arg(dirString);
    if (QDir(ysString).exists() == false) {
        QDir(dirString).mkdir(QString("YES"));
    }

    QString noString = QString("%1/NO").arg(dirString);
    if (QDir(noString).exists() == false) {
        QDir(dirString).mkdir(QString("NO"));
    }

    // KEEP A RUNNING COUNTER OF YES AND NO VIDEO FRAMES
    int ysCounter = 0;
    int noCounter = 0;

    // ITERATE THROUGH FRAME LIST
    for (int row = 0; row < tableWidget->rowCount(); row++) {
        // CHECK IF THE LABEL IS YES OR NO
        if (tableWidget->item(row, 2)->text() == QString("YES")) {
            // LOAD THE VIDEO FRAME FROM DISK
            LAUMemoryObject object(tableWidget->item(row, 0)->text(), tableWidget->item(row, 1)->text().toInt());

            // CREATE A FILENAME THAT INCLUDES THE RUNNING COUNTER
            QString frameString = QString("%1").arg(ysCounter++);
            while (frameString.length() < 4) {
                frameString.prepend("0");
            }
            frameString.prepend(QString("%1/frame").arg(ysString));
            frameString.append(".tif");

            // EXPORT THE VIDEO FRAME TO ITS OWN FILE IN THE YES DIRECTORY
            object.save(frameString);
        } else if (tableWidget->item(row, 2)->text() == QString("NO")) {
            // LOAD THE VIDEO FRAME FROM DISK
            LAUMemoryObject object(tableWidget->item(row, 0)->text(), tableWidget->item(row, 1)->text().toInt());

            // CREATE A FILENAME THAT INCLUDES THE RUNNING COUNTER
            QString frameString = QString("%1").arg(noCounter++);
            while (frameString.length() < 4) {
                frameString.prepend("0");
            }
            frameString.prepend(QString("%1/frame").arg(noString));
            frameString.append(".tif");

            // EXPORT THE VIDEO FRAME TO ITS OWN FILE IN THE YES DIRECTORY
            object.save(frameString);
        }
    }
}
