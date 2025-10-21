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

#ifndef LAUCAMERACLASSIFIERDIALOG_H
#define LAUCAMERACLASSIFIERDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QMap>
#include <QHash>

class QFormLayout;
class QComboBox;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCameraClassifierDialog : public QDialog
{
    Q_OBJECT

public:
    // Constructor with two string lists
    LAUCameraClassifierDialog(const QStringList& items, const QStringList& categories, QWidget* parent = nullptr);

    // Constructor with no args - loads from settings
    explicit LAUCameraClassifierDialog(QWidget* parent = nullptr);

    // Returns the mapping of items to their selected categories
    QMap<QString, QString> getSelections() const;
    
    // Static method to retrieve camera assignments from settings
    static QHash<QString, QString> getCameraAssignments();
    static void resetCameraAssignments();

private slots:
    void onOkClicked();

private:
    void initializeDialog();
    void populateDialog();
    void saveToSettings();
    void loadFromSettings();

private:
    QStringList m_items;
    QStringList m_categories;
    QFormLayout* m_formLayout;
    QList<QComboBox*> m_comboBoxes;
    QMap<QString, QString> m_savedSelections;
};

#endif // LAUCAMERACLASSIFIERDIALOG_H
