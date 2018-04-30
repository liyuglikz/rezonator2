#include "PumpWindow.h"

#include "widgets/Appearance.h"
#include "widgets/RichTextItemDelegate.h"
#include "widgets/ValuesEditorTS.h"
#include "widgets/OriOptionsGroup.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWidgets.h"

#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QToolButton>

//------------------------------------------------------------------------------
//                             PumpWindowStorable
//------------------------------------------------------------------------------

namespace PumpWindowStorable
{

SchemaWindow* createWindow(Schema* schema)
{
    return PumpWindow::create(schema);
}

} // namespace PumpWindowStorable

//------------------------------------------------------------------------------
//                                PumpsTable
//------------------------------------------------------------------------------

PumpsTable::PumpsTable(Schema* schema, QWidget *parent) : QTableWidget(0, COL_COUNT, parent), _schema(schema)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setItemDelegateForColumn(COL_PARAMS, new RichTextItemDelegate(this));
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(COL_IMAGE, QHeaderView::Fixed);
    horizontalHeader()->setMinimumSectionSize(_iconSize+6);
    horizontalHeader()->resizeSection(COL_IMAGE, _iconSize+6);
    horizontalHeader()->setSectionResizeMode(COL_LABEL, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(COL_PARAMS, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(COL_TITLE, QHeaderView::Stretch);
    horizontalHeader()->setHighlightSections(false);
    setHorizontalHeaderLabels({ tr("Typ"), tr("Label"), tr("Params"), tr("Title") });

    connect(this, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(doubleClicked(QTableWidgetItem*)));
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));

    populate();
}

void PumpsTable::adjustColumns()
{
    resizeColumnToContents(COL_LABEL);
    resizeColumnToContents(COL_PARAMS);
}

void PumpsTable::doubleClicked(QTableWidgetItem*)
{
    Z::PumpParams* pump = selected();
    if (pump) emit doubleClicked(pump);
}

void PumpsTable::showContextMenu(const QPoint& pos)
{
    if (_contextMenu)
        _contextMenu->popup(mapToGlobal(pos));
}

Z::PumpParams* PumpsTable::selected() const
{
    return schema()->pumps()->at(currentRow());
}

void PumpsTable::setSelected(Z::PumpParams *param)
{
    setCurrentCell(findRow(param), 0);
}

void PumpsTable::populate()
{
    clearContents();
    setRowCount(schema()->pumps()->size());
    for (int row = 0; row < schema()->pumps()->size(); row++)
    {
        auto pump = schema()->pumps()->at(row);
        createRow(row);
        populateRow(pump, row);
    }
    adjustColumns();
}

void PumpsTable::createRow(int row)
{
    QTableWidgetItem *it = new QTableWidgetItem();
    it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    setItem(row, COL_IMAGE, it);

    it = new QTableWidgetItem();
    Z::Gui::setSymbolFont(it);
    it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    it->setTextAlignment(Qt::AlignHCenter);
    setItem(row, COL_LABEL, it);

    it = new QTableWidgetItem();
    Z::Gui::setValueFont(it);
    it->setTextAlignment(Qt::AlignCenter);
    it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    setItem(row, COL_PARAMS, it);

    it = new QTableWidgetItem();
    it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    setItem(row, COL_TITLE, it);
}

void PumpsTable::populateRow(Z::PumpParams *pump, int row)
{
    auto pumpMode = Z::Pump::findByModeName(pump->modeName());
    if (pumpMode)
    {
        auto it = item(row, COL_IMAGE);
        it->setData(Qt::DecorationRole, QIcon(pumpMode->iconPath()).pixmap(_iconSize, _iconSize));
        it->setToolTip(pumpMode->displayName());
    }
    else
        qCritical() << "Unable to find mode for pump parameters";

    item(row, COL_LABEL)->setText(pump->label());
    item(row, COL_PARAMS)->setText(pump->displayStr());
    item(row, COL_TITLE)->setText("  " % pump->title());
}

void PumpsTable::schemaLoaded(Schema*)
{
    populate();
}

int PumpsTable::findRow(Z::PumpParams *pump)
{
    return schema()->pumps()->indexOf(pump);
}

//------------------------------------------------------------------------------
//                                PumpWindow
//------------------------------------------------------------------------------

PumpWindow* PumpWindow::_instance = nullptr;

PumpWindow* PumpWindow::create(Schema* owner)
{
    if (!_instance)
        _instance = new PumpWindow(owner);
    return _instance;
}

PumpWindow::PumpWindow(Schema *owner) : SchemaMdiChild(owner)
{
    setTitleAndIcon(tr("Pumps"), ":/window_icons/pump");

    _table = new PumpsTable(owner);
    setContent(_table);

    createActions();
    createMenuBar();
    createToolBar();

    _table->setContextMenu(_contextMenu);
    connect(_table, SIGNAL(doubleClicked(Z::Parameter*)), this, SLOT(editPump()));
    schema()->registerListener(_table);
}

PumpWindow::~PumpWindow()
{
    _instance = nullptr;

    schema()->unregisterListener(_table);
}

void PumpWindow::createActions()
{
    #define A_ Ori::Gui::action

    _actnPumpAdd = A_(tr("&Create..."), this, SLOT(createPump()), ":/toolbar/pump_add", Qt::CTRL | Qt::Key_Insert);
    _actnPumpDelete = A_(tr("&Delete"), this, SLOT(deletePump()), ":/toolbar/pump_delete", Qt::CTRL | Qt::Key_Delete);
    _actnPumpEdit = A_(tr("&Edit..."), this, SLOT(editPump()), ":/toolbar/pump_edit", Qt::Key_Enter);
    _actnPumpActivate = A_(tr("&Activate..."), this, SLOT(activatePump()), ":/toolbar/pump_on");

    #undef A_
}

void PumpWindow::createMenuBar()
{
    _windowMenu = Ori::Gui::menu(tr("&Pump"), this,
        { _actnPumpAdd, 0, _actnPumpEdit, _actnPumpActivate, 0, _actnPumpDelete });

    _contextMenu = Ori::Gui::menu(this,
        { _actnPumpEdit, _actnPumpActivate, 0, _actnPumpDelete });
}

void PumpWindow::createToolBar()
{
    populateToolbar({
        Ori::Gui::textToolButton(_actnPumpAdd),
        0,
        Ori::Gui::textToolButton(_actnPumpEdit),
        Ori::Gui::textToolButton(_actnPumpActivate),
        0,
        _actnPumpDelete
    });
}

void PumpWindow::createPump()
{

}

void PumpWindow::editPump()
{

}

void PumpWindow::deletePump()
{

}

void PumpWindow::activatePump()
{

}
