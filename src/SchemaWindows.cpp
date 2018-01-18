#include "SchemaWindows.h"
#include "WindowsManager.h"
#include "widgets/OriFlatToolBar.h"
#include "helpers/OriWidgets.h"

#include <QApplication>
#include <QBoxLayout>
#include <QDebug>
#include <QEvent>
#include <QMenu>
#include <QTimer>
#include <QToolButton>

//------------------------------------------------------------------------------
//                               SchemaWindow
//------------------------------------------------------------------------------

SchemaWindow::SchemaWindow(Schema *owner) : _schema(owner)
{
    WindowsManager::instance().registerWindow(_schema, this);
    if (_schema) _schema->registerListener(this);
}

SchemaWindow::~SchemaWindow()
{
    WindowsManager::instance().unregisterWindow(_schema, this);
    if (_schema) _schema->unregisterListener(this);
}

//------------------------------------------------------------------------------

QSize toolbarIconSize()
{
    return Settings::instance().smallToolbarImages? QSize(16,16): QSize(24,24);
}

void adjustIconSize(QToolBar* toolbar, const QSize& iconSize)
{
    toolbar->setIconSize(iconSize);

    for (auto a: toolbar->actions())
    {
        auto w = toolbar->widgetForAction(a);
        auto b = qobject_cast<QToolButton*>(w);
        if (b) b->setIconSize(iconSize);
    }

    toolbar->adjustSize();
}

//------------------------------------------------------------------------------
//                               SchemaToolWindow
//------------------------------------------------------------------------------

SchemaToolWindow::SchemaToolWindow(Schema *owner) : SchemaWindow(owner)
{
}

QToolBar* SchemaToolWindow::makeToolBar(const QString& title, bool flat)
{
    QToolBar* toolbar = flat? new Ori::Widgets::FlatToolBar(title): new QToolBar(title);
    toolbar->setIconSize(toolbarIconSize());
    toolbar->setObjectName("toolBar_" % title);
    _toolbars << toolbar;
    return toolbar;
}

QToolBar* SchemaToolWindow::makeToolBar(const QString& title, const std::initializer_list<QObject*>& items, bool flat)
{
    return Ori::Gui::populate(makeToolBar(title, flat), items);
}

QToolBar* SchemaToolWindow::makeToolBar(bool flat)
{
    return makeToolBar(QString(), flat);
}

QToolBar* SchemaToolWindow::makeToolBar(const std::initializer_list<QObject*>& items, bool flat)
{
    return makeToolBar(QString(), items, flat);
}

void SchemaToolWindow::settingsChanged()
{
    auto iconSize = toolbarIconSize();
    for (auto toolbar: _toolbars)
        adjustIconSize(toolbar, iconSize);
}

//------------------------------------------------------------------------------
//                               BasicMdiChild
//------------------------------------------------------------------------------

BasicMdiChild::BasicMdiChild(InitOptions options) : QMdiSubWindow()
{
    setAttribute(Qt::WA_DeleteOnClose);

    if (!options.testFlag(initNoDefaultWidget))
    {
        setWidget(new QWidget);
        _layout = new QVBoxLayout(widget());
        _layout->setMargin(0);
        _layout->setSpacing(0);

        if (!options.testFlag(initNoToolBar))
        {
            _toolbar = new Ori::Widgets::FlatToolBar;
            _toolbar->setIconSize(toolbarIconSize());
            _layout->addWidget(_toolbar);
        }
    }


}

void BasicMdiChild::populateToolbar(std::initializer_list<QObject*> items)
{
    Ori::Gui::populate(_toolbar, items);
}

void BasicMdiChild::setTitleAndIcon(const QString& title, const char* iconPath)
{
#ifdef Q_OS_MACOS
    // On MacOS the title and icon of MDI-child are tightly glued together
    // and that looks ugly, so we simulate intendation with space
    setWindowTitle(" " + title);
#else
    setWindowTitle(title);
#endif
    setWindowIcon(QIcon(iconPath));
}

void BasicMdiChild::setContent(QWidget *content, int row)
{
    if (content)
    {
        if (_layout)
            _layout->insertWidget(row, content);
        else
            setWidget(content);
    }
}

void BasicMdiChild::settingsChanged()
{
    if (_toolbar)
        adjustIconSize(_toolbar, toolbarIconSize());
}

//------------------------------------------------------------------------------
//                               SchemaMdiChild
//------------------------------------------------------------------------------

SchemaMdiChild::SchemaMdiChild(Schema *schema, InitOptions options) : BasicMdiChild(options), SchemaWindow(schema)
{

}

//------------------------------------------------------------------------------
//                               SchemaMdiArea
//------------------------------------------------------------------------------

SchemaMdiArea::SchemaMdiArea(QWidget *parent): QMdiArea(parent)
{
    updateBackground();
}

BasicMdiChild* SchemaMdiArea::activeChild() const
{
    return dynamic_cast<BasicMdiChild*>(activeSubWindow());
}

EditableWindow* SchemaMdiArea::activeEditableChild() const
{
    return dynamic_cast<EditableWindow*>(activeSubWindow());
}


void SchemaMdiArea::appendChild(QWidget* window)
{
    appendChild(qobject_cast<BasicMdiChild*>(window));
}

void SchemaMdiArea::appendChild(BasicMdiChild* window)
{
    if (window)
    {
        if (!subWindowList().contains(window))
        {
            addSubWindow(window);
            window->init();
            window->show();
        }
        setActiveSubWindow(window);

#ifdef Q_OS_LINUX
        // WORKAROUND: For some reasons, on Ubuntu (Unity and xfce) previous subwindow
        // is activated again after this window was activated. So we have to reactivate
        // it one more time slightly later. Reactivating can't help if done immediately,
        // but there is ugly flikering if delay is too long, should be better solution.
        QTimer::singleShot(100, [window, this](){ setActiveSubWindow(window); });
#endif
    }
}


void SchemaMdiArea::activateChild()
{
    auto action = qobject_cast<QAction*>(sender());
    auto window = reinterpret_cast<BasicMdiChild*>(action->data().value<void*>());
    activateChild(window);
}

void SchemaMdiArea::activateChild(QWidget *window)
{
    activateChild(qobject_cast<BasicMdiChild*>(window));
}

void SchemaMdiArea::activateChild(BasicMdiChild* window)
{
    if (window)
    {
        if (window->windowState() & Qt::WindowMinimized)
            window->showNormal();
        else if (!window->isVisible())
            window->show();
        setActiveSubWindow(window);
    }
}


void SchemaMdiArea::editableChild_Cut()
{
    EditableWindow *w = dynamic_cast<EditableWindow*>(activeSubWindow());
    if (w && w->canCut()) w->cut();
}

void SchemaMdiArea::editableChild_Copy()
{
    EditableWindow *w = dynamic_cast<EditableWindow*>(activeSubWindow());
    if (w && w->canCopy()) w->copy();
}

void SchemaMdiArea::editableChild_Paste()
{
    EditableWindow *w = dynamic_cast<EditableWindow*>(activeSubWindow());
    if (w && w->canPaste()) w->paste();
}

void SchemaMdiArea::editableChild_SelectAll()
{
    EditableWindow *w = dynamic_cast<EditableWindow*>(activeSubWindow());
    if (w) w->selectAll();
}

void SchemaMdiArea::populateWindowMenu()
{
    QMenu* menu = qobject_cast<QMenu*>(sender());
    if (!menu) return;

    auto subWindows = subWindowList();

    // Remove action of windows that are no more exist
    for (auto action : menu->actions())
    {
        if (action->data().isNull()) continue;
        // If we interpret variant data as QMdiSubWindow*, Qt seems to call for some of their
        // methods, and app is crashed because of window is already deleted. So use plain void*.
        auto window = reinterpret_cast<QMdiSubWindow*>(action->data().value<void*>());
        if (subWindows.contains(window)) continue;
        menu->removeAction(action);
        delete action;
    }

    // Append actions for windows that are not in menu
    auto actions = menu->actions();
    auto activeWindow = activeSubWindow();
    for (auto window : subWindows)
    {
        if (!window->isVisible()) continue;

        bool windowInMenu = false;
        for (auto action: actions)
            if (action->data().value<void*>() == window)
            {
                action->setChecked(window == activeWindow);
                windowInMenu = true;
                break;
            }
        if (windowInMenu) continue;

        auto action = menu->addAction(window->windowTitle(), this, SLOT(activateChild()));
        action->setCheckable(true);
        action->setChecked(window == activeWindow);
        action->setData(QVariant::fromValue<void*>(window));
    }
}

void SchemaMdiArea::settingsChanged()
{
    updateBackground();
}

void SchemaMdiArea::updateBackground()
{
    if (Settings::instance().showBackground)
        setBackground(QBrush(QPixmap(":/misc/mdi_background")));
    else
        setBackground(QBrush(palette().color(QPalette::Background)));
}

