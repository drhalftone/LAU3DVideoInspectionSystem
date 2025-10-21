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

#include "laucameraclassifierdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>
#include <QSettings>
#include <QGroupBox>
#include <QDialogButtonBox>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCameraClassifierDialog::LAUCameraClassifierDialog(const QStringList& items, const QStringList& categories, QWidget* parent) : QDialog(parent), m_items(items), m_categories(categories)
{
    initializeDialog();
    populateDialog();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCameraClassifierDialog::LAUCameraClassifierDialog(QWidget* parent) : QDialog(parent)
{
    loadFromSettings();
    initializeDialog();
    populateDialog();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QMap<QString, QString> LAUCameraClassifierDialog::getSelections() const
{
    QMap<QString, QString> selections;
    for (int i = 0; i < m_items.size(); ++i) {
        selections[m_items[i]] = m_comboBoxes[i]->currentText();
    }
    return selections;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QHash<QString, QString> LAUCameraClassifierDialog::getCameraAssignments()
{
    QHash<QString, QString> assignments;
    QSettings settings;
    
    // Load items
    QStringList items = settings.value("LAUCameraClassifierDialog/items").toStringList();
    
    // Load assignments
    settings.beginGroup("LAUCameraClassifierDialog/selections");
    for (const QString& item : items) {
        if (settings.contains(item)) {
            assignments.insert(item, settings.value(item).toString());
        }
    }
    settings.endGroup();
    
    return assignments;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraClassifierDialog::resetCameraAssignments()
{
    QSettings settings;
    settings.remove(QString("LAUCameraClassifierDialog/items"));
    settings.remove(QString("LAUCameraClassifierDialog/selections"));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraClassifierDialog::onOkClicked()
{
    // Check for uniqueness of selections
    QStringList selections;
    for (QComboBox* combo : m_comboBoxes) {
        selections << combo->currentText();
    }
    
    bool uniqueSelections = true;
    for (int i = 0; i < selections.size(); ++i) {
        for (int j = i + 1; j < selections.size(); ++j) {
            if (selections[i] == selections[j]) {
                uniqueSelections = false;
                break;
            }
        }
        if (!uniqueSelections) break;
    }
    
    if (!uniqueSelections) {
        QMessageBox::warning(this, "Duplicate Selections", 
            "Each item must be assigned a unique category. Please review your selections.");
        return;
    }
    
    // If everything is valid, save to settings and accept
    saveToSettings();
    accept();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraClassifierDialog::initializeDialog()
{
    setWindowTitle("Classify Items");
    resize(400, 300);
    
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create group box
    QGroupBox* groupBox = new QGroupBox("Camera assignments:");
    QVBoxLayout* groupBoxLayout = new QVBoxLayout(groupBox);
    
    // Form layout for items and combo boxes
    QFormLayout* formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    formLayout->setLabelAlignment(Qt::AlignLeft);
    
    // Create a widget to contain the form layout and make it expand
    QWidget* formWidget = new QWidget();
    formWidget->setLayout(formLayout);
    
    // Add form widget to group box
    groupBoxLayout->addWidget(formWidget);
    
    // Add group box to main layout
    mainLayout->addWidget(groupBox, 1); // Add with stretch factor
    m_formLayout = formLayout;
    
    // Dialog button box
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    
    // Connect signals
    connect(buttonBox, &QDialogButtonBox::accepted, this, &LAUCameraClassifierDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraClassifierDialog::populateDialog()
{
    m_comboBoxes.clear();
    
    for (const QString& item : m_items) {
        QComboBox* comboBox = new QComboBox();
        comboBox->addItems(m_categories);
        comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        
        // Find if this item has a saved selection
        if (m_savedSelections.contains(item)) {
            int index = m_categories.indexOf(m_savedSelections[item]);
            if (index >= 0) {
                comboBox->setCurrentIndex(index);
            }
        }
        
        QLabel* label = new QLabel(item + ":");
        m_formLayout->addRow(label, comboBox);
        m_comboBoxes.append(comboBox);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraClassifierDialog::saveToSettings()
{
    QSettings settings;
    
    // Save items and categories
    settings.setValue("LAUCameraClassifierDialog/items", m_items);
    settings.setValue("LAUCameraClassifierDialog/categories", m_categories);
    
    // Save selections
    settings.beginGroup("LAUCameraClassifierDialog/selections");
    for (int i = 0; i < m_items.size(); ++i) {
        settings.setValue(m_items[i], m_comboBoxes[i]->currentText());
    }
    settings.endGroup();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCameraClassifierDialog::loadFromSettings()
{
    QSettings settings;
    
    // Load items and categories
    m_items = settings.value("LAUCameraClassifierDialog/items").toStringList();
    m_categories = settings.value("LAUCameraClassifierDialog/categories").toStringList();
    
    // Load selections
    settings.beginGroup("LAUCameraClassifierDialog/selections");
    for (const QString& item : m_items) {
        if (settings.contains(item)) {
            m_savedSelections[item] = settings.value(item).toString();
        }
    }
    settings.endGroup();
}
