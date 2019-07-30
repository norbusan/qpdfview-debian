/*

Copyright 2013 Adam Reichold
Copyright 2013 Alexander Volkov

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

#ifndef PRINTOPTIONS_H
#define PRINTOPTIONS_H

namespace qpdfview
{

struct PrintOptions
{
    bool fitToPage;

    QString pageRanges;

#if QT_VERSION < QT_VERSION_CHECK(5,2,0)

    enum PageSet
    {
        AllPages = 0,
        EvenPages = 1,
        OddPages = 2
    };

    PageSet pageSet;

    enum NumberUp
    {
        SinglePage = 0,
        TwoPages = 1,
        FourPages = 2,
        SixPages = 3,
        NinePages = 4,
        SixteenPages = 5
    };

    NumberUp numberUp;

    enum NumberUpLayout
    {
        BottomTopLeftRight = 0,
        BottomTopRightLeft = 1,
        LeftRightBottomTop = 2,
        LeftRightTopBottom = 3,
        RightLeftBottomTop = 4,
        RightLeftTopBottom = 5,
        TopBottomLeftRight = 6,
        TopBottomRightLeft = 7
    };

    NumberUpLayout numberUpLayout;

    PrintOptions() : fitToPage(false), pageRanges(), pageSet(AllPages), numberUp(SinglePage), numberUpLayout(LeftRightTopBottom) {}

#else

    PrintOptions() : fitToPage(false), pageRanges() {}

#endif // QT_VERSION

};

} // qpdfview

#endif // PRINTOPTIONS_H
