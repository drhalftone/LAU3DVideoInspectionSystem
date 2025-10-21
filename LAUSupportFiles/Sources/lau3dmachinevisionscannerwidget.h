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

#ifndef LAU3DMACHINEVISIONSCANNERWIDGET_H
#define LAU3DMACHINEVISIONSCANNERWIDGET_H

#include <QDebug>
#include <QLabel>
#include <QSlider>
#include <QWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QSettings>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDoubleSpinBox>

#include "laumemoryobject.h"

class LAU3DMachineVisionScannerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LAU3DMachineVisionScannerWidget(QWidget *parent = nullptr, bool hasDepth = true, LAUVideoPlaybackDevice device = DeviceUndefined) : QWidget(parent), exposure(0), snrThreshold(0), mtnThreshold(0), expSlider(nullptr), snrSlider(nullptr), mtnSlider(nullptr), expSpinBox(nullptr), snrSpinBox(nullptr), mtnSpinBox(nullptr), sharpenCheckBox(nullptr)
    {
        this->setWindowFlags(Qt::Tool);
        this->setWindowTitle(QString("Camera Settings"));
        this->setLayout(new QGridLayout());
#ifdef Q_OS_WIN
        this->layout()->setContentsMargins(6, 6, 6, 6);
#else
        this->layout()->setContentsMargins(6, 6, 6, 6);
#endif
        this->layout()->setSpacing(3);
        this->setFixedSize(435, 90);

        QSettings settings;
        exposure     = settings.value(QString("LAU3DMachineVisionScannerWidget::exposure"), 5000).toInt();
        snrThreshold = settings.value(QString("LAU3DMachineVisionScannerWidget::snrThreshold"), 10).toInt();
        mtnThreshold = settings.value(QString("LAU3DMachineVisionScannerWidget::mtnThreshold"), 990).toInt();

        expSlider = new QSlider(Qt::Horizontal);
        expSlider->setMinimum(1);
        expSlider->setMaximum(2000000);
        expSlider->setValue(exposure);

        expSpinBox = new QSpinBox();
        expSpinBox->setFixedWidth(80);
        expSpinBox->setAlignment(Qt::AlignRight);
        expSpinBox->setMinimum(1);
        expSpinBox->setMaximum(2000000);
        expSpinBox->setValue(exposure);

        connect(expSlider, SIGNAL(valueChanged(int)), expSpinBox, SLOT(setValue(int)));
        connect(expSpinBox, SIGNAL(valueChanged(int)), expSlider, SLOT(setValue(int)));
        connect(expSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onUpdateExposurePrivate(int)));

        QLabel *label = new QLabel(QString("Exposure"));
        label->setToolTip(QString("exposure time of the camera in microseconds"));
        (reinterpret_cast<QGridLayout *>(this->layout()))->addWidget(label, 0, 0, 1, 1, Qt::AlignRight);
        (reinterpret_cast<QGridLayout *>(this->layout()))->addWidget(expSlider, 0, 1, 1, 1);
        (reinterpret_cast<QGridLayout *>(this->layout()))->addWidget(expSpinBox, 0, 2, 1, 1);

        if (hasDepth) {
            snrSlider = new QSlider(Qt::Horizontal);
            snrSlider->setMinimum(0);
            snrSlider->setMaximum(1000);
            snrSlider->setValue(snrThreshold);

            snrSpinBox = new QSpinBox();
            snrSpinBox->setFixedWidth(80);
            snrSpinBox->setAlignment(Qt::AlignRight);
            snrSpinBox->setMinimum(0);
            snrSpinBox->setMaximum(1000);
            snrSpinBox->setValue(snrThreshold);

            connect(snrSlider, SIGNAL(valueChanged(int)), snrSpinBox, SLOT(setValue(int)));
            connect(snrSpinBox, SIGNAL(valueChanged(int)), snrSlider, SLOT(setValue(int)));
            connect(snrSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onUpdateSnrThresholdPrivate(int)));

            label = new QLabel(QString("SNR Threshold"));
            label->setToolTip(QString("required streng of the scan signal"));
            (reinterpret_cast<QGridLayout *>(this->layout()))->addWidget(label, 1, 0, 1, 1, Qt::AlignRight);
            (reinterpret_cast<QGridLayout *>(this->layout()))->addWidget(snrSlider, 1, 1, 1, 1);
            (reinterpret_cast<QGridLayout *>(this->layout()))->addWidget(snrSpinBox, 1, 2, 1, 1);

            mtnSlider = new QSlider(Qt::Horizontal);
            mtnSlider->setMinimum(0);
            mtnSlider->setMaximum(1000);
            mtnSlider->setValue(mtnThreshold);

            mtnSpinBox = new QSpinBox();
            mtnSpinBox->setFixedWidth(80);
            mtnSpinBox->setAlignment(Qt::AlignRight);
            mtnSpinBox->setMinimum(0);
            mtnSpinBox->setMaximum(1000);
            mtnSpinBox->setValue(mtnThreshold);

            connect(mtnSlider, SIGNAL(valueChanged(int)), mtnSpinBox, SLOT(setValue(int)));
            connect(mtnSpinBox, SIGNAL(valueChanged(int)), mtnSlider, SLOT(setValue(int)));
            connect(mtnSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onUpdateMtnThresholdPrivate(int)));

            label = new QLabel(QString("MTN Threshold"));
            label->setToolTip(QString("amount of motion you can tolerate"));
            (reinterpret_cast<QGridLayout *>(this->layout()))->addWidget(label, 2, 0, 1, 1, Qt::AlignRight);
            (reinterpret_cast<QGridLayout *>(this->layout()))->addWidget(mtnSlider, 2, 1, 1, 1);
            (reinterpret_cast<QGridLayout *>(this->layout()))->addWidget(mtnSpinBox, 2, 2, 1, 1);
        } else {
            this->setFixedSize(435, 30);
        }

        // ADD SHARPEN FILTER CHECKBOX FOR SEEK THERMAL CAMERAS
        if (device == DeviceSeek) {
            sharpenCheckBox = new QCheckBox(QString("Sharpen Filter"));
            sharpenCheckBox->setToolTip(QString("Enable edge sharpening for better detail visibility"));
            sharpenCheckBox->setChecked(settings.value(QString("LAU3DMachineVisionScannerWidget::sharpenFilter"), true).toBool());
            
            connect(sharpenCheckBox, SIGNAL(toggled(bool)), this, SLOT(onUpdateSharpenFilterPrivate(bool)));
            
            (reinterpret_cast<QGridLayout *>(this->layout()))->addWidget(sharpenCheckBox, hasDepth ? 3 : 1, 1, 1, 2);
            this->setFixedSize(435, hasDepth ? 115 : 60);
        }
    }

    ~LAU3DMachineVisionScannerWidget()
    {
        QSettings settings;
        settings.setValue(QString("LAU3DMachineVisionScannerWidget::exposure"), exposure);
        settings.setValue(QString("LAU3DMachineVisionScannerWidget::snrThreshold"), snrThreshold);
        settings.setValue(QString("LAU3DMachineVisionScannerWidget::mtnThreshold"), mtnThreshold);
        if (sharpenCheckBox) {
            settings.setValue(QString("LAU3DMachineVisionScannerWidget::sharpenFilter"), sharpenCheckBox->isChecked());
        }

        qDebug() << "LAU3DMachineVisionScannerWidget::~LAU3DMachineVisionScannerWidget()";
    }

    void enableSNRWidget(bool state)
    {
        if (snrSlider){
            snrSlider->setEnabled(state);
        }
        if (snrSpinBox){
            snrSpinBox->setEnabled(state);
        }
    }

    void enableMTNWidget(bool state)
    {
        if (mtnSlider){
            mtnSlider->setEnabled(state);
        }
        if (mtnSpinBox){
            mtnSpinBox->setEnabled(state);
        }
    }

    void setExp(int val)
    {
        if (expSpinBox) {
            expSpinBox->setValue(val);
        }
    }

    void setSnr(int val)
    {
        if (snrSpinBox) {
            snrSpinBox->setValue(val);
        }
    }

    void setMtn(int val)
    {
        if (mtnSpinBox) {
            mtnSpinBox->setValue(val);
        }
    }

    int exp()
    {
        return (exposure);
    }

    int snr()
    {
        return (snrThreshold);
    }

    int mtn()
    {
        return (mtnThreshold);
    }

public slots:
    void onUpdateExposure(int val)
    {
        if (val != exposure) {
            exposure = val;
            if (expSpinBox) {
                expSpinBox->setValue(exposure);
            }
        }
    }

    void onUpdateSnrThreshold(int val)
    {
        if (val != snrThreshold) {
            snrThreshold = val;
            if (snrSpinBox) {
                snrSpinBox->setValue(snrThreshold);
            }
        }
    }

    void onUpdateMtnThreshold(int val)
    {
        if (val != mtnThreshold) {
            mtnThreshold = val;
            if (mtnSpinBox) {
                mtnSpinBox->setValue(mtnThreshold);
            }
        }
    }

private slots:
    void onUpdateExposurePrivate(int val)
    {
        if (val != exposure) {
            exposure = val;
            emit emitUpdateExposure(exposure);
        }
    }

    void onUpdateSnrThresholdPrivate(int val)
    {
        if (val != snrThreshold) {
            snrThreshold = val;
            emit emitUpdateSnrThreshold(snrThreshold);
        }
    }

    void onUpdateMtnThresholdPrivate(int val)
    {
        if (val != mtnThreshold) {
            mtnThreshold = val;
            emit emitUpdateMtnThreshold(mtnThreshold);
        }
    }

    void onUpdateSharpenFilterPrivate(bool state)
    {
        emit emitUpdateSharpenFilter(state);
    }

private:
    int exposure;
    int snrThreshold;
    int mtnThreshold;

    QSlider *expSlider;
    QSlider *snrSlider;
    QSlider *mtnSlider;

    QSpinBox *expSpinBox;
    QSpinBox *snrSpinBox;
    QSpinBox *mtnSpinBox;
    
    QCheckBox *sharpenCheckBox;

signals:
    void emitUpdateExposure(int val);
    void emitUpdateSnrThreshold(int val);
    void emitUpdateMtnThreshold(int val);
    void emitUpdateSharpenFilter(bool state);
};

#endif // LAU3DMACHINEVISIONSCANNERWIDGET_H
