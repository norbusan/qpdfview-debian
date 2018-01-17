/*

Copyright 2014 S. Razi Alavizadeh
Copyright 2012-2018 Adam Reichold
Copyright 2018 Pavel Sanda
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
#include <QMenu>
#include <QMouseEvent>
#include <QProcess>
#include <QScrollBar>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>

#include "searchmodel.h"

namespace qpdfview
{

namespace
{

inline bool isPrintable(const QString& string)
{
    foreach(const QChar& character, string)
    {
        if(!character.isPrint())
        {
            return false;
        }
    }

    return true;
}

inline QModelIndex firstIndex(const QModelIndexList& indexes)
{
    return !indexes.isEmpty() ? indexes.first() : QModelIndex();
}

} // anonymous

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

bool ProxyStyle::scrollableMenus() const
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

ToolTipMenu::ToolTipMenu(QWidget* parent) : QMenu(parent)
{
}

ToolTipMenu::ToolTipMenu(const QString& title, QWidget* parent) : QMenu(title, parent)
{
}

bool ToolTipMenu::event(QEvent* event)
{
    const QAction* const action = activeAction();

    if(event->type() == QEvent::ToolTip && action != 0 && !action->data().isNull())
    {
        QToolTip::showText(static_cast< QHelpEvent* >(event)->globalPos(), action->toolTip());
    }
    else
    {
        QToolTip::hideText();
    }

    return QMenu::event(event);
}

SearchableMenu::SearchableMenu(const QString& title, QWidget* parent) : ToolTipMenu(title, parent),
    m_searchable(false),
    m_text()
{
}

bool SearchableMenu::isSearchable() const
{
    return m_searchable;
}

void SearchableMenu::setSearchable(bool searchable)
{
    m_searchable = searchable;
}

void SearchableMenu::hideEvent(QHideEvent* event)
{
    QMenu::hideEvent(event);

    if(m_searchable && !event->spontaneous())
    {
        m_text = QString();

        foreach(QAction* action, actions())
        {
            action->setVisible(true);
        }
    }
}

void SearchableMenu::keyPressEvent(QKeyEvent* event)
{
    if(!m_searchable)
    {
        QMenu::keyPressEvent(event);
        return;
    }

    const QString text = event->text();

    if(!text.isEmpty() && isPrintable(text))
    {
        m_text.append(text);
    }
    else if(event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)
    {
        m_text.chop(1);
    }
    else
    {
        QMenu::keyPressEvent(event);
        return;
    }

    QAction* firstVisibleAction = 0;

    foreach(QAction* action, actions())
    {
        if(action->data().isNull()) // Modify only flagged actions
        {
            continue;
        }

        const bool visible = action->text().contains(m_text, Qt::CaseInsensitive);

        action->setVisible(visible);

        if(visible && firstVisibleAction == 0)
        {
            firstVisibleAction = action;
        }
    }

    setActiveAction(firstVisibleAction);

    QToolTip::showText(mapToGlobal(rect().topLeft()), tr("Search for '%1'...").arg(m_text), this);
}

TabBar::TabBar(QWidget* parent) : QTabBar(parent),
    m_dragIndex(-1)
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
    if(event->button() == Qt::MidButton)
    {
        const int index = tabAt(event->pos());

        if(index != -1)
        {
            emit tabCloseRequested(index);

            event->accept();
            return;
        }
    }
    else if(event->modifiers() == Qt::ShiftModifier && event->button() == Qt::LeftButton)
    {
        const int index = tabAt(event->pos());

        if(index != -1)
        {
            m_dragIndex = index;
            m_dragPos = event->pos();

            event->accept();
            return;
        }
    }

    QTabBar::mousePressEvent(event);
}

void TabBar::mouseMoveEvent(QMouseEvent* event)
{
    QTabBar::mouseMoveEvent(event);

    if(m_dragIndex != -1)
    {
        if((event->pos() - m_dragPos).manhattanLength() >= QApplication::startDragDistance())
        {
            emit tabDragRequested(m_dragIndex);

            m_dragIndex = -1;
        }
    }
}

void TabBar::mouseReleaseEvent(QMouseEvent* event)
{
    QTabBar::mouseReleaseEvent(event);

    m_dragIndex = -1;
}

TabWidget::TabWidget(QWidget* parent) : QTabWidget(parent),
    m_tabBarPolicy(TabBarAsNeeded),
    m_spreadTabs(false)
{
    TabBar* tabBar = new TabBar(this);

    tabBar->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tabBar, SIGNAL(tabDragRequested(int)), SIGNAL(tabDragRequested(int)));
    connect(tabBar, SIGNAL(customContextMenuRequested(QPoint)), SLOT(on_tabBar_customContextMenuRequested(QPoint)));

    setTabBar(tabBar);
}

int TabWidget::addTab(QWidget* const widget, const bool nextToCurrent,
                      const QString& label, const QString& toolTip)
{
    const int index = nextToCurrent
            ? insertTab(currentIndex() + 1, widget, label)
            : QTabWidget::addTab(widget, label);

    setTabToolTip(index, toolTip);
    setCurrentIndex(index);

    return index;
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

void TabWidget::previousTab()
{
    int index = currentIndex() - 1;

    if(index < 0)
    {
        index = count() - 1;
    }

    setCurrentIndex(index);
}

void TabWidget::nextTab()
{
    int index = currentIndex() + 1;

    if(index >= count())
    {
        index = 0;
    }

    setCurrentIndex(index);
}

void TabWidget::on_tabBar_customContextMenuRequested(QPoint pos)
{
    const int index = tabBar()->tabAt(pos);

    if(index != -1)
    {
        emit tabContextMenuRequested(tabBar()->mapToGlobal(pos), tabBar()->tabAt(pos));
    }
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

int TreeView::expandedDepth(const QModelIndex& index)
{
    if(index.isValid())
    {
        if(!isExpanded(index) || !model()->hasChildren(index))
        {
            return 0;
        }

        int depth = 0;

        for(int row = 0, rowCount = model()->rowCount(index); row < rowCount; ++row)
        {
            depth = qMax(depth, expandedDepth(index.child(row, 0)));
        }

        return 1 + depth;
    }
    else
    {
        int depth = 0;

        for(int row = 0, rowCount = model()->rowCount(); row < rowCount; ++row)
        {
            depth = qMax(depth, expandedDepth(model()->index(row, 0)));
        }

        return depth;
    }
}

void TreeView::expandToDepth(const QModelIndex& index, int depth)
{
    if(index.isValid())
    {
        if(depth > 0)
        {
            if(!isExpanded(index))
            {
                expand(index);
            }
        }

        if(depth > 1)
        {
            for(int row = 0, rowCount = model()->rowCount(index); row < rowCount; ++row)
            {
                expandToDepth(index.child(row, 0), depth - 1);
            }
        }
    }
    else
    {
        for(int row = 0, rowCount = model()->rowCount(); row < rowCount; ++row)
        {
            expandToDepth(model()->index(row, 0), depth);
        }
    }
}

void TreeView::collapseFromDepth(const QModelIndex& index, int depth)
{
    if(index.isValid())
    {
        if(depth <= 0)
        {
            if(isExpanded(index))
            {
                collapse(index);
            }
        }

        for(int row = 0, rowCount = model()->rowCount(index); row < rowCount; ++row)
        {
            collapseFromDepth(index.child(row, 0), depth - 1);
        }
    }
    else
    {
        for(int row = 0, rowCount = model()->rowCount(); row < rowCount; ++row)
        {
            collapseFromDepth(model()->index(row, 0), depth);
        }
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

void TreeView::keyPressEvent(QKeyEvent* event)
{
    const bool verticalKeys = event->key() == Qt::Key_Up || event->key() == Qt::Key_Down;
    const bool horizontalKeys = event->key() == Qt::Key_Left || event->key() == Qt::Key_Right;

    const QModelIndex selection = firstIndex(selectedIndexes());

    // If Shift is pressed, the view is scrolled up or down.
    if(event->modifiers().testFlag(Qt::ShiftModifier) && verticalKeys)
    {
        QScrollBar* scrollBar = verticalScrollBar();

        if(event->key() == Qt::Key_Up && scrollBar->value() > scrollBar->minimum())
        {
            scrollBar->triggerAction(QAbstractSlider::SliderSingleStepSub);

            event->accept();
            return;
        }
        else if(event->key() == Qt::Key_Down && scrollBar->value() < scrollBar->maximum())
        {
            scrollBar->triggerAction(QAbstractSlider::SliderSingleStepAdd);

            event->accept();
            return;
        }
    }

    // If Control is pressed, all children of the selected item are expanded or collapsed.
    if(event->modifiers().testFlag(Qt::ControlModifier) && horizontalKeys)
    {
        if(event->key() == Qt::Key_Left)
        {
            collapseAll(selection);
        }
        else if(event->key() == Qt::Key_Right)
        {
            expandAll(selection);
        }

        event->accept();
        return;
    }

    // If Shift is pressed, one level of children of the selected item are expanded or collapsed.
    if(event->modifiers().testFlag(Qt::ShiftModifier) && horizontalKeys)
    {
        const int depth = expandedDepth(selection);

        if(event->key() == Qt::Key_Left)
        {
            collapseFromDepth(selection, depth - 1);
        }
        else if(event->key() == Qt::Key_Right)
        {
            expandToDepth(selection, depth + 1);
        }

        event->accept();
        return;
    }

    QTreeView::keyPressEvent(event);
}

void TreeView::wheelEvent(QWheelEvent* event)
{
    const QModelIndex selection = firstIndex(selectedIndexes());

    // If Control is pressed, expand or collapse the selected entry.
    if(event->modifiers().testFlag(Qt::ControlModifier) && selection.isValid())
    {
        if(event->delta() > 0)
        {
            collapse(selection);
        }
        else
        {
            expand(selection);
        }

        // Fall through when Shift is also pressed.
        if(!event->modifiers().testFlag(Qt::ShiftModifier))
        {
            event->accept();
            return;
        }
    }

    // If Shift is pressed, move the selected entry up and down.
    if(event->modifiers().testFlag(Qt::ShiftModifier) && selection.isValid())
    {
        QModelIndex sibling;

        if(event->delta() > 0)
        {
            sibling = indexAbove(selection);
        }
        else
        {
            sibling = indexBelow(selection);
        }

        if(sibling.isValid())
        {
            setCurrentIndex(sibling);
        }

        event->accept();
        return;
    }

    QTreeView::wheelEvent(event);
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

MappingSpinBox::MappingSpinBox(TextValueMapper* mapper, QWidget* parent) : SpinBox(parent),
    m_mapper(mapper)
{
}

QString MappingSpinBox::textFromValue(int val) const
{
    bool ok = false;
    QString text = m_mapper->textFromValue(val, ok);

    if(!ok)
    {
        text = SpinBox::textFromValue(val);
    }

    return text;
}

int MappingSpinBox::valueFromText(const QString& text) const
{
    bool ok = false;
    int value = m_mapper->valueFromText(text, ok);

    if(!ok)
    {
        value = SpinBox::valueFromText(text);
    }

    return value;
}

QValidator::State MappingSpinBox::validate(QString& input, int& pos) const
{
    Q_UNUSED(input);
    Q_UNUSED(pos);

    return QValidator::Acceptable;
}

int getMappedNumber(MappingSpinBox::TextValueMapper* mapper,
                    QWidget* parent, const QString& title, const QString& caption,
                    int value, int min, int max, bool* ok, Qt::WindowFlags flags)
{
    QDialog* dialog = new QDialog(parent, flags | Qt::MSWindowsFixedSizeDialogHint);
    dialog->setWindowTitle(title);

    QLabel* label = new QLabel(dialog);
    label->setText(caption);

    MappingSpinBox* mappingSpinBox = new MappingSpinBox(mapper, dialog);
    mappingSpinBox->setRange(min, max);
    mappingSpinBox->setValue(value);
    mappingSpinBox->selectAll();

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

void SearchLineEdit::startTimer()
{
    m_timer->start();
}

void SearchLineEdit::stopTimer()
{
    m_timer->stop();
}

void SearchLineEdit::on_timeout()
{
    emit searchInitiated(text());
}

void SearchLineEdit::on_returnPressed(Qt::KeyboardModifiers modifiers)
{
    stopTimer();

    emit searchInitiated(text(), modifiers == Qt::ShiftModifier);
}

Splitter::Splitter(Qt::Orientation orientation, QWidget* parent) : QSplitter(orientation, parent),
    m_currentIndex(0)
{
    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(on_focusChanged(QWidget*,QWidget*)));
}

QWidget* Splitter::currentWidget() const
{
    return widget(m_currentIndex);
}

void Splitter::setCurrentWidget(QWidget* const currentWidget)
{
    for(int index = 0, count = this->count(); index < count; ++index)
    {
        QWidget* const widget = this->widget(index);

        if(currentWidget == widget)
        {
            if(m_currentIndex != index)
            {
                m_currentIndex = index;

                emit currentWidgetChanged(currentWidget);
            }

            return;
        }
    }
}

void Splitter::setUniformSizes()
{
    int size;

    switch(orientation())
    {
    default:
    case Qt::Horizontal:
        size = width();
        break;
    case Qt::Vertical:
        size = height();
        break;
    }

    QList< int > sizes;

    for(int index = 0, count = this->count(); index < count; ++index)
    {
        sizes.append(size / count);
    }

    setSizes(sizes);
}

void Splitter::on_focusChanged(QWidget* /* old */, QWidget* now)
{
    for(QWidget* currentWidget = now; currentWidget != 0; currentWidget = currentWidget->parentWidget())
    {
        if(currentWidget->parentWidget() == this)
        {
            setCurrentWidget(currentWidget);

            return;
        }
    }
}

void openInNewWindow(const QString& filePath, int page)
{
    QProcess::startDetached(
        QApplication::applicationFilePath(),
        QStringList() << QString("%2#%1").arg(page).arg(filePath)
    );
}

} // qpdfview
