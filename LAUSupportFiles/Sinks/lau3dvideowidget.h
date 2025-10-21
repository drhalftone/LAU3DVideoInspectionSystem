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

#ifndef LAU3DVIDEOWIDGET_H
#define LAU3DVIDEOWIDGET_H

#include <QDebug>
#include <QImage>
#include <QTimer>
#include <QWidget>
#include <QAction>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QElapsedTimer>

#include "lauscan.h"
#include "lau3dcameras.h"
#include "laucontroller.h"
#include "laumemoryobject.h"
#include "lauabstractfilter.h"
#include "lau3dvideoglwidget.h"
#include "lauvideoplayerlabel.h"
#ifndef EXCLUDE_LAUVIDEOPLAYERWIDGET
#include "lau3dvideoplayerwidget.h"
#else
#define MAXRECORDEDFRAMECOUNT 1
#endif

//#include "laustereovisionglfilter.h"
#ifdef LUCID
#include "laumachinelearningvideoframelabelerwidget.h"
#endif

using namespace LAU3DVideoParameters;

class LAU3DVideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LAU3DVideoWidget(LAUVideoPlaybackColor color = ColorXYZG, LAUVideoPlaybackDevice device = DevicePrimeSense,  QWidget *parent = nullptr);
    ~LAU3DVideoWidget();

    QSize size() const
    {
        if (camera) {
            return (QSize(camera->depthWidth(), camera->depthHeight()));
        } else {
            return (QSize(0, 0));
        }
    }

    int width() const
    {
        return (size().width());
    }

    int height() const
    {
        return (size().height());
    }

    int step() const
    {
        if (camera) {
            return (colors() * depth() * camera->depthWidth());
        } else {
            return (0);
        }
    }

    int depth() const
    {
        return (sizeof(float));
    }

    LAUVideoPlaybackColor color()
    {
        return (playbackColor);
    }

    int colors() const
    {
        return (LAU3DVideoParameters::colors(playbackColor));
    }

    QString make() const
    {
        if (camera) {
            return (camera->make());
        } else {
            return (QString());
        }
    }

    QString model() const
    {
        if (camera) {
            return (camera->model());
        } else {
            return (QString());
        }
    }

    QString serial() const
    {
        if (camera) {
            return (camera->serial());
        } else {
            return (QString());
        }
    }

    virtual bool isValid() const
    {
        if (camera) {
            return (camera->isValid());
        } else {
            return (false);
        }
    }

    virtual bool isNull() const
    {
        return (!isValid());
    }

    void setScreen(QScreen *screen)
    {
        QRect rect = screen->availableGeometry();
        screenWidget = new QWidget();
        screenWidget->setGeometry(rect);
        screenWidget->setLayout(new QVBoxLayout());
        screenWidget->layout()->setContentsMargins(0, 0, 0, 0);
        screenWidget->layout()->addWidget(glWidget);
    }

    void setLookUpTable(LAULookUpTable table)
    {
        glWidget->setLookUpTable(table);
    }

    void insertAction(QAction *action);
    void insertActions(QList<QAction *> actions);

    void insertFilter(QObject *newFilter, QSurface *srfc = nullptr);
    void insertFilters(QList<QObject *> filters);

    void prependFilter(QObject *newFilter);
    void prependFilters(QList<QObject *> filters);

    void appendFilter(QObject *newFilter)
    {
        insertFilter(newFilter);
    }

    void appendFilters(QList<QObject *> filters)
    {
        insertFilters(filters);
    }

    void onSetCamera(unsigned int val)
    {
        if (glWidget) {
            glWidget->onSetCamera(val);
        }
    }

public slots:
    void onError(QString string)
    {
        qDebug() << string;
    }

    void onCameraObjectDestroyed()
    {
        cameraObjectStillExists = false;
    }

    void onContextMenuTriggered();
    void onShowMachineVisionLabelingWidget();
    void onUpdateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject());

protected:
    void keyPressEvent(QKeyEvent *event)
    {
#ifdef ENABLECLASSIFIER
        if (labelWidget) {
            labelWidget->onKeyPress(event);
        } else {
#endif
            static unsigned int counter = 0;
            if (event->key() == Qt::Key_Up) {
                glWidget->onSetCamera(++counter);
            } else if (event->key() == Qt::Key_Down) {
                glWidget->onSetCamera(--counter);
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
#ifdef ENABLECLASSIFIER
        }
#endif
    }

    // LETS TELL THE CAMERA THAT IT CAN STOP ANY SCAN IT MIGHT BE IN THE MIDDLE
    // ITS UP TO THE CAMERA OBJECT IF IT WANTS TO RESPOND TO THIS
    void hideEvent(QHideEvent *){
        camera->stopCamera();
    }

    void showEvent(QShowEvent *event)
    {
        // IF WE HAVE A SCREEN WIDGET (I.E. AR SANDBOX MODE), THEN MAKE IT FULL SCREEN
        if (screenWidget) {
            screenWidget->showFullScreen();
        }

        // IF WE HAVE A PROJECTOR WIDGET (I.E. VIMBA LCG), THEN MAKE IT FULL SCREEN
        if (projectorWidget) {
            if (QGuiApplication::screens().count() > 1){
                projectorWidget->showFullScreen();
            } else {
                projectorWidget->show();
            }
        }

        onUpdateBuffer();

        // CALL THE UNDERLYING WIDGET'S SHOW EVENT HANDLER
        QWidget::showEvent(event);
    }

    virtual void updateBuffer(LAUMemoryObject depth = LAUMemoryObject(), LAUMemoryObject color = LAUMemoryObject(), LAUMemoryObject mapping = LAUMemoryObject())
    {
        Q_UNUSED(depth);
        Q_UNUSED(color);
        Q_UNUSED(mapping);
    }

    LAUMemoryObjectManager *frameBufferManager;
    LAUController *frameBufferManagerController;

    LAU3DCamera *camera;
    LAU3DVideoGLWidget *glWidget;
    LAU3DCameraController *cameraController;
    LAUVideoPlaybackColor playbackColor;
    LAUVideoPlaybackDevice playbackDevice;

#ifdef LUCID
    LAUMachineLearningVideoFrameLabelerWidget *labelWidget;
#else
    QWidget *labelWidget;
#endif

    QWidget *projectorWidget;
    QWidget *scannerWidget;
    QWidget *screenWidget;
    int counter;
    QElapsedTimer time;

    bool cameraObjectStillExists;

    QList<LAUModalityObject> framesList;
    QList<LAUAbstractFilterController *> filterControllers;

signals:
    void emitEnableEmitter(bool state);
    void emitBuffer(LAUMemoryObject depth, LAUMemoryObject color, LAUMemoryObject mapping);
};

#endif // LAU3DVIDEOWIDGET_H
