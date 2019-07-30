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
#include <QFutureWatcher>
#include <QRectF>

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
        ProgressRole,
        PageRole,
        RectRole,
        TextRole,
        MatchCaseRole,
        WholeWordsRole,
        MatchedTextRole,
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

    void updateProgress(DocumentView* view);

protected slots:
    void on_fetchSurroundingText_finished();

private:
    Q_DISABLE_COPY(SearchModel)

    static SearchModel* s_instance;
    SearchModel(QObject* parent = 0);

    QVector< DocumentView* > m_views;

    QModelIndex findView(DocumentView* view) const;
    QModelIndex findOrInsertView(DocumentView* view);


    typedef QPair< int, QRectF > Result;
    typedef QList< Result > Results;

    QHash< DocumentView*, Results* > m_results;


    typedef QPair< DocumentView*, QByteArray > TextCacheKey;
    typedef QPair< QString, QString > TextCacheObject;

    struct TextJob
    {
        TextCacheKey key;
        TextCacheObject* object;

        TextJob() : key(), object(0) {}
        TextJob(const TextCacheKey& key, TextCacheObject* object) : key(key), object(object) {}

    };

    typedef QFutureWatcher< TextJob > TextWatcher;

    mutable QCache< TextCacheKey, TextCacheObject > m_textCache;
    mutable QHash< TextCacheKey, TextWatcher* > m_textWatchers;

    QString fetchMatchedText(DocumentView* view, const Result& result) const;
    QString fetchSurroundingText(DocumentView* view, const Result& result) const;

    const TextCacheObject* fetchText(DocumentView* view, const Result& result) const;

    static TextCacheKey textCacheKey(DocumentView* view, const Result& result);
    static TextJob textJob(const TextCacheKey& key, const Result& result);

};

} // qpdfview

#endif // SEARCHMODEL_H
