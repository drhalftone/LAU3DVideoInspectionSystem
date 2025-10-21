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

#include "laumatrixtable.h"
#include "laujetrwidget.h"
#include <QApplication>
#include <QClipboard>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUMatrixTable::LAUMatrixTable(LAUJETRWidget *parent) : QTableWidget(parent), m_parentWidget(parent)
{
    ;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUMatrixTable::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_C && event->modifiers() == Qt::ControlModifier) {
        // Handle Ctrl+C - copy entire matrix to clipboard
        if (m_parentWidget && selectedItems().size() > 0) {
            QString matlabString = m_parentWidget->matrixToMatlabString(this);
            QApplication::clipboard()->setText(matlabString);
            return;
        }
    }
    else if (event->key() == Qt::Key_V && event->modifiers() == Qt::ControlModifier) {
        // Handle Ctrl+V - paste matrix from clipboard
        if (m_parentWidget && selectedItems().size() > 0) {
            QString clipboardText = QApplication::clipboard()->text();
            if (m_parentWidget->pasteFromMatlabString(this, clipboardText)) {
                return;
            }
        }
    }
    
    // Call base class for other key events
    QTableWidget::keyPressEvent(event);
}