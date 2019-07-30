/*

Copyright 2015 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef SEARCHITEMDELEGATE_H
#define SEARCHITEMDELEGATE_H

#include <QStyledItemDelegate>

namespace qpdfview
{

class SearchItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SearchItemDelegate(QObject* parent = 0);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
    void paintProgress(QPainter* painter, const QStyleOptionViewItem& option,
                       int progress) const;
    void paintText(QPainter* painter, const QStyleOptionViewItem& option,
                   const QString& matchedText, const QString& surroundingText) const;

};

} // qpdfview

#endif // SEARCHITEMDELEGATE_H
