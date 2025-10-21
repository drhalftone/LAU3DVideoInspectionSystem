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

#ifndef LAUMERGEDOCUMENTSWIDGET_H
#define LAUMERGEDOCUMENTSWIDGET_H

#include <QWidget>
#include <QtGlobal>
#include <QGroupBox>
#include <QGroupBox>
#include <QComboBox>
#include <QFormLayout>
#include <QPushButton>
#include <QOpenGLBuffer>
#include <QDoubleSpinBox>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QDialogButtonBox>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>

#include "laudocument.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUMergeColorsWidget : public QWidget
{
    Q_OBJECT

public:
    LAUMergeColorsWidget(LAUVideoPlaybackColor mstColor, LAUVideoPlaybackColor slvColor, QWidget *parent = nullptr);

    QStringList outputStrings();

public slots:
    void onOutputColorComboBoxChanged(int index);

private:
    LAUVideoPlaybackColor masterColor;
    LAUVideoPlaybackColor slaveColor;
    LAUVideoPlaybackColor outputColor;

    QComboBox *outputColorComboBox;

    QComboBox *aChannelComboBox;
    QComboBox *bChannelComboBox;
    QComboBox *cChannelComboBox;
    QComboBox *dChannelComboBox;
    QComboBox *eChannelComboBox;
    QComboBox *fChannelComboBox;
    QComboBox *gChannelComboBox;
    QComboBox *hChannelComboBox;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUMergeColorsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUMergeColorsDialog(LAUVideoPlaybackColor mstColor, LAUVideoPlaybackColor slvColor, QWidget *parent = nullptr) : QDialog(parent)
    {
        widget = new LAUMergeColorsWidget(mstColor, slvColor);

        this->setLayout(new QVBoxLayout());
        this->setWindowTitle("Merge Scan Dialog");
#ifdef Q_OS_WIN
        this->layout()->setContentsMargins(6, 6, 6, 6);
#else
        this->layout()->setContentsMargins(6, 6, 6, 6);
#endif
        this->layout()->addWidget(widget);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
        connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
        this->layout()->addWidget(buttonBox);
    }

    QStringList outputStrings()
    {
        if (widget) {
            return (widget->outputStrings());
        }
        return QStringList();
    }

protected:
    void showEvent(QShowEvent *event)
    {
        Q_UNUSED(event);
        this->setFixedSize(this->size());
    }

    void accept()
    {
        QDialog::accept();
    }

private:
    LAUMergeColorsWidget *widget;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUMergeDocumentsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LAUMergeDocumentsWidget(LAUDocument mstDoc = LAUDocument(), LAUDocument slvDoc = LAUDocument(), QWidget *parent = nullptr);

    LAUDocument mergeResult();
    bool readyToMerge(QString *string = nullptr);

    LAUVideoPlaybackColor masterColor() const
    {
        return (mstDocument.color());
    }

    LAUVideoPlaybackColor slaveColor() const
    {
        return (slvDocument.color());
    }

public slots:
    void onSetMasterScan();
    void onSetSlaveScan();

private:
    LAUDocument mstDocument;
    LAUDocument slvDocument;

    QGroupBox *masterBox;
    QGroupBox *slaveBox;

    QLineEdit *masterLineEdit;
    QLineEdit *slaveLineEdit;

    QDoubleSpinBox *minXSpinBox;
    QDoubleSpinBox *maxXSpinBox;
    QDoubleSpinBox *minYSpinBox;
    QDoubleSpinBox *maxYSpinBox;
    QDoubleSpinBox *dpiSpinBox;

    void updateXYLimits();
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUMergeDocumentsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUMergeDocumentsDialog(LAUDocument mstDoc = LAUDocument(), LAUDocument slvDoc = LAUDocument(), QWidget *parent = nullptr) : QDialog(parent), result(LAUDocument())
    {
        widget = new LAUMergeDocumentsWidget(mstDoc, slvDoc);

        this->setLayout(new QVBoxLayout());
        this->setWindowTitle("Merge Scan Dialog");
#ifdef Q_OS_WIN
        this->layout()->setContentsMargins(6, 6, 6, 6);
#else
        this->layout()->setContentsMargins(6, 6, 6, 6);
#endif
        this->layout()->addWidget(widget);
        ((QVBoxLayout *)(this->layout()))->addStretch(2);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
        connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
        this->layout()->addWidget(buttonBox);
    }

    LAUDocument mergeResult()
    {
        return (result);
    }

protected:
    void showEvent(QShowEvent *event)
    {
        Q_UNUSED(event);
        this->setFixedSize(this->size());
    }

    void accept()
    {
        if (widget) {
            result = widget->mergeResult();
            if (result.isValid()) {
                QDialog::accept();
            }
        }
    }

private:
    LAUDocument result;
    LAUMergeDocumentsWidget *widget;
};

#endif // LAUMERGEDOCUMENTSWIDGET_H
