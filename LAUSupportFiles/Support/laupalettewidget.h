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

#ifndef LAUPALETTEGEARWIDGET_H
#define LAUPALETTEGEARWIDGET_H

#include <QtCore>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QtWidgets>
#include <QtSerialPort>

#define LAUPALETTEOBJECTSIZE 80
#define LAUPALETTEOBJECTLAMBDA  (float)LAUPALETTEOBJECTSIZE/100.0f

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPaletteObject : public QObject
{
    Q_OBJECT

public:
    enum Orientation { OrientationA = 0, OrientationB = 1, OrientationC = 2, OrientationD = 3, OrientationE = 4 };
    enum Palette { PaletteBase = 0, PaletteButton = 1, PaletteDial = 2, PaletteSlider = 3, PaletteNone = 4 };

    LAUPaletteObject(unsigned int id, Palette typ, QObject *parent = 0) : QObject(parent), value(0), paletteID(id), paletteType(typ) { ; }
    LAUPaletteObject(unsigned int id, Palette typ, QTransform trans, QObject *parent = 0) : QObject(parent), value(0), paletteID(id), paletteType(typ), transform(trans) { ; }

    Palette type() const
    {
        return (paletteType);
    }

    unsigned int identity() const
    {
        return (paletteID);
    }

    QTransform position() const
    {
        return (transform);
    }

    void setPosition(QTransform tran)
    {
        transform = tran;
    }

    virtual void draw(QPainter *painter) const = 0;

    static QTransform neighborTransform(int i, QTransform transform);
    virtual Orientation orientation() const;
    virtual QPoint location(int a = 0) const;

public slots:
    virtual void onValueChanged(int val) = 0;
    virtual void onValueChanged(unsigned id, QList<int> samples) = 0;

protected:
    int value;

private:
    unsigned int paletteID;
    Palette paletteType;
    QTransform transform;

signals:
    void emitValueChanged(int val);
    void emitDialRotated(int val);
    void emitButtonPressed();
    void emitButtonReleased();
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPaletteBase : public LAUPaletteObject
{
    Q_OBJECT

public:
    LAUPaletteBase(unsigned int id, QObject *parent = 0) : LAUPaletteObject(id, PaletteBase, parent) { ; }
    LAUPaletteBase(unsigned int id, QTransform trans, QObject *parent = 0) : LAUPaletteObject(id, PaletteBase, trans, parent) { ; }

    void draw(QPainter *painter) const;

public slots:
    void onValueChanged(int val)
    {
        Q_UNUSED(val);
    }
    void onValueChanged(unsigned int id, QList<int> samples)
    {
        Q_UNUSED(id);
        Q_UNUSED(samples);
    }
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPaletteButton : public LAUPaletteObject
{
    Q_OBJECT

public:
    LAUPaletteButton(unsigned int id, QObject *parent = 0) : LAUPaletteObject(id, PaletteButton, parent) { ; }
    LAUPaletteButton(unsigned int id, QTransform trans, QObject *parent = 0) : LAUPaletteObject(id, PaletteButton, trans, parent) { ; }

    void draw(QPainter *painter) const;

public slots:
    void onValueChanged(int val)
    {
        Q_UNUSED(val);
    }

    void onValueChanged(unsigned id, QList<int> samples)
    {
        if (id == identity()) {
            value = samples.at(0);
            emit emitValueChanged(value);
            if (value == 1) {
                emit emitButtonPressed();
            } else {
                emit emitButtonReleased();
            }
        }
    }
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPaletteDial : public LAUPaletteObject
{
    Q_OBJECT

public:
    LAUPaletteDial(unsigned int id, QObject *parent = 0) : LAUPaletteObject(id, PaletteDial, parent), lastPosition(0), upDownFlag(true) { ; }
    LAUPaletteDial(unsigned int id, QTransform trans, QObject *parent = 0) : LAUPaletteObject(id, PaletteDial, trans, parent), lastPosition(0), upDownFlag(true) { ; }

    void draw(QPainter *painter) const;

public slots:
    void onValueChanged(int val)
    {
        Q_UNUSED(val);
    }

    void onValueChanged(unsigned int id, QList<int> samples)
    {
        if (id == identity()) {
            // SEE IF BUTTON WAS TOGGLED
            if (upDownFlag != (samples.at(0) == 0)) {
                upDownFlag = (samples.at(0) == 0);
                // SEE IF BUTTON WAS PRESSED OR RELEASED
                if (upDownFlag == true) {
                    emit emitButtonReleased();
                } else {
                    emit emitButtonPressed();
                }
            }
            value = samples.at(3);

            emit emitValueChanged(value);
            emit emitDialRotated(samples.at(2) - samples.at(1));
        }
    }

private:
    unsigned short lastPosition;
    bool upDownFlag;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPaletteSlider : public LAUPaletteObject
{
    Q_OBJECT

public:
    LAUPaletteSlider(unsigned int id, QObject *parent = 0) : LAUPaletteObject(id, PaletteSlider, parent) { ; }
    LAUPaletteSlider(unsigned int id, QTransform trans, QObject *parent = 0) : LAUPaletteObject(id, PaletteSlider, trans, parent) { ; }

    void draw(QPainter *painter) const;
    QPoint location(int a = 0) const;

public slots:
    void onValueChanged(int val)
    {
        Q_UNUSED(val);
    }
    void onValueChanged(unsigned int id, QList<int> samples)
    {
        if (id == identity()) {
            value = samples.at(0);
            switch (orientation()) {
                case LAUPaletteObject::OrientationA:
                    emit emitValueChanged(value);
                    break;
                case LAUPaletteObject::OrientationB:
                    emit emitValueChanged(value);
                    break;
                case LAUPaletteObject::OrientationC:
                    emit emitValueChanged(255 - value);
                    break;
                case LAUPaletteObject::OrientationD:
                    emit emitValueChanged(255 - value);
                    break;
                case LAUPaletteObject::OrientationE:
                    break;
            }
        }
    }
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPalette : public QObject
{
    Q_OBJECT

public:
    typedef struct {
        QPoint pos;
        LAUPaletteObject::Palette pal;
    } Packet;

    LAUPalette(QString portString = QString(), QObject *parent = 0);
#ifdef USETCP
    LAUPalette(QString ipAddr, int portNum, QObject *parent = 0);
#endif
    ~LAUPalette();

    bool isConnected(){
        if (port){
            return(port->isOpen());
        }
        return(false);
    }
    bool connectPort();
    bool disconnectPort();
    bool registerLayout(QList<LAUPalette::Packet> list);
    QRect boundingBox();
    void draw(QPainter *painter)
    {
        for (int n = 0; n < palettes.count(); n++) {
            palettes.at(n)->draw(painter);
        }
    }
    LAUPaletteObject *whatsAt(QPoint point);

private:
    QString ipAddress;
    int portNumber;
    QIODevice *port;
    QString errorString;
    QList<LAUPalette::Packet> packets;
    QList<LAUPaletteObject *> palettes;

    void processLayout(QJsonObject object, QTransform transform = QTransform());

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
#ifdef USETCP
    void onTcpError(QAbstractSocket::SocketError error);
#endif
    void onValueChanged(int val);
    void onDialRotated(int val);
    void onButtonPressed();
    void onButtonReleased();

signals:
    void emitUpdate();
    void emitConnected();
    void emitDisconnected();
    void emitError(QString string);

    void emitValueChanged(QPoint pos, int val);
    void emitDialRotated(QPoint pos, int val);
    void emitButtonPressed(QPoint pos);
    void emitButtonReleased(QPoint pos);
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPaletteLabel : public QLabel
{
    Q_OBJECT

public:
    LAUPaletteLabel(QWidget *parent = 0) : QLabel(parent) { ; }

    void setPalette(LAUPalette *pal)
    {
        palette = pal;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event);

private:
    LAUPalette *palette;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUPaletteWidget : public QWidget
{
    Q_OBJECT

public:
    LAUPaletteWidget(QString string = QString("Palette Gear"), QList<LAUPalette::Packet> list = QList<LAUPalette::Packet>(), QWidget *parent = 0);
    ~LAUPaletteWidget();

    void registerLayout(QList<LAUPalette::Packet> list);

    bool isConnected(){
        if (palette){
            return(palette->isConnected());
        }
        return(false);
    }

    virtual void paletteDisconnected() { ; }
    virtual void paletteConnected()  { ; }

public slots:
    virtual void onConnected();
    virtual void onDisconnected();
    virtual void onError(QString string);
    virtual void onValueChanged(QPoint pos, int val);
    virtual void onDialRotated(QPoint pos, int val);
    virtual void onButtonPressed(QPoint pos);
    virtual void onButtonReleased(QPoint pos);

protected:
    LAUPalette *palette;
    LAUPaletteLabel *label;
    unsigned int paletteState;
    QList<LAUPalette::Packet> packets;
    QString deviceString;
};

#endif // LAUPALETTEGEARWIDGET_H
