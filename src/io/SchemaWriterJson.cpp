#include "SchemaWriterJson.h"

#include "z_io_utils.h"
#include "../core/Schema.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

namespace Z {
namespace IO {
namespace Json {

QJsonObject writeParamValue(Z::Parameter* param)
{
    return QJsonObject({
        { "value", param->value().value() },
        { "unit", param->value().unit()->alias() }
    });
}

} // namespace Json
} // namespace IO
} // namespace Z

using namespace Z::IO::Json;

SchemaWriterJson::SchemaWriterJson(Schema *schema) : _schema(schema)
{
}

void SchemaWriterJson::writeToFile(const QString &fileName)
{
    auto text = writeToString();
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return _report.error(file.errorString());
    QTextStream stream(&file);
    stream << text;
    file.close();
}

QString SchemaWriterJson::writeToString()
{
    QJsonObject root;
    root["schema_version"] = Z::IO::Utils::currentVersion().str();

    writeGeneral(root);
    writeGlobalParams(root);
    writePump(root);
    writeElements(root);
    writeWindows(root);

    QJsonDocument doc(root);
    return doc.toJson();
}

void SchemaWriterJson::writeGeneral(QJsonObject& root)
{
    root["trip_type"] = TripTypes::info(_schema->tripType()).alias();

    root["named_params"] = QJsonObject({
        { "lambda", writeParamValue(&_schema->wavelength()) }
    });
}

void SchemaWriterJson::writeGlobalParams(QJsonObject& root)
{
    QJsonObject globalParams;
    for (Z::Parameter *p : *_schema->params())
        globalParams[p->alias()] = QJsonObject({
            { "label", p->label() },
            { "name", p->name() },
            { "descr", p->description() },
            { "category", p->category() },
            { "visible", p->visible() },
            { "dim", p->dim()->alias() },
            { "value", p->value().value() },
            { "unit", p->value().unit()->alias() },
        });
    root["global_params"] = globalParams;
}

void SchemaWriterJson::writePump(QJsonObject &root)
{
    // TODO:NEXT-VER
    //    auto nodePump = document()->createElement("pump");
    //    nodePump.setAttribute("mode", ENUM_ITEM_NAME(Z::Pump::PumpMode, _schema->pump().mode));

    //    WRITE_PUMP_MODE(waist, radius, distance, mi)
    //    WRITE_PUMP_MODE(front, radius, curvature, mi)
    //    WRITE_PUMP_MODE(complex, re, im, mi)
    //    WRITE_PUMP_MODE(icomplex, re, im, mi)
    //    WRITE_PUMP_MODE(vector, radius, angle, distance)
    //    WRITE_PUMP_MODE(sections, radius_1, radius_2, distance)

    //    root.appendChild(nodePump);
}

void SchemaWriterJson::writeElements(QJsonObject& root)
{
    QJsonArray elemsJson;
    for (Element* elem: _schema->elements())
    {
        QJsonObject elemJson;
        writeElement(elemJson, elem);
        elemsJson.append(elemJson);
    }
    root["elements"] = elemsJson;
}

void SchemaWriterJson::writeElement(QJsonObject& root, Element *elem)
{
    root["type"] = elem->type();
    root["label"] = elem->label();
    root["title"] = elem->title();
    root["is_disabled"] = elem->disabled();

    QJsonObject paramsJson;
    for (Z::Parameter* p : elem->params())
        paramsJson[p->alias()] = writeParamValue(p);
    root["params"] = paramsJson;
}

void SchemaWriterJson::writeWindows(QJsonObject& root)
{
/* TODO
    auto windowsNode = _writer->makeNode(root, "windows");

    auto windows = WindowsManager::instance().schemaWindows(_schema);
    for (auto window : windows)
    {
        auto storable = dynamic_cast<ISchemaStorable*>(window);
        if (!storable) continue;

        auto node = _writer->makeNode(windowsNode, "window");
        node.setAttribute("type", storable->type());
        storable->write(_writer, node);
    }
*/
}
