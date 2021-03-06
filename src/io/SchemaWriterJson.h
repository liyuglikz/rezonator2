#ifndef SCHEMA_WRITER_JSON_H
#define SCHEMA_WRITER_JSON_H

#include "../core/Report.h"

QT_BEGIN_NAMESPACE
class QJsonObject;
QT_END_NAMESPACE

class Element;
class Schema;
namespace Z {
class PumpParams;
}

class SchemaWriterJson
{
public:
    SchemaWriterJson(Schema *schema) : _schema(schema) {}

    void writeToFile(const QString& fileName);
    QString writeToString();

    const Z::Report& report() const { return _report; }

private:
    Schema *_schema;
    Z::Report _report;

    void writeGeneral(QJsonObject& root);
    void writeCustomParams(QJsonObject& root);
    void writePumps(QJsonObject &root);
    void writePump(QJsonObject &root, Z::PumpParams *pump);
    void writeElements(QJsonObject& root);
    void writeElement(QJsonObject& root, Element *elem);
    void writeParamLinks(QJsonObject& root);
    void writeFormulas(QJsonObject& root);
    void writeWindows(QJsonObject& root);
};

#endif // SCHEMA_WRITER_JSON_H
