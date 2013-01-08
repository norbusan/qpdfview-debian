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
