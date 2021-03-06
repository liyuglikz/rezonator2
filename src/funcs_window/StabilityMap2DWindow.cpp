#include "StabilityMap2DWindow.h"

#include "../VariableDialog.h"

StabilityMap2DWindow::StabilityMap2DWindow(Schema *schema) :
    PlotFuncWindowStorable(new StabilityMap2DFunction(schema))
{
}

bool StabilityMap2DWindow::configureInternal()
{
    return VariableDialog::TwoElemensDlg(schema(), function()->paramX(), function()->paramY(),
        tr("Contour Stability Map Parameters"), "func_stab_map_2d").run();
}
