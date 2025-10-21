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

#include "laudocument.h"

#include <QSettings>
#include <QStandardPaths>

using namespace libtiff;

int LAUDocument::allDocumentCounter = 0;
int LAUDocument::untitledDocumentCounter = 0;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUDocument::LAUDocument(QString filename, QWidget *widget) : fileString(filename), widgetInterface(widget)
{
    allDocumentCounter++;
    if (fileString.isEmpty() || QFileInfo(fileString).exists() == false) {
        fileString = QString("Untitled%1").arg(++untitledDocumentCounter);
        editFlag = false;
    } else {
        editFlag = true;
        loadFromDisk(fileString);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUDocument::~LAUDocument()
{
    allDocumentCounter--;
    while (editFlag) {
        int ret = QMessageBox::warning(nullptr, QString("LAU3DVideoRecorder Document"), QString("Save changes to the LAU3DVideoRecorder Document \"%1\" before closing?").arg(fileString), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (ret == QMessageBox::No) {
            break;
        }
        save();
    }
    editFlag = false;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocument::sendToCloud()
{
    // MAKE SURE WE HAVE SCANS TO SEND
    if (this->parentStringList().isEmpty()) {
        QMessageBox::warning(nullptr, QString("LAU3DVideoRecorder Document"), QString("Document is empty. Please add scans and trying again."));
        return;
    }

    // MAKE SURE THE VERSION OF THIS DOCUMENT ON DISK IS UP TO DATE
    if (isDirty()) {
        QMessageBox::warning(nullptr, QString("LAU3DVideoRecorder Document"), QString("Document has been modified from version on disk. Please save to disk and trying again."));
        return;
    }

#ifdef AZUREIOT
    // PASS THE FILENAME FROM DISK TO AN AZURE IOT DIALOG FOR SENDING
    LAUAzureIOTDialog(filename()).exec();
#endif
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocument::loadFromDisk(QString filename)
{
    if (filename.isEmpty()) {
        QSettings settings;
        QString directory = settings.value("LAUScan::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        filename = QFileDialog::getOpenFileName(0, QString("Open image from disk (*.lau)"), directory, QString("*.lau"));
        if (filename.isEmpty() == false) {
            settings.setValue("LAUScan::lastUsedDirectory", QFileInfo(filename).absolutePath());
        } else {
            return;
        }
    }

    // OPEN TIFF FILE FOR LOADING THE IMAGE FROM DISK
    TIFF *inTiff = TIFFOpen(filename.toLocal8Bit(), "r");
    if (inTiff) {
        int numDirectories = TIFFNumberOfDirectories(inTiff);
        QProgressDialog progressDialog(QString("Loading document..."), QString("Abort"), 0, numDirectories, widgetInterface, Qt::Sheet);
        progressDialog.setModal(Qt::WindowModal);
        progressDialog.show();
        for (int n = 0; n < numDirectories; n++) {
            if (progressDialog.wasCanceled()) {
                break;
            }
            progressDialog.setValue(n);
            qApp->processEvents();

            // SET THE CURRENT DIRECTORY
            TIFFSetDirectory(inTiff, (unsigned short)n);

            // LOAD THE CURRENT DIRECTORY INTO A NEW IMAGE AND ADD TO LIST
            LAUScan scan(inTiff);
            if (scan.isValid()) {
                imageList.append(scan);
            }
        }
        progressDialog.setValue(numDirectories);

        // CLOSE TIFF FILE
        TIFFClose(inTiff);
    } else {
        QMessageBox::warning(widgetInterface, QString("Load Document"), QString("Error opening tiff file: %1").arg(LAUMemoryObject::lastTiffErrorString));
    }

    // SET THE INPUT FILENAME AS THE FILE NAME STRING
    fileString = filename;
    editFlag = false;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUDocument::saveToDisk(QString filename)
{
    QSettings settings;

    if (filename.isNull()) {
        filename = QString("Untitled");
    }

    if (!filename.toLower().endsWith(QString(".lau"))) {
        filename = QString("%1.lau").arg(filename);
    }

    // ASK THE USER FOR A FILE TO SAVE
    if (filename.startsWith(QString("Untitled"))) {
        filename = getNextAvailableFilename();
        filename = QFileDialog::getSaveFileName(nullptr, QString("Save %1 to disk (*.lau)").arg(fileString), filename, QString("*.lau"));
    }

    if (!filename.isNull()) {
        if (!filename.toLower().endsWith(QString(".lau"))) {
            filename = QString("%1.lau").arg(filename);
        }
        QString directory = QFileInfo(filename).absolutePath();
        settings.setValue("LAUScan::lastUsedDirectory", directory);

        // OPEN TIFF FILE FOR SAVING THE IMAGE
        TIFF *outputTiff = TIFFOpen(filename.toLocal8Bit(), "w8");
        if (outputTiff) {
            QProgressDialog dialog(filename, QString(), 0, imageList.count(), widgetInterface, Qt::Sheet);
            for (int n = 0; n < imageList.count(); n++) {
                dialog.setValue(n);
                qApp->processEvents();

                // GRAB A LOCAL COPY OF THE SUBJECT IMAGE
                LAUScan image = imageList.at(n);

                // CALL THE IMAGE TO SAVE ITSELF IN THE CURRENT DIRECTORY
                image.save(outputTiff, n);
            }
            dialog.setValue(imageList.count());

            // CLOSE TIFF FILE
            TIFFClose(outputTiff);
        } else {
            QMessageBox::warning(widgetInterface, QString("Save Document"), QString("Error opening tiff file: %1").arg(LAUMemoryObject::lastTiffErrorString));
        }

        // SET EDIT FLAG BACK TO FALSE SINCE WE NOW HAVE THE MOST RECENT VERSION SAVED TO DISK
        editFlag = false;

        // PRESERVE THE NEW FILENAME, IF ITS CHANGED
        fileString = filename;

        // TELL USER THE FILE WAS SAVED SUCCESSFULLY
        return (true);
    }
    return (false);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QImage> LAUDocument::previews()
{
    QList<QImage> list;
    for (int n = 0; n < imageList.count(); n++) {
        // GRAB A LOCAL COPY OF THE SUBJECT IMAGE
        LAUScan image = imageList.at(n);
        list.append(image.preview(QSize(320, 320)));
    }
    return (list);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QImage LAUDocument::preview(QString parentName)
{
    for (int n = 0; n < imageList.count(); n++) {
        if (imageList.at(n).parentName() == parentName) {
            // GRAB A LOCAL COPY OF THE SUBJECT IMAGE
            LAUScan image = imageList.at(n);
            return (image.preview(QSize(320, 320)));
        }
    }
    return (QImage());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUDocument::parentStringList()
{
    QStringList stringList;
    for (int n = 0; n < imageList.count(); n++) {
        stringList.append(imageList.at(n).parentName());
    }
    return (stringList);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocument::orderChannels(QStringList orderList)
{
    for (int m = 0; m < orderList.count(); m++) {
        QString string = orderList.at(m);
        for (int n = m + 1; n < imageList.count(); n++) {
            if (imageList.at(n).parentName() == string) {
                imageList.swapItemsAt(n, m);
                makeDirty();
                break;
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocument::replaceImage(LAUScan scan)
{
    for (int n = 0; n < imageList.count(); n++) {
        if (imageList.at(n).parentName() == scan.parentName()) {
            imageList.replace(n, scan);
            makeDirty();
            return;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QStringList LAUDocument::insertImage(QString filename)
{
    // RETURN A LIST OF THE INCOMING IMAGES IN CASE THERE IS MORE THAN ONE
    QStringList stringList;
    QSettings settings;

    if (filename.isEmpty()) {
        QString directory = settings.value("LAUScan::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        if (QDir().exists(directory) == false) {
            directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        filename = QFileDialog::getOpenFileName(nullptr, QString("Open image from disk (*.lau)"), directory, QString("*.lau"));
    }

    if (filename.isEmpty() == false) {
        // STORE THE DIRECTORY TO SETTINGS
        settings.setValue("LAUScan::lastUsedDirectory", QFileInfo(filename).absolutePath());

        // OPEN TIFF FILE FOR LOADING THE IMAGE FROM DISK
        TIFF *inTiff = TIFFOpen(filename.toLocal8Bit(), "r");
        if (inTiff) {
            int numDirectories = TIFFNumberOfDirectories(inTiff);
            QProgressDialog progressDialog(QString("Loading document..."), QString("Abort"), 0, numDirectories, widgetInterface, Qt::Sheet);
            progressDialog.setModal(Qt::WindowModal);
            progressDialog.show();
            for (int n = 0; n < numDirectories; n++) {
                if (progressDialog.wasCanceled()) {
                    break;
                }
                progressDialog.setValue(n);
                qApp->processEvents();

                // SET THE CURRENT DIRECTORY
                TIFFSetDirectory(inTiff, (unsigned short)n);

                // LOAD THE CURRENT DIRECTORY INTO A NEW IMAGE AND ADD TO LIST
                LAUScan scan(inTiff);

                // CREATE A FILENAME FOR THE INCOMING IMAGE, IF IT DOESN'T ALREADY HAVE ONE
                if (scan.parentName().isEmpty() || this->exists(scan.parentName())) {
                    int index = 0;
                    while (1) {
                        QString imageString;
                        if (index < 10) {
                            imageString = QString("%1::image000%2").arg(filename).arg(n);
                        } else if (index < 100) {
                            imageString = QString("%1::image00%2").arg(filename).arg(n);
                        } else if (index < 1000) {
                            imageString = QString("%1::image0%2").arg(filename).arg(n);
                        } else {
                            imageString = QString("%1::image%2").arg(filename).arg(n);
                        }

                        if (this->exists(imageString) == false) {
                            scan.setParentName(imageString);
                            break;
                        }
                        index++;
                    }
                }

                // KEEP TRACK OF FILENAMES FOR THE USER
                stringList << scan.parentName();

                // INSERT CURRENT SCAN INTO OUR SCAN LIST
                insertImage(scan);
            }
            progressDialog.setValue(numDirectories);

            // CLOSE TIFF FILE
            TIFFClose(inTiff);
        } else {
            QMessageBox::warning(widgetInterface, QString("Insert image"), QString("Error opening tiff file: %1").arg(LAUMemoryObject::lastTiffErrorString));
        }
        editFlag = true;
    }
    return (stringList);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocument::insertImage(LAUScan image, int index)
{
    if (index == -1){
        imageList.append(image);
    } else {
        index = qMax(0, qMin(index, imageList.count()));
        imageList.insert(index, image);
    }
    editFlag = true;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocument::insertImages(QList<LAUScan> scans)
{
    for (int n = 0; n < scans.count(); n++) {
        insertImage(scans.at(n));
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUDocument::duplicateImage(QString string)
{
    // GET A COPY OF THE IMAGE THAT WE WANT TO DUPLICATE
    int index;
    LAUScan image = this->image(string, &index);

    // CHECK TO MAKE SURE WE HAVE A VALID IMAGE FROM OUR LIST
    if (image.isNull()) {
        return (QString());
    }

    // CHECK TO SEE IF IMAGE IS COPY OF AN ORIGINAL
    if (string.contains(QString(" COPY "))) {
        int index = string.indexOf(QString(" COPY "));
        string.truncate(index);
    }

    // NOW DERIVE A NEW NAME BASED ON THE CURRENT IMAGE'S NAME
    QString duplicateFileString;
    for (int n = 0; n < 1000; n++) {
        duplicateFileString = string;
        duplicateFileString.append(QString(" COPY %1").arg(n));
        if (!exists(duplicateFileString)) {
            break;
        }
    }

    // CHANGE THE NAME OF THE IMAGE, THEREBY FORCING A DEEP COPY
    image.setFilename(duplicateFileString);

    // INSERT THIS NEW IMAGE INTO OUR IMAGE LIST
    imageList.insert(index, image);
    this->makeDirty();

    // RETURN THE NAME OF THIS NEW DUPLICATE IMAGE TO THE CALLING CLASS
    return (duplicateFileString);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDocument::removeImage(QString parentName)
{
    editFlag = true;

    for (int n = 0; n < imageList.count(); n++) {
        if (imageList.at(n).parentName() == parentName) {
            imageList.removeAt(n);
            return;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUDocument::exists(QString parentName)
{
    for (int n = 0; n < imageList.count(); n++) {
        if (imageList.at(n).parentName() == parentName) {
            return (true);
        }
    }
    return (false);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAUDocument::indexOf(QString string)
{
    for (int n = 0; n < imageList.count(); n++) {
        if (imageList.at(n).parentName() == string) {
            return (n);
        }
    }
    return (-1);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUDocument::image(int index)
{
    if (index > -1 && index < imageList.count()) {
        return (imageList.at(index));
    }
    return (LAUScan());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUDocument::image(QString string, int *index)
{
    for (int n = 0; n < imageList.count(); n++) {
        if (imageList.at(n).parentName() == string) {
            if (index) {
                *index = n;
            }
            return (imageList.at(n));
        }
    }
    return (LAUScan());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUDocument::takeImage(QString string, int *index)
{
    for (int n = 0; n < imageList.count(); n++) {
        if (imageList.at(n).parentName() == string) {
            if (index) {
                *index = n;
            }
            return (imageList.takeAt(n));
        }
    }
    return (LAUScan());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<LAUScan> LAUDocument::takeImages(QStringList strings)
{
    QList<LAUScan> scans;
    for (int n = 0; n < strings.count(); n++) {
        LAUScan scan = takeImage(strings.at(n));
        if (scan.isValid()) {
            scans << scan;
        }
    }
    return (scans);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAUDocument::inspectImage(QString parentName)
{
    for (int n = 0; n < imageList.count(); n++) {
        if (imageList.at(n).parentName() == parentName) {
            LAUScan image = imageList.at(n);
            return (image.inspectImage());
        }
    }
    return (QDialog::Rejected);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QString LAUDocument::getNextAvailableFilename()
{
    QSettings settings;
    QString directory = settings.value("LAUScan::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(directory) == false) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QString filename;
    for (int frameCounter = 1; frameCounter < 1000; frameCounter++) {
        filename = QString("%1/Untitled%2.lau").arg(directory).arg(frameCounter);
        if (QFile::exists(filename) == false) {
            return (filename);
        }
    }
    // If all 999 filenames exist, return the last one anyway
    return (filename);
}
