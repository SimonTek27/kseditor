#include "ConfigSchema.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QListWidget>
#include <algorithm>

namespace ks {
namespace config {

// ─── SchemaProperty Implementation ────────────────────────────────────────

bool SchemaProperty::validate(const QJsonValue& value, QStringList* errors) const {
    // Type check
    switch (type) {
        case SchemaType::String:
            if (!value.isString()) {
                if (errors) errors->append(QString("Expected string, got %1").arg(value.type()));
                return false;
            }
            // Check constraints
            if (value.isString()) {
                QString s = value.toString();
                if (minLength > 0 && s.length() < minLength) {
                    if (errors) errors->append(QString("String length %1 below minimum %2").arg(s.length()).arg(minLength));
                    return false;
                }
                if (maxLength >= 0 && s.length() > maxLength) {
                    if (errors) errors->append(QString("String length %1 exceeds maximum %2").arg(s.length()).arg(maxLength));
                    return false;
                }
                if (!pattern.isEmpty()) {
                    QRegularExpression re(pattern);
                    if (!re.match(s).hasMatch()) {
                        if (errors) errors->append(QString("String does not match pattern: %1").arg(pattern));
                        return false;
                    }
                }
                if (!enumValues.isEmpty() && !enumValues.contains(s)) {
                    if (errors) errors->append(QString("Value '%1' not in allowed enum values").arg(s));
                    return false;
                }
            }
            break;
            
        case SchemaType::Number:
        case SchemaType::Integer:
            if (!value.isDouble() && !value.isDouble()) {
                if (errors) errors->append("Expected number");
                return false;
            }
            if (value.isDouble()) {
                double n = value.toDouble();
                if (type == SchemaType::Integer && n != qFloor(n)) {
                    if (errors) errors->append("Expected integer");
                    return false;
                }
                if (exclusiveMinimum && n <= minimum) {
                    if (errors) errors->append(QString("Value must be > %1").arg(minimum));
                    return false;
                }
                if (exclusiveMaximum && n >= maximum) {
                    if (errors) errors->append(QString("Value must be < %1").arg(maximum));
                    return false;
                }
                if (!exclusiveMinimum && n < minimum) {
                    if (errors) errors->append(QString("Value must be >= %1").arg(minimum));
                    return false;
                }
                if (!exclusiveMaximum && n > maximum) {
                    if (errors) errors->append(QString("Value must be <= %1").arg(maximum));
                    return false;
                }
                if (multipleOf > 0 && fmod(n, multipleOf) != 0) {
                    if (errors) errors->append(QString("Value must be multiple of %1").arg(multipleOf));
                    return false;
                }
            }
            break;
            
        case SchemaType::Boolean:
            if (!value.isBool()) {
                if (errors) errors->append("Expected boolean");
                return false;
            }
            break;
            
        case SchemaType::Array:
            if (!value.isArray()) {
                if (errors) errors->append("Expected array");
                return false;
            }
            if (value.isArray()) {
                QJsonArray arr = value.toArray();
                if (minItems > 0 && arr.size() < minItems) {
                    if (errors) errors->append(QString("Array has %1 items, minimum is %2").arg(arr.size()).arg(minItems));
                    return false;
                }
                if (maxItems >= 0 && arr.size() > maxItems) {
                    if (errors) errors->append(QString("Array has %1 items, maximum is %2").arg(arr.size()).arg(maxItems));
                    return false;
                }
                if (uniqueItems) {
                    // Check uniqueness (simplified)
                }
                if (items) {
                    for (int i = 0; i < arr.size(); ++i) {
                        if (!items->validate(arr[i], errors)) {
                            if (errors) errors->prepend(QString("Array item %1: ").arg(i));
                            return false;
                        }
                    }
                }
            }
            break;
            
        case SchemaType::Object:
            if (!value.isObject()) {
                if (errors) errors->append("Expected object");
                return false;
            }
            if (value.isObject()) {
                QJsonObject obj = value.toObject();
                // Check required fields
                for (const QString& req : required) {
                    if (!obj.contains(req)) {
                        if (errors) errors->append(QString("Required property '%1' missing").arg(req));
                        return false;
                    }
                }
                if (minProperties > 0 && obj.size() < minProperties) {
                    if (errors) errors->append(QString("Object has %1 properties, minimum is %2").arg(obj.size()).arg(minProperties));
                    return false;
                }
                if (maxProperties >= 0 && obj.size() > maxProperties) {
                    if (errors) errors->append(QString("Object has %1 properties, maximum is %2").arg(obj.size()).arg(maxProperties));
                    return false;
                }
                // Validate each property
                for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
                    if (properties.contains(it.key())) {
                        if (!properties[it.key()]->validate(it.value(), errors)) {
                            if (errors) errors->prepend(QString("Property '%1': ").arg(it.key()));
                            return false;
                        }
                    } else if (additionalProperties) {
                        if (!additionalProperties->validate(it.value(), errors)) {
                            return false;
                        }
                    } else if (!additionalProperties) {
                        if (errors) errors->append(QString("Additional property '%1' not allowed").arg(it.key()));
                        return false;
                    }
                }
            }
            break;
            
        case SchemaType::Enum:
            if (!enumValues.contains(value.toString())) {
                if (errors) errors->append(QString("Value not in enum: %1").arg(enumValues.join(", ")));
                return false;
            }
            break;
            
        default:
            break;
    }
    return true;
}

// ─── JsonSchema Implementation ────────────────────────────────────────────

bool JsonSchema::validate(const QJsonObject& data, QStringList* errors) const {
    if (root) {
        return root->validate(QJsonValue(data), errors);
    }
    return true;
}

QJsonObject JsonSchema::applyDefaults(const QJsonObject& data) const {
    QJsonObject result = data;
    if (root) {
        applyDefaultsRecursive(result, root);
    }
    return result;
}

void JsonSchema::applyDefaultsRecursive(QJsonObject& obj, const std::shared_ptr<SchemaProperty>& schema) const {
    if (!schema) return;
    
    if (schema->type == SchemaType::Object && schema->properties.size() > 0) {
        for (auto it = schema->properties.constBegin(); it != schema->properties.constEnd(); ++it) {
            const QString& key = it.key();
            auto propSchema = it.value();
            
            if (!obj.contains(key) && propSchema->defaultValue.isValid()) {
                obj[key] = QJsonValue::fromVariant(propSchema->defaultValue);
            } else if (obj.contains(key) && obj[key].isObject() && propSchema->type == SchemaType::Object) {
                QJsonObject nestedObj = obj[key].toObject();
                applyDefaultsRecursive(nestedObj, propSchema);
                obj[key] = nestedObj;
            }
        }
    }
}

QString JsonSchema::generateCppStruct() const {
    QString code = QString("// Generated from schema: %1\n\n").arg(title);
    code += "#include <QJsonObject>\n#include <QJsonArray>\n#include <QVector>\n#include <QMap>\n#include <QVariant>\n\n";
    code += generateCppStructRecursive(root, title);
    return code;
}

QString JsonSchema::generateCppStructRecursive(const std::shared_ptr<SchemaProperty>& schema, const QString& name) const {
    if (!schema || schema->type != SchemaType::Object) return QString();
    
    QString code = QString("struct %1 {\n").arg(name);
    
    for (auto it = schema->properties.constBegin(); it != schema->properties.constEnd(); ++it) {
        const QString& key = it.key();
        auto prop = it.value();
        QString memberName = key;
        memberName.replace(QRegularExpression("[^a-zA-Z0-9_]"), "_");
        
        QString typeName;
        switch (prop->type) {
            case SchemaType::String: typeName = "QString"; break;
            case SchemaType::Number: typeName = "double"; break;
            case SchemaType::Integer: typeName = "int"; break;
            case SchemaType::Boolean: typeName = "bool"; break;
            case SchemaType::Array: 
                if (prop->items && prop->items->type == SchemaType::Object) {
                    typeName = QString("QVector<%1>").arg(prop->items->properties.size() > 0 ? "QJsonObject" : "QVariant");
                } else {
                    typeName = "QJsonArray";
                }
                break;
            case SchemaType::Object:
                typeName = "QJsonObject";
                break;
            default: typeName = "QJsonValue"; break;
        }
        
        code += QString("    %1 %2;\n").arg(typeName).arg(memberName);
    }
    
    code += "};\n\n";
    return code;
}

QString JsonSchema::generateQmlForm() const {
    if (!root) return QString();
    
    QString qml = QString("import QtQuick 2.15\nimport QtQuick.Controls 2.15\nimport QtQuick.Layouts 1.15\n\n");
    qml += QString("Item {\n    id: %1Form\n    property variant formData: {}\n\n").arg(title);
    
    qml += "    ColumnLayout {\n        anchors.fill: parent\n        spacing: 10\n\n";
    
    if (root->properties.size() > 0) {
        for (auto it = root->properties.constBegin(); it != root->properties.constEnd(); ++it) {
            qml += generateQmlField(it.key(), it.value());
        }
    }
    
    qml += "    }\n}\n";
    return qml;
}

QString JsonSchema::generateQmlField(const QString& name, const std::shared_ptr<SchemaProperty>& prop) const {
    QString field;
    QString displayName = prop->title.isEmpty() ? name : prop->title;
    QString widgetType = prop->widget.isEmpty() ? "text" : prop->widget;
    
    field += QString("        Label { text: \"%1\" Layout.fillWidth: true }\n").arg(displayName);
    
    if (widgetType == "checkbox" || prop->type == SchemaType::Boolean) {
        field += QString("        CheckBox { id: %1Check; checked: formData.%2 }\n").arg(name).arg(name);
    } else if (widgetType == "select" || !prop->enumValues.isEmpty()) {
        field += QString("        ComboBox { id: %1Combo; model: %2; currentText: formData.%3 }\n")
            .arg(name)
            .arg(prop->enumValues.isEmpty() ? "[]" : QString("[%1]").arg(prop->enumValues.join("\", \"")))
            .arg(name);
    } else if (widgetType == "color") {
        field += QString("        ColorPicker { id: %1Color; color: formData.%2 }\n").arg(name).arg(name);
    } else if (widgetType == "file") {
        field += QString("        FilePicker { id: %1File; path: formData.%3 }\n").arg(name).arg(name);
    } else if (widgetType == "slider" || prop->widgetMin != 0 || prop->widgetMax > 0) {
        field += QString("        Slider { id: %1Slider; from: %2; to: %3; value: formData.%4; stepSize: %5 }\n")
            .arg(name).arg(prop->widgetMin).arg(prop->widgetMax).arg(name).arg(prop->widgetStep);
    } else if (widgetType == "code" || prop->format == "code") {
        field += QString("        TextArea { id: %1Code; text: formData.%2; wrapMode: TextArea.Wrap }\n").arg(name).arg(name);
    } else {
        field += QString("        TextField { id: %1Field; text: formData.%2; placeholderText: \"%3\" }\n")
            .arg(name).arg(name).arg(prop->placeholder);
    }
    
    field += "\n";
    return field;
}

// ─── SchemaBuilder Implementation ────────────────────────────────────────

SchemaBuilder::SchemaBuilder() {
    reset();
}

SchemaBuilder& SchemaBuilder::reset() {
    m_current = std::make_shared<SchemaProperty>();
    m_current->type = SchemaType::Object;
    m_properties.clear();
    m_required.clear();
    m_additionalProperties = true;
    m_additionalSchema.reset();
    return *this;
}

SchemaBuilder& SchemaBuilder::object(const QString& title) {
    m_current->type = SchemaType::Object;
    if (!title.isEmpty()) m_current->title = title;
    return *this;
}

SchemaBuilder& SchemaBuilder::required(const QStringList& fields) {
    m_required = fields;
    return *this;
}

SchemaBuilder& SchemaBuilder::property(const QString& name, std::shared_ptr<SchemaProperty> prop) {
    m_properties[name] = prop;
    return *this;
}

SchemaBuilder& SchemaBuilder::property(const QString& name, SchemaBuilder& builder) {
    return property(name, builder.build());
}

SchemaBuilder& SchemaBuilder::additionalProperties(bool allowed) {
    m_additionalProperties = allowed;
    return *this;
}

SchemaBuilder& SchemaBuilder::additionalProperties(std::shared_ptr<SchemaProperty> schema) {
    m_additionalSchema = schema;
    m_additionalProperties = true;
    return *this;
}

SchemaBuilder& SchemaBuilder::array(const QString& title) {
    m_current->type = SchemaType::Array;
    if (!title.isEmpty()) m_current->title = title;
    return *this;
}

SchemaBuilder& SchemaBuilder::items(std::shared_ptr<SchemaProperty> itemSchema) {
    m_current->items = itemSchema;
    return *this;
}

SchemaBuilder& SchemaBuilder::items(SchemaBuilder& builder) {
    return items(builder.build());
}

SchemaBuilder& SchemaBuilder::minItems(int n) {
    m_current->minItems = n;
    return *this;
}

SchemaBuilder& SchemaBuilder::maxItems(int n) {
    m_current->maxItems = n;
    return *this;
}

SchemaBuilder& SchemaBuilder::uniqueItems(bool unique) {
    m_current->uniqueItems = unique;
    return *this;
}

SchemaBuilder& SchemaBuilder::string(const QString& title) {
    m_current->type = SchemaType::String;
    if (!title.isEmpty()) m_current->title = title;
    return *this;
}

SchemaBuilder& SchemaBuilder::pattern(const QString& regex) {
    m_current->pattern = regex;
    return *this;
}

SchemaBuilder& SchemaBuilder::minLength(int n) {
    m_current->minLength = n;
    return *this;
}

SchemaBuilder& SchemaBuilder::maxLength(int n) {
    m_current->maxLength = n;
    return *this;
}

SchemaBuilder& SchemaBuilder::enumValues(const QStringList& values) {
    m_current->enumValues = values;
    return *this;
}

SchemaBuilder& SchemaBuilder::format(const QString& fmt) {
    m_current->format = fmt;
    return *this;
}

SchemaBuilder& SchemaBuilder::number(const QString& title) {
    m_current->type = SchemaType::Number;
    if (!title.isEmpty()) m_current->title = title;
    return *this;
}

SchemaBuilder& SchemaBuilder::integer(const QString& title) {
    m_current->type = SchemaType::Integer;
    if (!title.isEmpty()) m_current->title = title;
    return *this;
}

SchemaBuilder& SchemaBuilder::minimum(double n, bool exclusive) {
    m_current->minimum = n;
    m_current->exclusiveMinimum = exclusive;
    return *this;
}

SchemaBuilder& SchemaBuilder::maximum(double n, bool exclusive) {
    m_current->maximum = n;
    m_current->exclusiveMaximum = exclusive;
    return *this;
}

SchemaBuilder& SchemaBuilder::multipleOf(double n) {
    m_current->multipleOf = n;
    return *this;
}

SchemaBuilder& SchemaBuilder::boolean(const QString& title) {
    m_current->type = SchemaType::Boolean;
    if (!title.isEmpty()) m_current->title = title;
    return *this;
}

SchemaBuilder& SchemaBuilder::enumType(const QStringList& values) {
    m_current->type = SchemaType::Enum;
    m_current->enumValues = values;
    return *this;
}

SchemaBuilder& SchemaBuilder::title(const QString& t) {
    m_current->title = t;
    return *this;
}

SchemaBuilder& SchemaBuilder::description(const QString& d) {
    m_current->description = d;
    return *this;
}

SchemaBuilder& SchemaBuilder::defaultValue(const QVariant& value) {
    m_current->defaultValue = value;
    return *this;
}

SchemaBuilder& SchemaBuilder::widget(const QString& w, const QStringList& options) {
    m_current->widget = w;
    m_current->widgetOptions = options;
    return *this;
}

SchemaBuilder& SchemaBuilder::readOnly(bool ro) {
    m_current->readOnly = ro;
    return *this;
}

SchemaBuilder& SchemaBuilder::hidden(bool h) {
    m_current->hidden = h;
    return *this;
}

SchemaBuilder& SchemaBuilder::category(const QString& cat) {
    m_current->category = cat;
    return *this;
}

SchemaBuilder& SchemaBuilder::order(int o) {
    m_current->order = o;
    return *this;
}

SchemaBuilder& SchemaBuilder::placeholder(const QString& p) {
    m_current->placeholder = p;
    return *this;
}

SchemaBuilder& SchemaBuilder::helpText(const QString& text) {
    m_current->helpText = text;
    return *this;
}

SchemaBuilder& SchemaBuilder::visibleIf(const QString& expr) {
    m_current->visibleIf = expr;
    return *this;
}

SchemaBuilder& SchemaBuilder::enabledIf(const QString& expr) {
    m_current->enabledIf = expr;
    return *this;
}

SchemaBuilder& SchemaBuilder::ref(const QString& reference) {
    m_current->ref = reference;
    return *this;
}

std::shared_ptr<SchemaProperty> SchemaBuilder::build() {
    if (m_current->type == SchemaType::Object) {
        m_current->properties = m_properties;
        m_current->required = m_required;
        m_current->additionalProperties = m_additionalProperties ? m_additionalSchema : nullptr;
    }
    return m_current;
}

// Presets
std::shared_ptr<SchemaProperty> SchemaBuilder::filePicker(const QString& title, const QString& filter) {
    auto prop = std::make_shared<SchemaProperty>();
    prop->type = SchemaType::String;
    prop->title = title;
    prop->widget = "file";
    prop->widgetOptions = filter.split(";;");
    return prop;
}

std::shared_ptr<SchemaProperty> SchemaBuilder::directoryPicker(const QString& title) {
    auto prop = std::make_shared<SchemaProperty>();
    prop->type = SchemaType::String;
    prop->title = title;
    prop->widget = "directory";
    return prop;
}

std::shared_ptr<SchemaProperty> SchemaBuilder::colorPicker(const QString& title) {
    auto prop = std::make_shared<SchemaProperty>();
    prop->type = SchemaType::String;
    prop->title = title;
    prop->widget = "color";
    prop->format = "color";
    return prop;
}

std::shared_ptr<SchemaProperty> SchemaBuilder::slider(const QString& title, double min, double max, double step) {
    auto prop = std::make_shared<SchemaProperty>();
    prop->type = SchemaType::Number;
    prop->title = title;
    prop->widget = "slider";
    prop->minimum = min;
    prop->maximum = max;
    prop->widgetMin = min;
    prop->widgetMax = max;
    prop->widgetStep = step;
    return prop;
}

std::shared_ptr<SchemaProperty> SchemaBuilder::dropdown(const QString& title, const QStringList& options) {
    auto prop = std::make_shared<SchemaProperty>();
    prop->type = SchemaType::String;
    prop->title = title;
    prop->widget = "select";
    prop->enumValues = options;
    return prop;
}

std::shared_ptr<SchemaProperty> SchemaBuilder::codeEditor(const QString& title, const QString& language) {
    auto prop = std::make_shared<SchemaProperty>();
    prop->type = SchemaType::String;
    prop->title = title;
    prop->widget = "code";
    prop->format = language;
    return prop;
}

std::shared_ptr<SchemaProperty> SchemaBuilder::textArea(const QString& title) {
    auto prop = std::make_shared<SchemaProperty>();
    prop->type = SchemaType::String;
    prop->title = title;
    prop->widget = "textarea";
    return prop;
}

std::shared_ptr<SchemaProperty> SchemaBuilder::checkbox(const QString& title) {
    auto prop = std::make_shared<SchemaProperty>();
    prop->type = SchemaType::Boolean;
    prop->title = title;
    prop->widget = "checkbox";
    return prop;
}

// ─── ConfigSchemaRegistry Implementation ─────────────────────────────────

static ConfigSchemaRegistry* s_registryInstance = nullptr;

ConfigSchemaRegistry* ConfigSchemaRegistry::instance() {
    if (!s_registryInstance) s_registryInstance = new ConfigSchemaRegistry();
    return s_registryInstance;
}

ConfigSchemaRegistry::ConfigSchemaRegistry(QObject* parent) : QObject(parent) {
    registerBuiltinSchemas();
}

ConfigSchemaRegistry::~ConfigSchemaRegistry() {
    s_registryInstance = nullptr;
}

void ConfigSchemaRegistry::registerSchema(const QString& configType, std::shared_ptr<JsonSchema> schema) {
    m_schemas[configType] = schema;
    emit schemaRegistered(configType);
}

void ConfigSchemaRegistry::unregisterSchema(const QString& configType) {
    m_schemas.remove(configType);
    emit schemaUnregistered(configType);
}

std::shared_ptr<JsonSchema> ConfigSchemaRegistry::getSchema(const QString& configType) const {
    return m_schemas.value(configType);
}

QStringList ConfigSchemaRegistry::registeredTypes() const {
    return m_schemas.keys();
}

void ConfigSchemaRegistry::registerBuiltinSchemas() {
    registerSchema("csp_global", cspGlobalConfigSchema());
    registerSchema("csp_car", cspCarConfigSchema());
    registerSchema("csp_track", cspTrackConfigSchema());
    registerSchema("pp_filter", ppFilterSchema());
    registerSchema("weather", weatherConfigSchema());
    registerSchema("server", serverConfigSchema());
    registerSchema("career", careerConfigSchema());
}

std::shared_ptr<JsonSchema> ConfigSchemaRegistry::cspGlobalConfigSchema() {
    SchemaBuilder builder;
    builder.object("CSP Global Configuration")
        .title("CSP Global Settings")
        .description("Global configuration for Custom Shaders Patch extensions")
        .property("enabled", SchemaBuilder().boolean("Enable CSP").defaultValue(true).title("Enabled"))
        .property("version", SchemaBuilder().string("Config Version").defaultValue("1.0").title("Version"))
        .property("weather_fx", SchemaBuilder().object("Weather FX")
            .property("enabled", SchemaBuilder().boolean("Enable Weather FX").defaultValue(true))
            .property("time_multiplier", SchemaBuilder().number("Time Multiplier").defaultValue(1.0).minimum(0.1).maximum(100))
            .property("use_real_weather", SchemaBuilder().boolean("Use Real Weather").defaultValue(false))
            .property("parameters", SchemaBuilder().object("Weather Parameters").additionalProperties(true))
            .build())
        .property("lighting_fx", SchemaBuilder().object("Lighting FX")
            .property("enabled", SchemaBuilder().boolean("Enable Lighting FX").defaultValue(true))
            .property("dynamic_lights", SchemaBuilder().boolean("Dynamic Lights").defaultValue(true))
            .property("enable_occlusion", SchemaBuilder().boolean("Enable Occlusion").defaultValue(true))
            .property("ambient_multiplier", SchemaBuilder().number("Ambient Multiplier").defaultValue(1.0).minimum(0).maximum(10))
            .property("sun_multiplier", SchemaBuilder().number("Sun Multiplier").defaultValue(1.0).minimum(0).maximum(10))
            .build())
        .property("particles_fx", SchemaBuilder().object("Particles FX")
            .property("enabled", SchemaBuilder().boolean("Enable Particles FX").defaultValue(true))
            .property("enable_smoke", SchemaBuilder().boolean("Enable Smoke").defaultValue(true))
            .build());
    
    auto schema = std::make_shared<JsonSchema>();
    schema->id = "csp_global_config";
    schema->title = "CSP Global Config";
    schema->root = builder.build();
    return schema;
}

std::shared_ptr<JsonSchema> ConfigSchemaRegistry::cspCarConfigSchema() {
    SchemaBuilder builder;
    builder.object("CSP Car Configuration")
        .title("CSP Car Configuration")
        .description("Car-specific CSP extension settings")
        .property("car_name", SchemaBuilder().string("Car Name").title("Car Name"))
        .property("car_brand", SchemaBuilder().string("Car Brand").title("Brand"))
        .property("year", SchemaBuilder().integer("Year").minimum(1900).maximum(2030).title("Year"))
        .property("extensions", SchemaBuilder().object("Car Extensions")
            .property("reverse_lights", SchemaBuilder().boolean("Reverse Lights").defaultValue(true))
            .property("brake_lights", SchemaBuilder().boolean("Brake Lights").defaultValue(true))
            .property("turn_signals", SchemaBuilder().boolean("Turn Signals").defaultValue(true))
            .property("drs", SchemaBuilder().boolean("DRS").defaultValue(false))
            .property("kers", SchemaBuilder().boolean("KERS").defaultValue(false))
            .property("tire_wear", SchemaBuilder().boolean("Tire Wear Visual").defaultValue(true))
            .property("damage", SchemaBuilder().boolean("Damage Model").defaultValue(true))
            .build())
        .property("lights", SchemaBuilder().object("Light Configuration")
            .property("headlight_intensity", SchemaBuilder().number("Headlight Intensity").defaultValue(1.0).minimum(0).maximum(5))
            .property("taillight_intensity", SchemaBuilder().number("Taillight Intensity").defaultValue(1.0).minimum(0).maximum(5))
            .property("brake_intensity", SchemaBuilder().number("Brake Light Intensity").defaultValue(2.0).minimum(0).maximum(10))
            .property("reverse_intensity", SchemaBuilder().number("Reverse Light Intensity").defaultValue(1.0).minimum(0).maximum(5))
            .build())
        .property("physics", SchemaBuilder().object("Physics Extensions")
            .property("tire_model", SchemaBuilder().string("Tire Model").enumValues({"Pacejka89", "Pacejka96", "Pacejka2002", "Pacejka2011"}))
            .property("aero_map", SchemaBuilder().string("Aero Map").defaultValue("default"))
            .property("suspension_geometry", SchemaBuilder().object("Suspension Geometry").additionalProperties(true))
            .build());
    
    auto schema = std::make_shared<JsonSchema>();
    schema->id = "csp_car_config";
    schema->title = "CSP Car Config";
    schema->root = builder.build();
    return schema;
}

std::shared_ptr<JsonSchema> ConfigSchemaRegistry::cspTrackConfigSchema() {
    SchemaBuilder builder;
    builder.object("CSP Track Configuration")
        .title("CSP Track Configuration")
        .description("Track-specific CSP extension settings")
        .property("track_name", SchemaBuilder().string("Track Name").title("Track Name"))
        .property("layout", SchemaBuilder().string("Layout").title("Layout"))
        .property("extensions", SchemaBuilder().object("Track Extensions")
            .property("dynamic_lights", SchemaBuilder().boolean("Dynamic Lights").defaultValue(true))
            .property("rain_fx", SchemaBuilder().boolean("Rain Effects").defaultValue(true))
            .property("particles", SchemaBuilder().boolean("Particles").defaultValue(true))
            .property("wind", SchemaBuilder().boolean("Wind Effects").defaultValue(true))
            .build())
        .property("surfaces", SchemaBuilder().object("Surface Materials")
            .property("asphalt", SchemaBuilder().object("Asphalt").additionalProperties(true))
            .property("grass", SchemaBuilder().object("Grass").additionalProperties(true))
            .property("gravel", SchemaBuilder().object("Gravel").additionalProperties(true))
            .property("curb", SchemaBuilder().object("Curb").additionalProperties(true))
            .property("sand", SchemaBuilder().object("Sand").additionalProperties(true))
            .build())
        .property("lighting", SchemaBuilder().object("Track Lighting")
            .property("global_illumination", SchemaBuilder().boolean("Global Illumination").defaultValue(true))
            .property("shadow_distance", SchemaBuilder().number("Shadow Distance").defaultValue(200).minimum(50).maximum(1000))
            .property("light_probes", SchemaBuilder().boolean("Light Probes").defaultValue(true))
            .build())
        .property("cameras", SchemaBuilder().array("Cameras")
            .items(SchemaBuilder().object("Camera")
                .property("name", SchemaBuilder().string("Name"))
                .property("position", SchemaBuilder().array("Position").items(SchemaBuilder().number("").minimum(-10000).maximum(10000)).minItems(3).maxItems(3))
                .property("rotation", SchemaBuilder().array("Rotation").items(SchemaBuilder().number("").minimum(-360).maximum(360)).minItems(3).maxItems(3))
                .property("fov", SchemaBuilder().number("FOV").defaultValue(60).minimum(10).maximum(170))
                .build())
            .build());
    
    auto schema = std::make_shared<JsonSchema>();
    schema->id = "csp_track_config";
    schema->title = "CSP Track Config";
    schema->root = builder.build();
    return schema;
}

std::shared_ptr<JsonSchema> ConfigSchemaRegistry::ppFilterSchema() {
    SchemaBuilder builder;
    builder.object("Post-Processing Filter Chain")
        .title("Post-Processing Filter")
        .description("Post-processing filter configuration")
        .property("enabled", SchemaBuilder().boolean("Enable Filter Chain").defaultValue(true).title("Enabled"))
        .property("order", SchemaBuilder().integer("Order").minimum(0).maximum(100).title("Execution Order"))
        .property("filters", SchemaBuilder().array("Filters")
            .items(SchemaBuilder().object("Filter")
                .property("name", SchemaBuilder().string("Filter Name"))
                .property("type", SchemaBuilder().string("Filter Type")
                    .enumValues({"bloom", "tonemap", "color_grading", "vignette", "chromatic_aberration", 
                                "depth_of_field", "motion_blur", "lens_flare", "film_grain", "dirt",
                                "sharpen", "fxaa", "smaa", "caa", "dlss", "fsr"}))
                .property("enabled", SchemaBuilder().boolean("Enabled").defaultValue(true))
                .property("intensity", SchemaBuilder().number("Intensity").defaultValue(1.0).minimum(0).maximum(10))
                .property("parameters", SchemaBuilder().object("Parameters").additionalProperties(true))
                .build())
            .build());
    
    auto schema = std::make_shared<JsonSchema>();
    schema->id = "pp_filter";
    schema->title = "Post-Processing Filter";
    schema->root = builder.build();
    return schema;
}

std::shared_ptr<JsonSchema> ConfigSchemaRegistry::weatherConfigSchema() {
    SchemaBuilder builder;
    builder.object("Weather Configuration")
        .title("Weather Configuration")
        .description("Weather and time-of-day settings")
        .property("time_of_day", SchemaBuilder().number("Time of Day").defaultValue(12.0).minimum(0).maximum(24).title("Time of Day"))
        .property("time_multiplier", SchemaBuilder().number("Time Multiplier").defaultValue(1.0).minimum(0).maximum(100).title("Time Multiplier"))
        .property("weather_type", SchemaBuilder().string("Weather Type")
            .enumValues({"clear", "cloudy", "overcast", "light_rain", "heavy_rain", "storm", "fog", "snow"}))
        .property("cloud_coverage", SchemaBuilder().number("Cloud Coverage").defaultValue(0.5).minimum(0).maximum(1).title("Cloud Coverage"))
        .property("precipitation", SchemaBuilder().number("Precipitation").defaultValue(0.0).minimum(0).maximum(1).title("Precipitation"))
        .property("wind_speed", SchemaBuilder().number("Wind Speed").defaultValue(5.0).minimum(0).maximum(100).title("Wind Speed"))
        .property("wind_direction", SchemaBuilder().number("Wind Direction").defaultValue(0.0).minimum(0).maximum(360).title("Wind Direction"))
        .property("fog_density", SchemaBuilder().number("Fog Density").defaultValue(0.0).minimum(0).maximum(1).title("Fog Density"))
        .property("temperature", SchemaBuilder().number("Temperature").defaultValue(20.0).minimum(-40).maximum(50).title("Temperature (°C)"))
        .property("humidity", SchemaBuilder().number("Humidity").defaultValue(0.5).minimum(0).maximum(1).title("Humidity"))
        .property("keyframes", SchemaBuilder().array("Weather Keyframes")
            .items(SchemaBuilder().object("Keyframe")
                .property("time", SchemaBuilder().number("Time").minimum(0).maximum(24))
                .property("weather_type", SchemaBuilder().string("Weather Type"))
                .property("transition_duration", SchemaBuilder().number("Transition Duration").defaultValue(300))
                .build())
            .build());
    
    auto schema = std::make_shared<JsonSchema>();
    schema->id = "weather_config";
    schema->title = "Weather Configuration";
    schema->root = builder.build();
    return schema;
}

std::shared_ptr<JsonSchema> ConfigSchemaRegistry::serverConfigSchema() {
    SchemaBuilder builder;
    builder.object("Server Configuration")
        .title("Dedicated Server Configuration")
        .description("Assetto Corsa/ACC dedicated server settings")
        .property("name", SchemaBuilder().string("Server Name").title("Server Name"))
        .property("password", SchemaBuilder().string("Password").format("password").title("Password"))
        .property("admin_password", SchemaBuilder().string("Admin Password").format("password").title("Admin Password"))
        .property("max_clients", SchemaBuilder().integer("Max Clients").defaultValue(20).minimum(1).maximum(100).title("Max Clients"))
        .property("port", SchemaBuilder().integer("Port").defaultValue(9000).minimum(1024).maximum(65535).title("Port"))
        .property("http_port", SchemaBuilder().integer("HTTP Port").defaultValue(9001).minimum(1024).maximum(65535).title("HTTP Port"))
        .property("track", SchemaBuilder().string("Track").title("Track"))
        .property("track_layout", SchemaBuilder().string("Track Layout").title("Track Layout"))
        .property("cars", SchemaBuilder().array("Allowed Cars")
            .items(SchemaBuilder().string("Car").title("Car")).title("Cars"))
        .property("session", SchemaBuilder().object("Session Settings")
            .property("practice_duration", SchemaBuilder().integer("Practice Duration (min)").defaultValue(60).minimum(0).maximum(1440))
            .property("qualify_duration", SchemaBuilder().integer("Qualify Duration (min)").defaultValue(15).minimum(0).maximum(1440))
            .property("race_duration", SchemaBuilder().integer("Race Duration (min)").defaultValue(30).minimum(0).maximum(1440))
            .property("race_laps", SchemaBuilder().integer("Race Laps").defaultValue(0).minimum(0).maximum(999))
            .property("overtime", SchemaBuilder().boolean("Overtime").defaultValue(false))
            .property("wait_for_race_ready", SchemaBuilder().boolean("Wait for Race Ready").defaultValue(true))
            .build())
        .property("flags", SchemaBuilder().object("Race Flags")
            .property("yellow_flag", SchemaBuilder().boolean("Yellow Flag").defaultValue(true))
            .property("blue_flag", SchemaBuilder().boolean("Blue Flag").defaultValue(true))
            .property("black_flag", SchemaBuilder().boolean("Black Flag").defaultValue(true))
            .property("white_flag", SchemaBuilder().boolean("White Flag").defaultValue(false))
            .property("drive_through_penalty", SchemaBuilder().boolean("Drive-through Penalty").defaultValue(true))
            .property("stop_go_penalty", SchemaBuilder().boolean("Stop-Go Penalty").defaultValue(true))
            .build())
        .property("bop", SchemaBuilder().object("Balance of Performance")
            .property("enabled", SchemaBuilder().boolean("Enabled").defaultValue(false))
            .property("ballast", SchemaBuilder().object("Ballast").additionalProperties(true))
            .property("restrictor", SchemaBuilder().object("Restrictor").additionalProperties(true))
            .build())
        .property("entry_list", SchemaBuilder().array("Entry List")
            .items(SchemaBuilder().object("Entry")
                .property("car", SchemaBuilder().string("Car").title("Car"))
                .property("skin", SchemaBuilder().string("Skin").title("Skin"))
                .property("driver", SchemaBuilder().string("Driver").title("Driver"))
                .property("team", SchemaBuilder().string("Team").title("Team"))
                .property("nationality", SchemaBuilder().string("Nationality").title("Nationality"))
                .property("ballast_kg", SchemaBuilder().integer("Ballast (kg)").defaultValue(0).minimum(0).maximum(200))
                .property("restrictor", SchemaBuilder().number("Restrictor").defaultValue(0).minimum(0).maximum(1))
                .build())
            .build())
        .property("plugins", SchemaBuilder().array("Plugins")
            .items(SchemaBuilder().object("Plugin")
                .property("name", SchemaBuilder().string("Name"))
                .property("enabled", SchemaBuilder().boolean("Enabled").defaultValue(true))
                .property("config", SchemaBuilder().object("Config").additionalProperties(true))
                .build())
            .build());
    
    auto schema = std::make_shared<JsonSchema>();
    schema->id = "server_config";
    schema->title = "Server Configuration";
    schema->root = builder.build();
    return schema;
}

std::shared_ptr<JsonSchema> ConfigSchemaRegistry::careerConfigSchema() {
    SchemaBuilder builder;
    builder.object("Career Configuration")
        .title("Career Mode Configuration")
        .description("Career/championship/race configuration for career mode")
        .property("career_name", SchemaBuilder().string("Career Name").title("Career Name"))
        .property("career_description", SchemaBuilder().string("Description").widget("textarea").title("Description"))
        .property("difficulty", SchemaBuilder().string("Difficulty").enumValues({"beginner", "amateur", "professional", "legend"}).title("Difficulty"))
        .property("start_year", SchemaBuilder().integer("Start Year").defaultValue(2024).minimum(1950).maximum(2100).title("Start Year"))
        .property("calendar", SchemaBuilder().array("Calendar")
            .items(SchemaBuilder().object("Event")
                .property("name", SchemaBuilder().string("Event Name").title("Name"))
                .property("date", SchemaBuilder().string("Date").format("date").title("Date"))
                .property("track", SchemaBuilder().string("Track").title("Track"))
                .property("layout", SchemaBuilder().string("Layout").title("Layout"))
                .property("event_type", SchemaBuilder().string("Event Type")
                    .enumValues({"practice", "qualifying", "race", "time_trial", "drift", "drag", "elimination", "hotlap"}))
                .property("race_duration", SchemaBuilder().integer("Race Duration (min)").defaultValue(30).minimum(0).maximum(1440))
                .property("race_laps", SchemaBuilder().integer("Race Laps").defaultValue(0).minimum(0).maximum(999))
                .property("weather", SchemaBuilder().object("Weather").additionalProperties(true))
                .property("time_of_day", SchemaBuilder().number("Time of Day").defaultValue(12.0).minimum(0).maximum(24))
                .property("time_multiplier", SchemaBuilder().number("Time Multiplier").defaultValue(1.0).minimum(0).maximum(100))
                .property("required_license", SchemaBuilder().string("Required License").enumValues({"none", "D", "C", "B", "A", "S"}).defaultValue("none"))
                .property("rewards", SchemaBuilder().object("Rewards").additionalProperties(true))
                .build())
            .build())
        .property("championships", SchemaBuilder().array("Championships")
            .items(SchemaBuilder().object("Championship")
                .property("name", SchemaBuilder().string("Name"))
                .property("points_system", SchemaBuilder().object("Points System").additionalProperties(true))
                .property("events", SchemaBuilder().array("Events").items(SchemaBuilder().string("Event ID")))
                .property("dropped_scores", SchemaBuilder().integer("Dropped Scores").defaultValue(0))
                .build())
            .build())
        .property("special_events", SchemaBuilder().array("Special Events")
            .items(SchemaBuilder().object("Special Event")
                .property("name", SchemaBuilder().string("Name"))
                .property("type", SchemaBuilder().string("Type").enumValues({"time_trial", "drift", "drag", "elimination", "hotlap", "checkpoint", "gymkhana"}))
                .property("track", SchemaBuilder().string("Track"))
                .property("layout", SchemaBuilder().string("Layout"))
                .property("target_time", SchemaBuilder().number("Target Time").defaultValue(0))
                .property("car_restrictions", SchemaBuilder().array("Car Restrictions").items(SchemaBuilder().string("Car")))
                .build())
            .build())
        .property("ai_drivers", SchemaBuilder().array("AI Drivers")
            .items(SchemaBuilder().object("AI Driver")
                .property("name", SchemaBuilder().string("Name"))
                .property("nationality", SchemaBuilder().string("Nationality"))
                .property("aggression", SchemaBuilder().number("Aggression").defaultValue(0.5).minimum(0).maximum(1))
                .property("skill", SchemaBuilder().number("Skill").defaultValue(0.5).minimum(0).maximum(1))
                .property("consistency", SchemaBuilder().number("Consistency").defaultValue(0.5).minimum(0).maximum(1))
                .property("mistake_rate", SchemaBuilder().number("Mistake Rate").defaultValue(0.1).minimum(0).maximum(1))
                .property("preferred_cars", SchemaBuilder().array("Preferred Cars").items(SchemaBuilder().string("Car")))
                .build())
            .build())
        .property("unlock_tree", SchemaBuilder().object("Unlock Tree").additionalProperties(true));
    
    auto schema = std::make_shared<JsonSchema>();
    schema->id = "career_config";
    schema->title = "Career Configuration";
    schema->root = builder.build();
    return schema;
}

// ─── SchemaUiGenerator Implementation ────────────────────────────────────

SchemaUiGenerator::SchemaUiGenerator(QObject* parent) : QObject(parent) {}

SchemaUiGenerator::~SchemaUiGenerator() {}

QJsonObject SchemaUiGenerator::generateUi(const std::shared_ptr<JsonSchema>& schema) const {
    QJsonObject ui;
    ui["schema"] = schema->id;
    ui["title"] = schema->title;
    
    if (schema->root) {
        ui["fields"] = generateForm(schema->root);
    }
    
    return ui;
}

QJsonObject SchemaUiGenerator::generateForm(const std::shared_ptr<SchemaProperty>& prop) const {
    QJsonObject form;
    if (!prop) return form;
    
    form["name"] = prop->name;
    form["title"] = prop->title.isEmpty() ? prop->name : prop->title;
    form["type"] = static_cast<int>(prop->type);
    form["description"] = prop->description;
    form["default"] = QJsonValue::fromVariant(prop->defaultValue);
    form["widget"] = prop->widget;
    form["widgetOptions"] = QJsonArray::fromStringList(prop->widgetOptions);
    form["readOnly"] = prop->readOnly;
    form["hidden"] = prop->hidden;
    form["category"] = prop->category;
    form["order"] = prop->order;
    form["placeholder"] = prop->placeholder;
    form["helpText"] = prop->helpText;
    form["visibleIf"] = prop->visibleIf;
    form["enabledIf"] = prop->enabledIf;
    
    // Validation
    QJsonObject validation;
    if (prop->minimum > -std::numeric_limits<double>::infinity()) {
        validation["minimum"] = prop->minimum;
        validation["exclusiveMinimum"] = prop->exclusiveMinimum;
    }
    if (prop->maximum < std::numeric_limits<double>::infinity()) {
        validation["maximum"] = prop->maximum;
        validation["exclusiveMaximum"] = prop->exclusiveMaximum;
    }
    if (prop->multipleOf > 0) validation["multipleOf"] = prop->multipleOf;
    if (prop->minLength > 0) validation["minLength"] = prop->minLength;
    if (prop->maxLength >= 0) validation["maxLength"] = prop->maxLength;
    if (!prop->pattern.isEmpty()) validation["pattern"] = prop->pattern;
    if (!prop->enumValues.isEmpty()) validation["enum"] = QJsonArray::fromStringList(prop->enumValues);
    if (prop->minItems > 0) validation["minItems"] = prop->minItems;
    if (prop->maxItems >= 0) validation["maxItems"] = prop->maxItems;
    if (prop->uniqueItems) validation["uniqueItems"] = true;
    if (prop->minProperties > 0) validation["minProperties"] = prop->minProperties;
    if (prop->maxProperties >= 0) validation["maxProperties"] = prop->maxProperties;
    validation["required"] = QJsonArray::fromStringList(prop->required);
    form["validation"] = validation;
    
    // Children for object/array
    if (prop->type == SchemaType::Object) {
        QJsonArray children;
        for (auto it = prop->properties.constBegin(); it != prop->properties.constEnd(); ++it) {
            children.append(generateForm(it.value()));
        }
        form["children"] = children;
    } else if (prop->type == SchemaType::Array && prop->items) {
        form["itemType"] = generateForm(prop->items);
    }
    
    return form;
}

QWidget* SchemaUiGenerator::createWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const {
    return createWidgetForProperty(prop, parent);
}

QWidget* SchemaUiGenerator::createForm(const std::shared_ptr<JsonSchema>& schema, QWidget* parent) const {
    if (!schema || !schema->root) return nullptr;
    
    QWidget* widget = new QWidget(parent);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);
    
    if (schema->root && schema->root->type == SchemaType::Object) {
        for (auto it = schema->root->properties.constBegin(); it != schema->root->properties.constEnd(); ++it) {
            QWidget* fieldWidget = createWidgetForProperty(it.value(), widget);
            if (fieldWidget) layout->addWidget(fieldWidget);
        }
    }
    
    layout->addStretch();
    return widget;
}

QString SchemaUiGenerator::generateCppParser(const std::shared_ptr<JsonSchema>& schema, const QString& className) const {
    return schema->generateCppStruct();
}

QString SchemaUiGenerator::generateQmlForm(const std::shared_ptr<JsonSchema>& schema) const {
    return schema->generateQmlForm();
}

QString SchemaUiGenerator::generatePythonClass(const std::shared_ptr<JsonSchema>& schema, const QString& className) const {
    if (!schema || !schema->root) return QString();
    
    QString code = QString("class %1:\n    \"\"\"\n    %2\n    \"\"\"\n\n").arg(className).arg(schema->description);
    code += "    def __init__(self):\n";
    
    if (schema->root && schema->root->type == SchemaType::Object) {
        for (auto it = schema->root->properties.constBegin(); it != schema->root->properties.constEnd(); ++it) {
            const QString& key = it.key();
            auto prop = it.value();
            QString memberName = key;
            memberName.replace(QRegularExpression("[^a-zA-Z0-9_]"), "_");
            
            QString defaultVal = "None";
            if (prop->defaultValue.isValid()) {
                if (prop->type == SchemaType::String) defaultVal = QString("\"%1\"").arg(prop->defaultValue.toString());
                else if (prop->type == SchemaType::Number) defaultVal = prop->defaultValue.toString();
                else if (prop->type == SchemaType::Integer) defaultVal = prop->defaultValue.toString();
                else if (prop->type == SchemaType::Boolean) defaultVal = prop->defaultValue.toBool() ? "True" : "False";
                else if (prop->type == SchemaType::Array) defaultVal = "[]";
                else if (prop->type == SchemaType::Object) defaultVal = "{}";
            }
            
            code += QString("        self.%1 = %2\n").arg(memberName).arg(defaultVal);
        }
    }
    
    code += "\n    def to_dict(self):\n        return {\n";
    if (schema->root && schema->root->type == SchemaType::Object) {
        for (auto it = schema->root->properties.constBegin(); it != schema->root->properties.constEnd(); ++it) {
            const QString& key = it.key();
            QString memberName = key;
            memberName.replace(QRegularExpression("[^a-zA-Z0-9_]"), "_");
            code += QString("            '%1': self.%2,\n").arg(key).arg(memberName);
        }
    }
    code += "        }\n";
    
    return code;
}

QJsonObject SchemaUiGenerator::generateValidationRules(const std::shared_ptr<JsonSchema>& schema) const {
    QJsonObject rules;
    if (schema && schema->root) {
        collectValidationRules(schema->root, "", rules);
    }
    return rules;
}

void SchemaUiGenerator::collectValidationRules(const std::shared_ptr<SchemaProperty>& prop, const QString& path, QJsonObject& rules) const {
    if (!prop) return;
    
    QJsonObject propRules;
    if (prop->minimum > -std::numeric_limits<double>::infinity()) {
        propRules["minimum"] = prop->minimum;
        propRules["exclusiveMinimum"] = prop->exclusiveMinimum;
    }
    if (prop->maximum < std::numeric_limits<double>::infinity()) {
        propRules["maximum"] = prop->maximum;
        propRules["exclusiveMaximum"] = prop->exclusiveMaximum;
    }
    if (prop->multipleOf > 0) propRules["multipleOf"] = prop->multipleOf;
    if (prop->minLength > 0) propRules["minLength"] = prop->minLength;
    if (prop->maxLength >= 0) propRules["maxLength"] = prop->maxLength;
    if (!prop->pattern.isEmpty()) propRules["pattern"] = prop->pattern;
    if (!prop->enumValues.isEmpty()) propRules["enum"] = QJsonArray::fromStringList(prop->enumValues);
    if (prop->minItems > 0) propRules["minItems"] = prop->minItems;
    if (prop->maxItems >= 0) propRules["maxItems"] = prop->maxItems;
    if (prop->uniqueItems) propRules["uniqueItems"] = true;
    if (prop->minProperties > 0) propRules["minProperties"] = prop->minProperties;
    if (prop->maxProperties >= 0) propRules["maxProperties"] = prop->maxProperties;
    propRules["required"] = QJsonArray::fromStringList(prop->required);
    
    if (!propRules.isEmpty()) {
        rules[path.isEmpty() ? prop->name : path] = propRules;
    }
    
    if (prop->type == SchemaType::Object) {
        for (auto it = prop->properties.constBegin(); it != prop->properties.constEnd(); ++it) {
            QString childPath = path.isEmpty() ? it.key() : path + "." + it.key();
            collectValidationRules(it.value(), childPath, rules);
        }
    } else if (prop->type == SchemaType::Array && prop->items) {
        collectValidationRules(prop->items, path + "[]", rules);
    }
}

QWidget* SchemaUiGenerator::createWidgetForProperty(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const {
    if (!prop) return nullptr;
    if (prop->hidden) return nullptr;
    
    return createWidgetForType(prop, parent);
}

QWidget* SchemaUiGenerator::createWidgetForType(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const {
    if (!prop) return nullptr;
    
    switch (prop->type) {
        case SchemaType::String:
            return createStringWidget(prop, parent);
        case SchemaType::Number:
        case SchemaType::Integer:
            return createNumberWidget(prop, parent);
        case SchemaType::Boolean:
            return createBooleanWidget(prop, parent);
        case SchemaType::Enum:
            return createEnumWidget(prop, parent);
        case SchemaType::Object:
            return createObjectWidget(prop, parent);
        case SchemaType::Array:
            return createArrayWidget(prop, parent);
        default:
            return nullptr;
    }
}

QWidget* SchemaUiGenerator::createStringWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const {
    QWidget* widget = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* label = new QLabel(prop->title.isEmpty() ? prop->name : prop->title, widget);
    label->setMinimumWidth(150);
    layout->addWidget(label);
    
    if (prop->widget == "file" || prop->format == "file") {
        QLineEdit* edit = new QLineEdit(widget);
        edit->setPlaceholderText(prop->placeholder);
        QPushButton* btn = new QPushButton("Browse...", widget);
        layout->addWidget(edit);
        layout->addWidget(btn);
        // Connect browse button
    } else if (prop->widget == "directory" || prop->format == "directory") {
        QLineEdit* edit = new QLineEdit(widget);
        edit->setPlaceholderText(prop->placeholder);
        QPushButton* btn = new QPushButton("Browse...", widget);
        layout->addWidget(edit);
        layout->addWidget(btn);
    } else if (prop->widget == "color" || prop->format == "color") {
        QLineEdit* edit = new QLineEdit(widget);
        edit->setPlaceholderText("#RRGGBB");
        QPushButton* btn = new QPushButton("Pick...", widget);
        layout->addWidget(edit);
        layout->addWidget(btn);
    } else if (prop->widget == "textarea") {
        QTextEdit* edit = new QTextEdit(widget);
        edit->setPlaceholderText(prop->placeholder);
        layout->addWidget(edit);
    } else if (prop->widget == "code") {
        QPlainTextEdit* edit = new QPlainTextEdit(widget);
        edit->setPlaceholderText(prop->placeholder);
        layout->addWidget(edit);
    } else if (!prop->enumValues.isEmpty()) {
        QComboBox* combo = new QComboBox(widget);
        combo->addItems(prop->enumValues);
        layout->addWidget(combo);
    } else {
        QLineEdit* edit = new QLineEdit(widget);
        edit->setPlaceholderText(prop->placeholder);
        layout->addWidget(edit);
    }
    
    if (!prop->helpText.isEmpty()) {
        widget->setToolTip(prop->helpText);
    }
    
    return widget;
}

QWidget* SchemaUiGenerator::createNumberWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const {
    QWidget* widget = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* label = new QLabel(prop->title.isEmpty() ? prop->name : prop->title, widget);
    label->setMinimumWidth(150);
    layout->addWidget(label);
    
    if (prop->widget == "slider" || (prop->widgetMin != 0 || prop->widgetMax > 0)) {
        QSlider* slider = new QSlider(Qt::Horizontal, widget);
        slider->setMinimum(static_cast<int>(prop->widgetMin));
        slider->setMaximum(static_cast<int>(prop->widgetMax));
        slider->setSingleStep(static_cast<int>(prop->widgetStep));
        layout->addWidget(slider);
        
        QSpinBox* spin = new QSpinBox(widget);
        spin->setMinimum(static_cast<int>(prop->minimum));
        spin->setMaximum(static_cast<int>(prop->maximum));
        spin->setSingleStep(static_cast<int>(prop->widgetStep));
        layout->addWidget(spin);
    } else {
        if (prop->type == SchemaType::Integer) {
            QSpinBox* spin = new QSpinBox(widget);
            spin->setMinimum(static_cast<int>(prop->minimum));
            spin->setMaximum(static_cast<int>(prop->maximum));
            spin->setSingleStep(static_cast<int>(prop->widgetStep > 0 ? prop->widgetStep : 1));
            layout->addWidget(spin);
        } else {
            QDoubleSpinBox* spin = new QDoubleSpinBox(widget);
            spin->setMinimum(prop->minimum);
            spin->setMaximum(prop->maximum);
            spin->setSingleStep(prop->widgetStep > 0 ? prop->widgetStep : 0.01);
            spin->setDecimals(6);
            layout->addWidget(spin);
        }
    }
    
    if (!prop->helpText.isEmpty()) widget->setToolTip(prop->helpText);
    
    return widget;
}

QWidget* SchemaUiGenerator::createBooleanWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const {
    QWidget* widget = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QCheckBox* check = new QCheckBox(prop->title.isEmpty() ? prop->name : prop->title, widget);
    layout->addWidget(check);
    layout->addStretch();
    
    if (!prop->helpText.isEmpty()) widget->setToolTip(prop->helpText);
    
    return widget;
}

QWidget* SchemaUiGenerator::createEnumWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const {
    QWidget* widget = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* label = new QLabel(prop->title.isEmpty() ? prop->name : prop->title, widget);
    label->setMinimumWidth(150);
    layout->addWidget(label);
    
    QComboBox* combo = new QComboBox(widget);
    combo->addItems(prop->enumValues);
    layout->addWidget(combo);
    layout->addStretch();
    
    if (!prop->helpText.isEmpty()) widget->setToolTip(prop->helpText);
    
    return widget;
}

QWidget* SchemaUiGenerator::createObjectWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const {
    QGroupBox* group = new QGroupBox(prop->title.isEmpty() ? prop->name : prop->title, parent);
    QVBoxLayout* layout = new QVBoxLayout(group);
    layout->setSpacing(8);
    layout->setContentsMargins(10, 20, 10, 10);
    
    if (prop->properties.size() > 0) {
        for (auto it = prop->properties.constBegin(); it != prop->properties.constEnd(); ++it) {
            QWidget* child = createWidgetForProperty(it.value(), group);
            if (child) layout->addWidget(child);
        }
    }
    
    if (!prop->helpText.isEmpty()) group->setToolTip(prop->helpText);
    
    return group;
}

QWidget* SchemaUiGenerator::createArrayWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const {
    QWidget* widget = new QWidget(parent);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    
    QLabel* label = new QLabel(prop->title.isEmpty() ? prop->name : prop->title, widget);
    layout->addWidget(label);
    
    QPushButton* addBtn = new QPushButton("Add Item", widget);
    layout->addWidget(addBtn);
    
    QListWidget* list = new QListWidget(widget);
    layout->addWidget(list);
    
    if (!prop->helpText.isEmpty()) widget->setToolTip(prop->helpText);
    
    return widget;
}

QString SchemaUiGenerator::generateQmlField(const QString& name, const std::shared_ptr<SchemaProperty>& prop) const {
    QString field;
    QString displayName = prop->title.isEmpty() ? name : prop->title;
    QString widgetType = prop->widget.isEmpty() ? "text" : prop->widget;
    
    field += QString("        Label { text: \"%1\" Layout.fillWidth: true }\n").arg(displayName);
    
    if (widgetType == "checkbox" || prop->type == SchemaType::Boolean) {
        field += QString("        CheckBox { id: %1Check; checked: formData.%2 }\n").arg(name).arg(name);
    } else if (widgetType == "select" || !prop->enumValues.isEmpty()) {
        field += QString("        ComboBox { id: %1Combo; model: %2; currentText: formData.%3 }\n")
            .arg(name)
            .arg(prop->enumValues.isEmpty() ? "[]" : QString("[%1]").arg(prop->enumValues.join("\", \"")))
            .arg(name);
    } else if (widgetType == "color") {
        field += QString("        ColorPicker { id: %1Color; color: formData.%2 }\n").arg(name).arg(name);
    } else if (widgetType == "slider" || prop->widgetMin != 0 || prop->widgetMax > 0) {
        field += QString("        Slider { id: %1Slider; from: %2; to: %3; value: formData.%4; stepSize: %5 }\n")
            .arg(name).arg(prop->widgetMin).arg(prop->widgetMax).arg(name).arg(prop->widgetStep);
    } else if (widgetType == "code" || prop->format == "code") {
        field += QString("        TextArea { id: %1Code; text: formData.%2; wrapMode: TextArea.Wrap }\n").arg(name).arg(name);
    } else {
        field += QString("        TextField { id: %1Field; text: formData.%2; placeholderText: \"%3\" }\n")
            .arg(name).arg(name).arg(prop->placeholder);
    }
    
    field += "\n";
    return field;
}

// ─── ConfigValidator Implementation ──────────────────────────────────────

ConfigValidator::ConfigValidator(QObject* parent) : QObject(parent) {}

ConfigValidator::~ConfigValidator() {}

ConfigValidator::ValidationResult ConfigValidator::validate(const QJsonObject& data, const std::shared_ptr<JsonSchema>& schema) const {
    ValidationResult result;
    if (schema) {
        if (!schema->validate(data, &result.errors)) {
            result.valid = false;
        }
    }
    return result;
}

ConfigValidator::ValidationResult ConfigValidator::validateFile(const QString& filePath, const QString& configType) const {
    ValidationResult result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.valid = false;
        result.errors.append("Cannot open file: " + filePath);
        return result;
    }
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    
    if (parseError.error != QJsonParseError::NoError) {
        result.valid = false;
        result.errors.append("JSON parse error: " + parseError.errorString());
        return result;
    }
    
    auto schema = ConfigSchemaRegistry::instance()->getSchema(configType);
    return validate(doc.object(), schema);
}

ConfigValidator::ValidationResult ConfigValidator::validateCrossReferences(const QJsonObject& data, const QString& configType) const {
    ValidationResult result;
    // Implement cross-reference validation
    // e.g., check that referenced cars/tracks exist
    return result;
}

void ConfigValidator::addCustomValidator(const QString& configType, CustomValidator validator) {
    CustomValidatorEntry entry;
    entry.configType = configType;
    entry.validator = validator;
    m_customValidators.append(entry);
}

QJsonObject ConfigValidator::repair(const QJsonObject& data, const std::shared_ptr<JsonSchema>& schema) const {
    if (schema) {
        return schema->applyDefaults(data);
    }
    return data;
}

bool ConfigValidator::repairFile(const QString& filePath, const QString& configType) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    
    if (parseError.error != QJsonParseError::NoError) return false;
    
    auto schema = ConfigSchemaRegistry::instance()->getSchema(configType);
    QJsonObject repaired = repair(doc.object(), schema);
    
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument outDoc(repaired);
        file.write(outDoc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }
    return false;
}

ConfigValidator::ValidationResult ConfigValidator::validateProperty(const QString& path, const QJsonValue& value, const std::shared_ptr<SchemaProperty>& schema) const {
    ValidationResult result;
    if (schema) {
        QStringList errors;
        if (!schema->validate(value, &errors)) {
            result.valid = false;
            for (const QString& err : errors) {
                result.errors.append(path.isEmpty() ? err : path + ": " + err);
            }
        }
    }
    return result;
}

void ConfigValidator::addError(ValidationResult& result, const QString& path, const QString& message) const {
    result.valid = false;
    result.errors.append(path.isEmpty() ? message : path + ": " + message);
}

void ConfigValidator::addWarning(ValidationResult& result, const QString& path, const QString& message) const {
    result.warnings.append(path.isEmpty() ? message : path + ": " + message);
}

// ─── SchemaConfigManager Implementation ──────────────────────────────────

SchemaConfigManager::SchemaConfigManager(QObject* parent) : QObject(parent) {}

SchemaConfigManager::~SchemaConfigManager() {
    shutdown();
}

bool SchemaConfigManager::initialize(const QString& configRoot) {
    m_configRoot = configRoot;
    QDir dir(configRoot);
    if (!dir.exists()) dir.mkpath(".");
    
    loadAllConfigs();
    watchConfigFiles();
    return true;
}

void SchemaConfigManager::shutdown() {
    if (m_watcher) {
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }
    m_configs.clear();
    m_schemas.clear();
}

void SchemaConfigManager::loadAllConfigs() {
    QDir dir(m_configRoot);
    dir.setNameFilters({"*.json", "*.ini", "*.cfg"});
    dir.setFilter(QDir::Files);
    
    for (const QFileInfo& fileInfo : dir.entryInfoList()) {
        loadConfigFromFile(fileInfo.absoluteFilePath());
    }
}

void SchemaConfigManager::loadConfigFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    
    if (error.error != QJsonParseError::NoError) return;
    
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString configId = obj.value("id").toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
        QString type = obj.value("type").toString("unknown");
        
        SchemaConfigManager::ConfigEntry entry;
        entry.id = configId;
        entry.type = type;
        entry.name = obj.value("name").toString(QFileInfo(filePath).baseName());
        entry.filePath = filePath;
        entry.data = obj;
        entry.lastModified = QFileInfo(filePath).lastModified();
        entry.schemaVersion = obj.value("schema_version").toString("1.0");
        
        m_configs[configId] = entry;
    }
}

bool SchemaConfigManager::loadConfig(const QString& configId) {
    if (!m_configs.contains(configId)) return false;
    emit configLoaded(configId);
    return true;
}

bool SchemaConfigManager::saveConfig(const QString& configId) {
    if (!m_configs.contains(configId)) return false;
    
    SchemaConfigManager::ConfigEntry& entry = m_configs[configId];
    QFile file(entry.filePath);
    if (file.open(QIODevice::WriteOnly)) {
        entry.data["id"] = entry.id;
        entry.data["type"] = entry.type;
        entry.data["name"] = entry.name;
        entry.data["schema_version"] = entry.schemaVersion;
        
        QJsonDocument doc(entry.data);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        
        entry.modified = false;
        entry.lastModified = QDateTime::currentDateTime();
        emit configSaved(configId);
        return true;
    }
    return false;
}

bool SchemaConfigManager::createConfig(const QString& configId, const QString& type, const QString& name) {
    if (m_configs.contains(configId)) return false;
    
    auto schema = getSchema(type);
    if (!schema) return false;
    
    SchemaConfigManager::ConfigEntry entry;
    entry.id = configId;
    entry.type = type;
    entry.name = name;
    entry.filePath = m_configRoot + "/" + configId + ".json";
    entry.data = schema->applyDefaults(QJsonObject());
    entry.data["id"] = configId;
    entry.data["type"] = type;
    entry.data["name"] = name;
    entry.data["schema_version"] = "1.0";
    entry.created = QDateTime::currentDateTime();
    entry.modified = true;
    
    m_configs[configId] = entry;
    emit configCreated(configId);
    return true;
}

bool SchemaConfigManager::deleteConfig(const QString& configId) {
    if (!m_configs.contains(configId)) return false;
    
    SchemaConfigManager::ConfigEntry entry = m_configs[configId];
    QFile::remove(entry.filePath);
    m_configs.remove(configId);
    emit configDeleted(configId);
    return true;
}

bool SchemaConfigManager::duplicateConfig(const QString& configId, const QString& newName) {
    if (!m_configs.contains(configId)) return false;
    
    QString newId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    SchemaConfigManager::ConfigEntry entry = m_configs[configId];
    entry.id = newId;
    entry.name = newName;
    entry.filePath = m_configRoot + "/" + newId + ".json";
    entry.created = QDateTime::currentDateTime();
    entry.modified = true;
    
    m_configs[newId] = entry;
    emit configCreated(newId);
    return true;
}

QJsonObject SchemaConfigManager::getConfig(const QString& configId) const {
    return m_configs.value(configId).data;
}

bool SchemaConfigManager::setConfig(const QString& configId, const QJsonObject& data) {
    if (!m_configs.contains(configId)) return false;
    
    m_configs[configId].data = data;
    m_configs[configId].modified = true;
    emit configChanged(configId, "");
    return true;
}

QJsonValue SchemaConfigManager::getValue(const QString& configId, const QString& path) const {
    if (!m_configs.contains(configId)) return QJsonValue();
    
    QJsonObject obj = m_configs[configId].data;
    QStringList parts = path.split(".");
    QJsonValue val = obj;
    
    for (const QString& part : parts) {
        if (val.isObject()) {
            val = val.toObject().value(part);
        } else if (val.isArray()) {
            bool ok;
            int idx = part.toInt(&ok);
            if (ok) val = val.toArray()[idx];
            else return QJsonValue();
        } else {
            return QJsonValue();
        }
    }
    
    return val;
}

bool SchemaConfigManager::setValue(const QString& configId, const QString& path, const QJsonValue& value) {
    if (!m_configs.contains(configId)) return false;
    
    SchemaConfigManager::ConfigEntry& entry = m_configs[configId];
    QJsonObject obj = entry.data;
    QStringList parts = path.split(".");
    
    struct SetHelper {
        static void set(QJsonObject& node, const QStringList& parts, int idx, const QJsonValue& value) {
            if (idx == parts.size() - 1) {
                node[parts[idx]] = value;
                return;
            }
            if (!node.contains(parts[idx]) || !node[parts[idx]].isObject()) {
                node[parts[idx]] = QJsonObject();
            }
            QJsonObject child = node[parts[idx]].toObject();
            set(child, parts, idx + 1, value);
            node[parts[idx]] = child;
        }
    };
    
    SetHelper::set(obj, parts, 0, value);
    entry.data = obj;
    entry.modified = true;
    emit configChanged(configId, path);
    return true;
}

ConfigValidator::ValidationResult SchemaConfigManager::validate(const QString& configId) const {
    if (!m_configs.contains(configId)) {
        ConfigValidator::ValidationResult result;
        result.valid = false;
        result.errors.append("Config not found: " + configId);
        return result;
    }
    
    const SchemaConfigManager::ConfigEntry& entry = m_configs[configId];
    auto schema = getSchema(entry.type);
    return ConfigValidator().validate(entry.data, schema);
}

bool SchemaConfigManager::validateAndSave(const QString& configId) {
    auto result = validate(configId);
    if (result.valid) {
        return saveConfig(configId);
    }
    return false;
}

void SchemaConfigManager::registerSchema(const QString& type, std::shared_ptr<JsonSchema> schema) {
    m_schemas[type] = schema;
}

std::shared_ptr<JsonSchema> SchemaConfigManager::getSchema(const QString& type) const {
    auto it = m_schemas.find(type);
    if (it != m_schemas.end()) return it.value();
    return ConfigSchemaRegistry::instance()->getSchema(type);
}

bool SchemaConfigManager::importConfig(const QString& filePath, QString* newConfigId) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    
    if (error.error != QJsonParseError::NoError) return false;
    
    if (doc.isObject()) {
        QString configId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QJsonObject obj = doc.object();
        obj["id"] = configId;
        
        QString type = obj.value("type").toString("unknown");
        QString name = obj.value("name").toString(QFileInfo(filePath).baseName());
        
        SchemaConfigManager::ConfigEntry entry;
        entry.id = configId;
        entry.type = type;
        entry.name = name;
        entry.filePath = m_configRoot + "/" + configId + ".json";
        entry.data = obj;
        entry.created = QDateTime::currentDateTime();
        entry.modified = true;
        
        m_configs[configId] = entry;
        
        if (newConfigId) *newConfigId = configId;
        emit configCreated(configId);
        return true;
    }
    return false;
}

bool SchemaConfigManager::exportConfig(const QString& configId, const QString& filePath) const {
    if (!m_configs.contains(configId)) return false;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(m_configs[configId].data);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }
    return false;
}

bool SchemaConfigManager::exportAll(const QString& directory) const {
    QDir dir(directory);
    if (!dir.exists()) dir.mkpath(".");
    
    for (auto it = m_configs.constBegin(); it != m_configs.constEnd(); ++it) {
        QString filePath = dir.filePath(it.key() + ".json");
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(it.value().data);
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
        }
    }
    return true;
}

bool SchemaConfigManager::migrateConfig(const QString& configId, const QString& targetVersion) {
    // Implement version migration logic
    return true;
}

QStringList SchemaConfigManager::availableMigrations(const QString& configType) const {
    return {"1.0", "1.1", "2.0"};
}

QStringList SchemaConfigManager::listConfigs(const QString& type) const {
    QStringList result;
    for (auto it = m_configs.constBegin(); it != m_configs.constEnd(); ++it) {
        if (type.isEmpty() || it.value().type == type) {
            result.append(it.key());
        }
    }
    return result;
}

SchemaConfigManager::ConfigEntry SchemaConfigManager::getConfigInfo(const QString& configId) const {
    return m_configs.value(configId);
}

QStringList SchemaConfigManager::searchConfigs(const QString& query) const {
    QStringList result;
    QString lowerQuery = query.toLower();
    for (auto it = m_configs.constBegin(); it != m_configs.constEnd(); ++it) {
        if (it.value().name.toLower().contains(lowerQuery) ||
            it.value().type.toLower().contains(lowerQuery) ||
            it.key().contains(lowerQuery)) {
            result.append(it.key());
        }
    }
    return result;
}

void SchemaConfigManager::watchConfigFiles() {
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &SchemaConfigManager::onFileChanged);
    
    QStringList files;
    for (auto it = m_configs.constBegin(); it != m_configs.constEnd(); ++it) {
        files.append(it.value().filePath);
    }
    if (!files.isEmpty()) m_watcher->addPaths(files);
}

void SchemaConfigManager::onFileChanged(const QString& path) {
    // Reload config
    loadConfigFromFile(path);
    emit configChanged(QFileInfo(path).baseName(), "");
}

QString SchemaConfigManager::generateConfigId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString SchemaConfigManager::configFilePath(const QString& configId) const {
    return m_configRoot + "/" + configId + ".json";
}

void SchemaConfigManager::updateModifiedTime(const QString& configId) {
    if (m_configs.contains(configId)) {
        m_configs[configId].lastModified = QDateTime::currentDateTime();
        m_configs[configId].modified = true;
    }
}

} // namespace config
} // namespace ks