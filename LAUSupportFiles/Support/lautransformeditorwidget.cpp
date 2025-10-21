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

#include "lautransformeditorwidget.h"

#include <QDebug>
#include <QClipboard>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStringList>
#include <QApplication>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QHeaderView>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUTransformEditorDialog::LAUTransformEditorDialog(QMatrix4x4 trns, QWidget *parent) : QDialog(parent), localTransform(trns)
{
    this->setLayout(new QVBoxLayout());
    this->setWindowTitle(QString("Look-Up Table Transform Editor"));
    this->layout()->setContentsMargins(6, 6, 6, 6);
    this->layout()->setSpacing(6);

    // Create 4x4 matrix table (same styling as Extrinsic parameters group box)
    table = new QTableWidget(4, 4);
    table->setFixedSize(302, 122);
    table->horizontalHeader()->hide();
    table->verticalHeader()->hide();
    table->setShowGrid(true);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Set uniform column widths
    for (int col = 0; col < 4; ++col) {
        table->setColumnWidth(col, 75);
    }

    // Set uniform row heights
    for (int row = 0; row < 4; ++row) {
        table->setRowHeight(row, 30);
    }

    // Create validator for matrix elements
    QDoubleValidator *validator = new QDoubleValidator(this);
    validator->setDecimals(10);
    validator->setNotation(QDoubleValidator::ScientificNotation);

    // Add line edits to matrix table
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int index = row * 4 + col;
            matrixLineEdits[index] = new QLineEdit();
            matrixLineEdits[index]->setValidator(validator);
            matrixLineEdits[index]->setAlignment(Qt::AlignCenter);
            matrixLineEdits[index]->setText(QString("%1").arg(localTransform(row, col), 0, 'g', 7));
            connect(matrixLineEdits[index], &QLineEdit::textChanged, this, &LAUTransformEditorDialog::onLineEditChanged);
            table->setCellWidget(row, col, matrixLineEdits[index]);
        }
    }

    ((QVBoxLayout*)(this->layout()))->addWidget(table, 0, Qt::AlignHCenter);

    // Only OK and Cancel buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));

    this->layout()->addWidget(buttonBox);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUTransformEditorDialog::~LAUTransformEditorDialog()
{
    ;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTransformEditorDialog::onLineEditChanged()
{
    // Update localTransform from all line edits
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int index = row * 4 + col;
            bool ok = false;
            double value = matrixLineEdits[index]->text().toDouble(&ok);
            if (ok) {
                localTransform(row, col) = value;
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUTransformEditorDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_C && event->modifiers() == Qt::ControlModifier) {
        QString string;
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                int index = row * 4 + col;
                string.append(QString("%1\t").arg(matrixLineEdits[index]->text()));
            }
            string.append(QString("\r\n"));
        }
        qDebug() << "clipboard:" << string;
        QApplication::clipboard()->setText(string);
    } else if (event->key() == Qt::Key_V && event->modifiers() == Qt::ControlModifier) {
        QMatrix4x4 newTransform;
        QStringList strings = QApplication::clipboard()->text().split("\n", Qt::SkipEmptyParts);
        if (strings.count() != 4) {
            qDebug() << "clipboard doesn't have four rows" << strings.count();
            return;
        }
        for (int row = 0; row < strings.count(); row++) {
            QStringList columns = strings.at(row).simplified().split(" ");
            if (columns.count() != 4) {
                qDebug() << "clipboard row doesn't have four columns" << row << columns.count();
                return;
            }
            for (int col = 0; col < 4; col++) {
                bool flag = false;
                double value = columns.at(col).toDouble(&flag);
                if (flag) {
                    newTransform(row, col) = value;
                } else {
                    qDebug() << "clipboard column is not a floating point string" << row << col << columns.at(col);
                    return;
                }
            }
        }
        localTransform = newTransform;
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                int index = row * 4 + col;
                matrixLineEdits[index]->setText(QString("%1").arg(localTransform(row, col), 0, 'g', 7));
            }
        }
    }
}

