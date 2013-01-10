#ifndef PSMODEL_H
#define PSMODEL_H

#include "model.h"

class PSDocumentLoader : public QObject, DocumentLoader
{
    Q_OBJECT
    Q_INTERFACES(DocumentLoader)

public:
    PSDocumentLoader(QObject* parent = 0);

    Document* loadDocument(const QString& filePath) const;

};

#endif
