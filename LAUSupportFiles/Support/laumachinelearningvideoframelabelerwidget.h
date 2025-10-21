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

#ifndef LAUMACHINELEARNINGVIDEOFRAMELABELERWIDGET_H
#define LAUMACHINELEARNINGVIDEOFRAMELABELERWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "laumemoryobject.h"
#include "laudepthlabelerwidget.h"

class LAUMachineLearningVideoFrameLabelerWidget : public QWidget
{
    Q_OBJECT

public:
    LAUMachineLearningVideoFrameLabelerWidget(unsigned int depth, QWidget *parent = nullptr);
    ~LAUMachineLearningVideoFrameLabelerWidget();

public slots:
    void onKeyPress(QKeyEvent *keyEvent);
    void onCellActivated(int row, int col, int rowp, int colp);
    void onYesButtonClicked();
    void onNoButtonClicked();

    void onExportFramesToDisk();
    void onImportImagesFromDisk();
    void onLoadFromDisk();
    void onSaveToDisk();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event)
    {
        Q_UNUSED(event);
        if (paletteWidget) {
            paletteWidget->hide();
        }
    }

    bool eventFilter(QObject *obj, QEvent *event)
    {
        if (event->type() == QEvent::KeyPress) {
            if (obj == tableWidget) {
                QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
                this->onKeyPress(keyEvent);
                return (true);
            }
        }
        return (false);
    }

private:
    bool editFlag;
    unsigned int playbackDepth;
    QWidget *buttonWidget;
    QTableWidget *tableWidget;
    LAUDepthLabelerPaletteWidget *paletteWidget;

signals:
    void emitBuffer(QString, int);
};
#endif // LAUMACHINELEARNINGVIDEOFRAMELABELERWIDGET_H
