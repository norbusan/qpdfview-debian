/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2012-2014 Adam Reichold
Copyright 2014 Dorian Scholz

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

#include "miscellaneous.h"

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>

namespace
{

using namespace qpdfview;

QMetaMethod slotToMethod(const QMetaObject* metaObject, const char* slot)
{
    const int index = *slot == '1' ? metaObject->indexOfSlot(slot + 1) : -1;

    if(index < 0)
    {
        qWarning() << "Could not connect slot:" << slot;
    }

    return metaObject->method(index);
}

} // anonymous

namespace qpdfview
{

GraphicsCompositionModeEffect::GraphicsCompositionModeEffect(QPainter::CompositionMode compositionMode, QObject* parent) : QGraphicsEffect(parent),
    m_compositionMode(compositionMode)
{
}

void GraphicsCompositionModeEffect::draw(QPainter* painter)
{
    painter->save();

    painter->setCompositionMode(m_compositionMode);

    drawSource(painter);

    painter->restore();
}

TabBar::TabBar(QWidget* parent) : QTabBar(parent)
{
}

QSize TabBar::tabSizeHint(int index) const
{
    QSize size = QTabBar::tabSizeHint(index);

    const TabWidget* tabWidget = qobject_cast< TabWidget* >(parentWidget());

    if(tabWidget != 0 && tabWidget->spreadTabs())
    {
        switch(tabWidget->tabPosition())
        {
        default:
        case QTabWidget::North:
        case QTabWidget::South:
            size.setWidth(qMax(width() / count(), size.width()));
            break;
        case QTabWidget::East:
        case QTabWidget::West:
            size.setHeight(qMax(height() / count(), size.height()));
            break;
        }
    }

    return size;
}

void TabBar::mousePressEvent(QMouseEvent* event)
{
    QTabBar::mousePressEvent(event);

    if(event->button() == Qt::MidButton)
    {
        emit tabCloseRequested(tabAt(event->pos()));
    }
}

TabWidget::TabWidget(QWidget* parent) : QTabWidget(parent),
    m_tabBarPolicy(TabBarAsNeeded),
    m_spreadTabs(false)
{
    setTabBar(new TabBar(this));

    tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tabBar(), SIGNAL(customContextMenuRequested(QPoint)), SLOT(on_tabBar_customContextMenuRequested(QPoint)));
}

TabWidget::TabBarPolicy TabWidget::tabBarPolicy() const
{
    return m_tabBarPolicy;
}

void TabWidget::setTabBarPolicy(TabWidget::TabBarPolicy tabBarPolicy)
{
    m_tabBarPolicy = tabBarPolicy;

    switch(m_tabBarPolicy)
    {
    case TabBarAsNeeded:
        tabBar()->setVisible(count() > 1);
        break;
    case TabBarAlwaysOn:
        tabBar()->setVisible(true);
        break;
    case TabBarAlwaysOff:
        tabBar()->setVisible(false);
        break;
    }
}

bool TabWidget::spreadTabs() const
{
    return m_spreadTabs;
}

void TabWidget::setSpreadTabs(bool spreadTabs)
{
    if(m_spreadTabs != spreadTabs)
    {
        m_spreadTabs = spreadTabs;

        QResizeEvent resizeEvent(tabBar()->size(), tabBar()->size());
        QApplication::sendEvent(tabBar(), &resizeEvent);
    }
}

void TabWidget::on_tabBar_customContextMenuRequested(const QPoint& pos)
{
    emit tabContextMenuRequested(tabBar()->mapToGlobal(pos), tabBar()->tabAt(pos));
}

void TabWidget::tabInserted(int index)
{
    QTabWidget::tabInserted(index);

    if(m_tabBarPolicy == TabBarAsNeeded)
    {
        tabBar()->setVisible(count() > 1);
    }
}

void TabWidget::tabRemoved(int index)
{
    QTabWidget::tabRemoved(index);

    if(m_tabBarPolicy == TabBarAsNeeded)
    {
        tabBar()->setVisible(count() > 1);
    }
}

TreeView::TreeView(int expansionRole, QWidget* parent) : QTreeView(parent),
    m_expansionRole(expansionRole)
{
    connect(this, SIGNAL(expanded(QModelIndex)), SLOT(on_expanded(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)), SLOT(on_collapsed(QModelIndex)));
}

void TreeView::expandAbove(const QModelIndex& child)
{
    for(QModelIndex index = child.parent(); index.isValid(); index = index.parent())
    {
        expand(index);
    }
}

void TreeView::expandAll(const QModelIndex& index)
{
    if(index.isValid())
    {
        if(!isExpanded(index))
        {
            expand(index);
        }

        for(int row = 0; row < model()->rowCount(index); ++row)
        {
            expandAll(index.child(row, 0));
        }
    }
    else
    {
        QTreeView::expandAll();
    }
}

void TreeView::collapseAll(const QModelIndex& index)
{
    if(index.isValid())
    {
        if(isExpanded(index))
        {
            collapse(index);
        }

        for(int row = 0; row < model()->rowCount(index); ++row)
        {
            collapseAll(index.child(row, 0));
        }
    }
    else
    {
        QTreeView::collapseAll();
    }
}

void TreeView::restoreExpansion(const QModelIndex& index)
{
    if(index.isValid())
    {
        const bool expanded = model()->data(index, m_expansionRole).toBool();

        if(isExpanded(index) != expanded)
        {
            setExpanded(index, expanded);
        }
    }

    for(int row = 0; row < model()->rowCount(index); ++row)
    {
        restoreExpansion(model()->index(row, 0, index));
    }
}

void TreeView::contextMenuEvent(QContextMenuEvent* event)
{
    QTreeView::contextMenuEvent(event);

    if(!event->isAccepted())
    {
        QMenu menu;

        const QAction* expandAllAction = menu.addAction(tr("&Expand all"));
        const QAction* collapseAllAction = menu.addAction(tr("&Collapse all"));

        const QAction* action = menu.exec(event->globalPos());

        if(action == expandAllAction)
        {
            expandAll(indexAt(event->pos()));
        }
        else if(action == collapseAllAction)
        {
            collapseAll(indexAt(event->pos()));
        }
    }
}

void TreeView::on_expanded(const QModelIndex& index)
{
    model()->setData(index, true, m_expansionRole);
}

void TreeView::on_collapsed(const QModelIndex& index)
{
    model()->setData(index, false, m_expansionRole);
}

LineEdit::LineEdit(QWidget* parent) : QLineEdit(parent)
{
}

void LineEdit::mousePressEvent(QMouseEvent* event)
{
    QLineEdit::mousePressEvent(event);

    selectAll();
}

ComboBox::ComboBox(QWidget* parent) : QComboBox(parent)
{
    setLineEdit(new LineEdit(this));
}

SpinBox::SpinBox(QWidget* parent) : QSpinBox(parent)
{
    setLineEdit(new LineEdit(this));
}

void SpinBox::keyPressEvent(QKeyEvent* event)
{
    QSpinBox::keyPressEvent(event);

    if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        emit returnPressed();
    }
}

MappingSpinBox::MappingSpinBox(QObject* mapper, const char* textFromValue, const char* valueFromText, QWidget* parent) : SpinBox(parent),
    m_mapper(mapper)
{
    const QMetaObject* metaObject = mapper->metaObject();

    m_textFromValue = slotToMethod(metaObject, textFromValue);
    m_valueFromText = slotToMethod(metaObject, valueFromText);
}

QString MappingSpinBox::textFromValue(int val) const
{
    QString text;
    bool ok = false;

    const bool mapped = m_textFromValue.invoke(m_mapper, Qt::DirectConnection,
                                               Q_RETURN_ARG(QString, text), Q_ARG(int, val), Q_ARG(bool*, &ok));

    if(!mapped || !ok)
    {
        text = SpinBox::textFromValue(val);
    }

    return text;
}

int MappingSpinBox::valueFromText(const QString& text) const
{
    int val;
    bool ok = false;

    const bool mapped = m_valueFromText.invoke(m_mapper, Qt::DirectConnection,
                                               Q_RETURN_ARG(int, val), Q_ARG(QString, text), Q_ARG(bool*, &ok));

    if(!mapped || !ok)
    {
        val = SpinBox::valueFromText(text);
    }

    return val;
}

QValidator::State MappingSpinBox::validate(QString& input, int& pos) const
{
    Q_UNUSED(input);
    Q_UNUSED(pos);

    return QValidator::Acceptable;
}

ProgressLineEdit::ProgressLineEdit(QWidget* parent) : QLineEdit(parent),
    m_progress(0)
{    
}

int ProgressLineEdit::progress() const
{
    return m_progress;
}

void ProgressLineEdit::setProgress(int progress)
{
    if(m_progress != progress && progress >= 0 && progress <= 100)
    {
        m_progress = progress;

        update();
    }
}

void ProgressLineEdit::paintEvent(QPaintEvent* event)
{
    QLineEdit::paintEvent(event);

    QPainter painter(this);

    painter.setCompositionMode(QPainter::CompositionMode_Multiply);
    painter.fillRect(0, 0, m_progress * width() / 100, height(), palette().highlight());
}

void ProgressLineEdit::keyPressEvent(QKeyEvent* event)
{
    QLineEdit::keyPressEvent(event);

    if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        emit returnPressed(event->modifiers());
    }
}

SearchLineEdit::SearchLineEdit(QWidget* parent) : ProgressLineEdit(parent)
{
    m_timer = new QTimer(this);

    m_timer->setInterval(2000);
    m_timer->setSingleShot(true);

    connect(this, SIGNAL(textEdited(QString)), m_timer, SLOT(start()));
    connect(this, SIGNAL(returnPressed(Qt::KeyboardModifiers)), SLOT(on_returnPressed(Qt::KeyboardModifiers)));
    connect(m_timer, SIGNAL(timeout()), SLOT(on_timeout()));
}

void SearchLineEdit::startSearch()
{
    QTimer::singleShot(0, this, SLOT(on_timeout()));
}

void SearchLineEdit::stopTimer()
{
    m_timer->stop();
}

void SearchLineEdit::on_timeout()
{
    emit searchInitiated(text());
}

void SearchLineEdit::on_returnPressed(const Qt::KeyboardModifiers& modifiers)
{
    stopTimer();

    emit searchInitiated(text(), modifiers == Qt::ShiftModifier);
}

// taken from http://rosettacode.org/wiki/Roman_numerals/Decode#C.2B.2B
int Tools::romanToInt(const QString &text)
{
    if (text.size() == 1)
    {
        switch (text.at(0).toLower().toLatin1())
        {
        case 'i': return 1;
        case 'v': return 5;
        case 'x': return 10;
        case 'l': return 50;
        case 'c': return 100;
        case 'd': return 500;
        case 'm': return 1000;
        }
        return 0;
    }

    int result = 0;
    int pvs = 0;
    for (int i = text.size() - 1; i >= 0; --i)
    {
        const int inc = romanToInt(text.at(i));
        result += inc < pvs ? -inc : inc;
        pvs = inc;
    }

    return result;
}

// taken from http://rosettacode.org/wiki/Roman_numerals/Encode#C.2B.2B
QString Tools::intToRoman(int num)
{
    if (num >= 4000)
    {
        return QLatin1Char('?');
    }

    struct romandata_t { int value; char const* numeral; };
    static romandata_t const romandata[] =
    {
        1000, "m",
        900, "cm",
        500, "d",
        400, "cd",
        100, "c",
        90, "xc",
        50, "l",
        40, "xl",
        10, "x",
        9, "ix",
        5, "v",
        4, "iv",
        1, "i",
        0, NULL
    }; // end marker

    QString result;
    for (romandata_t const* current = romandata; current->value > 0; ++current)
    {
      while (num >= current->value)
      {
        result += QLatin1String(current->numeral);
        num  -= current->value;
      }
    }
    return result;
}

} // qpdfview
