#include "psmodel.h"

PSDocumentLoader::PSDocumentLoader(QObject* parent) : QObject(parent)
{
    setObjectName("PSDocumentLoader");
}

Document* PSDocumentLoader::loadDocument(const QString& filePath) const
{
    return 0;
}

Q_EXPORT_PLUGIN2(qpdfview_ps, PSDocumentLoader)
