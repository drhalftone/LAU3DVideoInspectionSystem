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

#ifndef LAUTRANSFORMEDITORWIDGET_H
#define LAUTRANSFORMEDITORWIDGET_H

#include <QWidget>
#include <QDialog>
#include <QKeyEvent>
#include <QMatrix4x4>
#include <QTableWidget>
#include <QLineEdit>

class LAUTransformEditorDialog : public QDialog
{
    Q_OBJECT

public:
    LAUTransformEditorDialog(QMatrix4x4 trns = QTransform(), QWidget *parent = nullptr);
    ~LAUTransformEditorDialog();

    const QMatrix4x4 transform()
    {
        return (localTransform);
    }

public slots:
    void onLineEditChanged();

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    QTableWidget *table;
    QLineEdit *matrixLineEdits[16];
    QMatrix4x4 localTransform;
};
#endif // LAUTRANSFORMEDITORWIDGET_H
