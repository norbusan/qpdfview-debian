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

#include <QSharedDataPointer>

#include "global.h"

namespace qpdfview
{

enum RenderFlag
{
    InvertColors = 1 << 0,
    ConvertToGrayscale = 1 << 1,
    TrimMargins = 1 << 2,
    DarkenWithPaperColor = 1 << 3,
    LightenWithPaperColor = 1 << 4
};

Q_DECLARE_FLAGS(RenderFlags, RenderFlag)

class RenderParam
{
public:
    RenderParam(int resolutionX = 72, int resolutionY = 72, qreal devicePixelRatio = 1.0,
                qreal scaleFactor = 1.0, Rotation rotation = RotateBy0,
                RenderFlags flags = 0) : d(new SharedData)
    {
        d->resolutionX = resolutionX;
        d->resolutionY = resolutionY;
        d->devicePixelRatio = devicePixelRatio;
        d->scaleFactor = scaleFactor;
        d->rotation = rotation;
        d->flags = flags;
    }

    int resolutionX() const { return d->resolutionX; }
    int resolutionY() const { return d->resolutionY; }
    void setResolution(int resolutionX, int resolutionY)
    {
        d->resolutionX = resolutionX;
        d->resolutionY = resolutionY;
    }

    qreal devicePixelRatio() const { return d->devicePixelRatio; }
    void setDevicePixelRatio(qreal devicePixelRatio) { d->devicePixelRatio = devicePixelRatio; }

    qreal scaleFactor() const { return d->scaleFactor; }
    void setScaleFactor(qreal scaleFactor) { d->scaleFactor = scaleFactor; }

    Rotation rotation() const { return d->rotation; }
    void setRotation(Rotation rotation) { d->rotation = rotation; }

    RenderFlags flags() const { return d->flags; }
    void setFlags(RenderFlags flags) { d->flags = flags; }
    void setFlag(RenderFlag flag, bool enabled = true)
    {
        if(enabled)
        {
            d->flags |= flag;
        }
        else
        {
            d->flags &= ~flag;
        }
    }

    bool invertColors() const { return d->flags.testFlag(InvertColors); }
    void setInvertColors(bool invertColors) { setFlag(InvertColors, invertColors); }

    bool convertToGrayscale() const { return d->flags.testFlag(ConvertToGrayscale); }
    void setConvertToGrayscale(bool convertToGrayscale) { setFlag(ConvertToGrayscale, convertToGrayscale); }

    bool trimMargins() const { return d->flags.testFlag(TrimMargins); }
    void setTrimMargins(bool trimMargins) { setFlag(TrimMargins, trimMargins); }

    bool darkenWithPaperColor() const { return d->flags.testFlag(DarkenWithPaperColor); }
    void setDarkenWithPaperColor(bool darkenWithPaperColor) { setFlag(DarkenWithPaperColor, darkenWithPaperColor); }

    bool lightenWithPaperColor() const { return d->flags.testFlag(LightenWithPaperColor); }
    void setLightenWithPaperColor(bool lightenWithPaperColor) { setFlag(LightenWithPaperColor, lightenWithPaperColor); }

    bool operator==(const RenderParam& other) const
    {
        if(d == other.d)
        {
            return true;
        }
        else
        {
            return d->resolutionX == other.d->resolutionX
                    && d->resolutionY == other.d->resolutionY
                    && qFuzzyCompare(d->devicePixelRatio, other.d->devicePixelRatio)
                    && qFuzzyCompare(d->scaleFactor, other.d->scaleFactor)
                    && d->rotation == other.d->rotation
                    && d->flags == other.d->flags;
        }
    }

    bool operator!=(const RenderParam& other) const { return !operator==(other); }

private:
    struct SharedData : public QSharedData
    {
        int resolutionX;
        int resolutionY;
        qreal devicePixelRatio;

        qreal scaleFactor;
        Rotation rotation;

        RenderFlags flags;

    };

    QSharedDataPointer< SharedData > d;

};

inline QDataStream& operator<<(QDataStream& stream, const RenderParam& that)
{
    stream << that.resolutionX()
           << that.resolutionY()
           << that.devicePixelRatio()
           << that.scaleFactor()
           << that.rotation()
           << that.flags();

   return stream;
}

} // qpdfview

#endif // RENDERPARAM_H

