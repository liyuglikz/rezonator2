#include "SchemaElemsTable.h"

#include "Appearance.h"
#include "ElementImagesProvider.h"
#include "RichTextItemDelegate.h"
#include "../funcs/FormatInfo.h"

#include <QDebug>
#include <QHeaderView>
#include <QMenu>
#include <QVariant>

SchemaElemsTable::SchemaElemsTable(Schema *schema, QWidget *parent) : QTableWidget(0, COL_COUNT, parent)
{
    _schema = schema;

    auto iconSize = ElementImagesProvider::instance().iconSize();

    int paramsOffsetY = 0;
#if defined(Q_OS_MAC)
    paramsOffsetY = 2;
#elif defined(Q_OS_LINUX)
    paramsOffsetY = 1;
#elif defined(Q_OS_WIN)
    paramsOffsetY = 2;
#endif

    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setItemDelegateForColumn(COL_PARAMS, new RichTextItemDelegate(paramsOffsetY, this));
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(COL_IMAGE, QHeaderView::Fixed);
    horizontalHeader()->setMinimumSectionSize(iconSize.width()+6);
    horizontalHeader()->resizeSection(COL_IMAGE, iconSize.width()+6);
    horizontalHeader()->setSectionResizeMode(COL_LABEL, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(COL_PARAMS, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(COL_TITLE, QHeaderView::Stretch);
    horizontalHeader()->setHighlightSections(false);
    setHorizontalHeaderLabels({ tr("Typ"), tr("Label"), tr("Parameters"), tr("Title") });

    connect(this, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(doubleClicked(QTableWidgetItem*)));
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
}

void SchemaElemsTable::adjustColumns()
{
    resizeColumnToContents(COL_LABEL);
    resizeColumnToContents(COL_PARAMS);
}

void SchemaElemsTable::doubleClicked(QTableWidgetItem*)
{
    Element* elem = selected();
    if (elem)
        emit doubleClicked(elem);
}

void SchemaElemsTable::showContextMenu(const QPoint& pos)
{
    if (_contextMenu)
        _contextMenu->popup(mapToGlobal(pos));
}

Element* SchemaElemsTable::selected() const
{
    int index = currentRow();
    return (index >= 0 and index < schema()->count())? schema()->element(index): nullptr;
}

void SchemaElemsTable::setSelected(Element *elem)
{
    setCurrentCell(_schema->indexOf(elem), 0);
}

Elements SchemaElemsTable::selection() const
{
    Elements elements;
    QVector<int> rows = selectedRows();
    for (int i = 0; i < rows.count(); i++)
        elements.append(schema()->element(rows[i]));
    return elements;
}

QVector<int> SchemaElemsTable::selectedRows() const
{
    QVector<int> result;
    QList<QTableWidgetSelectionRange> selection = selectedRanges();
    for (int i = 0; i < selection.count(); i++)
        for (int j = selection.at(i).topRow(); j <= selection.at(i).bottomRow(); j++)
            result.append(j);
    return result;
}

bool SchemaElemsTable::hasSelection() const
{
    return currentRow() > -1;
}

void SchemaElemsTable::populate()
{
    clearContents();
    setRowCount(schema()->count());
    for (int row = 0; row < schema()->count(); row++)
    {
        Element *elem = schema()->element(row);
        createRow(elem, row);
        populateRow(elem, row);
    }
    adjustColumns();
}

void SchemaElemsTable::createRow(Element *elem, int row)
{
    QTableWidgetItem *it = new QTableWidgetItem();
    it->setData(Qt::DecorationRole, QPixmap(ElementImagesProvider::instance().iconPath(elem->type())));
    it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    it->setToolTip(elem->typeName());
    setItem(row, COL_IMAGE, it);

    it = new QTableWidgetItem();
    Z::Gui::setSymbolFont(it);
    it->setTextAlignment(Qt::AlignHCenter | Qt::AlignCenter);
    it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    setItem(row, COL_LABEL, it);

    it = new QTableWidgetItem();
    it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    setItem(row, COL_PARAMS, it);

    it = new QTableWidgetItem();
    it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    setItem(row, COL_TITLE, it);
}

void SchemaElemsTable::populateRow(Element *elem, int row)
{
    item(row, COL_LABEL)->setText(" " % elem->label() % " ");
    item(row, COL_PARAMS)->setText(Z::Format::elemParamsHtml(schema(), elem));
    item(row, COL_TITLE)->setText("  " % elem->title());
    const QBrush& color = elem->disabled()? palette().shadow() : palette().text();
    item(row, COL_LABEL)->setForeground(color);
    item(row, COL_PARAMS)->setForeground(color);
    item(row, COL_TITLE)->setForeground(color);
}

void SchemaElemsTable::schemaLoaded(Schema*)
{
    populate();
}

void SchemaElemsTable::elementCreated(Schema*, Element* elem)
{
    populate();
    setSelected(elem);
}

void SchemaElemsTable::elementChanged(Schema *schema, Element *elem)
{
    int index = schema->indexOf(elem);
    if (index >= 0 && index < rowCount())
    {
        populateRow(elem, index);
        adjustColumns();
    }
}

void SchemaElemsTable::elementDeleting(Schema *schema, Element *elem)
{
    int index = schema->indexOf(elem);
    if (index >= 0 && index < rowCount())
    {
        removeRow(index);
        adjustColumns();
    }
}
