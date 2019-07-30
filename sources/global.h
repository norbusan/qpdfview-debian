/*

Copyright 2013, 2015 Adam Reichold
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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QtGlobal>
#include <qnumeric.h>

namespace qpdfview
{

enum Rotation
{
    RotateBy0 = 0,
    RotateBy90 = 1,
    RotateBy180 = 2,
    RotateBy270 = 3,
    NumberOfRotations = 4
};

enum RubberBandMode
{
    ModifiersMode = 0,
    CopyToClipboardMode = 1,
    AddAnnotationMode = 2,
    ZoomToSelectionMode = 3,
    NumberOfRubberBandModes = 4
};

enum LayoutMode
{
    SinglePageMode = 0,
    TwoPagesMode = 1,
    TwoPagesWithCoverPageMode = 2,
    MultiplePagesMode = 3,
    NumberOfLayoutModes = 4
};

enum ScaleMode
{
    ScaleFactorMode = 0,
    FitToPageWidthMode = 1,
    FitToPageSizeMode = 2,
    NumberOfScaleModes = 3
};

enum CompositionMode
{
    DefaultCompositionMode = 0,
    DarkenWithPaperColorMode = 1,
    LightenWithPaperColorMode = 2,
    NumberOfCompositionModes = 3
};

} // qpdfview

#endif // GLOBAL_H
