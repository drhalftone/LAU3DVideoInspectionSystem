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

#ifndef LAUTIFFVIEWERDIALOG_H
#define LAUTIFFVIEWERDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QStringList>
#include <QList>

#include "lautiffviewer.h"
#include "laulookuptable.h"
#include "laumemoryobject.h"

class LAUTiffViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUTiffViewerDialog(QWidget *parent = nullptr);
    ~LAUTiffViewerDialog();

    // Setup methods
    void setLookupTables(const QList<LAULookUpTable> &tables);
    void setTiffFilename(const QString &filename);

    // Access methods
    LAUTiffViewer* tiffViewer() const;
    LookUpTableBoundingBox getBoundingBox() const;

public slots:
    virtual void accept() override;

private:
    void setupUi();
    void applyLookupTablesToViewer();

    // UI Components
    QVBoxLayout *m_mainLayout;
    LAUTiffViewer *m_tiffViewer;
    QDialogButtonBox *m_buttonBox;

    // Data
    QList<LAULookUpTable> m_lookupTables;
    QString m_tiffFilename;
};

#endif // LAUTIFFVIEWERDIALOG_H