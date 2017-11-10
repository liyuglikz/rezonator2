#include "ElementPropsDialog.h"
#include "core/Element.h"
#include "widgets/ElementImagesProvider.h"
#include "widgets/ParamEditor.h"
#include "widgets/OriSvgView.h"
#include "helpers/OriWidgets.h"

#include <QApplication>
#include <QCheckBox>
#include <QFormLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTabWidget>

namespace Z {
namespace Dlgs {

ElementPropsDialog* editorDlg(Element *elem, QWidget *parent)
{
    switch (elem->paramsEditorKind())
    {
    case Z::ParamsEditor::None: return new ElementPropsDialog_None(elem, parent);
    case Z::ParamsEditor::List: return new ElementPropsDialog_List(elem, parent);
    case Z::ParamsEditor::ABCD: return new ElementPropsDialog_Abcd(elem, parent);
    }
    return new ElementPropsDialog(elem);
}

bool elementProps(Element *elem, QWidget *parent)
{
    return editorDlg(elem, parent)->exec() == QDialog::Accepted;
}

} // namespace Dlgs
} // namespace Z


//------------------------------------------------------------------------------
//                             ElementPropsDialog
//------------------------------------------------------------------------------

int __savedTabIndex = 0;

ElementPropsDialog::ElementPropsDialog(Element *elem, QWidget* parent) : RezonatorDialog(parent)
{
    _element = elem;

    setWindowTitle(_element->typeName());
    setWindowIcon(QIcon(":/window_icons/element"));
    setAttribute(Qt::WA_DeleteOnClose);
    connect(this, SIGNAL(finished(int)), this, SLOT(dialogFinished(int)));

    // common props
    _editorLabel = new QLineEdit;
    _editorTitle = new QLineEdit;
    Ori::Gui::adjustFont(_editorLabel);
    Ori::Gui::adjustFont(_editorTitle);

    auto layoutCommon = new QFormLayout;
    layoutCommon->addRow(tr("Label:"), _editorLabel);
    layoutCommon->addRow(tr("Title:"), _editorTitle);

    // parameters tab-set
    _tabs = new QTabWidget;
    // TODO:NEXT-VER _tabs->addTab(initPageOther(), tr("Other"));
    _tabs->addTab(initPageOutline(), tr("Outline"));

    mainLayout()->addLayout(layoutCommon);
    mainLayout()->addSpacing(6);
    mainLayout()->addWidget(_tabs);

//TODO    paramRejected = false;
//TODO    backupParams = element->params().getValues();

    _editorLabel->setFocus();
}

ElementPropsDialog::~ElementPropsDialog()
{
    __savedTabIndex = _tabs->currentIndex();
}

void ElementPropsDialog::showEvent(QShowEvent* event)
{
    RezonatorDialog::showEvent(event);

    _tabs->setCurrentIndex(__savedTabIndex);

    populate();
}

void ElementPropsDialog::setPageParams(QWidget* pageParams)
{
    _tabs->insertTab(0, pageParams, tr("Parameters"));
}

/* TODO:NEXT-VER
QWidget* ElementPropsDialog::initPageOther()
{
    return Ori::Gui::widgetV({
        _disabled = new QCheckBox(tr("Ignore this element in calculations")), 0 });
} */

QWidget* ElementPropsDialog::initPageOutline()
{
    auto outline = new Ori::Widgets::SvgView;
    outline->load(ElementImagesProvider::instance().drawingPath(_element->type()));
    return outline;
}

void ElementPropsDialog::populate()
{
    _editorLabel->setText(_element->label());
    _editorTitle->setText(_element->title());
    // TODO:NEXT-VER _disabled->setChecked(_element->disabled());

    populateParams();
}

void ElementPropsDialog::collect()
{
    _element->lock();

    _element->setLabel(_editorLabel->text());
    _element->setTitle(_editorTitle->text());
    // TODO:NEXT-VER _element->setDisabled(_disabled->isChecked());

    collectParams();

    _element->unlock();

    //if (!paramRejected)
//    {
        accept();
        close();
//    }

    //paramRejected = false;
}

//TODO void ElementPropsDialog::valueRejected(Z::Parameter *param, double value, const char *reason)
//{
//    Q_UNUSED(value)
//    paramRejected = true;
//    tabs->setCurrentIndex(0);
//    editorParams->focus(param);
//    QMessageBox::warning(this, qApp->applicationName(), qApp->translate("Schema errors", reason));
//}

void ElementPropsDialog::dialogFinished(int result)
{
//TODO    if (result == QDialog::Rejected)
//        element->params().setValues(backupParams);
}


//------------------------------------------------------------------------------

ElementPropsDialog_None::ElementPropsDialog_None(Element *elem, QWidget *parent) : ElementPropsDialog(elem, parent)
{
    auto label = new QLabel(tr("Element has no editable parameters"));
    label->setAlignment(Qt::AlignCenter);

    setPageParams(label);
}

//------------------------------------------------------------------------------

ElementPropsDialog_List::ElementPropsDialog_List(Element *elem, QWidget *parent) : ElementPropsDialog(elem, parent)
{
    _editors = new ParamsEditor(&elem->params());
    // TODO validation
//    connect(editorParams, SIGNAL(paramRejected(Z::Parameter*,double,const char*)),
//        this, SLOT(valueRejected(Z::Parameter*,double,const char*)));

    setPageParams(_editors);

    _editors->focus();
}

void ElementPropsDialog_List::populateParams()
{
    _editors->populate();
}

void ElementPropsDialog_List::collectParams()
{
    _editors->apply();
}

//------------------------------------------------------------------------------

ElementPropsDialog_Abcd::ElementPropsDialog_Abcd(Element *elem, QWidget *parent) : ElementPropsDialog(elem, parent)
{
    setPageParams(Ori::Gui::widgetV({
        _editorMt = new ParamsEditorAbcd(QString("T"), elem->params().mid(0, 4)),
        _editorMs = new ParamsEditorAbcd(QString("S"), elem->params().mid(4, 4))
    }));

    _editorMt->focus();
}

void ElementPropsDialog_Abcd::populateParams()
{
    _editorMt->populate();
    _editorMs->populate();
}

void ElementPropsDialog_Abcd::collectParams()
{
    _editorMt->apply();
    _editorMs->apply();
}
