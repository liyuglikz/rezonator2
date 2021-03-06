#include "ParamEditorEx.h"

#include "Appearance.h"
#include "FormulaEditor.h"
#include "ParamEditor.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWidgets.h"

#include <QDebug>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>

ParamEditorEx::ParamEditorEx(Z::Parameter *param, Z::Formulas *formulas, Z::Parameters *globalParams, QWidget *parent)
    : QWidget(parent), _param(param), _formulas(formulas), _globalParams(globalParams)
{
    _tmpParam = new Z::Parameter(param->dim(), param->alias());
    _tmpParam->setValue(param->value());

    _formula = _formulas->get(_param);
    _hasFormula = _formula;

    auto menu = new QMenu(this);
    _actnAddFormula = menu->addAction(tr("Add formula"), this, &ParamEditorEx::addFormula);
    _actnRemoveFormula = menu->addAction(tr("Remove formula"), this, &ParamEditorEx::removeFormula);
    _actnRemoveFormula->setVisible(false);

    _paramEditor = new ParamEditor(_tmpParam);

    auto optionsButton = new QPushButton;
    optionsButton->setFlat(true);
    optionsButton->setIcon(QIcon(":/toolbar/options"));
    optionsButton->setFixedWidth(24);
    connect(optionsButton, &QPushButton::clicked, [menu, optionsButton](){
        // button->setMenu() crashes the app on MacOS when button is clicked, so show manually
        menu->popup(optionsButton->mapToGlobal(optionsButton->rect().bottomLeft()));
    });

    Ori::Layouts::LayoutV({
        Ori::Layouts::LayoutH({
            _paramEditor,
            optionsButton
        })
        .setMargin(0)
        .setSpacing(0)
    })
    .setMargin(0)
    .useFor(this);

    toggleFormulaView();
}

ParamEditorEx::~ParamEditorEx()
{
    // Delete editor manually to be sure it is deleted BEFORE _tmpParam
    // It's because of editor uses its param in destructor and param must be still valid.
    delete _paramEditor;
    delete _tmpParam;
    if (_tmpFormula)
        delete _tmpFormula;
}

void ParamEditorEx::addFormula()
{
    _hasFormula = true;
    toggleFormulaView();
    _formulaEditor->setFocus();
}

void ParamEditorEx::removeFormula()
{
    _hasFormula = false;
    toggleFormulaView();
    _paramEditor->setFocus();
}

void ParamEditorEx::createFormulaEditor()
{
    enum { ROW_VALUE, ROW_CODE };

    _tmpFormula = new Z::Formula(_tmpParam);
    if (_formula)
    {
        _tmpFormula->setCode(_formula->code());
        _tmpFormula->assignDeps(_formula);
    }
    FormulaEditor::Options opts;
    opts.formula = _tmpFormula;
    opts.targetParam = _param;
    opts.globalParams = _globalParams;
    opts.formulas = _formulas;
    _formulaEditor = new FormulaEditor(opts);
    connect(_paramEditor, &ParamEditor::unitChanged, this, &ParamEditorEx::unitChanged);
    qobject_cast<QVBoxLayout*>(layout())->insertWidget(ROW_CODE, _formulaEditor);
}

void ParamEditorEx::toggleFormulaView()
{
    if (_hasFormula && !_tmpFormula)
        createFormulaEditor();

    if (_formulaEditor)
        _formulaEditor->setVisible(_hasFormula);

    _actnAddFormula->setVisible(!_hasFormula);
    _actnRemoveFormula->setVisible(_hasFormula);

    auto valueEditor = qobject_cast<QLineEdit*>(_paramEditor->valueEditor());
    Z::Gui::setFontStyle(valueEditor, false, _hasFormula);
    valueEditor->setReadOnly(_hasFormula);

    if (_formulaEditor)
    {
        _formulaEditor->calculate();
        _formulaEditor->setFocus();
    }
}

void ParamEditorEx::apply()
{
    if (_hasFormula)
    {
        if (!_formula)
        {
            _formula = new Z::Formula(_param);
            _formulas->put(_formula);
        }
        _formula->setCode(_tmpFormula->code());
        _formula->assignDeps(_tmpFormula);
    }
    else
    {
        if (_formula)
        {
            _formulas->free(_param);
            _formula = nullptr;
        }
        _paramEditor->apply();
    }
    _param->setValue(_tmpParam->value());
}

void ParamEditorEx::unitChanged(Z::Unit unit)
{
    // Assign new unit to the param in order to formula can consider it when calculate
    _tmpParam->setValue(Z::Value(_tmpParam->value().value(), unit));
    _formulaEditor->calculate();
}

