/***************************************************************************

Copyright (C) 2016  stefan.berke @ modular-audio-graphics.com

This source is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this software; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

****************************************************************************/

#ifndef QPROPSTEST_SRC_EXAMPLE1_H
#define QPROPSTEST_SRC_EXAMPLE1_H


#include <QtCore>
#include <QPoint>
#include <QColor>

#include "Properties.h"
#include "JsonInterface.h"

class Example1 : public QProps::JsonInterface
{
    Q_DECLARE_TR_FUNCTIONS(Example2)
public:

    Example1(const QString& name = QString())
        : p_props("example1")
    {
        // create the properties
        p_initProps();
        // init with stuff from constructor
        p_props.set("user-name", name);
    }

    // --- getter ---

    const QProps::Properties& properties() const { return p_props; }

    QString name() const     { return p_props.get("user-name").toString(); }
    QPoint  position() const { return p_props.get("pos").toPoint(); }
    QColor  color() const    { return p_props.get("color").value<QColor>(); }

    // --- setter ---

    void setProperties(const QProps::Properties& p) { p_props = p; }

    // --- io ---

    // interface to serialize to json
    QJsonObject toJson() const override
    {
        return p_props.toJson();
    }

    // interface to deserialize from json
    void fromJson(const QJsonObject& o) override
    {
        // this will throw any error
        p_props.fromJson(o);
    }


private:
    void p_initProps()
    {
        p_props.set("user-name", tr("name"),
                    tr("A user-defined name of the object"),
                    QString());

        p_props.set("pos", tr("position"),
                    tr("The position in pixels on screen"),
                    QPoint(0,0));
        // set the lowest possible position to 0,0
        p_props.setMin("pos", QPoint(0,0));

        p_props.set("color", tr("color"),
                    tr("The color of the object"),
                    QColor(Qt::green));
    }

    QProps::Properties p_props;
};



#endif // QPROPSTEST_SRC_EXAMPLE1_H

