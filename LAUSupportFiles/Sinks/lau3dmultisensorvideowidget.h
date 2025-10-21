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

#ifndef LAU3DMULTISENSORVIDEOWIDGET_H
#define LAU3DMULTISENSORVIDEOWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QDebug>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QComboBox>

#include "lau3dcamera.h"
#include "laucontroller.h"
#include "laumemoryobject.h"
#include "lau3dvideoglwidget.h"
#include "lauabstractfilter.h"
#include "laulookuptable.h"
#include "laubackgroundglfilter.h"

using namespace LAU3DVideoParameters;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAU3DMultiSensorVideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LAU3DMultiSensorVideoWidget(QList<LAUVideoPlaybackDevice> devices,
                                         LAUVideoPlaybackColor color = ColorXYZ,
                                         QWidget *parent = nullptr);
    ~LAU3DMultiSensorVideoWidget();

    bool isValid() const
    {
        return (sensorCount > 0);
    }

    bool isNull() const
    {
        return (sensorCount == 0);
    }

    int sensors() const
    {
        return (sensorCount);
    }

    QString error() const
    {
        return (errorString);
    }

    QString lastSavedBackgroundFile() const
    {
        return (lastSavedFilename);
    }

    int cameraCount() const
    {
        return cameras.count();
    }

    QString cameraMake(int index) const
    {
        if (index >= 0 && index < cameras.count()) {
            return cameras.at(index)->make();
        }
        return QString();
    }

    QString cameraModel(int index) const
    {
        if (index >= 0 && index < cameras.count()) {
            return cameras.at(index)->model();
        }
        return QString();
    }

    int cameraSensors(int index) const
    {
        if (index >= 0 && index < cameras.count()) {
            return cameras.at(index)->sensors();
        }
        return 0;
    }

    LAUVideoPlaybackDevice cameraDevice(int index) const
    {
        if (index >= 0 && index < cameras.count()) {
            return cameras.at(index)->device();
        }
        return DeviceUndefined;
    }

    void insertFilters(QList<QObject *> filters);

public slots:
    void onCameraError(QString string);
    void onCameraDeleted();
    void onUpdateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());
    void onChannelIndexChanged(int index);
#ifdef RAW_NIR_VIDEO
    void onCameraPositionChanged(int index);
    void onProgramCameraLabels();
#else
    void onRecordButtonClicked();
    void onResetButtonClicked();
#endif
    void onReceiveBackground(LAUMemoryObject background);

protected:
    void showEvent(QShowEvent *event)
    {
        qDebug() << "LAU3DMultiSensorVideoWidget::showEvent() - kicking off signal chain";
        onUpdateBuffer();
        QWidget::showEvent(event);
    }

    void keyPressEvent(QKeyEvent *event)
    {
        if (event->key() == Qt::Key_Up) {
            cameraIndex++;
            // Normalize to positive range using modulo
            cameraIndex = ((cameraIndex % sensorCount) + sensorCount) % sensorCount;
            qDebug() << ">>> KEY PRESS: Up arrow - setting cameraIndex to" << cameraIndex;
            glWidget->onSetCamera(cameraIndex);
#ifndef RAW_NIR_VIDEO
            // UPDATE LOOKUP TABLE FOR THE NEW SENSOR (USING MODULO TO WRAP)
            // Handle negative indices properly by ensuring positive modulo result
            // LUTs not used in RAW_NIR_VIDEO mode
            int sensorIndex = ((cameraIndex % sensorCount) + sensorCount) % sensorCount;
            if (sensorIndex >= 0 && sensorIndex < lookUpTables.count()) {
                const LAULookUpTable &lut = lookUpTables.at(sensorIndex);
                if (lut.isValid()) {
                    glWidget->setLookUpTable(lut);
                }
            }
#endif
        } else if (event->key() == Qt::Key_Down) {
            cameraIndex--;
            // Normalize to positive range using modulo
            cameraIndex = ((cameraIndex % sensorCount) + sensorCount) % sensorCount;
            qDebug() << ">>> KEY PRESS: Down arrow - setting cameraIndex to" << cameraIndex;
            glWidget->onSetCamera(cameraIndex);
#ifndef RAW_NIR_VIDEO
            // UPDATE LOOKUP TABLE FOR THE NEW SENSOR (USING MODULO TO WRAP)
            // Handle negative indices properly by ensuring positive modulo result
            // LUTs not used in RAW_NIR_VIDEO mode
            int sensorIndex = ((cameraIndex % sensorCount) + sensorCount) % sensorCount;
            if (sensorIndex >= 0 && sensorIndex < lookUpTables.count()) {
                const LAULookUpTable &lut = lookUpTables.at(sensorIndex);
                if (lut.isValid()) {
                    glWidget->setLookUpTable(lut);
                }
            }
#endif
        } else if (event->key() == Qt::Key_PageDown) {
            glWidget->onEnableTexture(false);
        } else if (event->key() == Qt::Key_PageUp) {
            glWidget->onEnableTexture(true);
        } else if (event->key() == Qt::Key_Escape) {
            if (this->parent()) {
                reinterpret_cast<QWidget *>(this->parent())->close();
            } else {
                this->close();
            }
        }
    }

signals:
    void emitBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);

private:
    int sensorCount;
    QString errorString;
    LAUVideoPlaybackColor playbackColor;

    QList<LAU3DCamera *> cameras;
    QList<LAU3DCameraController *> cameraControllers;
    QList<LAUModalityObject> frameBuffers;
    QList<LAUAbstractFilterController *> filterControllers;
    QList<LAUBackgroundGLFilter *> backgroundFilters;
    QMap<int, LAUMemoryObject> collectedBackgrounds;  // Map channel -> background
    QList<LAULookUpTable> lookUpTables;
    LAU3DVideoGLWidget *glWidget;

#ifdef RAW_NIR_VIDEO
    QComboBox *cameraPositionCombo;
    QStringList cameraPositions;  // Store position for each sensor
    QPushButton *recordButton;  // Program camera labels
#else
    QPushButton *recordButton;
    QPushButton *resetButton;
#endif
    QLabel *fpsLabel;

    int cameraIndex;  // Tracks which camera/sensor to display (used in keyPressEvent)
    int fpsCounter;   // Counts frames for FPS calculation (used in onUpdateBuffer)
    QElapsedTimer time;
    QString lastSavedFilename;

    class LAUCameraConnectionDialog *connectionDialog;

    // FPS monitoring for performance warnings
    float currentFps;
    QElapsedTimer fpsMonitorTimer;
    bool fpsWarningShown;
    bool savingBackground;  // Flag to pause FPS monitoring during save dialog
};

#endif // LAU3DMULTISENSORVIDEOWIDGET_H
