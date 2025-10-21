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

#ifndef LAUSETXYPLANEWIDGET_H
#define LAUSETXYPLANEWIDGET_H

#include <QObject>
#include <QComboBox>

#include "lau3dfiducialglwidget.h"

class LAUSetXYPlaneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUSetXYPlaneDialog(LAUScan scn, QWidget *parent = nullptr);
    ~LAUSetXYPlaneDialog();

    QMatrix4x4 transform() const
    {
        return(transformF);
    }

public slots:
    void onFiducialsUpdate(int count);

protected:
    void accept();
    void reject() override;
    void closeEvent(QCloseEvent *event) override;

private:
    QComboBox *oBox;
    QComboBox *xBox;
    QMatrix4x4 transformF;

    LAU3DFiducialGLWidget *glWidget;
    
    // Transform validation helper functions
    QVector3D transformPoint(const QMatrix4x4 &matrix, const QVector3D &point);
    bool validateOrthonormalTransform(const QMatrix4x4 &matrix, QString &errorMessage);
    bool isApproximatelyEqual(double a, double b, double epsilon = 1e-3);
    bool isApproximatelyEqual(const QVector3D &a, const QVector3D &b, double epsilon = 1e-3);
    double vectorLength(const QVector3D &vector);
    double dotProduct(const QVector3D &a, const QVector3D &b);
    QVector3D crossProduct(const QVector3D &a, const QVector3D &b);
};

#endif // LAUSETXYPLANEWIDGET_H
