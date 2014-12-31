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
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <qmath.h>
#include <QMenu>
#include <QMouseEvent>
#include <QTextLayout>
#include <QTimer>
#include <QVBoxLayout>

#include "searchmodel.h"

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

void emphasizeText(const QString& text, bool matchCase, const QString& surroundingText, QTextLayout& textLayout)
{
    QFont font = textLayout.font();
    font.setWeight(QFont::Light);
    textLayout.setFont(font);


    QList< QTextLayout::FormatRange > additionalFormats;

    for(int position = 0; (position = surroundingText.indexOf(text, position, matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive)) != -1; position += text.length())
    {
        QTextLayout::FormatRange formatRange;
        formatRange.start = position;
        formatRange.length = text.length();
        formatRange.format.setFontWeight(QFont::Bold);

        additionalFormats.append(formatRange);
    }

    textLayout.setAdditionalFormats(additionalFormats);
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

ProxyStyle::ProxyStyle() : QProxyStyle(),
    m_scrollableMenus(false)
{
}

bool ProxyStyle::scrollableMenus()
{
    return m_scrollableMenus;
}

void ProxyStyle::setScrollableMenus(bool scrollableMenus)
{
    m_scrollableMenus = scrollableMenus;
}

int ProxyStyle::styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const
{
    if(m_scrollableMenus && hint == QStyle::SH_Menu_Scrollable)
    {
        return 1;
    }

    return QProxyStyle::styleHint(hint, option, widget, returnData);
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

        for(int row = 0, rowCount = model()->rowCount(index); row < rowCount; ++row)
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

        for(int row = 0, rowCount = model()->rowCount(index); row < rowCount; ++row)
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
        const bool expanded = index.data(m_expansionRole).toBool();

        if(isExpanded(index) != expanded)
        {
            setExpanded(index, expanded);
        }
    }

    for(int row = 0, rowCount = model()->rowCount(index); row < rowCount; ++row)
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

int getMappedNumber(QObject* mapper, const char* textFromValue, const char* valueFromText,
                    QWidget* parent, const QString& title, const QString& caption,
                    int value, int min, int max, bool* ok, Qt::WindowFlags flags)
{
    QDialog* dialog = new QDialog(parent, flags | Qt::MSWindowsFixedSizeDialogHint);
    dialog->setWindowTitle(title);

    QLabel* label = new QLabel(dialog);
    label->setText(caption);

    MappingSpinBox* mappingSpinBox = new MappingSpinBox(mapper, textFromValue, valueFromText, dialog);
    mappingSpinBox->setRange(min, max);
    mappingSpinBox->setValue(value);

    QDialogButtonBox* dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
    QObject::connect(dialogButtonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    QObject::connect(dialogButtonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

    dialog->setLayout(new QVBoxLayout(dialog));
    dialog->layout()->addWidget(label);
    dialog->layout()->addWidget(mappingSpinBox);
    dialog->layout()->addWidget(dialogButtonBox);

    dialog->setFocusProxy(mappingSpinBox);

    const int dialogResult = dialog->exec();
    const int number = mappingSpinBox->value();

    delete dialog;

    if(ok)
    {
        *ok = dialogResult == QDialog::Accepted;
    }

    return number;
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

    QRect highlightedRect = rect();
    highlightedRect.setWidth(m_progress * highlightedRect.width() / 100);

    painter.setCompositionMode(QPainter::CompositionMode_Multiply);
    painter.fillRect(highlightedRect, palette().highlight());
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

SearchItemDelegate::SearchItemDelegate(QObject* parent) : QStyledItemDelegate(parent)
{
}

void SearchItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    const int progress = index.data(SearchModel::ProgressRole).toInt();

    if(progress != 0)
    {
        paintProgress(painter, option, progress);
        return;
    }

    const QString text = index.data(SearchModel::TextRole).toString();
    const QString surroundingText = index.data(SearchModel::SurroundingTextRole).toString();
    const bool matchCase = index.data(SearchModel::MatchCaseRole).toBool();

    if(!text.isEmpty() && !surroundingText.isEmpty())
    {
        paintSurroundingText(painter, option, text, surroundingText, matchCase);
        return;
    }
}

void SearchItemDelegate::paintProgress(QPainter* painter, const QStyleOptionViewItem& option,
                                       int progress) const
{
    QRect highlightedRect = option.rect;
    highlightedRect.setWidth(progress * highlightedRect.width() / 100);

    painter->save();

    painter->setCompositionMode(QPainter::CompositionMode_Multiply);
    painter->fillRect(highlightedRect, option.palette.highlight());

    painter->restore();
}

void SearchItemDelegate::paintSurroundingText(QPainter* painter, const QStyleOptionViewItem& option,
                                              const QString& text, const QString& surroundingText, bool matchCase) const
{
    const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
    const QRect textRect = option.rect.adjusted(textMargin, 0, -textMargin, 0);
    const QString elidedText = option.fontMetrics.elidedText(surroundingText, option.textElideMode, textRect.width());

    QTextOption textOption;
    textOption.setWrapMode(QTextOption::NoWrap);
    textOption.setTextDirection(surroundingText.isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);
    textOption.setAlignment(QStyle::visualAlignment(textOption.textDirection(), option.displayAlignment));

    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setText(elidedText);
    textLayout.setFont(option.font);

    emphasizeText(text, matchCase, surroundingText, textLayout);


    textLayout.beginLayout();

    QTextLine textLine = textLayout.createLine();

    if(!textLine.isValid())
    {
        return;
    }

    textLine.setLineWidth(textRect.width());

    textLayout.endLayout();


    const QSize layoutSize(textRect.width(), qFloor(textLine.height()));
    const QRect layoutRect = QStyle::alignedRect(option.direction, option.displayAlignment, layoutSize, textRect);

    painter->save();

    painter->setClipping(true);
    painter->setClipRect(layoutRect);

    textLine.draw(painter, layoutRect.topLeft());

    painter->restore();
}

} // qpdfview
