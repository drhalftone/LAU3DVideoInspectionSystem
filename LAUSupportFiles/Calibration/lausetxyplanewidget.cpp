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

#include "lausetxyplanewidget.h"
#include "lautransformeditorwidget.h"

#include <QDebug>
#include <QGroupBox>
#include <QSettings>
#include <QDialogButtonBox>
#include <QCloseEvent>

#include <Eigen/Dense>
#include <Eigen/SVD>
#include <vector>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUSetXYPlaneDialog::LAUSetXYPlaneDialog(LAUScan scn, QWidget *parent) : QDialog{parent}
{
    this->setWindowTitle(QString("Set XY-Plane Dialog"));
    this->setLayout(new QVBoxLayout());
    this->layout()->setContentsMargins(6, 6, 6, 6);

    // Restore dialog geometry from QSettings
    QSettings settings;
    settings.beginGroup("DialogGeometry");
    QByteArray geometry = settings.value("LAUSetXYPlaneDialog/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    settings.endGroup();

    // CREATE A GLWIDGET TO DISPLAY THE SCAN
    glWidget = new LAU3DFiducialGLWidget(scn);
    glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    glWidget->setFocusPolicy(Qt::StrongFocus);
    glWidget->setMinimumSize(320, 240);
    glWidget->onEnableFiducials(true);
    connect(glWidget, SIGNAL(emitFiducialsChanged(int)), this, SLOT(onFiducialsUpdate(int)));
    this->layout()->addWidget(glWidget);

    QGroupBox *box = new QGroupBox();
    box->setLayout(new QHBoxLayout());
    box->layout()->setContentsMargins(6, 6, 6, 6);
    this->layout()->addWidget(box);

    ((QHBoxLayout *)(box->layout()))->addStretch();

    oBox = new QComboBox();
    box->layout()->addWidget(new QLabel("Origin:"));
    box->layout()->addWidget(oBox);

    xBox = new QComboBox();
    box->layout()->addWidget(new QLabel("X-axis:"));
    box->layout()->addWidget(xBox);

    // ADD STANDARD OK AND CANCEL BUTTONS
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &LAUSetXYPlaneDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &LAUSetXYPlaneDialog::reject);
    this->layout()->addWidget(buttonBox);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUSetXYPlaneDialog::~LAUSetXYPlaneDialog()
{
    // Qt6: OpenGL widget is cleaned up in closeEvent() to prevent issues
    // Just save geometry here
    QSettings settings;
    settings.beginGroup("DialogGeometry");
    settings.setValue("LAUSetXYPlaneDialog/geometry", saveGeometry());
    settings.endGroup();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUSetXYPlaneDialog::onFiducialsUpdate(int count)
{
    // ADD NEW LETTERS IF NEEDED
    while (count > oBox->count()){
        oBox->addItem(QString("Point %1").arg(QChar('A' + count - 1)), oBox->count()+1);
        xBox->addItem(QString("Point %1").arg(QChar('A' + count - 1)), xBox->count()+1);

        if (xBox->count() == 2){
            xBox->setCurrentIndex(1);
        }
    }

    // REMOVE LETTERS IF NEEDED
    while (count < oBox->count()){
        oBox->removeItem(oBox->count());
        xBox->removeItem(xBox->count());
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUSetXYPlaneDialog::accept()
{
    // MAKE SURE THE USER HAS SELECTED A MINIMUM NUMBER OF POINTS FOR FITTING THE PLANE
    if (oBox->count() < 10){
        QMessageBox::information(this, QString("Set XY-Plane Dialog"), QString("It is required that you select at least 10 points for fitting the XY plane."));
        return;
    }

    // MAKE SURE THE USER SPECIFIED A UNIQUE ORIGIN AND X-AXIS COORDINATE
    if (oBox->currentIndex() == xBox->currentIndex()){
        QMessageBox::information(this, QString("Set XY-Plane Dialog"), QString("Select two unique fiducials for your origin and x-axis coordinate."));
        return;
    }

    // EXTRACT THE FIDUCIALS
    QList<QVector3D> fiducials = glWidget->fiducials();

    // MAKE SURE WE FOUND ENOUGH CLUSTERS TO DO A DECENT INTERPOLATION
    unsigned int N = (unsigned int)fiducials.count();

    // ALLOCATE SPACE FOR DATA VECTORS
    std::vector<double> xVec(N);
    std::vector<double> yVec(N);
    std::vector<double> zVec(N);

    double mX = 0.0, mY = 0.0, mZ = 0.0;
    for (unsigned int n = 0; n < N; n++) {
        xVec[n] = (double)fiducials.at(n).x();
        yVec[n] = (double)fiducials.at(n).y();
        zVec[n] = (double)fiducials.at(n).z();
        qDebug() << xVec[n] << yVec[n] << zVec[n];

        mX += xVec[n];
        mY += yVec[n];
        mZ += zVec[n];
    }

    // DIVIDE THE SUM OF POINTS BY THE NUMBER OF POINTS TO GET THE CENTROID
    mX /= (double)N;
    mY /= (double)N;
    mZ /= (double)N;

    // SUBTRACT OUT THE MEAN VALUE
    for (unsigned int n = 0; n < N; n++) {
        xVec[n] -= mX;
        yVec[n] -= mY;
        zVec[n] -= mZ;
    }

    // GET THE SCALE FACTOR SO THAT OUR COVARIANCE MATRIX IS MANAGEABLE
    double scaleFactor = 2.0 / qMax(qAbs(mX), qMax(qAbs(mY), qAbs(mZ)));

    // RESCALE THE COORDINATES
    for (unsigned int n = 0; n < N; n++) {
        xVec[n] *= scaleFactor;
        yVec[n] *= scaleFactor;
        zVec[n] *= scaleFactor;
        qDebug() << xVec[n] << yVec[n] << zVec[n];
    }

    // CREATE THE COVARIANCE MATRIX
    Eigen::Matrix3d A = Eigen::Matrix3d::Zero();
    for (unsigned int n = 0; n < N; n++) {
        A(0,0) += xVec[n] * xVec[n];
        A(0,1) += xVec[n] * yVec[n];
        A(0,2) += xVec[n] * zVec[n];

        A(1,0) += yVec[n] * xVec[n];
        A(1,1) += yVec[n] * yVec[n];
        A(1,2) += yVec[n] * zVec[n];

        A(2,0) += zVec[n] * xVec[n];
        A(2,1) += zVec[n] * yVec[n];
        A(2,2) += zVec[n] * zVec[n];
    }

    qDebug() << A(0,0) << A(0,1) << A(0,2);
    qDebug() << A(1,0) << A(1,1) << A(1,2);
    qDebug() << A(2,0) << A(2,1) << A(2,2);

    // PERFORM SINGULAR VALUE DECOMPOSITION
    Eigen::JacobiSVD<Eigen::Matrix3d> svd(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Eigen::Matrix3d V = svd.matrixV();

    qDebug() << V(0,0) << V(0,1) << V(0,2);
    qDebug() << V(1,0) << V(1,1) << V(1,2);
    qDebug() << V(2,0) << V(2,1) << V(2,2);

    QMatrix4x4 transformA(1.0, 0.0, 0.0, -mX, 0.0, 1.0, 0.0, -mY, 0.0, 0.0, 1.0, -mZ, 0.0, 0.0, 0.0, 1.0);
    QMatrix4x4 transformB(V(0,0), V(0,1), V(0,2), 0.0,
                          V(1,0), V(1,1), V(1,2), 0.0,
                          V(2,0), V(2,1), V(2,2), 0.0,
                             0.0,    0.0,    0.0, 1.0);

    // TRANSFORM OUR FIDUCIALS TO THE NEW PLANE
    for (int n = 0; n < fiducials.count(); n++){
        QVector4D point = QVector4D(fiducials.at(n).x(), fiducials.at(n).y(), fiducials.at(n).z(), 1.0);
        point = transformA * point;
        point = transformB * point;
        fiducials.replace(n, QVector3D(point.x()/point.w(), point.y()/point.w(), point.z()/point.w()));
    }

    // NOW LET'S SET THE ORIGIN
    QVector4D oPt(fiducials.at(oBox->currentIndex()).x(), fiducials.at(oBox->currentIndex()).y(), 0.0, 1.0);
    QVector4D xPt(fiducials.at(xBox->currentIndex()).x(), fiducials.at(xBox->currentIndex()).y(), 0.0, 1.0);

    // CREATE TRANSFORM TO MAKE OBOX POINT THE ORIGIN
    QMatrix4x4 transformC(1.0, 0.0, 0.0, -oPt.x(), 0.0, 1.0, 0.0, -oPt.y(), 0.0, 0.0, 1.0, -oPt.z(), 0.0, 0.0, 0.0, 1.0);

    // TRANSFORM THE O AND X POINTS
    oPt = transformC * oPt;
    xPt = transformC * xPt;

    // CREATE 3D NORMALIZED X-AXIS ROTATION VECTOR
    QVector3D xRt = QVector3D(xPt.x()/xPt.w(), xPt.y()/xPt.w(), xPt.z()/xPt.w()).normalized();

    // GET THE Y POINT FROM THE CROSS PRODUCT OF THE X PT AND THE Z AXIS
    QVector3D yRt = QVector3D::crossProduct(QVector3D(0.0, 0.0, 1.0), xRt);

    // ROTATE THE XY PLANE TO SET OUT X AND Y AXES
    QMatrix4x4 transformD(xRt.x(), xRt.y(), xRt.z(),   0.0,
                          yRt.x(), yRt.y(), yRt.z(),   0.0,
                              0.0,     0.0,     1.0,   0.0,
                              0.0,     0.0,     0.0,   1.0);

    // COMBINE ALL THE COMPONENT TRANSFORMS INTO A SINGLE TRANSFORM
    transformF = transformD * transformC * transformB * transformA;

    // VALIDATE THE TRANSFORMATION MATRIX BEFORE SHOWING TO USER
    QString validationError;
    if (!validateOrthonormalTransform(transformF, validationError)) {
        qWarning() << "Transform validation failed:" << validationError;
        QMessageBox::warning(this, QString("Transform Validation Warning"),
                           QString("The computed transformation matrix may have issues:\n\n%1\n\n"
                                   "Please verify the results carefully.").arg(validationError));
        return;  // Don't accept if validation failed
    }

    qDebug() << "Transform validation passed - orthonormal properties preserved";

    // Accept the dialog with the computed transform
    QDialog::accept();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUSetXYPlaneDialog::reject()
{
    // Qt6: Explicitly call base class reject to ensure proper cleanup
    // and prevent event propagation issues to parent dialog
    QDialog::reject();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUSetXYPlaneDialog::closeEvent(QCloseEvent *event)
{
    // Accept the close event and call base class handler
    QDialog::closeEvent(event);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector3D LAUSetXYPlaneDialog::transformPoint(const QMatrix4x4 &matrix, const QVector3D &point)
{
    QVector4D homogeneous(point.x(), point.y(), point.z(), 1.0);
    QVector4D transformed = matrix * homogeneous;
    
    // Convert back to 3D by dividing by w component
    if (transformed.w() != 0.0) {
        return QVector3D(transformed.x() / transformed.w(), 
                        transformed.y() / transformed.w(), 
                        transformed.z() / transformed.w());
    }
    return QVector3D(transformed.x(), transformed.y(), transformed.z());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUSetXYPlaneDialog::isApproximatelyEqual(double a, double b, double epsilon)
{
    return qAbs(a - b) < epsilon;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUSetXYPlaneDialog::isApproximatelyEqual(const QVector3D &a, const QVector3D &b, double epsilon)
{
    return isApproximatelyEqual(a.x(), b.x(), epsilon) &&
           isApproximatelyEqual(a.y(), b.y(), epsilon) &&
           isApproximatelyEqual(a.z(), b.z(), epsilon);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
double LAUSetXYPlaneDialog::vectorLength(const QVector3D &vector)
{
    return sqrt(vector.x() * vector.x() + vector.y() * vector.y() + vector.z() * vector.z());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
double LAUSetXYPlaneDialog::dotProduct(const QVector3D &a, const QVector3D &b)
{
    return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
QVector3D LAUSetXYPlaneDialog::crossProduct(const QVector3D &a, const QVector3D &b)
{
    return QVector3D(a.y() * b.z() - a.z() * b.y(),
                     a.z() * b.x() - a.x() * b.z(),
                     a.x() * b.y() - a.y() * b.x());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUSetXYPlaneDialog::validateOrthonormalTransform(const QMatrix4x4 &matrix, QString &errorMessage)
{
    // Define the standard orthonormal basis points
    QVector3D o(0, 0, 0);  // Origin
    QVector3D x(1, 0, 0);  // X-axis unit vector
    QVector3D y(0, 1, 0);  // Y-axis unit vector
    QVector3D z(0, 0, 1);  // Z-axis unit vector

    // Transform all points
    QVector3D o_transformed = transformPoint(matrix, o);
    QVector3D x_transformed = transformPoint(matrix, x);
    QVector3D y_transformed = transformPoint(matrix, y);
    QVector3D z_transformed = transformPoint(matrix, z);

    // Calculate the transformed vectors from origin
    QVector3D ox_transformed = x_transformed - o_transformed;  // (x-o) after transform
    QVector3D oy_transformed = y_transformed - o_transformed;  // (y-o) after transform
    QVector3D oz_transformed = z_transformed - o_transformed;  // (z-o) after transform

    qDebug() << "Transform validation:";
    qDebug() << "Original points: O:" << o << "X:" << x << "Y:" << y << "Z:" << z;
    qDebug() << "Transformed points: O':" << o_transformed << "X':" << x_transformed << "Y':" << y_transformed << "Z':" << z_transformed;
    qDebug() << "Transformed vectors: OX':" << ox_transformed << "OY':" << oy_transformed << "OZ':" << oz_transformed;

    const double tolerance = 1e-3;  // Allow 0.999 to be close enough to 1.0

    // Test 1: Check that distances are preserved (should be 1.0)
    double distanceOX = vectorLength(ox_transformed);
    double distanceOY = vectorLength(oy_transformed);
    double distanceOZ = vectorLength(oz_transformed);

    qDebug() << "Distances - OX':" << distanceOX << "OY':" << distanceOY << "OZ':" << distanceOZ;

    if (!isApproximatelyEqual(distanceOX, 1.0, tolerance)) {
        errorMessage = QString("Distance O to X should be 1.0, got %1").arg(distanceOX);
        return false;
    }
    if (!isApproximatelyEqual(distanceOY, 1.0, tolerance)) {
        errorMessage = QString("Distance O to Y should be 1.0, got %1").arg(distanceOY);
        return false;
    }
    if (!isApproximatelyEqual(distanceOZ, 1.0, tolerance)) {
        errorMessage = QString("Distance O to Z should be 1.0, got %1").arg(distanceOZ);
        return false;
    }

    // Test 2: Check orthogonality (dot products should be 0)
    double dotXY = dotProduct(ox_transformed, oy_transformed);
    double dotXZ = dotProduct(ox_transformed, oz_transformed);
    double dotYZ = dotProduct(oy_transformed, oz_transformed);

    qDebug() << "Dot products - X'·Y':" << dotXY << "X'·Z':" << dotXZ << "Y'·Z':" << dotYZ;

    if (!isApproximatelyEqual(dotXY, 0.0, tolerance)) {
        errorMessage = QString("X and Y vectors should be perpendicular, dot product = %1").arg(dotXY);
        return false;
    }
    if (!isApproximatelyEqual(dotXZ, 0.0, tolerance)) {
        errorMessage = QString("X and Z vectors should be perpendicular, dot product = %1").arg(dotXZ);
        return false;
    }
    if (!isApproximatelyEqual(dotYZ, 0.0, tolerance)) {
        errorMessage = QString("Y and Z vectors should be perpendicular, dot product = %1").arg(dotYZ);
        return false;
    }

    // Test 3: Check right-hand rule: (x-o) × (y-o) = (z-o)
    QVector3D crossProductXY = crossProduct(ox_transformed, oy_transformed);
    
    qDebug() << "Cross product X' × Y':" << crossProductXY;
    qDebug() << "Expected Z':" << oz_transformed;

    if (!isApproximatelyEqual(crossProductXY, oz_transformed, tolerance)) {
        errorMessage = QString("Cross product X×Y should equal Z vector. Got X×Y=(%1,%2,%3), expected Z=(%4,%5,%6)")
                      .arg(crossProductXY.x()).arg(crossProductXY.y()).arg(crossProductXY.z())
                      .arg(oz_transformed.x()).arg(oz_transformed.y()).arg(oz_transformed.z());
        return false;
    }

    qDebug() << "✓ Transform validation passed - all orthonormal properties preserved";
    return true;
}
