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

#include "laumergedocumentswidget.h"

#include <QLabel>
#include <QSettings>
#include <QLineEdit>
#include <QGridLayout>
#include <QStandardPaths>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMergeColorsWidget::LAUMergeColorsWidget(LAUVideoPlaybackColor mstColor, LAUVideoPlaybackColor slvColor, QWidget *parent) : QWidget(parent), masterColor(mstColor), slaveColor(slvColor)
{
    this->setWindowTitle("Merge Colors Combo Box");
    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->layout()->setSpacing(6);

    QStringList strings;
    strings << "Gray";
    strings << "RGB";
    strings << "RGBA";
    strings << "XYZ";
    strings << "XYZW";
    strings << "XYZG";
    strings << "XYZRGB";
    strings << "XYZWRGBA";

    QGroupBox *box = new QGroupBox("Output Color Space:");
    box->setLayout(new QVBoxLayout());
    box->layout()->setContentsMargins(0, 0, 0, 0);
    box->layout()->setSpacing(6);
    this->layout()->addWidget(box);

    outputColorComboBox = new QComboBox();
    outputColorComboBox->addItems(strings);
    outputColorComboBox->setCurrentIndex(strings.count() - 1);
    connect(outputColorComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onOutputColorComboBoxChanged(int)));
    box->layout()->addWidget(outputColorComboBox);

    strings.clear();
    if (masterColor == ColorGray) {
        strings << "Master Gray";
    } else if (masterColor == ColorRGB) {
        strings << "Master Red";
        strings << "Master Green";
        strings << "Master Blue";
    } else if (masterColor == ColorRGBA) {
        strings << "Master Red";
        strings << "Master Green";
        strings << "Master Blue";
        strings << "Master Alpha";
    } else if (masterColor == ColorXYZ) {
        strings << "Master X";
        strings << "Master Y";
        strings << "Master Z";
    } else if (masterColor == ColorXYZW) {
        strings << "Master X";
        strings << "Master Y";
        strings << "Master Z";
        strings << "Master W";
    } else if (masterColor == ColorXYZG) {
        strings << "Master X";
        strings << "Master Y";
        strings << "Master Z";
        strings << "Master Gray";
    } else if (masterColor == ColorXYZRGB) {
        strings << "Master X";
        strings << "Master Y";
        strings << "Master Z";
        strings << "Master Red";
        strings << "Master Green";
        strings << "Master Blue";
    } else if (masterColor == ColorXYZWRGBA) {
        strings << "Master X";
        strings << "Master Y";
        strings << "Master Z";
        strings << "Master W";
        strings << "Master Red";
        strings << "Master Green";
        strings << "Master Blue";
        strings << "Master Alpha";
    }

    if (slaveColor == ColorGray) {
        strings << "Slave Gray";
    } else if (slaveColor == ColorRGB) {
        strings << "Slave Red";
        strings << "Slave Green";
        strings << "Slave Blue";
    } else if (slaveColor == ColorRGBA) {
        strings << "Slave Red";
        strings << "Slave Green";
        strings << "Slave Blue";
        strings << "Slave Alpha";
    } else if (slaveColor == ColorXYZ) {
        strings << "Slave X";
        strings << "Slave Y";
        strings << "Slave Z";
    } else if (slaveColor == ColorXYZW) {
        strings << "Slave X";
        strings << "Slave Y";
        strings << "Slave Z";
        strings << "Slave W";
    } else if (slaveColor == ColorXYZG) {
        strings << "Slave X";
        strings << "Slave Y";
        strings << "Slave Z";
        strings << "Slave Gray";
    } else if (slaveColor == ColorXYZRGB) {
        strings << "Slave X";
        strings << "Slave Y";
        strings << "Slave Z";
        strings << "Slave Red";
        strings << "Slave Green";
        strings << "Slave Blue";
    } else if (slaveColor == ColorXYZWRGBA) {
        strings << "Slave X";
        strings << "Slave Y";
        strings << "Slave Z";
        strings << "Slave W";
        strings << "Slave Red";
        strings << "Slave Green";
        strings << "Slave Blue";
        strings << "Slave Alpha";
    }
    strings << "All Ones";
    strings << "All Zeros";
    strings << "All NaNs";

    aChannelComboBox = new QComboBox();
    aChannelComboBox->addItems(strings);
    if (strings.contains(QString("Master X"))) {
        aChannelComboBox->setCurrentText(QString("Master X"));
    } else if (strings.contains(QString("Slave X"))) {
        aChannelComboBox->setCurrentText(QString("Slave X"));
    }
    aChannelComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    bChannelComboBox = new QComboBox();
    bChannelComboBox->addItems(strings);
    if (strings.contains(QString("Master Y"))) {
        bChannelComboBox->setCurrentText(QString("Master Y"));
    } else if (strings.contains(QString("Slave Y"))) {
        bChannelComboBox->setCurrentText(QString("Slave Y"));
    }
    bChannelComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    cChannelComboBox = new QComboBox();
    cChannelComboBox->addItems(strings);
    if (strings.contains(QString("Master Z"))) {
        cChannelComboBox->setCurrentText(QString("Master Z"));
    } else if (strings.contains(QString("Slave Z"))) {
        cChannelComboBox->setCurrentText(QString("Slave Z"));
    }
    cChannelComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    dChannelComboBox = new QComboBox();
    dChannelComboBox->addItems(strings);
    if (strings.contains(QString("Master W"))) {
        dChannelComboBox->setCurrentText(QString("Master W"));
    } else if (strings.contains(QString("Slave W"))) {
        dChannelComboBox->setCurrentText(QString("Slave W"));
    }
    dChannelComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    eChannelComboBox = new QComboBox();
    eChannelComboBox->addItems(strings);
    if (strings.contains(QString("Master Red"))) {
        eChannelComboBox->setCurrentText(QString("Master Red"));
    } else if (strings.contains(QString("Slave Red"))) {
        eChannelComboBox->setCurrentText(QString("Slave Red"));
    } else if (strings.contains(QString("Master Gray"))) {
        eChannelComboBox->setCurrentText(QString("Master Gray"));
    } else if (strings.contains(QString("Slave Gray"))) {
        eChannelComboBox->setCurrentText(QString("Slave Gray"));
    }
    eChannelComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    fChannelComboBox = new QComboBox();
    fChannelComboBox->addItems(strings);
    if (strings.contains(QString("Master Green"))) {
        fChannelComboBox->setCurrentText(QString("Master Green"));
    } else if (strings.contains(QString("Slave Green"))) {
        fChannelComboBox->setCurrentText(QString("Slave Green"));
    } else if (strings.contains(QString("Master Gray"))) {
        fChannelComboBox->setCurrentText(QString("Master Gray"));
    } else if (strings.contains(QString("Slave Gray"))) {
        fChannelComboBox->setCurrentText(QString("Slave Gray"));
    }
    fChannelComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    gChannelComboBox = new QComboBox();
    gChannelComboBox->addItems(strings);
    if (strings.contains(QString("Master Blue"))) {
        gChannelComboBox->setCurrentText(QString("Master Blue"));
    } else if (strings.contains(QString("Slave Blue"))) {
        gChannelComboBox->setCurrentText(QString("Slave Blue"));
    } else if (strings.contains(QString("Master Gray"))) {
        gChannelComboBox->setCurrentText(QString("Master Gray"));
    } else if (strings.contains(QString("Slave Gray"))) {
        gChannelComboBox->setCurrentText(QString("Slave Gray"));
    }
    gChannelComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    hChannelComboBox = new QComboBox();
    hChannelComboBox->addItems(strings);
    if (strings.contains(QString("Master Alpha"))) {
        hChannelComboBox->setCurrentText(QString("Master Alpha"));
    } else if (strings.contains(QString("Slave Alpha"))) {
        hChannelComboBox->setCurrentText(QString("Slave Alpha"));
    } else if (strings.contains(QString("Master Gray"))) {
        hChannelComboBox->setCurrentText(QString("Master Gray"));
    } else if (strings.contains(QString("Slave Gray"))) {
        hChannelComboBox->setCurrentText(QString("Slave Gray"));
    }
    hChannelComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    box = new QGroupBox("Output Channel Assignments:");
    box->setLayout(new QFormLayout());
    box->layout()->setContentsMargins(0, 0, 0, 0);
    box->layout()->setSpacing(6);
    this->layout()->addWidget(box);

    ((QFormLayout *)(box->layout()))->addRow(QString("X:"), aChannelComboBox);
    ((QFormLayout *)(box->layout()))->addRow(QString("Y:"), bChannelComboBox);
    ((QFormLayout *)(box->layout()))->addRow(QString("Z:"), cChannelComboBox);
    ((QFormLayout *)(box->layout()))->addRow(QString("W:"), dChannelComboBox);
    ((QFormLayout *)(box->layout()))->addRow(QString("R:"), eChannelComboBox);
    ((QFormLayout *)(box->layout()))->addRow(QString("G:"), fChannelComboBox);
    ((QFormLayout *)(box->layout()))->addRow(QString("B:"), gChannelComboBox);
    ((QFormLayout *)(box->layout()))->addRow(QString("A:"), hChannelComboBox);

    onOutputColorComboBoxChanged(outputColorComboBox->currentIndex());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeColorsWidget::onOutputColorComboBoxChanged(int index)
{
    aChannelComboBox->setEnabled(false);
    bChannelComboBox->setEnabled(false);
    cChannelComboBox->setEnabled(false);
    dChannelComboBox->setEnabled(false);
    eChannelComboBox->setEnabled(false);
    fChannelComboBox->setEnabled(false);
    gChannelComboBox->setEnabled(false);
    hChannelComboBox->setEnabled(false);

    switch (index) {
        case 0:
            fChannelComboBox->setEnabled(true);
            break;
        case 1:
            eChannelComboBox->setEnabled(true);
            fChannelComboBox->setEnabled(true);
            gChannelComboBox->setEnabled(true);
            break;
        case 2:
            eChannelComboBox->setEnabled(true);
            fChannelComboBox->setEnabled(true);
            gChannelComboBox->setEnabled(true);
            hChannelComboBox->setEnabled(true);
            break;
        case 3:
            aChannelComboBox->setEnabled(true);
            bChannelComboBox->setEnabled(true);
            cChannelComboBox->setEnabled(true);
            break;
        case 4:
            aChannelComboBox->setEnabled(true);
            bChannelComboBox->setEnabled(true);
            cChannelComboBox->setEnabled(true);
            dChannelComboBox->setEnabled(true);
            break;
        case 5:
            aChannelComboBox->setEnabled(true);
            bChannelComboBox->setEnabled(true);
            cChannelComboBox->setEnabled(true);
            fChannelComboBox->setEnabled(true);
            break;
        case 6:
            aChannelComboBox->setEnabled(true);
            bChannelComboBox->setEnabled(true);
            cChannelComboBox->setEnabled(true);
            eChannelComboBox->setEnabled(true);
            fChannelComboBox->setEnabled(true);
            gChannelComboBox->setEnabled(true);
            break;
        case 7:
            aChannelComboBox->setEnabled(true);
            bChannelComboBox->setEnabled(true);
            cChannelComboBox->setEnabled(true);
            dChannelComboBox->setEnabled(true);
            eChannelComboBox->setEnabled(true);
            fChannelComboBox->setEnabled(true);
            gChannelComboBox->setEnabled(true);
            hChannelComboBox->setEnabled(true);
            break;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUMergeColorsWidget::outputStrings()
{
    QStringList strings;

    strings << outputColorComboBox->currentText();
    switch (outputColorComboBox->currentIndex()) {
        case 0:
            strings << fChannelComboBox->currentText();
            break;
        case 1:
            strings << eChannelComboBox->currentText();
            strings << fChannelComboBox->currentText();
            strings << gChannelComboBox->currentText();
            break;
        case 2:
            strings << eChannelComboBox->currentText();
            strings << fChannelComboBox->currentText();
            strings << gChannelComboBox->currentText();
            strings << hChannelComboBox->currentText();
            break;
        case 3:
            strings << aChannelComboBox->currentText();
            strings << bChannelComboBox->currentText();
            strings << cChannelComboBox->currentText();
            break;
        case 4:
            strings << aChannelComboBox->currentText();
            strings << bChannelComboBox->currentText();
            strings << cChannelComboBox->currentText();
            strings << dChannelComboBox->currentText();
            break;
        case 5:
            strings << aChannelComboBox->currentText();
            strings << bChannelComboBox->currentText();
            strings << cChannelComboBox->currentText();
            strings << fChannelComboBox->currentText();
            break;
        case 6:
            strings << aChannelComboBox->currentText();
            strings << bChannelComboBox->currentText();
            strings << cChannelComboBox->currentText();
            strings << eChannelComboBox->currentText();
            strings << fChannelComboBox->currentText();
            strings << gChannelComboBox->currentText();
            break;
        case 7:
            strings << aChannelComboBox->currentText();
            strings << bChannelComboBox->currentText();
            strings << cChannelComboBox->currentText();
            strings << dChannelComboBox->currentText();
            strings << eChannelComboBox->currentText();
            strings << fChannelComboBox->currentText();
            strings << gChannelComboBox->currentText();
            strings << hChannelComboBox->currentText();
            break;
    }
    return (strings);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMergeDocumentsWidget::LAUMergeDocumentsWidget(LAUDocument mstDoc, LAUDocument slvDoc, QWidget *parent) : QWidget(parent), mstDocument(mstDoc), slvDocument(slvDoc)
{
    mstDocument.makeClean();
    slvDocument.makeClean();

    this->setWindowTitle("Rasterize Scan Tool");
    this->setLayout(new QVBoxLayout());
    this->setMinimumWidth(600);
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->layout()->setSpacing(6);

    QGroupBox *box = new QGroupBox("Files:");
    box->setLayout(new QGridLayout());
    box->layout()->setContentsMargins(6, 6, 6, 6);
    box->layout()->setSpacing(6);

    QPushButton *button = new QPushButton("Set Master:");
    connect(button, SIGNAL(clicked()), this, SLOT(onSetMasterScan()));
    ((QGridLayout *)(box->layout()))->addWidget(button, 0, 0);

    masterLineEdit = new QLineEdit();
    masterLineEdit->setReadOnly(true);
    if (mstDocument.isValid()) {
        masterLineEdit->setText(mstDocument.filename());
    }
    ((QGridLayout *)(box->layout()))->addWidget(masterLineEdit, 0, 1);

    button = new QPushButton("Set Slave:");
    connect(button, SIGNAL(clicked()), this, SLOT(onSetSlaveScan()));
    ((QGridLayout *)(box->layout()))->addWidget(button, 1, 0);

    slaveLineEdit = new QLineEdit();
    slaveLineEdit->setReadOnly(true);
    if (slvDocument.isValid()) {
        slaveLineEdit->setText(slvDocument.filename());
    }
    ((QGridLayout *)(box->layout()))->addWidget(slaveLineEdit, 1, 1);

    this->layout()->addWidget(box);

    box = new QGroupBox("Raster Settings:");
    box->setVisible(false);
    box->setLayout(new QGridLayout());
    box->layout()->setContentsMargins(6, 6, 6, 6);
    box->layout()->setSpacing(6);

    minXSpinBox = new QDoubleSpinBox();
    minXSpinBox->setMinimum(-100.0);
    minXSpinBox->setMaximum(100.0);
    minXSpinBox->setSingleStep(1.0);
    ((QGridLayout *)(box->layout()))->addWidget(new QLabel("Min X:"), 0, 0);
    ((QGridLayout *)(box->layout()))->addWidget(minXSpinBox, 0, 1);

    maxXSpinBox = new QDoubleSpinBox();
    maxXSpinBox->setMinimum(-100.0);
    maxXSpinBox->setMaximum(100.0);
    maxXSpinBox->setSingleStep(1.0);
    ((QGridLayout *)(box->layout()))->addWidget(new QLabel("Max X:"), 1, 0);
    ((QGridLayout *)(box->layout()))->addWidget(maxXSpinBox, 1, 1);

    minYSpinBox = new QDoubleSpinBox();
    minYSpinBox->setMinimum(-100.0);
    minYSpinBox->setMaximum(100.0);
    minYSpinBox->setSingleStep(1.0);
    ((QGridLayout *)(box->layout()))->addWidget(new QLabel("Min Y:"), 0, 2);
    ((QGridLayout *)(box->layout()))->addWidget(minYSpinBox, 0, 3);

    maxYSpinBox = new QDoubleSpinBox();
    maxYSpinBox->setMinimum(-100.0);
    maxYSpinBox->setMaximum(100.0);
    maxYSpinBox->setSingleStep(1.0);
    ((QGridLayout *)(box->layout()))->addWidget(new QLabel("Max Y:"), 1, 2);
    ((QGridLayout *)(box->layout()))->addWidget(maxYSpinBox, 1, 3);

    dpiSpinBox = new QDoubleSpinBox();
    dpiSpinBox->setMinimum(100.0);
    dpiSpinBox->setMaximum(1200.0);
    dpiSpinBox->setSingleStep(100.0);
    ((QGridLayout *)(box->layout()))->addWidget(new QLabel("dpi:"), 2, 0);
    ((QGridLayout *)(box->layout()))->addWidget(dpiSpinBox, 2, 1);

    ((QLabel *)(((QGridLayout *)(box->layout()))->itemAtPosition(0, 0)->widget()))->setAlignment(Qt::AlignRight);
    ((QLabel *)(((QGridLayout *)(box->layout()))->itemAtPosition(1, 0)->widget()))->setAlignment(Qt::AlignRight);
    ((QLabel *)(((QGridLayout *)(box->layout()))->itemAtPosition(0, 2)->widget()))->setAlignment(Qt::AlignRight);
    ((QLabel *)(((QGridLayout *)(box->layout()))->itemAtPosition(1, 2)->widget()))->setAlignment(Qt::AlignRight);
    ((QLabel *)(((QGridLayout *)(box->layout()))->itemAtPosition(2, 0)->widget()))->setAlignment(Qt::AlignRight);

    this->layout()->addWidget(box);

    // UPDATE XY LIMITS FOR RASTERIZING BASED ON MASTER SCAN
    updateXYLimits();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeDocumentsWidget::onSetMasterScan()
{
    QSettings settings;
    QString directory = settings.value("LAUMergeScansWidget::onSetMasterScan", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QString filename = QFileDialog::getOpenFileName(this, QString("Open image from disk (*.lau)"), directory, QString("*.lau"));
    if (filename.isEmpty() == true) {
        return;
    }

    if (slvDocument.isValid()){
        if (slvDocument.images().count() != LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(filename)){
            QMessageBox::warning(this, QString("Merge Scans"), QString("Master and Slave documents must have the same number of scans."));
            return;
        } else if (slvDocument.images().first().width() != LAUMemoryObject::howManyColumnsDoesThisTiffFileHave(filename)){
            QMessageBox::warning(this, QString("Merge Scans"), QString("Master and Slave documents must have the same width."));
            return;
        } else if (slvDocument.images().first().height() != LAUMemoryObject::howManyRowsDoesThisTiffFileHave(filename)){
            QMessageBox::warning(this, QString("Merge Scans"), QString("Master and Slave documents must have the same height."));
            return;
        }
    }

    LAUDocument document = LAUDocument(filename, this);
    if (document.isValid()) {
        mstDocument = document;
        settings.setValue("LAUMergeScansWidget::onSetMasterScan", QFileInfo(filename).absolutePath());
        masterLineEdit->setText(filename);

        // UPDATE XY LIMITS FOR RASTERIZING BASED ON MASTER SCAN
        updateXYLimits();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeDocumentsWidget::onSetSlaveScan()
{
    QSettings settings;
    QString directory = settings.value("LAUMergeScansWidget::onSetSlaveScan", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QString filename = QFileDialog::getOpenFileName(this, QString("Open image from disk (*.lau)"), directory, QString("*.lau"));
    if (filename.isEmpty()) {
        return;
    }

    if (mstDocument.isValid()){
        if (mstDocument.images().count() != LAUMemoryObject::howManyDirectoriesDoesThisTiffFileHave(filename)){
            QMessageBox::warning(this, QString("Merge Scans"), QString("Master and Slave documents must have the same number of scans."));
            return;
        } else if (mstDocument.images().first().width() != LAUMemoryObject::howManyColumnsDoesThisTiffFileHave(filename)){
            QMessageBox::warning(this, QString("Merge Scans"), QString("Master and Slave documents must have the same width."));
            return;
        } else if (mstDocument.images().first().height() != LAUMemoryObject::howManyRowsDoesThisTiffFileHave(filename)){
            QMessageBox::warning(this, QString("Merge Scans"), QString("Master and Slave documents must have the same height."));
            return;
        }
    }

    LAUDocument document = LAUDocument(filename, this);
    if (document.isValid()) {
        slvDocument = document;
        settings.setValue("LAUMergeScansWidget::onSetSlaveScan", QFileInfo(filename).absolutePath());
        slaveLineEdit->setText(filename);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMergeDocumentsWidget::updateXYLimits()
{
    // INITIALIZE PRIVATE VARIABLES
    if (mstDocument.isValid()) {
        float xMin = +1e6f;
        float xMax = -1e6f;
        float yMin = +1e6f;
        float yMax = -1e6f;
        for (int n = 0; n < mstDocument.images().count(); n++) {
            xMin = qMin(xMin, mstDocument.images().at(n).minX());
            xMax = qMax(xMax, mstDocument.images().at(n).maxX());
            yMin = qMin(yMin, mstDocument.images().at(n).minY());
            yMax = qMax(yMax, mstDocument.images().at(n).maxY());
        }
        minXSpinBox->setValue(xMin);
        maxXSpinBox->setValue(xMax);
        minYSpinBox->setValue(yMin);
        maxYSpinBox->setValue(yMax);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUMergeDocumentsWidget::readyToMerge(QString *string)
{
    if (mstDocument.isValid() == false) {
        if (string) {
            *string = QString("Master document not valid.");
        }
        return (false);
    }

    LAUVideoPlaybackColor mstColor = mstDocument.images().first().color();
    if (mstColor == ColorGray || mstColor == ColorRGB || mstColor == ColorRGBA) {
        if (string) {
            *string = QString("Master document must contain XYZ.");
        }
        return (false);
    }

    if (slvDocument.isValid() == false) {
        if (string) {
            *string = QString("Slave document not valid.");
        }
        return (false);
    }

    LAUVideoPlaybackColor slvColor = slvDocument.images().first().color();
    if (slvColor == ColorXYZ || mstColor == ColorXYZW) {
        if (string) {
            *string = QString("Slave document must contain RGB or Gray.");
        }
        return (false);
    }

    if (mstDocument.images().count() != slvDocument.images().count()) {
        QMessageBox::warning(this, QString("Merge Scans"), QString("Master and Slave documents must have the same number of scans."));
        return (false);
    }

    if (mstDocument.images().first().width() != slvDocument.images().first().width()) {
        QMessageBox::warning(this, QString("Merge Scans"), QString("Master and Slave documents must have the same width."));
        return (false);
    }

    if (mstDocument.images().first().height() != slvDocument.images().first().height()) {
        QMessageBox::warning(this, QString("Merge Scans"), QString("Master and Slave documents must have the same height."));
        return (false);
    }

    return (true);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUDocument LAUMergeDocumentsWidget::mergeResult()
{
    // MAKE SURE MASTER AND SLAVE DOCUMENTS ARE PROPERLY ASSIGNED
    if (readyToMerge() == false) {
        return (LAUDocument());
    }

    // TEST THE SLAVE COLOR TO SEE IF WE CAN OUTPUT XYZG OR XYZRGB
    LAUVideoPlaybackColor slvColor = slvDocument.images().first().color();
    Q_UNUSED(slvColor);

    // SET THE CHANNEL STRINGS AS NEEDED BASED ON OUTPUT COLOR SPACE
    QStringList channelStrings;

    // ASK THE USER TO SPECIFY THE OUTPUT COLOR AND MAP THE MASTER AND SLAVE CHANNELS TO IT
    LAUMergeColorsDialog d(mstDocument.color(), slvDocument.color());
    if (d.exec() == QDialog::Accepted) {
        channelStrings = d.outputStrings();
    }

    // MAKE SURE USER SELECTED STRINGS FROM THE CHANNEL SELECTION DIALOG
    if (channelStrings.count() < 2) {
        return (LAUDocument());
    }

    // GET THE SIZE PARAMETERS FOR OUR OUTPUT DOCUMENT
    unsigned int masterWidth = mstDocument.images().first().width();
    unsigned int masterHeight = mstDocument.images().first().height();

    // CREATE A MEMORY OBJECT TO BUFFER THE CURRENT MERGED SCAN
    // WHILE MAKING SURE WE HAVE THE RIGHT NOW OF CHANNEL STRINGS
    LAUScan image;
    if (channelStrings.first() == QString("Gray")) {
        image = LAUScan(masterWidth, masterHeight, ColorGray);
    } else if (channelStrings.first() == QString("RGB")) {
        image = LAUScan(masterWidth, masterHeight, ColorRGB);
    } else if (channelStrings.first() == QString("RGBA")) {
        image = LAUScan(masterWidth, masterHeight, ColorRGBA);
    } else if (channelStrings.first() == QString("XYZ")) {
        image = LAUScan(masterWidth, masterHeight, ColorXYZ);
    } else if (channelStrings.first() == QString("XYZW")) {
        image = LAUScan(masterWidth, masterHeight, ColorXYZW);
    } else if (channelStrings.first() == QString("XYZG")) {
        image = LAUScan(masterWidth, masterHeight, ColorXYZG);
    } else if (channelStrings.first() == QString("XYZRGB")) {
        image = LAUScan(masterWidth, masterHeight, ColorXYZRGB);
    } else if (channelStrings.first() == QString("XYZWRGBA")) {
        image = LAUScan(masterWidth, masterHeight, ColorXYZWRGBA);
    }

    // CREATE A PROGRESS DIALOG SO USER CAN ABORT
    QProgressDialog progressDialog(QString("Merging scans..."), QString("Abort"), 0, mstDocument.images().count(), this, Qt::Sheet);
    progressDialog.setModal(Qt::WindowModal);
    progressDialog.show();

    // CREATE A DOCUMENT TO HOLD THE OUTPUT SCANS
    LAUDocument newDocument;

    // ITERATE THROUGH EACH SCAN ONE BY ONE
    for (int ind = 0; ind < mstDocument.images().count(); ind++) {
        if (progressDialog.wasCanceled()) {
            progressDialog.setValue(progressDialog.maximum());
            return (newDocument);
        }
        progressDialog.setValue(ind);
        qApp->processEvents();

        // LOAD SCANS FROM THE MASTER AND SLAVE DOCUMENTS FROM DISK
        LAUScan mstScan = mstDocument.images().at(ind);
        LAUScan slvScan = slvDocument.images().at(ind);

        // COPY TIFFTAGS OVER FROM MASTER SCAN
        image = image + mstScan;

        // ITERATE THROUGH EVERY CHANNEL ONE BY ONE
        for (int s = 1; s < channelStrings.count(); s++) {
            QString string = channelStrings.at(s);
            if (string.contains("All")) {
                // GET COORDINATES TO INPUT BUFFER FOR CURRENT COLOR
                int toChannels = image.colors();
                int toColor = s - 1;

                // SET PIXELS OVER ONE CHANNEL AT A TIME
                float pixel = 0.0f;
                if (string.contains("Zeros")) {
                    pixel = 0.0f;
                } else if (string.contains("Ones")) {
                    pixel = 1.0f;
                } else if (string.contains("NaNs")) {
                    pixel = qQNaN();
                }

                for (unsigned int row = 0; row < image.height(); row++) {
                    float *toBuffer = (float *)image.scanLine(row);
                    for (unsigned int col = 0; col < image.width(); col++) {
                        toBuffer[toChannels * col + toColor] = pixel;
                    }
                }
            } else {
                LAUScan fmScan = (string.contains("Master")) ? mstScan : slvScan;

                int toChannels = image.colors();
                int fmChannels = fmScan.colors();
                int toColor = s - 1;
                int fmColor = 0;

                if (string.contains("Gray")) {
                    fmColor = 0;
                } else if (string.contains("X")) {
                    fmColor = 0;
                } else if (string.contains("Y")) {
                    fmColor = 1;
                } else if (string.contains("Z")) {
                    fmColor = 2;
                } else if (string.contains("W")) {
                    fmColor = 3;
                } else if (string.contains("Red")) {
                    if (fmScan.colors() == 3) {
                        fmColor = 0;
                    } else if (fmScan.colors() == 4) {
                        fmColor = 0;
                    } else if (fmScan.colors() == 6) {
                        fmColor = 3;
                    } else if (fmScan.colors() == 8) {
                        fmColor = 4;
                    }
                } else if (string.contains("Green")) {
                    if (fmScan.colors() == 3) {
                        fmColor = 1;
                    } else if (fmScan.colors() == 4) {
                        fmColor = 1;
                    } else if (fmScan.colors() == 6) {
                        fmColor = 4;
                    } else if (fmScan.colors() == 8) {
                        fmColor = 5;
                    }
                } else if (string.contains("Blue")) {
                    if (fmScan.colors() == 3) {
                        fmColor = 2;
                    } else if (fmScan.colors() == 4) {
                        fmColor = 2;
                    } else if (fmScan.colors() == 6) {
                        fmColor = 5;
                    } else if (fmScan.colors() == 8) {
                        fmColor = 6;
                    }
                } else if (string.contains("Alpha")) {
                    if (fmScan.colors() == 4) {
                        fmColor = 3;
                    } else if (fmScan.colors() == 8) {
                        fmColor = 7;
                    }
                }

                // COPY PIXELS OVER ONE CHANNEL AT A TIME
                for (unsigned int row = 0; row < image.height(); row++) {
                    float *toBuffer = (float *)image.scanLine(row);
                    float *fmBuffer = (float *)fmScan.constScanLine(row);
                    for (unsigned int col = 0; col < image.width(); col++) {
                        toBuffer[toChannels * col + toColor] = fmBuffer[fmChannels * col + fmColor];
                    }
                }
            }
        }
        // ADD THE NEW SCAN TO OUR DOCUMENT
        image.updateLimits();
        newDocument.insertImage(image);
    }
    newDocument.makeClean();
    progressDialog.setValue(progressDialog.maximum());

    // GIVE THE RESULTING DOCUMENT BACK TO THE USER
    return (newDocument);
}
