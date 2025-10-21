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

#include "lauprojectscantocameraglfilter.h"
#include <locale.h>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUProjectScanToCameraDialog::LAUProjectScanToCameraDialog(LAUScan scn, QWidget *parent) : QDialog(parent), scan(scn), glFilter(nullptr), validFlag(false)
{
    this->setLayout(new QVBoxLayout());
    this->setWindowTitle(QString("Bilateral Filter Dialog"));
#ifdef Q_OS_WIN
    this->layout()->setContentsMargins(6, 6, 6, 6);
#else
    this->layout()->setContentsMargins(6, 6, 6, 6);
#endif

    // KEEP A LOCAL COPY OF THE INCOMING SCAN FOR THE OUTPUT
    result = scan;

    // CREATE A GLWIDGET TO DISPLAY THE SCAN
    scanWidget = new LAU3DFiducialGLWidget(result.width(), result.height(), result.color());
    scanWidget->onUpdateBuffer(result);
    scanWidget->setLimits(result.minX(), result.maxX(), result.minY(), result.maxY(), result.minZ(), result.maxZ(), result.centroid().x(), result.centroid().y(), result.centroid().z());
    scanWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scanWidget->setFocusPolicy(Qt::StrongFocus);
    scanWidget->setMinimumSize(320, 240);
    scanWidget->onEnableFiducials(false);
    this->layout()->addWidget(scanWidget);

    QWidget *widget = new QWidget();
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    widget->setLayout(new QGridLayout());
    widget->setContentsMargins(0, 0, 0, 0);
    this->layout()->addWidget(widget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
    this->layout()->addWidget(buttonBox);

    // INITIALIZE THE CURVATURE FILTER
    if (scan.isValid()) {
        glFilter = new LAUProjectScanToCameraGLFilter(scan.width(), scan.height(), scan.color());
    }

    // ASK USER TO SELECT A LOOK UP TABLE
    LAULookUpTable table((QString()));
    if (table.isValid()) {
        glFilter->setLookUpTable(table);
    }

    // SET VALID FLAG SO USER KNOWS THEY CAN EXECUTE THIS DIALOG
    validFlag = scan.isValid() && table.isValid();
    glFilter->onUpdateScan(scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUProjectScanToCameraDialog::~LAUProjectScanToCameraDialog()
{
    ;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUProjectScanToCameraDialog::onPreview()
{
    if (glFilter) {
        glFilter->onUpdateScan(scan);
        glFilter->grabScan((float *)result.pointer());
        scanWidget->onUpdateBuffer(result);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUProjectScanToCameraGLFilter::LAUProjectScanToCameraGLFilter(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent) : QOpenGLContext(parent)
{
    // INITIALIZE PRIVATE VARIABLES
    numCols = cols;
    numRows = rows;
    playbackColor = color;
    frameBufferObject = nullptr;
    pixelMappingBufferObject = nullptr;
    textureScan = nullptr;

    // SEE IF THE USER GAVE US A TARGET SURFACE, IF NOT, THEN CREATE AN OFFSCREEN SURFACE BY DEFAULT
    surface = new QOffscreenSurface();
    ((QOffscreenSurface *)surface)->create();

    // NOW SEE IF WE HAVE A VALID PROCESSING CONTEXT FROM THE USER, AND THEN SPIN IT INTO ITS OWN THREAD
    this->setFormat(surface->format());
    this->create();
    this->initialize();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUProjectScanToCameraGLFilter::LAUProjectScanToCameraGLFilter(LAUScan scan, unsigned int rds, QWidget *parent) : QOpenGLContext(parent)
{
    Q_UNUSED(rds);
    // INITIALIZE PRIVATE VARIABLES
    numCols = scan.width();
    numRows = scan.height();
    playbackColor = scan.color();
    frameBufferObject = nullptr;
    pixelMappingBufferObject = nullptr;
    textureScan = nullptr;

    // SEE IF THE USER GAVE US A TARGET SURFACE, IF NOT, THEN CREATE AN OFFSCREEN SURFACE BY DEFAULT
    surface = new QOffscreenSurface();
    ((QOffscreenSurface *)surface)->create();

    // NOW SEE IF WE HAVE A VALID PROCESSING CONTEXT FROM THE USER, AND THEN SPIN IT INTO ITS OWN THREAD
    this->setFormat(surface->format());
    this->create();
    this->initialize();

    this->onUpdateScan(scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUProjectScanToCameraGLFilter::~LAUProjectScanToCameraGLFilter()
{
    if (surface && makeCurrent(surface)) {
        if (pixelMappingBufferObject){
            delete pixelMappingBufferObject;
        }
        if (frameBufferObject) {
            delete frameBufferObject;
        }
        if (textureScan) {
            delete textureScan;
        }
        if (wasInitialized()) {
            vertexArrayObject.release();
        }
        doneCurrent();
        delete surface;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUProjectScanToCameraGLFilter::initialize()
{
    if (makeCurrent(surface)) {
        initializeOpenGLFunctions();
        glClearColor(NAN, NAN, NAN, NAN);

        // GET CONTEXT OPENGL-VERSION
        qDebug() << "Really used OpenGl: " << format().majorVersion() << "." << format().minorVersion();
        qDebug() << "OpenGl information: VENDOR:       " << reinterpret_cast<const char *>(glGetString(GL_VENDOR));
        qDebug() << "                    RENDERDER:    " << reinterpret_cast<const char *>(glGetString(GL_RENDERER));
        qDebug() << "                    VERSION:      " << reinterpret_cast<const char *>(glGetString(GL_VERSION));
        qDebug() << "                    GLSL VERSION: " << reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));

        // CREATE THE VERTEX ARRAY OBJECT FOR FEEDING VERTICES TO OUR SHADER PROGRAMS
        vertexArrayObject.create();
        vertexArrayObject.bind();

        // CREATE A BUFFER TO HOLD THE ROW AND COLUMN COORDINATES OF IMAGE PIXELS FOR THE TEXEL FETCHES
        vertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        vertexBuffer.create();
        vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        if (vertexBuffer.bind()) {
            vertexBuffer.allocate(16 * sizeof(float));
            float *vertices = (float *)vertexBuffer.map(QOpenGLBuffer::WriteOnly);
            if (vertices) {
                vertices[0]  = -1.0;
                vertices[1]  = -1.0;
                vertices[2]  = 0.0;
                vertices[3]  = 1.0;
                vertices[4]  = +1.0;
                vertices[5]  = -1.0;
                vertices[6]  = 0.0;
                vertices[7]  = 1.0;
                vertices[8]  = +1.0;
                vertices[9]  = +1.0;
                vertices[10] = 0.0;
                vertices[11] = 1.0;
                vertices[12] = -1.0;
                vertices[13] = +1.0;
                vertices[14] = 0.0;
                vertices[15] = 1.0;

                vertexBuffer.unmap();
            } else {
                qDebug() << QString("Unable to map vertexBuffer from GPU.");
            }
            vertexBuffer.release();
        }

        // CREATE AN INDEX BUFFER FOR THE INCOMING DEPTH VIDEO DRAWN AS POINTS
        // CREATE AN INDEX BUFFER FOR THE INCOMING DEPTH VIDEO DRAWN AS POINTS
        indexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        indexBuffer.create();
        indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        if (indexBuffer.bind()) {
            indexBuffer.allocate(6 * sizeof(unsigned int));
            unsigned int *indices = (unsigned int *)indexBuffer.map(QOpenGLBuffer::WriteOnly);
            if (indices) {
                indices[0] = 0;
                indices[1] = 1;
                indices[2] = 2;
                indices[3] = 0;
                indices[4] = 2;
                indices[5] = 3;
                indexBuffer.unmap();
            } else {
                qDebug() << QString("indexBuffer buffer mapped from GPU.");
            }
            indexBuffer.release();
        }

        pixlVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        pixlVertexBuffer.create();
        pixlVertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        if (pixlVertexBuffer.bind()) {
            pixlVertexBuffer.allocate(static_cast<int>(numCols * numRows * 2 * sizeof(float)));
            float *vertices = (float *)pixlVertexBuffer.map(QOpenGLBuffer::WriteOnly);
            if (vertices) {
                for (unsigned int row = 0; row < numRows; row++) {
                    for (unsigned int col = 0; col < numCols; col++) {
                        vertices[2 * (col + row * numCols) + 0] = col;
                        vertices[2 * (col + row * numCols) + 1] = row;
                    }
                }
                pixlVertexBuffer.unmap();
            } else {
                qDebug() << QString("Unable to map vertexBuffer from GPU.");
            }
        }

        // CREATE AN INDEX BUFFER FOR THE RESULTING POINT CLOUD DRAWN AS TRIANGLES
        int index = 0;
        pixlIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        pixlIndexBuffer.create();
        pixlIndexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        if (pixlIndexBuffer.bind()) {
            pixlIndexBuffer.allocate(static_cast<int>(numRows * numCols * 6 * sizeof(unsigned int)));
            unsigned int *indices = (unsigned int *)pixlIndexBuffer.map(QOpenGLBuffer::WriteOnly);
            if (indices) {
                for (unsigned int row = 0; row < numRows - 1; row++) {
                    for (unsigned int col = 0; col < numCols - 1; col++) {
                        indices[index++] = (row + 0) * numCols + (col + 0);
                        indices[index++] = (row + 0) * numCols + (col + 1);
                        indices[index++] = (row + 1) * numCols + (col + 1);

                        indices[index++] = (row + 0) * numCols + (col + 0);
                        indices[index++] = (row + 1) * numCols + (col + 1);
                        indices[index++] = (row + 1) * numCols + (col + 0);
                    }
                }
                pixlIndexBuffer.unmap();
            } else {
                qDebug() << QString("Unable to map indiceBuffer from GPU.");
            }
        }

        // CREATE GLSL PROGRAM FOR PROCESSING THE INCOMING VIDEO
        setlocale(LC_NUMERIC, "C");
        program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/SCANTOGRAY/ScanToGrayFilters/scanToMask.vert");
        program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/SCANTOGRAY/ScanToGrayFilters/scanToMask.frag");
        program.link();

        pixelMappingProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/FILTERS/SCANTOGRAY/ScanToGrayFilters/scanToMaskMapping.vert");
        pixelMappingProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/FILTERS/SCANTOGRAY/ScanToGrayFilters/scanToMaskMapping.frag");
        pixelMappingProgram.link();
        setlocale(LC_ALL, "");

        // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE INCOMING VIDEO
        textureScan = new QOpenGLTexture(QOpenGLTexture::Target2D);
        textureScan->setSize(numCols, numRows);
        textureScan->setFormat(QOpenGLTexture::RGBA32F);
        textureScan->setWrapMode(QOpenGLTexture::ClampToBorder);
        textureScan->setMinificationFilter(QOpenGLTexture::Nearest);
        textureScan->setMagnificationFilter(QOpenGLTexture::Nearest);
        textureScan->allocateStorage();

        // CREATE A FORMAT OBJECT FOR CREATING THE FRAME BUFFER
        QOpenGLFramebufferObjectFormat frameBufferObjectFormat;
        frameBufferObjectFormat.setInternalTextureFormat(GL_RGBA32F);

        frameBufferObject = new QOpenGLFramebufferObject(QSize(textureScan->width(), textureScan->height()), frameBufferObjectFormat);
        frameBufferObject->release();

        pixelMappingBufferObject = new QOpenGLFramebufferObject(QSize(textureScan->width()+100, textureScan->height()+100), frameBufferObjectFormat);
        pixelMappingBufferObject->release();

        // RELEASE THIS CONTEXT AS THE CURRENT GL CONTEXT
        doneCurrent();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUProjectScanToCameraGLFilter::onUpdateScan(LAUScan scan)
{
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        switch (playbackColor) {
            case ColorGray:
                textureScan->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            case ColorRGB:
            case ColorXYZ:
            case ColorXYZRGB:
                textureScan->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            case ColorRGBA:
            case ColorXYZW:
            case ColorXYZG:
            case ColorXYZWRGBA:
                textureScan->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());
                break;
            default:
                break;
        }

        // BUILD MAP OF UNDISTORTED PIXEL COORDINATES TO DISTORTED COORDINATE
        if (pixelMappingBufferObject->bind()) {
            if (pixelMappingProgram.bind()) {
                // CLEAR THE FRAME BUFFER OBJECT
                glViewport(0, 0, pixelMappingBufferObject->width(), pixelMappingBufferObject->height());
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                if (pixlVertexBuffer.bind()) {
                    if (pixlIndexBuffer.bind()) {
                        pixelMappingProgram.setUniformValue("qt_size", QPointF(table.width(), table.height()));
                        pixelMappingProgram.setUniformValue("qt_fx", (float)table.intrinsics().fx);
                        pixelMappingProgram.setUniformValue("qt_cx", (float)table.intrinsics().cx);
                        pixelMappingProgram.setUniformValue("qt_fy", (float)table.intrinsics().fy);
                        pixelMappingProgram.setUniformValue("qt_cy", (float)table.intrinsics().cy);
                        pixelMappingProgram.setUniformValue("qt_k1", (float)table.intrinsics().k1);
                        pixelMappingProgram.setUniformValue("qt_k2", (float)table.intrinsics().k2);
                        pixelMappingProgram.setUniformValue("qt_k3", (float)table.intrinsics().k3);
                        pixelMappingProgram.setUniformValue("qt_p1", (float)table.intrinsics().p1);
                        pixelMappingProgram.setUniformValue("qt_p2", (float)table.intrinsics().p2);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(static_cast<unsigned int>(pixelMappingProgram.attributeLocation("qt_vertex")), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
                        pixelMappingProgram.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, static_cast<int>((numCols - 1) * (numRows - 1) * 6), GL_UNSIGNED_INT, nullptr);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        pixlIndexBuffer.release();
                    }
                    pixlVertexBuffer.release();
                }
                pixelMappingProgram.release();
            }
            pixelMappingBufferObject->release();
        }

        //LAUMemoryObject object(pixelMappingBufferObject->width(), pixelMappingBufferObject->height(), 4, sizeof(float));
        //glBindTexture(GL_TEXTURE_2D, pixelMappingBufferObject->texture());
        //glPixelStorei(GL_PACK_ALIGNMENT, 1);
        //glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, object.constPointer());
        //object.save(QString("/Users/dllau/pixelMappingBufferObject.tif"));

        // GENERATE MAPPING OF SCAN POINTS TO DISTORTED PIXEL COORDINATES
        if (frameBufferObject && frameBufferObject->bind()) {
            if (program.bind()) {
                // CLEAR THE FRAME BUFFER OBJECT
                glViewport(0, 0, frameBufferObject->width(), frameBufferObject->height());
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // BIND THE TEXTURE HOLDING THE XYZG SCAN
                        glActiveTexture(GL_TEXTURE0);
                        textureScan->bind();
                        program.setUniformValue("qt_texture",   0);

                        // BIND THE TEXTURE FROM THE MAPPING FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, pixelMappingBufferObject->texture());
                        program.setUniformValue("qt_mapping", 1);

                        // SET THE CAMERA EXTRINSIC ROTATION PROJECTION MATRIX
                        program.setUniformValue("qt_projection", table.transform());    // THIS MATRIX IS THE PROJECTION MATRIX

                        // SET THE WORLD TO UNDISTORTED CAMERA COORDINATE PARAMETERS
                        program.setUniformValue("qt_fx", (float)table.intrinsics().fx);
                        program.setUniformValue("qt_cx", (float)table.intrinsics().cx);
                        program.setUniformValue("qt_fy", (float)table.intrinsics().fy);
                        program.setUniformValue("qt_cy", (float)table.intrinsics().cy);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(program.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        program.enableAttributeArray("qt_vertex");
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                program.release();
            }
            frameBufferObject->release();
        }

        LAUMemoryObject object(frameBufferObject->width(), frameBufferObject->height(), 4, sizeof(float));
        glBindTexture(GL_TEXTURE_2D, frameBufferObject->texture());
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, object.constPointer());
        object.save(QString("/Users/dllau/pixelBufferObject.tif"));

        // RELEASE GLCONTEXT
        doneCurrent();
    }
    emit emitScan(scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUProjectScanToCameraGLFilter::grabScan(float *buffer)
{
    if (buffer) {
        // SET THE GRAPHICS CARD CONTEXT TO THIS ONE
        makeCurrent(surface);

        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        glBindTexture(GL_TEXTURE_2D, frameBufferObject->texture());
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        switch (playbackColor) {
            case ColorGray:
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, (unsigned char *)buffer);
                break;
            case ColorRGB:
            case ColorXYZ:
            case ColorXYZRGB:
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, (unsigned char *)buffer);
                break;
            case ColorRGBA:
            case ColorXYZW:
            case ColorXYZG:
            case ColorXYZWRGBA:
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)buffer);
                break;
            default:
                break;
        }
    }
}
