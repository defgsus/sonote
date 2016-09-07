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

#include <QLayout>
#include <QMap>

#include "PropertiesView.h"
#include "PropertyWidget.h"
#include "io/Properties.h"

namespace Sonot {

struct PropertiesView::Private
{
    Private(PropertiesView* p)
        : p         (p)
        , props     ("place-holder")
        , container (nullptr)
        , stretch   (nullptr)
        , layout    (nullptr)
    { }

    void createWidgtes();
    void updateWidgetVis();

    PropertiesView* p;
    Properties props;
    QMap<QString, PropertyWidget*> widgets;
    QWidget *container, *stretch;
    QVBoxLayout* layout;

};

PropertiesView::PropertiesView(QWidget *parent)
    : QScrollArea   (parent)
    , p_            (new Private(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

PropertiesView::~PropertiesView()
{
    delete p_;
}

const Properties& PropertiesView::properties() const { return p_->props; }

void PropertiesView::setProperties(const Properties & p)
{
    p_->props = p;
    p_->createWidgtes();
}

void PropertiesView::setProperties(const QList<Properties>& list)
{
    p_->props.clear();
    for (auto & p : list)
        p_->props.unify(p);

    p_->createWidgtes();
}

void PropertiesView::clear()
{
    p_->props.clear();
    p_->createWidgtes();
}

void PropertiesView::Private::createWidgtes()
{
    p->setUpdatesEnabled(false);

    // delete previous widgets
    for (auto w : widgets)
    {
#if 0
        w->deleteLater();
#else
        // XXX For some reasons, widgets dont get removed properly
        w->setParent(0);
        delete w;
#endif
    }
    widgets.clear();
    if (stretch)
    {
        stretch->deleteLater();
        stretch = nullptr;
    }

    // widget container
    if (!container)
    {
        container = new QWidget(p);
        container->setObjectName("_properties_container");
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        container->setMinimumWidth(300);
        container->setMaximumWidth(1024);
        p->setWidget(container);
    }

    if (!layout)
    {
        // layout for property widgets
        layout = new QVBoxLayout(container);
        layout->setMargin(1);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    }

    // create one for each property
    for (auto i = props.begin(); i != props.end(); ++i)
    {
        auto widget = new PropertyWidget(i.key(), &props, p);
        widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

        // keep track
        widgets.insert(i.key(), widget);

        // install in layout
        qDebug() << "ADD " << i.value().name() << i.value().index();
        layout->insertWidget(i.value().index(), widget);

        // connect signals
        QString key = i.key();
        connect(widget, &PropertyWidget::valueChanged, [=]()
        {
            // copy to internal properties
            props.set(key, widget->value());
            // get change to visibilty
            if (props.callUpdateVisibility())
                updateWidgetVis();
            emit p->propertyChanged(key);
        });
    }

    // a "stretch" that can be deleted later
    stretch = new QWidget(container);
    layout->addWidget(stretch, 2);

    // initial visibility
    props.callUpdateVisibility();
    updateWidgetVis();

    p->setUpdatesEnabled(true);
}

void PropertiesView::Private::updateWidgetVis()
{
    for (auto i = props.begin(); i != props.end(); ++i)
    {
        auto j = widgets.find(i.key());
        if (j != widgets.end())
            j.value()->setVisible( i.value().isVisible() );
    }
}


} // namespace Sonot
