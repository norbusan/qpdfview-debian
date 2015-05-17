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

#ifndef RENDERPARAM_H
#define RENDERPARAM_H

#include "global.h"

namespace qpdfview
{

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

    bool operator!=(const RenderResolution& other) const { return !operator==(other); }

};

struct RenderParam
{
    RenderResolution resolution;

    qreal scaleFactor;
    Rotation rotation;

    bool invertColors;
    bool convertToGrayscale;
    bool trimMargins;

    RenderParam(const RenderResolution& resolution = RenderResolution(),
                qreal scaleFactor = 1.0, Rotation rotation = RotateBy0,
                bool invertColors = false, bool convertToGrayscale = false, bool trimMargins = false) :
        resolution(resolution),
        scaleFactor(scaleFactor),
        rotation(rotation),
        invertColors(invertColors),
        convertToGrayscale(convertToGrayscale),
        trimMargins(trimMargins) {}

    bool operator==(const RenderParam& other) const
    {
        return resolution == other.resolution
            && qFuzzyCompare(scaleFactor, other.scaleFactor)
            && rotation == other.rotation
            && invertColors == other.invertColors
            && convertToGrayscale == other.convertToGrayscale
            && trimMargins == other.trimMargins;
    }

    bool operator!=(const RenderParam& other) const { return !operator==(other); }

};

} // qpdfview

#endif // RENDERPARAM_H

