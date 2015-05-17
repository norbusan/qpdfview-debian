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

enum RenderFlag
{
    InvertColors = 1 << 0,
    ConvertToGrayscale = 1 << 1,
    TrimMargins = 1 << 2
};

Q_DECLARE_FLAGS(RenderFlags, RenderFlag)

struct RenderParam
{
    int resolutionX;
    int resolutionY;
    qreal devicePixelRatio;

    qreal scaleFactor;
    Rotation rotation;

    RenderFlags flags;

    bool invertColors() const { return flags.testFlag(InvertColors); }
    bool convertToGrayscale() const { return flags.testFlag(ConvertToGrayscale); }
    bool trimMargins() const { return flags.testFlag(TrimMargins); }

    void setFlag(RenderFlag flag, bool on)
    {
        if(on)
        {
            flags |= flag;
        }
        else
        {
            flags &= ~flag;
        }
    }

    RenderParam(int resolutionX = 72, int resolutionY = 72,
                qreal devicePixelRatio = 1.0,
                qreal scaleFactor = 1.0, Rotation rotation = RotateBy0,
                RenderFlags flags = 0) :
        resolutionX(resolutionX),
        resolutionY(resolutionY),
        devicePixelRatio(devicePixelRatio),
        scaleFactor(scaleFactor),
        rotation(rotation),
        flags(flags) {}

    bool operator==(const RenderParam& other) const
    {
        return resolutionX == other.resolutionX
                && resolutionY == other.resolutionY
                && qFuzzyCompare(devicePixelRatio, other.devicePixelRatio)
                && qFuzzyCompare(scaleFactor, other.scaleFactor)
                && rotation == other.rotation
                && flags == other.flags;
    }

    bool operator!=(const RenderParam& other) const { return !operator==(other); }

};

inline QDataStream& operator<<(QDataStream& stream, const RenderParam& that)
{
    stream << that.resolutionX
           << that.resolutionY
           << that.devicePixelRatio
           << that.scaleFactor
           << that.rotation
           << static_cast< int >(that.flags);

   return stream;
}

} // qpdfview

#endif // RENDERPARAM_H

