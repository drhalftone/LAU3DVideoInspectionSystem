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

#ifndef LAUDEPTHLABELERWIDGET_H
#define LAUDEPTHLABELERWIDGET_H

#include <QWidget>
#include <laupalettewidget.h>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUDepthLabelerPaletteWidget : public LAUPaletteWidget
{
    Q_OBJECT

public:
    LAUDepthLabelerPaletteWidget(QWidget *parent = 0);
    ~LAUDepthLabelerPaletteWidget();

protected:
    void onConnected()
    {
        LAUPaletteWidget::onConnected();
        //QMessageBox::information(this, "Depth Labeler Palette", QString("Ready for labeling!"));
    }

    void onDisconnected()
    {
        LAUPaletteWidget::onDisconnected();
        QMessageBox::warning(this, "Depth Labeler Palette", QString("Not ready for labeling!"));
    }

public slots:
    void onDialRotated(QPoint pos, int val);
    void onValueChanged(QPoint pos, int val);
    void onButtonPressed(QPoint pos);
    void onButtonReleased(QPoint pos);

private:

signals:

};
#endif // LAUDEPTHLABELERWIDGET_H
