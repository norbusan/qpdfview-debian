/*

Copyright 2014 S. Razi Alavizadeh
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

#ifndef SEARCHMODEL_H
#define SEARCHMODEL_H

#include <QAbstractItemModel>
#include <QCache>
#include <QRectF>
#include <QThread>

namespace qpdfview
{

class DocumentView;

class SearchModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    static SearchModel* instance();
    ~SearchModel();


    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

    QModelIndex parent(const QModelIndex& child) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    enum
    {
        CountRole = Qt::UserRole + 1,
        PageRole,
        RectRole,
        TextRole,
        MatchCaseRole,
        SurroundingTextRole
    };

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;


    DocumentView* viewForIndex(const QModelIndex& index) const;


    bool hasResults(DocumentView* view) const;
    bool hasResultsOnPage(DocumentView* view, int page) const;
    int numberOfResultsOnPage(DocumentView* view, int page) const;
    QList< QRectF > resultsOnPage(DocumentView* view, int page) const;

    enum FindDirection
    {
        FindNext,
        FindPrevious
    };

    QPersistentModelIndex findResult(DocumentView* view, const QPersistentModelIndex& currentResult, int currentPage, FindDirection direction) const;

    void insertResults(DocumentView* view, int page, const QList< QRectF >& resultsOnPage);
    void clearResults(DocumentView* view);

protected slots:
    void on_textThread_finished();

private:
    Q_DISABLE_COPY(SearchModel)

    static SearchModel* s_instance;
    SearchModel(QObject* parent = 0);

    QList< DocumentView* > m_views;

    QModelIndex findView(DocumentView* view) const;
    QModelIndex findOrInsertView(DocumentView* view);


    typedef QPair< int, QRectF > Result;
    typedef QList< Result > Results;

    QHash< DocumentView*, Results* > m_results;


    typedef QPair< DocumentView*, QByteArray > TextCacheKey;
    typedef QString TextCacheObject;

    struct TextThread : public QThread
    {
        DocumentView* view;
        Result result;

        QString text;

        void run();

    };

    mutable QCache< TextCacheKey, TextCacheObject > m_textCache;
    mutable TextThread m_textThread;

    static TextCacheKey surroundingTextCacheKey(DocumentView* view, const Result& result);
    QString surroundingText(DocumentView* view, const Result& result) const;

};

} // qpdfview

#endif // SEARCHMODEL_H
