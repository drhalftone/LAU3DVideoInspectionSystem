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

#ifndef LAU3DFIDUCIALGLWIDGET_H
#define LAU3DFIDUCIALGLWIDGET_H

#include <QPen>
#include <QList>
#include <QBrush>
#include <QImage>
#include <QLabel>
#include <QWidget>
#include <QDialog>
#include <QString>
#include <QVector3D>
#include <QPainter>
#include <QFileInfo>
#include <QKeyEvent>
#include <QSettings>
#include <QTransform>
#include <QTabWidget>
#include <QClipboard>
#include <QMessageBox>
#include <QTextStream>
#include <QFileDialog>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QStandardPaths>
#include <QTableWidgetItem>
#include <QDialogButtonBox>
#include <QAbstractItemView>

#include "lauscan.h"
#include "lau3dscanglwidget.h"

using namespace LAU3DVideoParameters;

class LAUFiducialLabel;
class LAUFiducialPoint;
class LAU3DFiducialWidget;
class LAUFiducialDialog;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUFiducialTool : public QWidget
{
    Q_OBJECT

public:
    explicit LAUFiducialTool(QWidget *parent = nullptr);

public slots:
    void onSetCurrentIndex(int r)
    {
        table->setCurrentCell(r, 0);
    }

    void onSetCurrentIndex(int r, int c, int rp, int cp)
    {
        Q_UNUSED(c);
        Q_UNUSED(rp);
        Q_UNUSED(cp);
        emit emitCurrentIndex(r);
    }

    void onFiducialsChanged(QVector3D point, int index, QVector3D color);
    void onFiducialsChanged(QList<QVector3D> points, QList<QVector3D> colors);

protected:
    void keyPressEvent(QKeyEvent *event);
    void closeEvent(QCloseEvent *event) override;

private:
    QList<QVector3D> fiducialList;
    QTableWidget *table;

signals:
    void emitCurrentIndex(int n);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUFiducialDistanceTool : public QWidget
{
    Q_OBJECT

public:
    explicit LAUFiducialDistanceTool(QWidget *parent = nullptr);

public slots:
    void onFiducialsChanged(QVector3D point, int index, QVector3D color);
    void onFiducialsChanged(QList<QVector3D> points, QList<QVector3D> colors);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QList<QVector3D> fiducialList;
    QTableWidget *table;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DFiducialGLWidget : public LAU3DScanGLWidget
{
    Q_OBJECT

public:
    LAU3DFiducialGLWidget(LAUScan scan, QWidget *parent = nullptr);
    LAU3DFiducialGLWidget(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent = nullptr);
    ~LAU3DFiducialGLWidget();

    QList<QVector3D> fiducials() const
    {
        return (fiducialList);
    }

    QList<QVector3D> colors() const
    {
        return (colorsList);
    }

    QList<QPoint> rowColumns() const
    {
        return (rowColumnList);
    }

    QMatrix4x4 symmetry() const;

public slots:
    void onKeyPressEvent(QKeyEvent *event)
    {
        keyPressEvent(event);
    }

    void onEnableFiducials(bool state)
    {
        action->setChecked(state);
        enableFiducialFlag = state;
        if (tool) {
            if (state) {
                if (sandboxEnabled() == false){
                    if (distanceTool){
                        distanceTool->show();
                    }
                    tool->show();
                }
            } else {
                if (distanceTool){
                    distanceTool->hide();
                }
                tool->hide();
            }
        }
        update();
    }

    void onSetFiducials(QList<QVector3D> fiducials, QList<QVector3D> colors)
    {
        fiducialList = fiducials;
        colorsList = colors;
        emitFiducialsChanged(fiducialList, colorsList);
        updateFiducialProjectionMatrix();
        update();
    }

    void onSetFiducials(QList<QVector3D> fiducials)
    {
        fiducialList = fiducials;
        while (colorsList.count() < fiducialList.count()) {
            colorsList << QVector3D(0.0, 0.0, 0.0);
        }
        emitFiducialsChanged(fiducialList, colorsList);
        updateFiducialProjectionMatrix();
        update();
    }

    void onSetFiducials(QList<QPoint> rowColumns);
    void onSetFiducials(QPoint point, int index);
    void onSetFiducials(QVector3D point, int index);

    void onSetActivePointIndex(int n)
    {
        if (currentActivePointIndex != n) {
            currentActivePointIndex = n;
            emit emitActivePointIndexChanged(currentActivePointIndex);
            update();
        }
    }

protected:
    void updateFiducialProjectionMatrix();

    void showEvent(QShowEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);

    void resizeGL(int w, int h)
    {
        LAU3DScanGLWidget::resizeGL(w, h);
        updateFiducialProjectionMatrix();
    }
    void initializeGL();
    void paintGL();

    QAction *action;
    LAUFiducialTool *tool;
    LAUFiducialDistanceTool *distanceTool;
    LAUScan localScan;

    QOpenGLBuffer fiducialVertexBuffer, fiducialIndiceBuffer;
    QOpenGLShaderProgram fiducialProgram;
    QOpenGLTexture *fiducialTextures[26];
    QList<QPoint> rowColumnList;
    QList<QVector3D> fiducialList;
    QList<QVector3D> colorsList;

    float fiducialRadius;
    bool fiducialDragMode;
    bool enableFiducialFlag;
    QMatrix4x4 fiducialProjection;
    int maxNumberFiducials;
    int currentActivePointIndex;
    LAUMemoryObject rowColumnMap;
    LAUMemoryObject screenMap;
    LAUMemoryObject colorMap;

signals:
    void emitActivePointIndexChanged(int);
    void emitFiducialsChanged(int);
    void emitFiducialsChanged(QPoint, int);
    void emitFiducialsChanged(QVector3D, int);
    void emitFiducialsChanged(QList<QPoint>);
    void emitFiducialsChanged(QList<QVector3D>);
    void emitFiducialsChanged(QVector3D, int, QVector3D);
    void emitFiducialsChanged(QList<QVector3D>, QList<QVector3D>);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUFiducialPoint
{
public:
    LAUFiducialPoint(int col = 0, int row = 0, float xi = 0.0f, float yi = 0.0f, float zi = 0.0f, float ri = 0.0f, float gi = 0.0f, float bi = 0.0f, QString str = QString()) : cc(col), rr(row), xp(xi), yp(yi), zp(zi), rp(ri), gp(gi), bp(bi), string(str) { ; }
    LAUFiducialPoint(const LAUFiducialPoint &other)
    {
        rr = other.rr;
        cc = other.cc;
        xp = other.xp;
        yp = other.yp;
        zp = other.zp;
        rp = other.rp;
        gp = other.gp;
        bp = other.bp;
        string = other.string;
    }
    LAUFiducialPoint &operator = (const LAUFiducialPoint &other)
    {
        if (this != &other) {
            rr = other.rr;
            cc = other.cc;
            xp = other.xp;
            yp = other.yp;
            zp = other.zp;
            rp = other.rp;
            gp = other.gp;
            bp = other.bp;
            string = other.string;
        }
        return (*this);
    }

    int row() const
    {
        return (rr);
    }

    int col() const
    {
        return (cc);
    }

    float x() const
    {
        return (xp);
    }

    float y() const
    {
        return (yp);
    }

    float z() const
    {
        return (zp);
    }

    float r() const
    {
        return (rp);
    }

    float g() const
    {
        return (gp);
    }

    float b() const
    {
        return (bp);
    }

    bool isValid() const
    {
        return ((qIsNaN(xp) | qIsNaN(yp) | qIsNaN(zp)) == false);
    }

    QVector3D point() const
    {
        return (QVector3D(xp, yp, zp));
    }

    QVector3D color() const
    {
        return (QVector3D(rp, gp, bp));
    }

    QString label() const
    {
        return (string);
    }

    void setRow(int rp)
    {
        rr = rp;
    }

    void setCol(int cp)
    {
        cc = cp;
    }

    void setX(float xi)
    {
        xp = xi;
    }

    void setY(float yi)
    {
        yp = yi;
    }

    void setZ(float zi)
    {
        zp = zi;
    }

    void setR(float ri)
    {
        rp = ri;
    }

    void setG(float gi)
    {
        gp = gi;
    }

    void setB(float bi)
    {
        bp = bi;
    }

    void setLabel(QString str)
    {
        string = str;
    }

    void saveTo(QTextStream *stream) const
    {
        *stream << string << QString(",") << cc << QString(",") << rr << QString(",") << xp << QString(",") << yp << QString(",") << zp << QString(",") << rp << QString(",") << gp << QString(",") << bp << QString("\n");
    }

    void loadFrom(QTextStream *stream)
    {
        QStringList strings = stream->readLine().split(",");
        string = strings.takeFirst();
        cc = strings.takeFirst().toInt();
        rr = strings.takeFirst().toInt();
        xp = strings.takeFirst().toFloat();
        yp = strings.takeFirst().toFloat();
        zp = strings.takeFirst().toFloat();
        rp = strings.takeFirst().toFloat();
        gp = strings.takeFirst().toFloat();
        bp = strings.takeFirst().toFloat();
    }

private:
    int cc, rr;
    float xp, yp, zp;
    float rp, gp, bp;
    QString string;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DFiducialWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LAU3DFiducialWidget(LAUScan scan = LAUScan(), QWidget *parent = nullptr);
    ~LAU3DFiducialWidget();

    void save(QString filename = QString());
    void load(QString filename = QString());

    void displayScan(QWidget *parent = nullptr);

    QMatrix4x4 symmetry() const
    {
        return (scanWidget->symmetry());
    }

    QList<QVector3D> points() const;
    QList<QVector3D> colors() const;
    QList<LAUFiducialPoint> fiducials() const
    {
        return (pointList);
    }

    LAUScan scan() const
    {
        return (localScan);
    }

    static QString lastDirectoryString;

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    QTableWidget *table;
    QString filenameString;
    QString scanFileString;
    QToolButton *newButton;
    QToolButton *delButton;
    QToolButton *upButton;
    QToolButton *dwnButton;
    LAUFiducialLabel *fiducialLabel;
    LAU3DFiducialGLWidget *scanWidget;
    LAUScan localScan;

    QList<LAUFiducialPoint> pointList;

private slots:
    void onAddItem(int col = -1, int row = -1);
    void onDeleteItem();
    void onMoveUpItem();
    void onMoveDownItem();

    void onUpdatePoint(QString label, int col, int row);

signals:
    void emitUpdate();
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUFiducialLabel : public QLabel
{
    Q_OBJECT

public:
    explicit LAUFiducialLabel(QImage img = QImage(), QWidget *parent = nullptr) : QLabel(parent)
    {
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setImage(img);
    }
    ~LAUFiducialLabel() { ; }

    void setImage(QImage in)
    {
        image = in;
        this->setFixedSize(image.width(), image.height());
    }

    int height() const
    {
        return (image.height());
    }

    int width() const
    {
        return (image.width());
    }

    QRgb pixel(int col, int row) const
    {
        return (image.pixel(col, row));
    }

public slots:
    void updatePoint(LAUFiducialPoint point);

    void setCurrentPoint(int currentRow, int currentColumn = 0, int previousRow = 0, int previousColumn = 0)
    {
        Q_UNUSED(currentColumn);
        Q_UNUSED(previousRow);
        Q_UNUSED(previousColumn);
        currentActivePointIndex = currentRow;
        update();
    }

    void setPointList(QList<LAUFiducialPoint> list)
    {
        pointList = list;
        update();
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent *event)
    {
        emit emitDoubleClick(event->pos().x(), event->pos().y());
    }
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
    void paintEvent(QPaintEvent *);

private:
    QImage image;
    bool buttonDownFlag;
    int currentActivePointIndex;
    QList<LAUFiducialPoint> pointList;

signals:
    void emitDoubleClick(int col, int row);
    void emitPointMoved(QString label, int col, int row);
    void emitCurrentPointChanged(int);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUFiducialDialog : public QDialog
{
    Q_OBJECT

public:
    LAUFiducialDialog(LAUScan scan, QWidget *parent = nullptr) : QDialog(parent)
    {
        widget = new LAU3DFiducialWidget(scan, this);
        this->setWindowTitle(widget->windowTitle());

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
        connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));

        this->setLayout(new QVBoxLayout());
#ifdef Q_OS_WIN
        this->layout()->setContentsMargins(6, 6, 6, 6);
#else
        this->layout()->setContentsMargins(6, 6, 6, 6);
#endif
        this->layout()->setSpacing(6);
        this->layout()->addWidget(widget);
        ((QVBoxLayout *)this->layout())->addStretch();
        this->layout()->addWidget(buttonBox);
    }
    ~LAUFiducialDialog() { ; }

protected:
    void accept()
    {
        if (QMessageBox::warning(this, QString("Fiducial Widget"), QString("Save file to disk?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            widget->save();
        }
        QDialog::accept();
    }
    void reject()
    {
        QDialog::reject();
    }

private:
    LAU3DFiducialWidget *widget;
};

#endif // LAU3DFIDUCIALGLWIDGET_H
