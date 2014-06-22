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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QPair>

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

struct RenderResolution
{
    int resolutionX;
    int resolutionY;
    qreal devicePixelRatio;

    RenderResolution(int resolutionX = 72, int resolutionY = 72,
                     qreal devicePixelRatio = 1.0) :
        resolutionX(resolutionX),
        resolutionY(resolutionY),
        devicePixelRatio(devicePixelRatio) {}

    bool operator==(const RenderResolution& other) const
    {
        return resolutionX == other.resolutionX
                && resolutionY == other.resolutionY
                && qFuzzyCompare(devicePixelRatio, other.devicePixelRatio);
    }

    bool operator!=(const RenderResolution& other) const
    {
        return !operator==(other);
    }

    bool operator<(const RenderResolution& other) const
    {
        return (resolutionX < other.resolutionX)
                || (resolutionX == other.resolutionX && resolutionY < other.resolutionY)
                || (resolutionX == other.resolutionX && resolutionY == other.resolutionY && devicePixelRatio < other.devicePixelRatio);
    }

};

struct RenderParam
{
    RenderResolution resolution;

    qreal scaleFactor;
    Rotation rotation;
    bool invertColors;

    RenderParam(const RenderResolution& resolution = RenderResolution(),
                qreal scaleFactor = 1.0, Rotation rotation = RotateBy0, bool invertColors = false) :
        resolution(resolution),
        scaleFactor(scaleFactor),
        rotation(rotation),
        invertColors(invertColors) {}

    bool operator==(const RenderParam& other) const
    {
        return resolution == other.resolution
                && qFuzzyCompare(scaleFactor, other.scaleFactor)
                && rotation == other.rotation
                && invertColors == other.invertColors;
    }

    bool operator!=(const RenderParam& other) const
    {
        return !operator==(other);
    }

    bool operator<(const RenderParam& other) const
    {
        return (resolution < other.resolution)
                || (resolution == other.resolution && scaleFactor < other.scaleFactor)
                || (resolution == other.resolution && qFuzzyCompare(scaleFactor, other.scaleFactor) && invertColors < other.invertColors);
    }

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

typedef QPair< int, QString > Jump;
typedef QList< QPair< int, QString > > JumpList;

} // qpdfview

#endif // GLOBAL_H
