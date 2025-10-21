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

#include "laudepthlabelerwidget.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUDepthLabelerPaletteWidget::LAUDepthLabelerPaletteWidget(QWidget *parent) : LAUPaletteWidget(QString("Depth Video Labeler"), QList<LAUPalette::Packet>(), parent)
{
    // SET THE WINDOWS LAYOUT
    this->setWindowTitle("LAUDepthLabelerWidget");

    // REGISTER THE PALETTE WIDGETS
    QList<LAUPalette::Packet> packets;
    LAUPalette::Packet packet;
    packet.pal = LAUPaletteObject::PaletteSlider;
    packet.pos = QPoint(1, 1);
    packets << packet;
    packet.pal = LAUPaletteObject::PaletteSlider;
    packet.pos = QPoint(2, 1);
    packets << packet;
    packet.pal = LAUPaletteObject::PaletteButton;
    packet.pos = QPoint(0, 1);
    packets << packet;
    packet.pal = LAUPaletteObject::PaletteButton;
    packet.pos = QPoint(0, 2);
    packets << packet;
    packet.pal = LAUPaletteObject::PaletteDial;
    packet.pos = QPoint(1, 2);
    packets << packet;
    packet.pal = LAUPaletteObject::PaletteButton;
    packet.pos = QPoint(2, 2);
    packets << packet;

    this->registerLayout(packets);

    QSettings settings;
    QRect rect = settings.value("LAUDepthLabelerPaletteWidget::geometry", QRect(0, 0, 0, 0)).toRect();
    if (rect.width() > 0) {
        this->setGeometry(rect);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUDepthLabelerPaletteWidget::~LAUDepthLabelerPaletteWidget()
{
    QSettings settings;
    settings.setValue("LAUDepthLabelerPaletteWidget::geometry", this->geometry());

    qDebug() << "LAUDepthLabelerPaletteWidget::~LAUDepthLabelerPaletteWidget()";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDepthLabelerPaletteWidget::onDialRotated(QPoint pos, int val)
{
    if (pos == QPoint(1, 2)) {
        qDebug() << "DIAL ROTATED" << val;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDepthLabelerPaletteWidget::onValueChanged(QPoint pos, int val)
{
    if (pos == QPoint(1, 1)) {
        qDebug() << "LEFT SLIDER" << val;
    } else if (pos == QPoint(2, 1)) {
        qDebug() << "RIGHT SLIDER" << val;
    } else if (pos == QPoint(1, 2)) {
        qDebug() << "DIAL" << val;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDepthLabelerPaletteWidget::onButtonPressed(QPoint pos)
{
    if (pos == QPoint(0, 2)) {
        qDebug() << "LEFT BUTTON";
    } else if (pos == QPoint(2, 2)) {
        qDebug() << "RIGHT BUTTON";
    } else if (pos == QPoint(0, 1)) {
        qDebug() << "TOP BUTTON";
    } else if (pos == QPoint(1, 2)) {
        qDebug() << "DIAL BUTTON";
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUDepthLabelerPaletteWidget::onButtonReleased(QPoint pos)
{
    if (pos == QPoint(0, 2)) {
        qDebug() << "LEFT BUTTON";
    } else if (pos == QPoint(2, 2)) {
        qDebug() << "RIGHT BUTTON";
    } else if (pos == QPoint(0, 1)) {
        qDebug() << "TOP BUTTON";
    } else if (pos == QPoint(1, 2)) {
        qDebug() << "DIAL BUTTON";
    }
}
