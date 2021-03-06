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

#ifndef QPROPS_SRC_JSONINTERFACEHELPER_H
#define QPROPS_SRC_JSONINTERFACEHELPER_H

#include <vector>

#include <QString>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QStringList>
#include <QVector>

#include "qprops_global.h"

namespace QProps {

/** Collection of helper functions for json value conversion.
    Most functions throw QProps::Exception with the className
    defined in the constructor */
class // QPROPS_SHARED_EXPORT XXX not working yet
        JsonInterfaceHelper
{
public:

    // --- types ---

    /** @defgroup json_qvariant Json - QVariant conversion
        @{ */
    /** For mapping supported QMetaType::Type to string */
    struct VariantType
    {
        QMetaType::Type metaType;
        /** C++ name */
        QString typeName,
        /** Name as in QVariant::typeName() */
                variantName;
        bool isVector;

        bool isValid() const { return metaType != QMetaType::UnknownType; }
    };
    /** @} */

    // --- ctor ---

    JsonInterfaceHelper(const char* className)
        : p_classname_(className) { }
    JsonInterfaceHelper(const QString& className)
        : p_classname_(className) { }

    // --- info ---

    /** The name given in constructor */
    const QString& className() const { return p_classname_; }

    /** Description of current context */
    QString context() const
        { return p_context_.isEmpty() ? QString() : p_context_.last(); }

    /** Multiline string containing all current contexts */
    QString contextTrace() const;

    void beginContext(const QString& c) { p_context_.append(c); }
    void endContext() { p_context_.pop_back(); }

    /** Returns the name of the json type */
    static const char* typeName(const QJsonValue&);

    /** @ingroup json_qvariant @{ */

    /** Returns the VariantType struct corresponding to type,
        or invalid VariantType if not supported */
    static const VariantType& typeFromQMeta(QMetaType::Type);

    /** Returns the VariantType struct corresponding to the
        QVariant::typeName() string,
        or invalid VariantType if not supported */
    static const VariantType& typeFromQVariantName(const QString& typeName);
    static const VariantType& typeFromQVariant(const QVariant& v)
        { return typeFromQVariantName(v.typeName()); }

    /** @} */

    // -- convert to json --

    /** Wraps the type into a json value.
        Unwrap the value with expect() or expectChild() */
    template <typename T>
    QJsonValue wrap(const T&);

    /** @ingroup json_qvariant
        Wraps a QVariant into a json value.
        For most extended (compound) types,
        an appropriate json object will be returned.
        @param explicitTypeInfo If true, the exact type of the QVariant
        is stored in any case and the returned json value
        is always an object. If type info is not stored for a
        non-compound QVariant the type might be lost on deserializing
        a json stream, e.g. int/uint/int64 convert to double,
        or QColor converts to QString.
        @see expectQVariant() for converting back. */
    QJsonValue wrap(const QVariant& v, bool explicitTypeInfo = true);

    /** Converts the vector of type T to a json array.
        Unwrap with fromArray() */
    template <typename T>
    QJsonArray toArray(const std::vector<T>&);


    // -- throwing getters --

    /** Converts the json value to type T.
        @throws Sonot::Exception if not convertible. */
    template <typename T>
    T expect(const QJsonValue&);

    /** Gets the child value and converts it to type T.
        @throws Sonot::Exception if child not found or not convertible. */
    template <typename T>
    T expectChild(const QJsonObject& parent, const QString& key);

    /** @ingroup json_qvariant
        Converts @p v to a QVariant.
        Supports simple and compound types stored with wrap(QVariant).
        If @p type is not QMetaType::UnknownType, an explicit conversion
        to the QVariant type is attempted. */
    QVariant expectQVariant(
                const QJsonValue& v,
                QMetaType::Type expectedType = QMetaType::UnknownType);

    /** Converts the QJsonValue to a QJsonArray.
        @throws Sonot::Exception if not an array. */
    QJsonArray expectArray(const QJsonValue&);

    /** Converts the QJsonValue to a QJsonObject.
        @throws Sonot::Exception if not an object. */
    QJsonObject expectObject(const QJsonValue&);

    /** Returns the child value.
        @throws Sonot::Exception if child not found. */
    QJsonValue expectChildValue(const QJsonObject& parent, const QString& key);

    /** Returns the child of type object.
        @throws Sonot::Exception if child not found or not convertible. */
    QJsonObject expectChildObject(const QJsonObject& parent, const QString& key);

    /** Returns the child of type array.
        @throws Sonot::Exception if child not found or not convertible. */
    QJsonArray expectChildArray(const QJsonObject& parent, const QString& key);

    /** @ingroup json_qvariant
        Returns the QVariant stored in child.
        @throws Sonot::Exception if child not found or not convertible. */
    QVariant expectChildQVariant(
            const QJsonObject& parent, const QString& key,
            QMetaType::Type expectedType = QMetaType::UnknownType);

    /** Converts the json array to a vector of type T.
        Previous contents of @p dst are erased.
        @throws Sonot::Exception if an element is not convertible to T. */
    template <typename T>
    void fromArray(std::vector<T>& dst, const QJsonArray& src);

    /** Converts the json value to a vector of type T.
        Previous contents of @p dst are erased.
        @throws Sonot::Exception if @p src is not an array
        or an element is not convertible to T. */
    template <typename T>
    void fromArray(std::vector<T>& dst, const QJsonValue& src);



private:

    template <typename T>
    void p_expectArray_(const QJsonValue& src, std::vector<T>& dst,
                        size_t size, const QString& forType);
    QVariant p_convert_(const QVariant& v, QMetaType::Type newType);
    static void p_createMap_();

    QString p_classname_;
    QStringList p_context_;
    static QMap<int, VariantType> p_imap_;
    static QMap<QString, VariantType> p_vmap_, p_cmap_;
    static VariantType p_invalid_;
};


} // namespace QProps


#endif // QPROPS_SRC_JSONINTERFACEHELPER_H

