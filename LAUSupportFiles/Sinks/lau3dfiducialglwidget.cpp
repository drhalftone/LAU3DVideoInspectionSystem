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

#include "lau3dfiducialglwidget.h"
#include "lauconstants.h"
#include "locale.h"
#include "math.h"

QString LAU3DFiducialWidget::lastDirectoryString;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUFiducialDistanceTool::LAUFiducialDistanceTool(QWidget *parent) : QWidget(parent)
{
    this->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_DeleteOnClose, false); // Don't delete widget on close, just hide it
    this->setWindowTitle(QString("Distances"));
    this->setFocusPolicy(Qt::StrongFocus); // Allow window to receive keyboard focus
    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);

    table = new QTableWidget();
    table->setRowCount(8);
    table->setColumnCount(10);
    table->setFixedWidth(820);
    table->setAlternatingRowColors(true);
    table->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    table->setFocusPolicy(Qt::StrongFocus); // Allow table to receive keyboard and mouse focus
    for (int n = 0; n < 8; n++){
        table->setColumnWidth(n, 100);
        table->setHorizontalHeaderItem(n, new QTableWidgetItem(QString("%1").arg(QChar(65 + n % 26))));
        table->setVerticalHeaderItem(n, new QTableWidgetItem(QString("%1").arg(QChar(65 + n % 26))));
    }

    this->layout()->addWidget(table);
    this->setFixedWidth(820);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialDistanceTool::onFiducialsChanged(QVector3D point, int index, QVector3D color)
{
    Q_UNUSED(color);

    fiducialList.replace(index, point);
    for (int m = 0; m < 8; m++){
        if (m < fiducialList.count()){
            double distance = QVector3D(fiducialList.at(index)).distanceToPoint(fiducialList.at(m));
            table->setItem(index, m, new QTableWidgetItem(QString("%1").arg(distance, 0, 'f', 3)));
            table->setItem(m, index, new QTableWidgetItem(QString("%1").arg(distance, 0, 'f', 3)));
        } else {
            table->setItem(m, index, new QTableWidgetItem(QString()));
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialDistanceTool::onFiducialsChanged(QList<QVector3D> points, QList<QVector3D> colors)
{
    Q_UNUSED(colors);
    fiducialList = points;
    for (int n = 0; n < 8; n++){
        for (int m = 0; m < 8; m++){
            if (qMax(n,m) < fiducialList.count()){
                double distance = QVector3D(fiducialList.at(n)).distanceToPoint(fiducialList.at(m));
                table->setItem(n, m, new QTableWidgetItem(QString("%1").arg(distance, 0, 'f', 3)));
            } else {
                table->setItem(n, m, new QTableWidgetItem(QString()));
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialDistanceTool::closeEvent(QCloseEvent *event)
{
    // Hide the tool instead of closing it so it can be shown again
    hide();
    event->accept();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUFiducialTool::LAUFiducialTool(QWidget *parent) : QWidget(parent)
{
    this->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_DeleteOnClose, false); // Don't delete widget on close, just hide it
    this->setWindowTitle(QString("Fiducials"));
    this->setFocusPolicy(Qt::StrongFocus); // Allow window to receive keyboard focus
    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(0, 0, 0, 0);

    table = new QTableWidget();
    table->setRowCount(0);
    table->setColumnCount(6);
    table->setFixedWidth(620);
    table->setColumnWidth(0, 100);
    table->setColumnWidth(1, 100);
    table->setColumnWidth(2, 100);
    table->setColumnWidth(3, 100);
    table->setColumnWidth(4, 100);
    table->setColumnWidth(5, 100);
    table->setAlternatingRowColors(true);
    table->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setFocusPolicy(Qt::StrongFocus); // Allow table to receive keyboard and mouse focus
    table->setHorizontalHeaderItem(0, new QTableWidgetItem(QString("X")));
    table->setHorizontalHeaderItem(1, new QTableWidgetItem(QString("Y")));
    table->setHorizontalHeaderItem(2, new QTableWidgetItem(QString("Z")));
    table->setHorizontalHeaderItem(3, new QTableWidgetItem(QString("R")));
    table->setHorizontalHeaderItem(4, new QTableWidgetItem(QString("G")));
    table->setHorizontalHeaderItem(5, new QTableWidgetItem(QString("B")));
    connect(table, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(onSetCurrentIndex(int, int, int, int)));

    this->layout()->addWidget(table);
    this->setFixedWidth(620);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_C && event->modifiers() == Qt::ControlModifier) {
        QString string;
        for (int row = 0; row < table->rowCount(); row++) {
            for (int col = 0; col < table->columnCount(); col++) {
                string.append(QString("%1\t").arg(table->item(row, col)->text()));
            }
            string.append(QString("\r\n"));
        }
        qDebug() << "clipboard:" << string.data()[0] << string.data()[1] << string.data()[2];
        QApplication::clipboard()->setText(string);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialTool::closeEvent(QCloseEvent *event)
{
    // Hide the tool instead of closing it so it can be shown again
    hide();
    event->accept();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialTool::onFiducialsChanged(QVector3D point, int index, QVector3D color)
{
    if (table) {
        table->item(index, 0)->setText(QString("%1").arg(static_cast<double>(point.x()), 0, 'f', 3));
        table->item(index, 1)->setText(QString("%1").arg(static_cast<double>(point.y()), 0, 'f', 3));
        table->item(index, 2)->setText(QString("%1").arg(static_cast<double>(point.z()), 0, 'f', 3));
        table->item(index, 3)->setText(QString("%1").arg(static_cast<double>(color.x()), 0, 'f', 3));
        table->item(index, 4)->setText(QString("%1").arg(static_cast<double>(color.y()), 0, 'f', 3));
        table->item(index, 5)->setText(QString("%1").arg(static_cast<double>(color.z()), 0, 'f', 3));
        table->selectRow(index);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialTool::onFiducialsChanged(QList<QVector3D> points, QList<QVector3D> colors)
{
    table->setRowCount(points.count());
    for (int r = 0; r < points.count(); r++) {
        table->setItem(r, 0, new QTableWidgetItem(QString("%1").arg(static_cast<double>(points.at(r).x()), 0, 'f', 3)));
        table->setItem(r, 1, new QTableWidgetItem(QString("%1").arg(static_cast<double>(points.at(r).y()), 0, 'f', 3)));
        table->setItem(r, 2, new QTableWidgetItem(QString("%1").arg(static_cast<double>(points.at(r).z()), 0, 'f', 3)));
        table->setItem(r, 3, new QTableWidgetItem(QString("%1").arg(static_cast<double>(colors.at(r).x()), 0, 'f', 3)));
        table->setItem(r, 4, new QTableWidgetItem(QString("%1").arg(static_cast<double>(colors.at(r).y()), 0, 'f', 3)));
        table->setItem(r, 5, new QTableWidgetItem(QString("%1").arg(static_cast<double>(colors.at(r).z()), 0, 'f', 3)));
        table->setVerticalHeaderItem(r, new QTableWidgetItem(QString("%1").arg(QChar(65 + r % 26))));
        for (int c = 0; c < table->columnCount(); c++) {
            table->item(r, c)->setFlags(table->item(r, c)->flags() & ~Qt::ItemIsEditable);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DFiducialWidget::LAU3DFiducialWidget(LAUScan scan, QWidget *parent) : QWidget(parent), filenameString(QString()), scanWidget(nullptr)
{
    QSettings settings;
    lastDirectoryString = settings.value(QString("LAU3DFiducialWidget::lastDirectoryString"), QStandardPaths::displayName(QStandardPaths::DocumentsLocation)).toString();
    if (QDir().exists(lastDirectoryString) == false) {
        lastDirectoryString = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    this->setFocusPolicy(Qt::StrongFocus);
    this->setLayout(new QHBoxLayout());

    fiducialLabel = new LAUFiducialLabel(QImage(":/Images/sample.tif"));
    connect(fiducialLabel, SIGNAL(emitPointMoved(QString, int, int)), this, SLOT(onUpdatePoint(QString, int, int)));
    connect(fiducialLabel, SIGNAL(emitDoubleClick(int, int)), this, SLOT(onAddItem(int, int)));

    QWidget *widgetA = new QWidget();
    widgetA->setLayout(new QHBoxLayout);
    widgetA->layout()->setContentsMargins(0, 0, 0, 0);
    widgetA->layout()->setSpacing(0);
    widgetA->layout()->addWidget(fiducialLabel);
    widgetA->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->layout()->addWidget(widgetA);

    newButton = new QToolButton();
    newButton->setText(QString("add"));
    newButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(newButton, SIGNAL(clicked()), this, SLOT(onAddItem()));

    delButton = new QToolButton();
    delButton->setText(QString("delete"));
    delButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(delButton, SIGNAL(clicked()), this, SLOT(onDeleteItem()));

    upButton  = new QToolButton();
    upButton->setText(QString("up"));
    upButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(upButton, SIGNAL(clicked()), this, SLOT(onMoveUpItem()));

    dwnButton = new QToolButton();
    dwnButton->setText(QString("down"));
    dwnButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(dwnButton, SIGNAL(clicked()), this, SLOT(onMoveDownItem()));

    widgetA = new QWidget();
    widgetA->setLayout(new QHBoxLayout);
    widgetA->setFixedWidth(328);
    widgetA->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    widgetA->layout()->setContentsMargins(0, 0, 0, 0);
    widgetA->layout()->setSpacing(0);
    widgetA->layout()->addWidget(newButton);
    widgetA->layout()->addWidget(delButton);
    widgetA->layout()->addWidget(upButton);
    widgetA->layout()->addWidget(dwnButton);

    table = new QTableWidget();
    table->setRowCount(0);
    table->setColumnCount(3);
    table->setFixedWidth(328);
    table->setColumnWidth(0, 100);
    table->setColumnWidth(1, 100);
    table->setColumnWidth(2, 100);
    table->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    table->setHorizontalHeaderItem(0, new QTableWidgetItem(QString("X")));
    table->setHorizontalHeaderItem(1, new QTableWidgetItem(QString("Y")));
    table->setHorizontalHeaderItem(2, new QTableWidgetItem(QString("Z")));
    connect(table, SIGNAL(currentCellChanged(int, int, int, int)), fiducialLabel, SLOT(setCurrentPoint(int, int, int, int)));
    connect(fiducialLabel, SIGNAL(emitCurrentPointChanged(int)), table, SLOT(selectRow(int)));

    QWidget *widgetB = new QWidget();
    widgetB->setLayout(new QVBoxLayout);
    widgetB->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    widgetB->layout()->setContentsMargins(0, 0, 0, 0);
    widgetB->layout()->setSpacing(0);
    widgetB->layout()->addWidget(table);
    widgetB->layout()->addWidget(widgetA);

    this->layout()->addWidget(widgetB);
    if (scan.isValid()) {
        if (scan.color() == ColorXYZRGB) {
            localScan = scan;
        } else {
            localScan = scan.convertToColor(ColorXYZRGB);
        }
        fiducialLabel->setImage(localScan.preview(localScan.size()));
        scanFileString = scan.parentName();
        this->setWindowTitle(scan.parentName());
    } else {
        this->load();
    }
    displayScan();
    table->setFocusPolicy(Qt::NoFocus);
    fiducialLabel->setFocusPolicy(Qt::StrongFocus);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DFiducialWidget::~LAU3DFiducialWidget()
{
    QSettings settings;
    settings.setValue(QString("LAU3DFiducialWidget::lastDirectoryString"), lastDirectoryString);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QVector3D> LAU3DFiducialWidget::points() const
{
    QList<QVector3D> list;
    for (int n = 0; n < pointList.count(); n++) {
        list << pointList.at(n).point();
    }
    return (list);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QList<QVector3D> LAU3DFiducialWidget::colors() const
{
    QList<QVector3D> list;
    for (int n = 0; n < pointList.count(); n++) {
        list << pointList.at(n).color();
    }
    return (list);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialWidget::load(QString filename)
{
    if (filename.isNull()) {
        filename = QFileDialog::getOpenFileName(nullptr, QString("Load scan file from disk (*.txt;*.tif)"), lastDirectoryString, QString("*.txt *.tif"));
        if (filename.isNull()) {
            return;
        } else {
            lastDirectoryString = QFileInfo(filename).absolutePath();
        }
    }

    if (filename.endsWith(".tif")) {
        scanFileString = filename;
        localScan = LAUScan(filename).convertToColor(ColorXYZRGB);
        fiducialLabel->setImage(localScan.preview(localScan.size()));
        this->setWindowTitle(scanFileString);
    } else {
        // OPEN TIFF FILE FOR LOADING THE IMAGE
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
            filenameString = filename;
            this->setWindowTitle(filenameString);

            QTextStream stream(&file);
            scanFileString = stream.readLine();
            localScan = LAUScan(scanFileString).convertToColor(ColorXYZRGB);
            fiducialLabel->setImage(localScan.preview(localScan.size()));
            while (stream.atEnd() == false) {
                LAUFiducialPoint point;
                point.loadFrom(&stream);

                // GET THE CURRENT NUMBER OF ITEMS IN TABLE
                int n = table->rowCount();

                // DONT LET THE USER ADD MORE THAN 26 POINTS SINCE WE RUN OUT OF LABELS
                if (n >= 26) {
                    break;
                }

                // CREATE A NEW ROW
                table->setRowCount(n + 1);
                table->setCurrentCell(n, 0);

                table->setItem(n, 0, new QTableWidgetItem(QString("%1").arg(static_cast<double>(point.x()))));
                table->setItem(n, 1, new QTableWidgetItem(QString("%1").arg(static_cast<double>(point.y()))));
                table->setItem(n, 2, new QTableWidgetItem(QString("%1").arg(static_cast<double>(point.z()))));
                table->setVerticalHeaderItem(n, new QTableWidgetItem(point.label()));

                table->item(n, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                table->item(n, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                table->item(n, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

                // PRESERVE THE NEW POINT IN OUR POINT LIST
                pointList << point;

                // UPDATE THE IMAGE LABEL TO SHOW THE NEW POINT
                fiducialLabel->setPointList(pointList);
                fiducialLabel->setCurrentPoint(n);
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialWidget::displayScan(QWidget *parent)
{
    if (localScan.isNull() == false && scanWidget == nullptr) {
        scanWidget = new LAU3DFiducialGLWidget(localScan, parent);
        scanWidget->show();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialWidget::save(QString filename)
{
    if (filename.isNull()) {
        filename = QFileDialog::getSaveFileName(nullptr, QString("Save scan file to disk (*.txt)"), lastDirectoryString, QString("*.txt"));
        if (filename.isNull()) {
            return;
        } else {
            lastDirectoryString = QFileInfo(filename).absolutePath();
        }
    }

    // OPEN TIFF FILE FOR SAVING THE IMAGE
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        filenameString = filename;
        this->setWindowTitle(filenameString);

        QTextStream stream(&file);
        stream << scanFileString << QString("\n");
        for (int n = 0; n < pointList.count(); n++) {
            pointList.at(n).saveTo(&stream);
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialWidget::onAddItem(int col, int row)
{
    // GET THE CURRENT NUMBER OF ITEMS IN TABLE
    int n = table->rowCount();

    // DONT LET THE USER ADD MORE THAN 26 POINTS SINCE WE RUN OUT OF LABELS
    if (n >= 26) {
        return;
    }

    // CREATE A NEW ROW
    table->setRowCount(n + 1);
    table->setCurrentCell(n, 0);

    // SET THE ROW AND COLUMN COORDINATES
    if (col == -1) {
        col = fiducialLabel->width() / 2;
    }
    if (row == -1) {
        row = fiducialLabel->height() / 2;
    }

    // GET THE X,Y,Z COORDINATE FOR THE INCOMING PIXEL COORDINATE
    QVector<float> pixel = localScan.pixel(col, row);

    // CREATE A NEW POINT AND ADD TO TABLE
    LAUFiducialPoint point(col, row, pixel[0], pixel[1], pixel[2], pixel[3], pixel[4], pixel[5], QString(QChar(65 + n)));

    // ADD NEW POINT TO TABLE LIST
    table->setItem(n, 0, new QTableWidgetItem(QString("%1").arg(static_cast<double>(point.x()))));
    table->setItem(n, 1, new QTableWidgetItem(QString("%1").arg(static_cast<double>(point.y()))));
    table->setItem(n, 2, new QTableWidgetItem(QString("%1").arg(static_cast<double>(point.z()))));
    table->setVerticalHeaderItem(n, new QTableWidgetItem(point.label()));

    // MAKE TABLE LIST ITEM READ ONLY
    table->item(n, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    table->item(n, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    table->item(n, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    // PRESERVE THE NEW POINT IN OUR POINT LIST
    pointList << point;

    // ADD POINT TO LABEL
    fiducialLabel->setPointList(pointList);
    fiducialLabel->setCurrentPoint(n);

    QList<QVector3D> vectorList;
    QList<QVector3D> colorsList;
    for (int n = 0; n < pointList.count(); n++) {
        vectorList << QVector3D(pointList.at(n).x(), pointList.at(n).y(), pointList.at(n).z());
        colorsList << QVector3D(pointList.at(n).r(), pointList.at(n).g(), pointList.at(n).b());
    }

    if (scanWidget) {
        scanWidget->onSetFiducials(vectorList, colorsList);
    }

    // EMIT THE UPDATE SIGNAL SO THE USER CAN RESPOND AS NEEDED
    emit emitUpdate();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialWidget::onDeleteItem()
{
    // GET THE CURRENT TABLE ROW
    int k = table->currentRow();

    // RETURN IF THERE IS NO CURRENT ROW
    if (k == -1) {
        return;
    }

    // GRAB THE LABEL OF THE CURRENT ROW SO WE KNOW WHICH POINT TO DELETE
    QString label = table->verticalHeaderItem(k)->text();

    // FIND THE POINT IN THE LIST AND DELETE IT
    for (int n = 0; n < pointList.count(); n++) {
        if (pointList.at(n).label() == label) {
            pointList.removeAt(n);
            table->removeRow(n);
            break;
        }
    }

    // RESET ALL THE POINT LABELS
    for (int n = 0; n < pointList.count(); n++) {
        LAUFiducialPoint point = pointList.at(n);
        point.setLabel(QString(QChar(65 + n)));
        table->setVerticalHeaderItem(n, new QTableWidgetItem(point.label()));
        pointList.replace(n, point);
    }

    // SELECT THE CURRENT ROW SO USER CAN PRESS DELETE BUTTON OVER AND OVER
    k = qMin(k, table->rowCount() - 1);
    if (k >= 0) {
        table->setCurrentCell(k, 0);
        fiducialLabel->setCurrentPoint(k);
    }
    fiducialLabel->setPointList(pointList);

    QList<QVector3D> vectorList;
    QList<QVector3D> colorsList;
    for (int n = 0; n < pointList.count(); n++) {
        vectorList << QVector3D(pointList.at(n).x(), pointList.at(n).y(), pointList.at(n).z());
        colorsList << QVector3D(pointList.at(n).r(), pointList.at(n).g(), pointList.at(n).b());
    }
    if (scanWidget) {
        scanWidget->onSetFiducials(vectorList, colorsList);
    }

    // EMIT THE UPDATE SIGNAL SO THE USER CAN RESPOND AS NEEDED
    emit emitUpdate();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialWidget::onMoveUpItem()
{
    // GET THE CURRENT TABLE ROW
    int k = table->currentRow();

    // RETURN IF THERE IS NO ROOM TO MOVE ITEM UP
    if (k <= 0) {
        return;
    }

    // SWAP THE VALUES IN THE LIST
    LAUFiducialPoint pointA = pointList.at(k);
    LAUFiducialPoint pointB = pointList.at(k - 1);

    QString labelA = pointA.label();
    pointA.setLabel(pointB.label());
    pointB.setLabel(labelA);

    pointList.replace(k, pointB);
    pointList.replace(k - 1, pointA);
    fiducialLabel->setPointList(pointList);

    // NOW CHANGE THE CELLS IN OUR TABLE TO MATCH
    table->setItem(k, 0, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointB.x()))));
    table->setItem(k, 1, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointB.y()))));
    table->setItem(k, 2, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointB.z()))));
    table->setVerticalHeaderItem(k, new QTableWidgetItem(pointB.label()));
    table->item(k, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    table->item(k, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    table->item(k, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    table->setItem(k - 1, 0, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointA.x()))));
    table->setItem(k - 1, 1, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointA.y()))));
    table->setItem(k - 1, 2, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointA.z()))));
    table->setVerticalHeaderItem(k - 1, new QTableWidgetItem(pointA.label()));
    table->item(k - 1, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    table->item(k - 1, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    table->item(k - 1, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    table->setCurrentCell(k - 1, 0);
    fiducialLabel->setCurrentPoint(k - 1);

    if (scanWidget) {
        scanWidget->onSetFiducials(points(), colors());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialWidget::onMoveDownItem()
{
    // GET THE CURRENT TABLE ROW
    int k = table->currentRow();

    // RETURN IF THERE IS NO ROOM TO MOVE ITEM UP
    if (k < 0 || k == table->rowCount() - 1) {
        return;
    }

    // SWAP THE VALUES IN THE LIST
    LAUFiducialPoint pointA = pointList.at(k);
    LAUFiducialPoint pointB = pointList.at(k + 1);

    QString labelA = pointA.label();
    pointA.setLabel(pointB.label());
    pointB.setLabel(labelA);

    pointList.replace(k, pointB);
    pointList.replace(k + 1, pointA);
    fiducialLabel->setPointList(pointList);

    // NOW CHANGE THE CELLS IN OUR TABLE TO MATCH
    table->setItem(k, 0, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointB.x()))));
    table->setItem(k, 1, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointB.y()))));
    table->setItem(k, 2, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointB.z()))));
    table->setVerticalHeaderItem(k, new QTableWidgetItem(pointB.label()));
    table->item(k, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    table->item(k, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    table->item(k, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    table->setItem(k + 1, 0, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointA.x()))));
    table->setItem(k + 1, 1, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointA.y()))));
    table->setItem(k + 1, 2, new QTableWidgetItem(QString("%1").arg(static_cast<double>(pointA.z()))));
    table->setVerticalHeaderItem(k + 1, new QTableWidgetItem(pointA.label()));
    table->item(k + 1, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    table->item(k + 1, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    table->item(k + 1, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    table->setCurrentCell(k + 1, 0);
    fiducialLabel->setCurrentPoint(k + 1);

    if (scanWidget) {
        scanWidget->onSetFiducials(points(), colors());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialWidget::onUpdatePoint(QString label, int col, int row)
{
    for (int n = 0; n < pointList.count(); n++) {
        if (pointList.at(n).label() == label) {
            // GET THE XYZ COORDINATE FOR THE INCOMING PIXEL COORDINATE
            QVector<float> pixel = localScan.pixel(col, row);

            // GRAB A LOCAL COPY OF THE  SUBJECT POINT FROM THE POINT LIST
            LAUFiducialPoint point = pointList.at(n);
            point.setRow(row);
            point.setCol(col);
            point.setX(pixel[0]);
            point.setY(pixel[1]);
            point.setZ(pixel[2]);

            // UPDATE THE POINT IN OUR POINT LIST
            pointList.replace(n, point);

            // UPDATE LABELS IN THE TABLE VIEW
            table->setItem(n, 0, new QTableWidgetItem(QString("%1").arg(static_cast<double>(point.x()))));
            table->setItem(n, 1, new QTableWidgetItem(QString("%1").arg(static_cast<double>(point.y()))));
            table->setItem(n, 2, new QTableWidgetItem(QString("%1").arg(static_cast<double>(point.z()))));

            // CREATE A LIST OF 3D POINTS FOR THE OPENGL DISPLAY
            QList<QVector3D> vectorList;
            QList<QVector3D> colorsList;
            for (int n = 0; n < pointList.count(); n++) {
                vectorList << QVector3D(pointList.at(n).x(), pointList.at(n).y(), pointList.at(n).z());
                colorsList << QVector3D(pointList.at(n).r(), pointList.at(n).g(), pointList.at(n).b());
            }
            if (scanWidget) {
                scanWidget->onSetFiducials(vectorList, colorsList);
            }

            // EMIT THE UPDATE SIGNAL SO THE USER CAN RESPOND AS NEEDED
            emit emitUpdate();
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialWidget::keyPressEvent(QKeyEvent *event)
{
    // RETURN IF THERE IS NO ROOM TO MOVE ITEM UP
    int k = table->currentRow();

    if (k < 0) {
        return;
    }

    // SWAP THE VALUES IN THE LIST
    LAUFiducialPoint point = pointList.at(k);
    if (event->key() == Qt::Key_Right) {
        point.setCol(qMin((int)point.col() + 1, (int)localScan.width() - 1));
    } else if (event->key() == Qt::Key_Left) {
        point.setCol(qMax((int)point.col() - 1, 0));
    } else if (event->key() == Qt::Key_Up) {
        point.setRow(qMax((int)point.row() - 1, 0));
    } else if (event->key() == Qt::Key_Down) {
        point.setRow(qMin((int)point.row() + 1, (int)localScan.height() - 1));
    }
    pointList.replace(k, point);
    fiducialLabel->updatePoint(point);
    onUpdatePoint(point.label(), point.col(), point.row());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialLabel::updatePoint(LAUFiducialPoint point)
{
    for (int n = 0; n < pointList.count(); n++) {
        if (point.label() == pointList.at(n).label()) {
            pointList.replace(n, point);
            update();
            return;
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialLabel::mousePressEvent(QMouseEvent *event)
{
    // CHECK FOR RIGHT BUTTON CLICK
    if (event->button() != Qt::LeftButton) {
        return;
    }

    buttonDownFlag = false;

    int minDist = 100;
    for (int n = 0; n < pointList.count(); n++) {
        LAUFiducialPoint point = pointList.at(n);

        int x = point.col() - event->position().x();
        int y = point.row() - event->position().y();

        int distance = x * x + y * y;
        if (distance <= minDist) {
            minDist = distance;
            buttonDownFlag = true;
            currentActivePointIndex = n;
        }
    }

    if (buttonDownFlag) {
        emit emitCurrentPointChanged(currentActivePointIndex);
        update();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (buttonDownFlag && currentActivePointIndex >= 0 && currentActivePointIndex < pointList.count()) {
        LAUFiducialPoint point = pointList.at(currentActivePointIndex);
        point.setRow(qMin(qMax(0, (int)event->position().y()), image.height() - 1));
        point.setCol(qMin(qMax(0, (int)event->position().x()), image.width() - 1));
        pointList.replace(currentActivePointIndex, point);

        emit emitPointMoved(point.label(), point.col(), point.row());
    } else {
        return;
    }
    update();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialLabel::mouseReleaseEvent(QMouseEvent *)
{
    buttonDownFlag = false;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUFiducialLabel::paintEvent(QPaintEvent *)
{
    QPainter painter;

    float xScaleFactor = (float)this->width() / (float)image.width();
    float yScaleFactor = (float)this->height() / (float)image.height();

    yScaleFactor = qMin(xScaleFactor, yScaleFactor);

    QTransform transform;
    transform.scale(yScaleFactor, yScaleFactor);

    painter.begin(this);
    painter.setTransform(transform);
    painter.drawImage(0, 0, image);

    painter.setBrush(QBrush(Qt::red, Qt::SolidPattern));
    painter.setPen(QPen(QBrush(Qt::black), 3, Qt::SolidLine));
    for (int n = 0; n < pointList.count(); n++) {
        if (n != currentActivePointIndex) {
            LAUFiducialPoint point = pointList.at(n);
            painter.drawEllipse(point.col() - 10, point.row() - 10, 20, 20);
            painter.drawText(point.col() - 10, point.row() - 10, 20, 20, Qt::AlignCenter | Qt::AlignHCenter, point.label());
        }
    }

    if (currentActivePointIndex >= 0 && currentActivePointIndex < pointList.count()) {
        LAUFiducialPoint point = pointList.at(currentActivePointIndex);
        painter.setBrush(QBrush(Qt::yellow, Qt::SolidPattern));
        painter.drawEllipse(point.col() - 10, point.row() - 10, 20, 20);
        painter.drawText(point.col() - 10, point.row() - 10, 20, 20, Qt::AlignCenter | Qt::AlignHCenter, point.label());
    }

    painter.end();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DFiducialGLWidget::LAU3DFiducialGLWidget(LAUScan scan, QWidget *parent) : LAU3DScanGLWidget(scan, parent), localScan(scan), fiducialRadius(0.30f), fiducialDragMode(false), enableFiducialFlag(false), currentActivePointIndex(-1), tool(nullptr), distanceTool(nullptr)
{
    // MAKE SURE WE CAN SEND VECTOR3D LISTS AS SIGNALS
    qRegisterMetaType< QList<QVector3D> >("QList<QVector3D>");

    // ENABLE THE SYMMETRY FILTER
    enableSymmetry(false);

    for (unsigned int n = 0; n < 26; n++) {
        fiducialTextures[n] = nullptr;
    }

    // Tool will be created in showEvent() to avoid Qt6 Windows OpenGL parenting issues
    action = this->menu()->addAction(QString("Enable Fiducials"), this, SLOT(onEnableFiducials(bool)));
    action->setCheckable(true);
    action->setChecked(enableFiducialFlag);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DFiducialGLWidget::LAU3DFiducialGLWidget(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent) : LAU3DScanGLWidget(cols, rows, color, parent), fiducialRadius(0.30f), fiducialDragMode(false), enableFiducialFlag(false), currentActivePointIndex(-1), tool(nullptr), distanceTool(nullptr)
{
    // MAKE SURE WE CAN SEND VECTOR3D LISTS AS SIGNALS
    qRegisterMetaType< QList<QVector3D> >("QList<QVector3D>");

    for (unsigned int n = 0; n < 26; n++) {
        fiducialTextures[n] = nullptr;
    }

    // CREATE A LOCAL SCAN OBJECT
    localScan = LAUScan(cols, rows, color);

    // Tool will be created in showEvent() to avoid Qt6 Windows OpenGL parenting issues
    action = this->menu()->addAction(QString("Enable Fiducials"), this, SLOT(onEnableFiducials(bool)));
    action->setCheckable(true);
    action->setChecked(enableFiducialFlag);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAU3DFiducialGLWidget::~LAU3DFiducialGLWidget()
{
    if (wasInitialized()) {
        makeCurrent();
        for (unsigned int n = 0; n < 26; n++) {
            if (fiducialTextures[n]) {
                delete fiducialTextures[n];
            }
        }
    }

    if (tool) {
        delete tool;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QMatrix4x4 LAU3DFiducialGLWidget::symmetry() const
{
    // SEE IF WE HAVE THREE FIDUCIALS BY WHICH TO CALCULATE OUR TRANSFORM
    if (fiducialList.count() < LAU_MIN_FIDUCIAL_COUNT) {
        return (QMatrix4x4());
    }

    // DEFINE THE XYZ-VECTORS
    QVector3D xVec = (fiducialList.at(0) - fiducialList.at(1)).normalized();
    QVector3D yVec = (fiducialList.at(2) - fiducialList.at(1)).normalized();
    QVector3D zVec = QVector3D().normal(xVec, yVec);

    // MAKE SURE Y VECTOR IS PERPENDICULAR TO THE X VECTOR
    yVec = QVector3D().normal(zVec, xVec);

    // DEFINE ROTATION VECTOR
    QMatrix4x4 rotMat(xVec.x(), xVec.y(), xVec.z(), 0.0f,
                      zVec.x(), zVec.y(), zVec.z(), 0.0f,
                      yVec.x(), yVec.y(), yVec.z(), 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f);

    // DEFINE THE TRANSLATION MATRIX
    QMatrix4x4 trnMat(1.0f, 0.0f, 0.0f, -fiducialList.at(1).x(),
                      0.0f, 1.0f, 0.0f, -fiducialList.at(1).y(),
                      0.0f, 0.0f, 1.0f, -fiducialList.at(1).z(),
                      0.0f, 0.0f, 0.0f, 1.0f);

    // RETURN THE TRANSFORM THAT FIRST PERFORMS TRANSLATION AND THEN ROTATION
    return (rotMat * trnMat);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::onSetFiducials(QList<QPoint> rowColumns)
{
    rowColumnList = rowColumns;
    fiducialList.clear();
    colorsList.clear();

    // GRAB A COPY OF THE SCAN SITTING ON THE GPU
    copyScan((float*)localScan.constPointer());

    // SEARCH LOCAL SCAN FOR XYZ+RGB COORDINATES
    if (localScan.isValid()){
        for (int n = 0; n < rowColumnList.count(); n++){
            QVector<float> pixel = localScan.pixel(rowColumnList.at(n));
            switch (localScan.color()){
            case ColorGray:
                fiducialList << QVector3D(NAN, NAN, NAN);
                colorsList << QVector3D(pixel[0], pixel[0], pixel[0]);
                break;
            case ColorRGB:
            case ColorRGBA:
                fiducialList << QVector3D(NAN, NAN, NAN);
                colorsList << QVector3D(pixel[0], pixel[1], pixel[2]);
                break;
            case ColorXYZ:
            case ColorXYZW:
                fiducialList << QVector3D(pixel[0], pixel[1], pixel[2]);
                colorsList << QVector3D(0.0f, 0.0f, 0.0f);
                break;
            case ColorXYZG:
                fiducialList << QVector3D(pixel[0], pixel[1], pixel[2]);
                colorsList << QVector3D(pixel[3], pixel[3], pixel[3]);
                break;
            case ColorXYZRGB:
                fiducialList << QVector3D(pixel[0], pixel[1], pixel[2]);
                colorsList << QVector3D(pixel[3], pixel[4], pixel[5]);
                break;
            case ColorXYZWRGBA:
                fiducialList << QVector3D(pixel[0], pixel[1], pixel[2]);
                colorsList << QVector3D(pixel[4], pixel[5], pixel[6]);
                break;
            case ColorUndefined:
                break;
            }
        }
    }
    currentActivePointIndex = fiducialList.count() - 1;
    emitFiducialsChanged(fiducialList, colorsList);
    updateFiducialProjectionMatrix();
    update();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::onSetFiducials(QPoint point, int index)
{
    // REPLACE THE INCOMING POINT IN OUR LIST OF COORDINATES
    rowColumnList.replace(index, point);

    // GRAB A COPY OF THE SCAN SITTING ON THE GPU
    copyScan((float*)localScan.constPointer());

    // SEARCH LOCAL SCAN FOR XYZ+RGB COORDINATES
    if (localScan.isValid()){
        QVector<float> pixel = localScan.pixel(rowColumnList.at(index));
        switch (localScan.color()){
        case ColorGray:
            fiducialList.replace(index, QVector3D(NAN, NAN, NAN));
            colorsList.replace(index, QVector3D(pixel[0], pixel[0], pixel[0]));
            break;
        case ColorRGB:
        case ColorRGBA:
            fiducialList.replace(index, QVector3D(NAN, NAN, NAN));
            colorsList.replace(index, QVector3D(pixel[0], pixel[1], pixel[2]));
            break;
        case ColorXYZ:
        case ColorXYZW:
            fiducialList.replace(index, QVector3D(pixel[0], pixel[1], pixel[2]));
            colorsList.replace(index, QVector3D(0.0f, 0.0f, 0.0f));
            break;
        case ColorXYZG:
            fiducialList.replace(index, QVector3D(pixel[0], pixel[1], pixel[2]));
            colorsList.replace(index, QVector3D(pixel[3], pixel[3], pixel[3]));
            break;
        case ColorXYZRGB:
            fiducialList.replace(index, QVector3D(pixel[0], pixel[1], pixel[2]));
            colorsList.replace(index, QVector3D(pixel[3], pixel[4], pixel[5]));
            break;
        case ColorXYZWRGBA:
            fiducialList.replace(index, QVector3D(pixel[0], pixel[1], pixel[2]));
            colorsList.replace(index, QVector3D(pixel[4], pixel[5], pixel[6]));
            break;
        case ColorUndefined:
            break;
        }
    }
    currentActivePointIndex = index;
    emitFiducialsChanged(fiducialList, colorsList);
    updateFiducialProjectionMatrix();
    update();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::onSetFiducials(QVector3D point, int index)
{
    // REPLACE THE INCOMING POINT IN OUR LIST OF COORDINATES
    fiducialList.replace(index, point);
    currentActivePointIndex = index;

    emitFiducialsChanged(fiducialList, colorsList);
    updateFiducialProjectionMatrix();
    update();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::updateFiducialProjectionMatrix()
{
    // INITIALIZE THE PROJECTION MATRIX TO IDENTITY
    float fov = qMin(120.0f, qMax(verticalFieldOfView, 0.5f));
    float aspectRatio = (float)localWidth / (float)localHeight;

    fiducialProjection.setToIdentity();
    fiducialProjection.perspective(fov, aspectRatio, 1.0f, 10000.0f);
    fiducialProjection.lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, zMax), QVector3D(0.0f, 1.0f, 0.0f));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::showEvent(QShowEvent *event)
{
    // Call base class implementation
    LAU3DScanGLWidget::showEvent(event);

    // Create fiducial tools on first show to avoid Qt6 Windows OpenGL parenting issues
    if (!tool) {
        // Create tool with this widget as parent for proper window management
        tool = new LAUFiducialTool(this);
        tool->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        connect(this, SIGNAL(emitFiducialsChanged(QVector3D, int, QVector3D)), tool, SLOT(onFiducialsChanged(QVector3D, int, QVector3D)));
        connect(this, SIGNAL(emitFiducialsChanged(QList<QVector3D>, QList<QVector3D>)), tool, SLOT(onFiducialsChanged(QList<QVector3D>, QList<QVector3D>)));
        connect(this, SIGNAL(emitActivePointIndexChanged(int)), tool, SLOT(onSetCurrentIndex(int)));
        connect(tool, SIGNAL(emitCurrentIndex(int)), this, SLOT(onSetActivePointIndex(int)));

#ifdef ENABLEDISTANCETOOL
        // Create distance tool with this widget as parent for proper window management
        distanceTool = new LAUFiducialDistanceTool(this);
        distanceTool->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        connect(this, SIGNAL(emitFiducialsChanged(QVector3D, int, QVector3D)), distanceTool, SLOT(onFiducialsChanged(QVector3D, int, QVector3D)));
        connect(this, SIGNAL(emitFiducialsChanged(QList<QVector3D>, QList<QVector3D>)), distanceTool, SLOT(onFiducialsChanged(QList<QVector3D>, QList<QVector3D>)));
#endif

        // Show tools if fiducials are already enabled
        if (enableFiducialFlag) {
            if (sandboxEnabled() == false) {
#ifdef ENABLEDISTANCETOOL
                if (distanceTool) {
                    distanceTool->show();
                }
#endif
                tool->show();
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::mousePressEvent(QMouseEvent *event)
{
    // TELL THE WORLD THAT WE HAVE BEEN ACTIVATED
    emit emitActivated();

    // SET THE FIDUCIAL DRAGGING FLAGS TO FALSE UNTIL WE KNOW BETTER
    fiducialDragMode = false;

    if (enableFiducialFlag && event->button() == Qt::LeftButton && fiducialList.count() > 0) {
        // SEE IF WE ARE IN CLOSE PROXIMITY TO A FIDUCIAL
        float x = 2.0f * (float)event->pos().x() / (float)this->width() - 1.0f;
        float y = 2.0f * (float)event->pos().y() / (float)this->height() - 1.0f;

        // SET THE THRESHOLD FOR HOW CLOSE A MOUSE CLICK NEEDS TO BE TO BE CONSIDERED A FIDUCIAL CLICK
        float tolerance = 5.0f; //2.0f * fiducialRadius / zoomFactor; //(float)width() / 40.0f * fiducialRadius / zoomFactor;

        // STORE THE CLOSEST FIDUCIAL
        QVector3D closestFiducial(1e9, 1e9, -1e9);

        for (int n = 0; n < fiducialList.count(); n++) {
            // CALCULATE THE WINDOW COORDINATE OF THE CURRENT FIDUCIAL
            QVector3D fiducial = fiducialList.at(n);

            // SKIP INVALID FIDUCIALS (sentinel values close to -999)
            // Use absolute distance check since Z coordinates can be negative
            if (qAbs(fiducial.x() - (-999.0f)) < 50.0f || qAbs(fiducial.y() - (-999.0f)) < 50.0f || qAbs(fiducial.z() - (-999.0f)) < 50.0f) {
                continue;
            }

            QVector4D coordinate = projection * QVector4D(fiducial.x(), fiducial.y(), fiducial.z(), 1.0f);

            // SKIP FIDUCIALS BEHIND THE CAMERA OR AT INFINITY (w == 0)
            if (qFuzzyIsNull(coordinate.w())) {
                continue;
            }

            coordinate = coordinate / coordinate.w();

            // CALCULATE THE DISTANCE FROM THE FIDUCIAL TO THE EVENT COORDINATE IN PIXELS
            QVector2D position((coordinate.x() - x) / 2.0f * (float)width(), (coordinate.y() + y) / 2.0f * (float)height());

            qDebug() << position.length() << tolerance << fiducialRadius << zoomFactor;

            // SEE IF THIS FIDUCIAL IS WITHIN A CLOSE PROXIMITY OF THE MOUSE CLICK
            if (position.length() < tolerance) {
                if (fiducial.z() > closestFiducial.z()) {
                    fiducialDragMode = true;
                    currentActivePointIndex = n;
                    closestFiducial = fiducial;
                    emit emitActivePointIndexChanged(currentActivePointIndex);
                }
            }
        }

        // CHECK TO SEE IF WE SHOULD ENABLE FIDUCIAL TRACKING
        if (fiducialDragMode) {
            // AND WE NEED TO GRAB A COPY OF THE MOUSE BUFFER TO DRAGGING THE FIDUCIAL
#ifdef SANDBOX
            rowColumnMap = grabMouseBuffer(LAU3DScanGLWidget::MouseModeRowColumn);
#endif
            screenMap = grabMouseBuffer(LAU3DScanGLWidget::MouseModeXYZ);
            colorMap = grabMouseBuffer(LAU3DScanGLWidget::MouseModeRGB);

            // UPDATE THE SCREEN SO THAT WE AT LEAST CHANGE THE COLOR OF THE CURRENT FIDUCIAL
            update();
        } else {
            // LET THE UNDERLYING CLASS HANDLE THE EVENT
            LAU3DScanGLWidget::mousePressEvent(event);
        }
    } else {
        // LET THE UNDERLYING CLASS HANDLE THE EVENT
        LAU3DScanGLWidget::mousePressEvent(event);

        // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
        updateFiducialProjectionMatrix();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (fiducialDragMode) {
        fiducialDragMode = false;

        // EMIT THE FIDUCIALS/COLORS TO THE USER
        emitFiducialsChanged(fiducialList);
        emitFiducialsChanged(fiducialList, colorsList);
        emitFiducialsChanged(rowColumnList);
    } else {
        // LET THE UNDERLYING CLASS HANDLE THE EVENT
        LAU3DScanGLWidget::mouseReleaseEvent(event);

        // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
        updateFiducialProjectionMatrix();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (enableFiducialFlag && event->button() == Qt::LeftButton) {
        // WE NEED TO GRAB A COPY OF THE MOUSE BUFFER TO POSITION THE FIDUCIAL
#ifdef SANDBOX
        rowColumnMap = grabMouseBuffer(LAU3DScanGLWidget::MouseModeRowColumn);
#endif
        screenMap = grabMouseBuffer(LAU3DScanGLWidget::MouseModeXYZ);
        colorMap = grabMouseBuffer(LAU3DScanGLWidget::MouseModeRGB);

        // CONVERT THE WIDGET COORDINATE TO A SCREEN MAP COORDINATE
        int row = (1.0f - (float)event->pos().y() / (float)height()) * (int)screenMap.height();
        int col = (float)event->pos().x() / (float)width() * (int)screenMap.width();

        if (row >= 0 && row < (int)screenMap.height() && col >= 0 && col < (int)screenMap.width()) {
            // GRAB THE XYZW-PIXEL FOR THE CURRENT MOUSE POSITION
            float *pixel = &((float *)screenMap.constScanLine(row))[4 * col];
            float *color = &((float *)colorMap.constScanLine(row))[4 * col];
#ifdef SANDBOX
            float *coord = &((float *)rowColumnMap.constScanLine(row))[4 * col];
#endif
            // MAKE SURE THAT THE W-COORDINATE IS NOT BACKGROUND (background pixels have alpha < -0.5)
            if (pixel[3] > -0.5f) {
                // APPEND THE FIDUCIAL LIST WITH THE CURRENT MOUSE POSITION
                fiducialList.append(QVector3D(pixel[0], pixel[1], pixel[2]));
                colorsList.append(QVector3D(color[0], color[1], color[2]));
                currentActivePointIndex = fiducialList.count() - 1;
                emit emitActivePointIndexChanged(currentActivePointIndex);

                // EMIT THE FIDUCIALS/COLORS TO THE USER
                emitFiducialsChanged(fiducialList.count());
                emitFiducialsChanged(fiducialList);
                emitFiducialsChanged(fiducialList, colorsList);
#ifdef SANDBOX
                rowColumnList.append(QPoint((int)coord[0], (int)coord[1]));
                emitFiducialsChanged(rowColumnList);
#endif
                // DRAW THE NEW FIDUCIAL ON SCREEN
                update();
            } else {
                // IF THE USER DOUBLE CLICKS AN AREA WITH NO SCAN DATA, THEN CALL
                // THE UNDERLYING CLASS TO HANDLE THE DOUBLE CLICK
                LAU3DScanGLWidget::mouseDoubleClickEvent(event);

                // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
                updateFiducialProjectionMatrix();
            }
        }
    } else {
        // LET THE UNDERLYING CLASS HANDLE THE EVENT
        LAU3DScanGLWidget::mouseDoubleClickEvent(event);

        // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
        updateFiducialProjectionMatrix();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (fiducialDragMode && currentActivePointIndex >= 0) {
        // CONVERT THE WIDGET COORDINATE TO A SCREEN MAP COORDINATE
        int row = (1.0f - (float)event->pos().y() / (float)height()) * (int)screenMap.height();
        int col = (float)event->pos().x() / (float)width() * (int)screenMap.width();

        if (row >= 0 && row < (int)screenMap.height() && col >= 0 && col < (int)screenMap.width()) {
            // GRAB THE XYZW-PIXEL FOR THE CURRENT MOUSE POSITION
            float *pixel = &((float *)screenMap.constScanLine(row))[4 * col];
            float *color = &((float *)colorMap.constScanLine(row))[4 * col];
#ifdef SANDBOX
            float *coord = &((float *)rowColumnMap.constScanLine(row))[4 * col];
#endif
            // MAKE SURE THAT THE W-COORDINATE IS NOT BACKGROUND (background pixels have alpha < -0.5)
            if (pixel[3] > -0.5f) {
                // REPLACE THE FIDUCIAL WITH THE CURRENT MOUSE POSITION
                QVector3D fiducial = QVector3D(pixel[0], pixel[1], pixel[2]);
                QVector3D colors = QVector3D(color[0], color[1], color[2]);
                fiducialList.replace(currentActivePointIndex, fiducial);
                colorsList.replace(currentActivePointIndex, colors);

                // EMIT THE ALTERED FIDUCIALS/COLORS
                emit emitFiducialsChanged(fiducial, currentActivePointIndex);
                emit emitFiducialsChanged(fiducial, currentActivePointIndex, colors);          
#ifdef SANDBOX
                QPoint point = QPoint((int)coord[0], (int)coord[1]);
                rowColumnList.replace(currentActivePointIndex, point);
                emit emitFiducialsChanged(point, currentActivePointIndex);
#endif
                // UPDATE THE DISPLAY
                update();
            }
        }
    } else {
        // LET THE UNDERLYING CLASS HANDLE THE EVENT
        LAU3DScanGLWidget::mouseMoveEvent(event);

        // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
        updateFiducialProjectionMatrix();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::keyPressEvent(QKeyEvent *event)
{
    // IF THERE IS NO CURRENTLY ACTIVE POINTS, THEN QUIT
    if (enableFiducialFlag && currentActivePointIndex >= 0) {
        // CHECK FOR AN UP OR DOWN ARROW TO CHANGE THE CURRENT FIDUCIAL
        // CHECK FOR DELETE TO REMOVE A FIDUCIAL FROM THE LIST
        if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Up) {
            currentActivePointIndex = (currentActivePointIndex + 1) % fiducialList.count();
            emit emitActivePointIndexChanged(currentActivePointIndex);

            // EMIT THE FIDUCIALS/COLORS TO THE USER
            emit emitFiducialsChanged(fiducialList.at(currentActivePointIndex), currentActivePointIndex);
            emit emitFiducialsChanged(fiducialList.at(currentActivePointIndex), currentActivePointIndex, colorsList.at(currentActivePointIndex));
        } else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Down) {
            currentActivePointIndex = (currentActivePointIndex + fiducialList.count() - 1) % fiducialList.count();
            emit emitActivePointIndexChanged(currentActivePointIndex);

            // EMIT THE FIDUCIALS/COLORS TO THE USER
            emit emitFiducialsChanged(fiducialList.at(currentActivePointIndex), currentActivePointIndex);
            emit emitFiducialsChanged(fiducialList.at(currentActivePointIndex), currentActivePointIndex, colorsList.at(currentActivePointIndex));
        } else if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
            fiducialList.removeAt(currentActivePointIndex);
            currentActivePointIndex = qMin(currentActivePointIndex, fiducialList.count() - 1);
            emit emitActivePointIndexChanged(currentActivePointIndex);

            // EMIT THE FIDUCIALS/COLORS TO THE USER
            emitFiducialsChanged(fiducialList.count());
            emitFiducialsChanged(fiducialList);
            emitFiducialsChanged(fiducialList, colorsList);
        }
        update();
    } else {
        // LET THE UNDERLYING CLASS HANDLE THE EVENT
        LAU3DScanGLWidget::keyPressEvent(event);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::wheelEvent(QWheelEvent *event)
{
    // IGNORE WHEEL EVENTS IF WE ARE IN DRAG MODE
    if (fiducialDragMode == false) {
        // LET THE UNDERLYING CLASS HANDLE THE EVENT
        LAU3DScanGLWidget::wheelEvent(event);

        // UPDATE THE FIDUCIAL PROJECTION MATRIX IN CASE THE PROJECTION MATRIX CHANGED
        updateFiducialProjectionMatrix();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::initializeGL()
{
    LAU3DScanGLWidget::initializeGL();

    // CREATE THE VERTEX BUFFER TO HOLD THE XYZ COORDINATES PLUS THE
    // TEXTURE COORDINATES (5 FLOATS PER VERTEX, 4 VERTICES PER SIDE, 6 SIDES)
    fiducialVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    fiducialVertexBuffer.create();
    fiducialVertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (fiducialVertexBuffer.bind()) {
        fiducialVertexBuffer.allocate(120 * sizeof(float));
        float *vertices = (float *)fiducialVertexBuffer.map(QOpenGLBuffer::WriteOnly);
        int i = 0;
        if (vertices) {
            // TOP/BOTTOM SURFACES
            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = 0;
            vertices[i++] = 0;
            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = 1;
            vertices[i++] = 0;
            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = 1;
            vertices[i++] = 1;
            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = 0;
            vertices[i++] = 1;

            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = 0;
            vertices[i++] = 1;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = 1;
            vertices[i++] = 1;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = 1;
            vertices[i++] = 0;
            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = 0;
            vertices[i++] = 0;

            // LEFT/RIGHT SURFACES
            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = 0;
            vertices[i++] = 0;
            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = 0;
            vertices[i++] = 1;
            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = 1;
            vertices[i++] = 1;
            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = 1;
            vertices[i++] = 0;

            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = 0;
            vertices[i++] = 0;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = 0;
            vertices[i++] = 1;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = 1;
            vertices[i++] = 1;
            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = 1;
            vertices[i++] = 0;

            // FRONT/BACK SURFACES
            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = 0;
            vertices[i++] = 0;
            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = 1;
            vertices[i++] = 0;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = 1;
            vertices[i++] = 1;
            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = 0;
            vertices[i++] = 1;

            vertices[i++] = -1;
            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = 0;
            vertices[i++] = 0;
            vertices[i++] = +1;
            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = 1;
            vertices[i++] = 0;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = 1;
            vertices[i++] = 1;
            vertices[i++] = -1;
            vertices[i++] = +1;
            vertices[i++] = +1;
            vertices[i++] = 0;
            vertices[i++] = 1;
            fiducialVertexBuffer.unmap();
        } else {
            qDebug() << QString("fiducialVertexBuffer buffer mapped from GPU.");
        }
        fiducialVertexBuffer.release();
    }

    // CREATE THE FIDUCIAL INDEX BUFFER
    fiducialIndiceBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    fiducialIndiceBuffer.create();
    fiducialIndiceBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (fiducialIndiceBuffer.bind()) {
        fiducialIndiceBuffer.allocate(6 * 6 * sizeof(unsigned int));
        unsigned int *indices = (unsigned int *)fiducialIndiceBuffer.map(QOpenGLBuffer::WriteOnly);
        int i = 0;
        if (indices) {
            for (unsigned int j = 0; j < 6; j++) {
                indices[i++] = 4 * j + 0;
                indices[i++] = 4 * j + 1;
                indices[i++] = 4 * j + 2;
                indices[i++] = 4 * j + 0;
                indices[i++] = 4 * j + 2;
                indices[i++] = 4 * j + 3;
            }
            fiducialIndiceBuffer.unmap();
        } else {
            qDebug() << QString("No fiducialIndiceBuffer mapped from GPU.");
        }
        fiducialIndiceBuffer.release();
    }

    // CREATE TEXTURES FOR FIDUCIAL CUBES
    for (unsigned int n = 0; n < 26; n++) {
        QImage image(QString(":/Fiducials/letter%1.tif").arg(char(65 + n)));
        if (image.isNull()){
            fiducialTextures[n] = new QOpenGLTexture(QOpenGLTexture::Target2D);
        } else {
            fiducialTextures[n] = new QOpenGLTexture(image.flipped(Qt::Horizontal | Qt::Vertical));
            fiducialTextures[n]->setWrapMode(QOpenGLTexture::ClampToBorder);
            fiducialTextures[n]->setMinificationFilter(QOpenGLTexture::Nearest);
            fiducialTextures[n]->setMagnificationFilter(QOpenGLTexture::Nearest);
        }
    }

    // COMPILE THE SHADER LANGUAGE PROGRAM FOR DRAWING FIDUCIALS
    setlocale(LC_NUMERIC, "C");
    if (!fiducialProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/MISC/drawFiducials.vert")) {
        close();
    } else if (!fiducialProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/MISC/drawFiducials.frag")) {
        close();
    } else if (!fiducialProgram.link()) {
        close();
    }
    setlocale(LC_ALL, "");

    // MAKE SURE TO INITIALIZE THE FIDUCIAL PROJECTION MATRIX
    updateFiducialProjectionMatrix();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAU3DFiducialGLWidget::paintGL()
{
    // UPDATE THE SYMMETRY MATRIX IF WE HAVE ENOUGH FIDUCIALS
    if (fiducialList.count() > 2) {
        setSymmetryTransform(symmetry());
    }

    // SET THE BACKGROUND TO BLACK FOR SANDBOX MODE
    if (sandboxEnabled()){
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // CALLS THE BASECLASSES PAINT METHOD IN ORDER TO DISPLAY THE LAU SCAN SURFACES
    LAU3DScanGLWidget::paintGL();

    // SEE IF WE SHOULD CLEAR THE BUFFER WHEN THERE IS A TEXTURE TO DISPLAY
    if (sandboxEnabled() && textureEnabled()){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // CREATE A LOCAL PROJECTION TRANSFORM
    QMatrix4x4 localTransform = sandboxEnabled() ? projection * scanTransform() : projection;

    // BIND THE FIDUCIAL PROGRAM SO THAT WE CAN NOW DRAW THE FIDUCIALS FOR THE SCAN
    if (enableFiducialFlag && fiducialList.count() > 0 && fiducialProgram.bind()) {
        // BIND THE FIDUCIAL VERTEX BUFFER
        if (fiducialVertexBuffer.bind()) {
            // BIND THE FIDUCIAL INDICE BUFFER
            if (fiducialIndiceBuffer.bind()) {
                // ENABLE THE ATTRIBUTE ARRAY
                glVertexAttribPointer(fiducialProgram.attributeLocation("qt_vertexA"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
                fiducialProgram.enableAttributeArray("qt_vertexA");

                // ENABLE THE ATTRIBUTE ARRAY
                glVertexAttribPointer(fiducialProgram.attributeLocation("qt_vertexB"), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const void *)(3 * sizeof(float)));
                fiducialProgram.enableAttributeArray("qt_vertexB");

                // DERIVE THE PROJECTION MATRIX
                fiducialProgram.setUniformValue("qt_projection", localTransform);

                // ITERATE THROUGH THE FIDUCIAL, LIST DRAWING EACH ONE AT A TIME
                for (int n = 0; n < fiducialList.count(); n++) {
                    // MAKE SURE WE AREN'T TRYING TO DISPLAY A NAN POINT
                    if (qIsNaN(fiducialList.at(n).x())) {
                        continue;
                    }

                    // BIND THE FIDUCIAL TEXTURE
                    glActiveTexture(GL_TEXTURE1);
                    fiducialTextures[n % 26]->bind();

                    if (n == currentActivePointIndex) {
                        // SET THE COLOR INVERSE TRANSFORM MATRIX
                        fiducialProgram.setUniformValue("qt_colorMat", QMatrix4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
                    } else {
                        // SET THE COLOR IDENTITY TRANSFORM MATRIX
                        fiducialProgram.setUniformValue("qt_colorMat", QMatrix4x4());
                    }

                    // PASS THE HANDLE TO OUR FRAME BUFFER OBJECT'S TEXTURE
                    fiducialProgram.setUniformValue("qt_texture", 1);

#ifdef DONTCOMPILE
                    // CALCULATE THE POSITION OF THE SCENE FOCAL POINT
                    QVector4D pointA = fiducialProjection * QVector4D(fiducialList.at(n).x(), fiducialList.at(n).y(), fiducialList.at(n).z(), 1.0f);
                    pointA = pointA / pointA.w();
                    QVector4D pointB = fiducialProjection.inverted() * QVector4D(fiducialRadius, 0.0f, pointA.z(), pointA.w());
                    pointB = pointB / pointB.w();
#else
                    bool okay = true;
                    QMatrix4x4 matrix = localTransform.inverted(&okay);
                    if (okay) {
                        // GET THE SCREEN COORDINATE BY APPLYING THE LOCAL TRANSFORM MATRIX
                        QVector4D cntPt = localTransform * QVector4D(fiducialList.at(n).x(), fiducialList.at(n).y(), fiducialList.at(n).z(), 1.0f);

                        // APPLY THE SANDBOX TRANSFORM IF IN SANDBOX MODE
                        if (sandboxEnabled()){
                            cntPt.setX(cntPt.x() / cntPt.z());
                            cntPt.setY(cntPt.y() / cntPt.z());
                            cntPt.setZ(cntPt.w());
                            cntPt.setW(1.0);

                            QVector4D lftPt = localTransform * QVector4D(fiducialList.at(n).x() + 1, fiducialList.at(n).y(), fiducialList.at(n).z(), 1.0f);
                            lftPt.setX(lftPt.x() / lftPt.z());
                            lftPt.setY(lftPt.y() / lftPt.z());
                            lftPt.setZ(lftPt.w());
                            lftPt.setW(1.0);

                            // GET DISTANCE BETWEEN CENTER AND LEFT POINT IN UNITS OF PIXELS
                            double delta = qAbs(lftPt.x() - cntPt.x()) * (double)this->width();

                            // GET THE SCALE FACTOR NECESSARY TO SCALE DELTA TO 5.0 PIXELS
                            fiducialRadius = 7.0/delta;
                        } else {
                            cntPt = cntPt / cntPt.w();

                            // CHOOSE A RADIUS EQUAL TO FIVE PIXELS ON THE DISPLAY
                            float rds = 5.0f / (float)this->width();

                            QVector4D lftPt = matrix * QVector4D(cntPt.x() - rds, cntPt.y(), cntPt.z(), 1.0);
                            lftPt = lftPt / lftPt.w();

                            QVector4D rghPt = matrix * QVector4D(cntPt.x() + rds, cntPt.y(), cntPt.z(), 1.0);
                            rghPt = rghPt / rghPt.w();

                            // CALCULATE THE FIDUCIAL RADIUS AS A FUNCTION OF THE DISPLAY DPI
                            fiducialRadius = lftPt.toVector3D().distanceToPoint(rghPt.toVector3D());
                        }
                    }
#endif
                    if (sandboxEnabled()){
                        fiducialProgram.setUniformValue("qt_arg", (int)2);
                    } else {
                        fiducialProgram.setUniformValue("qt_arg", (int)0);
                    }

                    // SPECIFY THE SIZE OF THE FRAME BUFFER OBJECT'S TEXTURE
                    fiducialProgram.setUniformValue("qt_radius", (float)fiducialRadius); //qFabs(pointB.x()));

                    // SET FIDUCIAL COORDINATE
                    fiducialProgram.setUniformValue("qt_fiducial", fiducialList.at(n));

                    // DRAW THE ELEMENTS
                    glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, nullptr);

                    // RELEASE THE TEXTURE BUFFER
                    fiducialTextures[n % 26]->release();
                }
                // RELEASE THE INDICE BUFFER
                fiducialIndiceBuffer.release();
            }
            // RELEASE THE VERTEX BUFFER
            fiducialVertexBuffer.release();
        }
        // RELEASE THE PROGRAM
        fiducialProgram.release();
    }
}
