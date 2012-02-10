#include "documentmodel.h"

DocumentModel::DocumentModel(QObject *parent) :
    QObject(parent),
    m_document(0),
    m_pageList(),
    m_filePath(),
    m_index(-1),
    m_displayMode(PagingMode),
    m_scaleMode(ScaleFactorMode),
    m_scaleFactor(1.0)
{
}

DocumentModel::~DocumentModel()
{
    if(m_document) { delete m_document; }
    while(!m_pageList.isEmpty()) { delete m_pageList.takeFirst(); }
}


bool DocumentModel::load(const QString &filePath)
{
    Poppler::Document *document = Poppler::Document::load(filePath);

    if(document)
    {
        if(m_document) { delete m_document; }
        while(!m_pageList.isEmpty()) { delete m_pageList.takeFirst(); }

        m_document = document;
        for(int index=0;index<document->numPages();index++) { m_pageList.append(document->page(index)); }

        document->setRenderBackend(Poppler::Document::ArthurBackend);
        document->setRenderHint(Poppler::Document::Antialiasing);
        document->setRenderHint(Poppler::Document::TextAntialiasing);

        m_filePath = filePath;
        m_index = 1;

        emit documentChanged(m_filePath);
        emit indexChanged(m_index);
    }

    return !document;
}

bool DocumentModel::reload() {
    if(m_document)
    {
        Poppler::Document *document = Poppler::Document::load(m_filePath);

        if(document)
        {
            if(m_document) { delete m_document; }
            while(!m_pageList.isEmpty()) { delete m_pageList.takeFirst(); }

            m_document = document;
            for(int index=0;index<document->numPages();index++) { m_pageList.append(document->page(index)); }

            document->setRenderBackend(Poppler::Document::ArthurBackend);
            document->setRenderHint(Poppler::Document::Antialiasing);
            document->setRenderHint(Poppler::Document::TextAntialiasing);

            if(m_index > m_pageList.size())
            {
                m_index = 1;
            }

            emit documentChanged(m_filePath);
            emit indexChanged(m_index);
        }

        return !document;
    }
    else
    {
        return false;
    }
}

bool DocumentModel::save(const QString &filePath) const
{
    if(m_document)
    {
        Poppler::PDFConverter *pdfConverter = m_document->pdfConverter();
        pdfConverter->setOutputFileName(filePath);

        bool result = pdfConverter->convert();

        delete pdfConverter;

        return result;
    }
    else
    {
        return false;
    }
}


void DocumentModel::setIndex(const int &index)
{
    if(m_document && m_index != index && index >= 1 &&  index <= m_pageList.size())
    {
        switch(m_displayMode)
        {
        case PagingMode:
        case ScrollingMode:
            m_index = index;
            break;
        case DoublePagingMode:
        case DoubleScrollingMode:
            if(index%2==0)
            {
                m_index = index-1;
            }
            else
            {
                m_index = index;
            }
            break;
        }

        emit indexChanged(m_index);
    }
}

void DocumentModel::previousPage()
{
    if(m_document)
    {
        switch(m_displayMode)
        {
        case PagingMode:
        case ScrollingMode:
            if(m_index > 1)
            {
                m_index -= 1;

                emit indexChanged(m_index);
            }
            break;
        case DoublePagingMode:
        case DoubleScrollingMode:
            if(m_index > 2)
            {
                m_index -= 2;

                emit indexChanged(m_index);
            }
            break;
        }
    }
}

void DocumentModel::nextPage()
{
    if(m_document)
    {
        switch(m_displayMode)
        {
        case PagingMode:
        case ScrollingMode:
            if(m_index <= m_pageList.size()-1)
            {
                m_index += 1;

                emit indexChanged(m_index);
            }
            break;
        case DoublePagingMode:
        case DoubleScrollingMode:
            if(m_index <= m_pageList.size()-2)
            {
                m_index += 2;

                emit indexChanged(m_index);
            }
            break;
        }
    }
}

void DocumentModel::firstPage()
{
    if(m_document && m_index != 1)
    {
        m_index = 1;

        emit indexChanged(m_index);
    }
}

void DocumentModel::lastPage()
{
    if(m_document)
    {
        switch(m_displayMode)
        {
        case PagingMode:
        case ScrollingMode:
            if(m_index != m_pageList.size())
            {
                m_index = m_pageList.size();

                emit indexChanged(m_index);
            }
            break;
        case DoublePagingMode:
        case DoubleScrollingMode:
            if(m_pageList.size()%2==0)
            {
                if(m_index != m_pageList.size()-1)
                {
                    m_index = m_pageList.size()-1;

                    emit indexChanged(m_index);
                }
            }
            else
            {
                if(m_index != m_pageList.size())
                {
                    m_index = m_pageList.size();

                    emit indexChanged(m_index);
                }
            }
            break;
        }
    }
}


void DocumentModel::setDisplayMode(const DocumentModel::DisplayModes &displayMode)
{
    if(m_displayMode != displayMode)
    {
        m_displayMode = displayMode;

        emit displayModeChanged(m_displayMode);

        if(m_displayMode == DoublePagingMode || m_displayMode == DoubleScrollingMode)
        {
            if(m_index%2==0)
            {
                m_index = m_index-1;

                emit indexChanged(m_index);
            }
        }
    }
}


void DocumentModel::setScaleMode(const DocumentModel::ScaleModes &scaleMode)
{
    if(m_scaleMode != scaleMode)
    {
        m_scaleMode = scaleMode;

        emit scaleModeChanged(m_scaleMode);
    }
}

void DocumentModel::setScaleFactor(const qreal &scaleFactor)
{
    if(m_scaleFactor != scaleFactor && scaleFactor >= 0.25 && scaleFactor <= 4.0)
    {
        m_scaleFactor = scaleFactor;

        emit scaleFactorChanged(m_scaleFactor);
    }
}
