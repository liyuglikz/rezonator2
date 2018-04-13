#ifndef SCHEMA_READER_JSON_H
#define SCHEMA_READER_JSON_H

#include "../core/Report.h"

QT_BEGIN_NAMESPACE
class QJsonObject;
QT_END_NAMESPACE

class Element;
class Schema;

class SchemaReaderJson
{
public:
    SchemaReaderJson(Schema *schema);

    void readFromFile(const QString& fileName);
    void readFromString(const QString& text);
    void readFromUtf8(const QByteArray& data);

    const Z::Report& report() const { return _report; }

private:
    Schema *_schema;
    Z::Report _report;

    void readGeneral(const QJsonObject& root);
    void readGlobalParams(const QJsonObject& root);
    void readGlobalParam(const QJsonObject& root, const QString& alias);
    void readPump(const QJsonObject& root);
    void readElements(const QJsonObject& root);
    void readElement(const QJsonObject& root);
    void readStorables(const QJsonObject& root);
};

#endif // SCHEMA_READER_JSON_H