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

#include "searchitemdelegate.h"

#include <QApplication>
#include <qmath.h>
#include <QPainter>
#include <QTextLayout>

#include "searchmodel.h"

namespace qpdfview
{

SearchItemDelegate::SearchItemDelegate(QObject* parent) : QStyledItemDelegate(parent)
{
}

void SearchItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    if(index.column() != 0)
    {
        return;
    }

    const int progress = index.data(SearchModel::ProgressRole).toInt();

    if(progress != 0)
    {
        paintProgress(painter, option, progress);
        return;
    }

    const QString matchedText = index.data(SearchModel::MatchedTextRole).toString();
    const QString surroundingText = index.data(SearchModel::SurroundingTextRole).toString();

    if(!matchedText.isEmpty() && !surroundingText.isEmpty())
    {
        paintText(painter, option, matchedText, surroundingText);
        return;
    }
}

void SearchItemDelegate::paintProgress(QPainter* painter, const QStyleOptionViewItem& option,
                                       int progress) const
{
    QRect highlightedRect = option.rect;
    highlightedRect.setWidth(progress * highlightedRect.width() / 100);

    painter->save();

    painter->setCompositionMode(QPainter::CompositionMode_Multiply);
    painter->fillRect(highlightedRect, option.palette.highlight());

    painter->restore();
}

void SearchItemDelegate::paintText(QPainter* painter, const QStyleOptionViewItem& option,
                                   const QString& matchedText, const QString& surroundingText) const
{
    const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    const QRect textRect = option.rect.adjusted(textMargin, 0, -textMargin, 0);
    const QString elidedText = option.fontMetrics.elidedText(surroundingText, option.textElideMode, textRect.width());

    QTextOption textOption;
    textOption.setWrapMode(QTextOption::NoWrap);
    textOption.setTextDirection(elidedText.isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);
    textOption.setAlignment(QStyle::visualAlignment(textOption.textDirection(), option.displayAlignment));

    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setText(elidedText);
    textLayout.setFont(option.font);

    QFont font = textLayout.font();
    font.setWeight(QFont::Light);
    textLayout.setFont(font);


    QList< QTextLayout::FormatRange > additionalFormats;

    for(int index = 0; (index = elidedText.indexOf(matchedText, index)) != -1; index += matchedText.length())
    {
        QTextLayout::FormatRange formatRange;
        formatRange.start = index;
        formatRange.length = matchedText.length();
        formatRange.format.setFontWeight(QFont::Bold);

        additionalFormats.append(formatRange);
    }

    textLayout.setAdditionalFormats(additionalFormats);


    textLayout.beginLayout();

    QTextLine textLine = textLayout.createLine();

    if(!textLine.isValid())
    {
        return;
    }

    textLine.setLineWidth(textRect.width());

    textLayout.endLayout();


    const QSize layoutSize(textRect.width(), qFloor(textLine.height()));
    const QRect layoutRect = QStyle::alignedRect(option.direction, option.displayAlignment, layoutSize, textRect);

    painter->save();

    painter->setClipping(true);
    painter->setClipRect(layoutRect);

    textLine.draw(painter, layoutRect.topLeft());

    painter->restore();
}

} // qpdfview
