#ifndef FUNCTION_BASE_H
#define FUNCTION_BASE_H

#include "../core/Element.h"
#include "../core/Variable.h"

#define FUNC_NAME(s)\
    QString name() const override { return s; }

#define FUNC_ALIAS(s)\
    static QString _alias_() { return s; }\
    QString alias() const override { return s; }

class Schema;
class RoundTripCalculator;

/**
    Base class for all functions.
*/
class FunctionBase
{
public:
    enum FunctionState
    {
        Ok, ///< Function result is valid and actual.
        Frozen, ///< Function result has no relations to current schema anymore but valid.
        Dead, ///< Function result has no meaning anymore. Function window should be closed.
    };
public:
    FunctionBase(Schema *schema) : _schema(schema) {}
    virtual ~FunctionBase();

    Schema* schema() const { return _schema; }

    /// Returns display name of function.
    virtual QString name() const = 0;

    /// Function alias is used for saving/restoring of function windows.
    virtual QString alias() const { return QString(); }

    virtual QString helpTopic() const { return QString(); }

protected:
    Schema *_schema;
};

/**
    Base class for all information functions.
    Information function is a function presenting its calculation
    results in textual form. These results is generally displayed
    in special information popup window.
*/
class InfoFunction : public FunctionBase
{
public:
    InfoFunction(Schema *schema) : FunctionBase(schema) {}
    virtual FunctionState elementDeleting(Element*);
    virtual QString calculate() { return ""; }
};

#endif // FUNCTION_BASE_H
