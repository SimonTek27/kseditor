#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QFileSystemWatcher>
#include <QVariant>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QUrl>
#include <QRegularExpression>

namespace ks {
namespace config {

// ─── JSON Schema Definition ────────────────────────────────────────────────

enum class SchemaType {
    Object,
    Array,
    String,
    Number,
    Integer,
    Boolean,
    Null,
    Enum,
    Reference
};

struct SchemaProperty {
    QString name;
    SchemaType type = SchemaType::String;
    QString title;
    QString description;
    QString format;  // "email", "uri", "date", "color", "file", "directory", etc.
    
    // String constraints
    QString pattern;
    int minLength = 0;
    int maxLength = -1;
    QStringList enumValues;
    
    // Number constraints
    double minimum = -std::numeric_limits<double>::infinity();
    double maximum = std::numeric_limits<double>::infinity();
    bool exclusiveMinimum = false;
    bool exclusiveMaximum = false;
    double multipleOf = 0;
    
    // Array constraints
    int minItems = 0;
    int maxItems = -1;
    bool uniqueItems = false;
    std::shared_ptr<SchemaProperty> items;  // For array of objects
    
    // Object constraints
    int minProperties = 0;
    int maxProperties = -1;
    QStringList required;
    QMap<QString, std::shared_ptr<SchemaProperty>> properties;
    std::shared_ptr<SchemaProperty> additionalProperties;
    
    // Validation
    bool validate(const QJsonValue& value, QStringList* errors = nullptr) const;
    
    // Reference
    QString ref;  // JSON reference
    
    // Default value
    QVariant defaultValue;
    
    // UI hints
    QString widget;  // "text", "textarea", "select", "checkbox", "color", "slider", "file", "directory", "code"
    QStringList widgetOptions;
    int widgetMin = 0;
    int widgetMax = 100;
    double widgetStep = 1.0;
    bool readOnly = false;
    bool hidden = false;
    QString placeholder;
    QString helpText;
    QString category;
    int order = 0;
    
    // Conditional visibility
    QString visibleIf;  // Expression like "otherProperty == 'value'"
    QString enabledIf;
};

struct JsonSchema {
    QString id;
    QString schema = "http://json-schema.org/draft-07/schema#";
    QString title;
    QString description;
    QString version;
    std::shared_ptr<SchemaProperty> root;
    QMap<QString, std::shared_ptr<SchemaProperty>> definitions;
    
    // Validation
    bool validate(const QJsonObject& data, QStringList* errors = nullptr) const;
    QJsonObject applyDefaults(const QJsonObject& data) const;
    
    // Code generation
    QString generateCppStruct() const;
    QString generateQmlForm() const;

private:
    void applyDefaultsRecursive(QJsonObject& obj, const std::shared_ptr<SchemaProperty>& schema) const;
    QString generateCppStructRecursive(const std::shared_ptr<SchemaProperty>& schema, const QString& name) const;
    QString generateQmlField(const QString& name, const std::shared_ptr<SchemaProperty>& prop) const;
};

// ─── Schema Builder ────────────────────────────────────────────────────────

class SchemaBuilder {
public:
    SchemaBuilder();
    
    // Object types
    SchemaBuilder& object(const QString& title = QString());
    SchemaBuilder& required(const QStringList& fields);
    SchemaBuilder& property(const QString& name, std::shared_ptr<SchemaProperty> prop);
    SchemaBuilder& property(const QString& name, SchemaBuilder& builder);
    SchemaBuilder& additionalProperties(bool allowed);
    SchemaBuilder& additionalProperties(std::shared_ptr<SchemaProperty> schema);
    
    // Array types
    SchemaBuilder& array(const QString& title = QString());
    SchemaBuilder& items(std::shared_ptr<SchemaProperty> itemSchema);
    SchemaBuilder& items(SchemaBuilder& builder);
    SchemaBuilder& minItems(int n);
    SchemaBuilder& maxItems(int n);
    SchemaBuilder& uniqueItems(bool unique);
    
    // String types
    SchemaBuilder& string(const QString& title = QString());
    SchemaBuilder& pattern(const QString& regex);
    SchemaBuilder& minLength(int n);
    SchemaBuilder& maxLength(int n);
    SchemaBuilder& enumValues(const QStringList& values);
    SchemaBuilder& format(const QString& fmt);
    
    // Number types
    SchemaBuilder& number(const QString& title = QString());
    SchemaBuilder& integer(const QString& title = QString());
    SchemaBuilder& minimum(double n, bool exclusive = false);
    SchemaBuilder& maximum(double n, bool exclusive = false);
    SchemaBuilder& multipleOf(double n);
    
    // Boolean
    SchemaBuilder& boolean(const QString& title = QString());
    
    // Enum
    SchemaBuilder& enumType(const QStringList& values);
    
    // Common modifiers
    SchemaBuilder& title(const QString& t);
    SchemaBuilder& description(const QString& d);
    SchemaBuilder& defaultValue(const QVariant& value);
    SchemaBuilder& widget(const QString& w, const QStringList& options = {});
    SchemaBuilder& readOnly(bool ro = true);
    SchemaBuilder& hidden(bool h = true);
    SchemaBuilder& category(const QString& cat);
    SchemaBuilder& order(int o);
    SchemaBuilder& placeholder(const QString& p);
    SchemaBuilder& helpText(const QString& text);
    SchemaBuilder& visibleIf(const QString& expr);
    SchemaBuilder& enabledIf(const QString& expr);
    SchemaBuilder& ref(const QString& reference);
    
    std::shared_ptr<SchemaProperty> build();
    SchemaBuilder& reset();
    
    // Presets
    static std::shared_ptr<SchemaProperty> filePicker(const QString& title = QString(), const QString& filter = QString());
    static std::shared_ptr<SchemaProperty> directoryPicker(const QString& title = QString());
    static std::shared_ptr<SchemaProperty> colorPicker(const QString& title = QString());
    static std::shared_ptr<SchemaProperty> slider(const QString& title, double min, double max, double step = 1.0);
    static std::shared_ptr<SchemaProperty> dropdown(const QString& title, const QStringList& options);
    static std::shared_ptr<SchemaProperty> codeEditor(const QString& title, const QString& language = "javascript");
    static std::shared_ptr<SchemaProperty> textArea(const QString& title = QString());
    static std::shared_ptr<SchemaProperty> checkbox(const QString& title = QString());

private:
    std::shared_ptr<SchemaProperty> m_current;
    QMap<QString, std::shared_ptr<SchemaProperty>> m_properties;
    QStringList m_required;
    bool m_additionalProperties = true;
    std::shared_ptr<SchemaProperty> m_additionalSchema;
};

// ─── Config Schema Registry ────────────────────────────────────────────────

class ConfigSchemaRegistry : public QObject {
    Q_OBJECT

public:
    static ConfigSchemaRegistry* instance();
    
    explicit ConfigSchemaRegistry(QObject* parent = nullptr);
    ~ConfigSchemaRegistry() override;

    // Registration
    void registerSchema(const QString& configType, std::shared_ptr<JsonSchema> schema);
    void unregisterSchema(const QString& configType);
    std::shared_ptr<JsonSchema> getSchema(const QString& configType) const;
    QStringList registeredTypes() const;

    // Built-in schemas
    void registerBuiltinSchemas();
    
    // AC/CSP specific schemas
    static std::shared_ptr<JsonSchema> cspGlobalConfigSchema();
    static std::shared_ptr<JsonSchema> cspCarConfigSchema();
    static std::shared_ptr<JsonSchema> cspTrackConfigSchema();
    static std::shared_ptr<JsonSchema> ppFilterSchema();
    static std::shared_ptr<JsonSchema> weatherConfigSchema();
    static std::shared_ptr<JsonSchema> serverConfigSchema();
    static std::shared_ptr<JsonSchema> careerConfigSchema();

signals:
    void schemaRegistered(const QString& configType);
    void schemaUnregistered(const QString& configType);

private:
    QMap<QString, std::shared_ptr<JsonSchema>> m_schemas;
};

// ─── Schema-Driven UI Generator ────────────────────────────────────────────

class SchemaUiGenerator : public QObject {
    Q_OBJECT

public:
    struct UiElement {
        QString type;  // "widget", "layout", "group", "tab", "scroll"
        QString objectName;
        QString propertyName;
        QJsonObject properties;
        QVector<UiElement> children;
    };

    explicit SchemaUiGenerator(QObject* parent = nullptr);
    ~SchemaUiGenerator() override;

    // Generate UI from schema
    QJsonObject generateUi(const std::shared_ptr<JsonSchema>& schema) const;
    QJsonObject generateForm(const std::shared_ptr<SchemaProperty>& prop) const;
    QWidget* createWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent = nullptr) const;
    QWidget* createForm(const std::shared_ptr<JsonSchema>& schema, QWidget* parent = nullptr) const;
    
    // Code generation
    QString generateCppParser(const std::shared_ptr<JsonSchema>& schema, const QString& className) const;
    QString generateQmlForm(const std::shared_ptr<JsonSchema>& schema) const;
    QString generatePythonClass(const std::shared_ptr<JsonSchema>& schema, const QString& className) const;

    // Validation UI
    QJsonObject generateValidationRules(const std::shared_ptr<JsonSchema>& schema) const;

signals:
    void widgetCreated(QWidget* widget, const QString& propertyName);
    void validationError(const QString& property, const QString& error);

private:
    QWidget* createWidgetForProperty(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const;
    QWidget* createWidgetForType(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const;
    void setupWidgetProperties(QWidget* widget, const std::shared_ptr<SchemaProperty>& prop) const;
    
    // Widget factories
    QWidget* createStringWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const;
    QWidget* createNumberWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const;
    QWidget* createBooleanWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const;
    QWidget* createEnumWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const;
    QWidget* createFileWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const;
    QWidget* createColorWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const;
    QWidget* createObjectWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const;
    QWidget* createArrayWidget(const std::shared_ptr<SchemaProperty>& prop, QWidget* parent) const;
    void collectValidationRules(const std::shared_ptr<SchemaProperty>& prop, const QString& path, QJsonObject& rules) const;
    QString generateQmlField(const QString& name, const std::shared_ptr<SchemaProperty>& prop) const;
};

// ─── Config Validator ──────────────────────────────────────────────────────

class ConfigValidator : public QObject {
    Q_OBJECT

public:
    struct ValidationResult {
        bool valid = true;
        QVector<QString> errors;
        QVector<QString> warnings;
        QVector<QString> infos;
        
        void addError(const QString& path, const QString& message);
        void addWarning(const QString& path, const QString& message);
        void addInfo(const QString& path, const QString& message);
        QString toString() const;
    };

    explicit ConfigValidator(QObject* parent = nullptr);
    ~ConfigValidator() override;

    // Validate against schema
    ValidationResult validate(const QJsonObject& data, const std::shared_ptr<JsonSchema>& schema) const;
    ValidationResult validateFile(const QString& filePath, const QString& configType) const;
    
    // Cross-reference validation
    ValidationResult validateCrossReferences(const QJsonObject& data, const QString& configType) const;
    
    // Custom validators
    using CustomValidator = std::function<ValidationResult(const QJsonObject&)>;
    void addCustomValidator(const QString& configType, CustomValidator validator);
    
    // Repair
    QJsonObject repair(const QJsonObject& data, const std::shared_ptr<JsonSchema>& schema) const;
    bool repairFile(const QString& filePath, const QString& configType) const;

signals:
    void validationStarted(const QString& filePath);
    void validationFinished(const QString& filePath, bool valid);

private:
    struct CustomValidatorEntry {
        QString configType;
        CustomValidator validator;
    };
    QVector<CustomValidatorEntry> m_customValidators;
    
    ValidationResult validateProperty(const QString& path, const QJsonValue& value, 
                                      const std::shared_ptr<SchemaProperty>& schema) const;
    void addError(ValidationResult& result, const QString& path, const QString& message) const;
    void addWarning(ValidationResult& result, const QString& path, const QString& message) const;
};

// ─── Config Manager with Schema Support ────────────────────────────────────

class SchemaConfigManager : public QObject {
    Q_OBJECT

public:
    struct ConfigEntry {
        QString id;
        QString type;
        QString name;
        QString filePath;
        QJsonObject data;
        QDateTime lastModified;
        QDateTime created;
        bool modified = false;
        QString schemaVersion;
    };

    explicit SchemaConfigManager(QObject* parent = nullptr);
    ~SchemaConfigManager() override;

    // Lifecycle
    bool initialize(const QString& configRoot);
    void shutdown();

    // Config operations
    bool loadConfig(const QString& configId);
    bool saveConfig(const QString& configId);
    bool createConfig(const QString& configId, const QString& type, const QString& name);
    bool deleteConfig(const QString& configId);
    bool duplicateConfig(const QString& configId, const QString& newName);

    // Data access
    QJsonObject getConfig(const QString& configId) const;
    bool setConfig(const QString& configId, const QJsonObject& data);
    QJsonValue getValue(const QString& configId, const QString& path) const;
    bool setValue(const QString& configId, const QString& path, const QJsonValue& value);

    // Validation
    ConfigValidator::ValidationResult validate(const QString& configId) const;
    bool validateAndSave(const QString& configId);

    // Schema management
    void registerSchema(const QString& type, std::shared_ptr<JsonSchema> schema);
    std::shared_ptr<JsonSchema> getSchema(const QString& type) const;

    // Import/Export
    bool importConfig(const QString& filePath, QString* newConfigId = nullptr);
    bool exportConfig(const QString& configId, const QString& filePath) const;
    bool exportAll(const QString& directory) const;

    // Versioning
    bool migrateConfig(const QString& configId, const QString& targetVersion);
    QStringList availableMigrations(const QString& configType) const;

    // Query
    QStringList listConfigs(const QString& type = QString()) const;
    ConfigEntry getConfigInfo(const QString& configId) const;
    QStringList searchConfigs(const QString& query) const;

signals:
    void configLoaded(const QString& configId);
    void configSaved(const QString& configId);
    void configCreated(const QString& configId);
    void configChanged(const QString& configId, const QString& path);
    void configDeleted(const QString& configId);
    void validationFailed(const QString& configId, const QString& errors);

private:
    void loadAllConfigs();
    void loadConfigFromFile(const QString& filePath);
    void watchConfigFiles();
    void onFileChanged(const QString& path);
    QString generateConfigId() const;
    QString configFilePath(const QString& configId) const;
    void updateModifiedTime(const QString& configId);

    QString m_configRoot;
    QMap<QString, ConfigEntry> m_configs;
    QMap<QString, std::shared_ptr<JsonSchema>> m_schemas;
    QFileSystemWatcher* m_watcher = nullptr;
};

} // namespace config
} // namespace ks