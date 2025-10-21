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

#ifndef LAUMATRIX_TABLE_H
#define LAUMATRIX_TABLE_H

#include <QTableWidget>
#include <QKeyEvent>

// Forward declaration
class LAUJETRWidget;

class LAUMatrixTable : public QTableWidget
{
    Q_OBJECT
    
public:
    explicit LAUMatrixTable(LAUJETRWidget *parent = nullptr);
    
protected:
    void keyPressEvent(QKeyEvent *event) override;
    
private:
    LAUJETRWidget *m_parentWidget;
};

#endif // LAUMATRIX_TABLE_H