/*

Copyright 2014 Adam Reichold

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

#include <QMutex>

extern "C"
{

#include <mupdf/fitz.h>

}

#include "model.h"

class FitzPlugin;

namespace Model
{
    class FitzPage : public Page
    {
        friend class FitzDocument;

    public:
        ~FitzPage();

        QSizeF size() const;

        QImage render(qreal horizontalResolution, qreal verticalResolution, Rotation rotation, const QRect& boundingRect) const;

    private:
        Q_DISABLE_COPY(FitzPage)

        FitzPage(QMutex* mutex, fz_context* context, fz_document* document, fz_page* page);

        mutable QMutex* m_mutex;
        fz_context* m_context;
        fz_document* m_document;
        fz_page* m_page;

    };

    class FitzDocument : public Document
    {
        friend class ::FitzPlugin;

    public:
        ~FitzDocument();

        int numberOfPages() const;

        Page* page(int index) const;

    private:
        Q_DISABLE_COPY(FitzDocument)

        FitzDocument(fz_context* context, fz_document* document);

        mutable QMutex m_mutex;
        fz_context* m_context;
        fz_document* m_document;

    };
}

class FitzPlugin : public QObject, Plugin
{
    Q_OBJECT
    Q_INTERFACES(Plugin)

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

    Q_PLUGIN_METADATA(IID "local.qpdfview.Plugin")

#endif // QT_VERSION

public:
    FitzPlugin(QObject* parent = 0);
    ~FitzPlugin();

    Model::Document* loadDocument(const QString& filePath) const;

private:
    QMutex m_mutex[FZ_LOCK_MAX];
    fz_locks_context m_locks_context;
    fz_context* m_context;

    static void lock(void* user, int lock);
    static void unlock(void* user, int lock);

};
