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

#ifndef LAUBACKGROUNDFILTERMAINWINDOW_H
#define LAUBACKGROUNDFILTERMAINWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QTimer>
#include <QCloseEvent>
#include "lau3dmultisensorvideowidget.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUBackgroundFilterMainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LAUBackgroundFilterMainWindow(QWidget *parent = nullptr);
    ~LAUBackgroundFilterMainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    LAU3DMultiSensorVideoWidget *widget;
};

#endif // LAUBACKGROUNDFILTERMAINWINDOW_H
