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

#include "QProps/error.h"

#include "Bar.h"

namespace Sonot {

struct Bar::Private
{
    Private(Bar* p)
        : p     (p)
    { }

    Bar* p;
    std::vector<Notes> rows;
};

Bar::Bar()
    : p_    (new Private(this))
{

}

Bar::~Bar()
{
    delete p_;
}

Bar::Bar(const Bar& o)
    : Bar()
{
    *this = o;
}

Bar& Bar::operator = (const Bar& o)
{
    p_->rows = o.p_->rows;
    return *this;
}


Bar::ConstIter Bar::begin() const { return p_->rows.begin(); }
Bar::ConstIter Bar::end() const { return p_->rows.end(); }
Bar::Iter Bar::begin() { return p_->rows.begin(); }
Bar::Iter Bar::end() { return p_->rows.end(); }

bool Bar::isEmpty() const { return p_->rows.empty(); }
size_t Bar::numRows() const { return p_->rows.size(); }

bool Bar::operator == (const Bar& o) const
{
    return p_->rows == o.p_->rows;
}

size_t Bar::maxNumberNotes() const
{
    size_t m = 0;
    for (const Notes& n : p_->rows)
        m = std::max(m, n.length());
    return m;
}

const Notes& Bar::operator[](size_t i) const
{
    QPROPS_ASSERT_LT(i, numRows(), "");
    return p_->rows[i];
}

Notes& Bar::operator[](size_t i)
{
    QPROPS_ASSERT_LT(i, numRows(), "");
    return p_->rows[i];
}

void Bar::resize(size_t numRows)
{
    p_->rows.resize(numRows);
}

void Bar::setNotes(size_t row, const Notes &n)
{
    QPROPS_ASSERT_LT(row, numRows(), "");
    p_->rows[row] = n;
}

void Bar::append(const Notes &n)
{
    p_->rows.push_back(n);
}

void Bar::insert(size_t i, const Notes &n)
{
    if (i >= p_->rows.size())
        p_->rows.push_back(n);
    else
        p_->rows.insert(p_->rows.begin() + i, n);
}

void Bar::remove(size_t i)
{
    QPROPS_ASSERT_LT(i, numRows(), "");
    p_->rows.erase(p_->rows.begin() + i);
}

} // namespace Sonot
