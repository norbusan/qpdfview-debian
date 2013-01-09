/*

Copyright 2012 Adam Reichold

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

#include "model.h"

#include <QString>
#include <QFileInfo>

#include "pdfmodel.h"
#include "psmodel.h"

Document* Document::load(const QString& filePath)
{
    QFileInfo fileInfo(filePath);

#ifdef WITH_PDF

    if(fileInfo.suffix() == "pdf")
    {
        return PDFDocument::load(filePath);
    }

#endif // WITH_PDF

    /* TODO
#ifdef WITH_PS

    if(fileInfo.suffix() == "ps")
    {
        return PSDocument::load(filePath);
    }

#endif // WITH_PS
    */

    return 0;
}
