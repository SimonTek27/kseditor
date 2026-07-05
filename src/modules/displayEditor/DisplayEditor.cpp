#include "DisplayEditor.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSettings>
#include <QUuid>
#include <QImage>
#include <QPainter>
#include <QFontDatabase>

ksDisplayEditor::ksDisplayEditor(QObject *parent)
    : QObject(parent), m_displaySize(800, 480), m_backgroundColor(0, 0, 0)
{
    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(16); // ~60 FPS
    connect(m_animationTimer, &QTimer::timeout, this, &ksDisplayEditor::onAnimationTick);

    // Initialize physics values
    m_physicsValues[DataSource::SPEED] = 0.0;
    m_physicsValues[DataSource::RPM] = 0.0;
    m_physicsValues[DataSource::GEAR] = 0.0;
    m_physicsValues[DataSource::LAP_TIME] = 0.0;
    m_physicsValues[DataSource::FUEL] = 100.0;
    m_physicsValues[DataSource::TIRE_TEMP] = 25.0;
    m_physicsValues[DataSource::STEERING_ANGLE] = 0.0;
    m_physicsValues[DataSource::BRAKE_BIAS] = 60.0;
    m_physicsValues[DataSource::TIRE_PRESSURE] = 32.0;
    m_physicsValues[DataSource::SUSPENSION_TRAVEL] = 0.0;
}

ksDisplayEditor::~ksDisplayEditor()
{
}

// ============================================================================
// File operations
// ============================================================================

bool ksDisplayEditor::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QString content = file.readAll();
    file.close();

    if (filePath.endsWith(".lua", Qt::CaseInsensitive)) {
        return parseLuaFile(content);
    } else if (filePath.endsWith(".ini", Qt::CaseInsensitive)) {
        return parseIniFile(content);
    } else if (filePath.endsWith(".json", Qt::CaseInsensitive)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) return false;
        return loadFromJson(doc.object());
    }

    return false;
}

bool ksDisplayEditor::saveToFile(const QString& filePath)
{
    if (filePath.endsWith(".lua", Qt::CaseInsensitive)) {
        return saveToLua(filePath);
    } else if (filePath.endsWith(".ini", Qt::CaseInsensitive)) {
        return exportToIni(filePath);
    } else if (filePath.endsWith(".json", Qt::CaseInsensitive)) {
        return exportToJson(filePath);
    }

    return false;
}

bool ksDisplayEditor::loadFromLua(const QString& luaFilePath)
{
    QFile file(luaFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QString content = file.readAll();
    file.close();

    return parseLuaFile(content);
}

bool ksDisplayEditor::saveToLua(const QString& luaFilePath)
{
    QFile file(luaFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << generateLuaScript();
    file.close();
    return true;
}

// ============================================================================
// Element management
// ============================================================================

void ksDisplayEditor::addElement(const DisplayElement& element)
{
    DisplayElement elem = element;
    if (elem.id.isEmpty()) {
        elem.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    }
    m_elements.append(elem);
    emit elementAdded(elem.id);
    emit displayChanged();
}

void ksDisplayEditor::removeElement(const QString& elementId)
{
    for (int i = 0; i < m_elements.size(); ++i) {
        if (m_elements[i].id == elementId) {
            m_elements.removeAt(i);
            emit elementRemoved(elementId);
            emit displayChanged();
            return;
        }
    }
}

void ksDisplayEditor::updateElement(const QString& elementId, const DisplayElement& element)
{
    for (int i = 0; i < m_elements.size(); ++i) {
        if (m_elements[i].id == elementId) {
            m_elements[i] = element;
            m_elements[i].id = elementId;
            emit elementUpdated(elementId);
            emit displayChanged();
            return;
        }
    }
}

DisplayElement* ksDisplayEditor::getElement(const QString& elementId)
{
    for (auto& elem : m_elements) {
        if (elem.id == elementId) return &elem;
    }
    return nullptr;
}

QVector<DisplayElement>& ksDisplayEditor::getAllElements()
{
    return m_elements;
}

void ksDisplayEditor::clearElements()
{
    m_elements.clear();
    emit displayChanged();
}

// ============================================================================
// Display settings
// ============================================================================

void ksDisplayEditor::setDisplayName(const QString& name)
{
    m_displayName = name;
    emit displayChanged();
}

QString ksDisplayEditor::getDisplayName() const
{
    return m_displayName;
}

void ksDisplayEditor::setDisplaySize(const QSize& size)
{
    m_displaySize = size;
    emit displayChanged();
}

QSize ksDisplayEditor::getDisplaySize() const
{
    return m_displaySize;
}

void ksDisplayEditor::setBackgroundImage(const QString& imagePath)
{
    m_backgroundImage = imagePath;
    emit displayChanged();
}

QString ksDisplayEditor::getBackgroundImage() const
{
    return m_backgroundImage;
}

void ksDisplayEditor::setBackgroundColor(const QColor& color)
{
    m_backgroundColor = color;
    emit displayChanged();
}

QColor ksDisplayEditor::getBackgroundColor() const
{
    return m_backgroundColor;
}

// ============================================================================
// Validation
// ============================================================================

bool ksDisplayEditor::validateConfig()
{
    m_errors.clear();

    if (m_displaySize.width() <= 0 || m_displaySize.height() <= 0) {
        m_errors.append("Display size must be positive");
    }

    for (const auto& elem : m_elements) {
        if (elem.id.isEmpty()) {
            m_errors.append("Element has empty ID");
        }
        if (elem.position.x() < 0 || elem.position.y() < 0) {
            m_errors.append(QString("Element '%1' has negative position").arg(elem.id));
        }
        if (elem.size.width() <= 0 || elem.size.height() <= 0) {
            m_errors.append(QString("Element '%1' has invalid size").arg(elem.id));
        }
        if (elem.fontSize <= 0) {
            m_errors.append(QString("Element '%1' has invalid font size").arg(elem.id));
        }
        if (elem.updateInterval <= 0) {
            m_errors.append(QString("Element '%1' has invalid update interval").arg(elem.id));
        }
    }

    return m_errors.isEmpty();
}

QStringList ksDisplayEditor::getErrors() const
{
    return m_errors;
}

// ============================================================================
// Export
// ============================================================================

bool ksDisplayEditor::exportToIni(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << generateIniContent();
    file.close();
    return true;
}

bool ksDisplayEditor::exportToJson(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << generateJsonContent();
    file.close();
    return true;
}

bool ksDisplayEditor::exportAsImage(const QString& filePath, int width, int height)
{
    int w = width > 0 ? width : m_displaySize.width();
    int h = height > 0 ? height : m_displaySize.height();

    if (w <= 0 || h <= 0) return false;

    QImage image(w, h, QImage::Format_ARGB32);
    image.fill(m_backgroundColor);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    if (!m_backgroundImage.isEmpty()) {
        QImage bg(m_backgroundImage);
        if (!bg.isNull()) {
            painter.drawImage(0, 0, bg.scaled(w, h, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
    }

    for (const auto& elem : m_elements) {
        if (!elem.visible) continue;

        painter.save();

        switch (elem.type) {
            case ElementType::TEXT: {
                QFont font(elem.fontFamily, elem.fontSize);
                painter.setFont(font);
                painter.setPen(elem.color);
                QString label = QStringLiteral("%1: %2")
                    .arg(dataSourceToString(elem.source))
                    .arg(elem.decimalPlaces > 0
                        ? QString::number(getPhysicsValue(elem.source), 'f', elem.decimalPlaces)
                        : QString::number(static_cast<int>(getPhysicsValue(elem.source))));
                painter.drawText(QRect(elem.position, elem.size), Qt::AlignCenter, label);
                break;
            }
            case ElementType::ANIMATED_TEXT: {
                QFont font(elem.fontFamily, elem.fontSize);
                painter.setFont(font);
                painter.setPen(elem.color);
                double val = getPhysicsValue(elem.source);
                QString display = elem.decimalPlaces > 0
                    ? QString::number(val, 'f', elem.decimalPlaces)
                    : QString::number(static_cast<int>(val));
                if (elem.animation.type != AnimationType::NONE) {
                    auto it = m_animations.find(elem.id);
                    if (it != m_animations.end()) {
                        painter.setOpacity(it.value().currentOpacity);
                        painter.translate(it.value().currentOffset);
                        painter.scale(it.value().currentScale, it.value().currentScale);
                        painter.rotate(it.value().currentRotation);
                        QColor c = it.value().currentColor.isValid() ? it.value().currentColor : elem.color;
                        painter.setPen(c);
                    }
                }
                painter.drawText(QRect(elem.position, elem.size), Qt::AlignCenter, display);
                break;
            }
            case ElementType::IMAGE: {
                if (!elem.imagePath.isEmpty()) {
                    QImage img(elem.imagePath);
                    if (!img.isNull()) {
                        painter.drawImage(elem.position.x(), elem.position.y(),
                                         img.scaled(elem.size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    }
                }
                break;
            }
            case ElementType::BAR: {
                QRect barRect(elem.position, elem.size);
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(40, 40, 40));
                painter.drawRoundedRect(barRect, 2, 2);
                painter.setBrush(elem.color);
                double physVal = getPhysicsValue(elem.source);
                double range = qMax(1, elem.maxValue - elem.minValue);
                double fraction = qBound(0.0, (physVal - elem.minValue) / range, 1.0);
                int fillWidth = static_cast<int>(fraction * elem.size.width());
                painter.drawRoundedRect(elem.position.x(), elem.position.y(), fillWidth, elem.size.height(), 2, 2);
                break;
            }
            case ElementType::CIRCLE: {
                painter.setPen(QPen(elem.color, 2));
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(elem.position, elem.size.width() / 2, elem.size.height() / 2);
                break;
            }
            case ElementType::DIGIT_GROUP: {
                QFont font(elem.fontFamily, elem.fontSize);
                painter.setFont(font);
                painter.setPen(elem.color);
                double val = getPhysicsValue(elem.source);
                painter.drawText(QRect(elem.position, elem.size), Qt::AlignCenter,
                                 QString::number(static_cast<int>(val)));
                break;
            }
            case ElementType::PROGRESS_RING: {
                int cx = elem.position.x() + elem.size.width() / 2;
                int cy = elem.position.y() + elem.size.height() / 2;
                int radius = qMin(elem.size.width(), elem.size.height()) / 2 - 4;
                painter.setPen(QPen(QColor(40, 40, 40), 4));
                painter.setBrush(Qt::NoBrush);
                painter.drawArc(cx - radius, cy - radius, radius * 2, radius * 2, 0, 360 * 16);
                double physVal = getPhysicsValue(elem.source);
                double range = qMax(1, elem.maxValue - elem.minValue);
                double fraction = qBound(0.0, (physVal - elem.minValue) / range, 1.0);
                painter.setPen(QPen(elem.color, 4));
                painter.drawArc(cx - radius, cy - radius, radius * 2, radius * 2,
                                90 * 16, -static_cast<int>(fraction * 360 * 16));
                QFont valFont(elem.fontFamily, qMax(8, elem.fontSize / 3));
                painter.setFont(valFont);
                painter.setPen(elem.color);
                painter.drawText(QRect(elem.position, elem.size), Qt::AlignCenter,
                                 QString::number(static_cast<int>(physVal)));
                break;
            }
        }

        painter.restore();
    }

    return image.save(filePath);
}

// ============================================================================
// Parsing methods
// ============================================================================

bool ksDisplayEditor::parseIniFile(const QString& content)
{
    m_elements.clear();

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ksEditor", "display");
    settings.setValue("dummy", content);

    QStringList lines = content.split('\n');
    QString currentSection;
    QMap<QString, QString> currentValues;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(';') || trimmed.startsWith('#')) continue;

        QRegularExpression sectionRe("^\\[(.+)\\]$");
        QRegularExpressionMatch sectionMatch = sectionRe.match(trimmed);
        if (sectionMatch.hasMatch()) {
            if (!currentSection.isEmpty() && !currentValues.isEmpty()) {
                parseIniSection(currentSection, currentValues);
            }
            currentSection = sectionMatch.captured(1);
            currentValues.clear();
            continue;
        }

        int eqPos = trimmed.indexOf('=');
        if (eqPos > 0) {
            QString key = trimmed.left(eqPos).trimmed();
            QString value = trimmed.mid(eqPos + 1).trimmed();
            currentValues[key] = value;
        }
    }

    if (!currentSection.isEmpty() && !currentValues.isEmpty()) {
        parseIniSection(currentSection, currentValues);
    }

    emit displayChanged();
    return true;
}

void ksDisplayEditor::parseIniSection(const QString& section, const QMap<QString, QString>& values)
{
    if (section == "Display") {
        if (values.contains("Name")) m_displayName = values["Name"];
        if (values.contains("Width") && values.contains("Height")) {
            m_displaySize = QSize(values["Width"].toInt(), values["Height"].toInt());
        }
        if (values.contains("BackgroundColor")) {
            m_backgroundColor = hexToColor(values["BackgroundColor"]);
        }
        if (values.contains("BackgroundImage")) {
            m_backgroundImage = values["BackgroundImage"];
        }
        return;
    }

    if (section.startsWith("Element_")) {
        DisplayElement elem;
        elem.id = section.mid(8);

        if (values.contains("Type")) elem.type = stringToElementType(values["Type"]);
        if (values.contains("Source")) elem.source = stringToDataSource(values["Source"]);
        if (values.contains("X") && values.contains("Y")) {
            elem.position = QPoint(values["X"].toInt(), values["Y"].toInt());
        }
        if (values.contains("Width") && values.contains("Height")) {
            elem.size = QSize(values["Width"].toInt(), values["Height"].toInt());
        }
        if (values.contains("Color")) elem.color = hexToColor(values["Color"]);
        if (values.contains("FontSize")) elem.fontSize = values["FontSize"].toInt();
        if (values.contains("FontFamily")) elem.fontFamily = values["FontFamily"];
        if (values.contains("DecimalPlaces")) elem.decimalPlaces = values["DecimalPlaces"].toInt();
        if (values.contains("Visible")) elem.visible = (values["Visible"] != "0");
        if (values.contains("UpdateInterval")) elem.updateInterval = values["UpdateInterval"].toInt();
        if (values.contains("MinValue")) elem.minValue = values["MinValue"].toInt();
        if (values.contains("MaxValue")) elem.maxValue = values["MaxValue"].toInt();
        if (values.contains("ImagePath")) elem.imagePath = values["ImagePath"];

        m_elements.append(elem);
    }
}

bool ksDisplayEditor::parseLuaFile(const QString& content)
{
    m_elements.clear();

    QRegularExpression nameRe("display\\.name\\s*=\\s*\"([^\"]*)\"");
    QRegularExpressionMatch nameMatch = nameRe.match(content);
    if (nameMatch.hasMatch()) m_displayName = nameMatch.captured(1);

    QRegularExpression sizeRe("display\\.size\\s*=\\s*\\{(\\d+)\\s*,\\s*(\\d+)\\}");
    QRegularExpressionMatch sizeMatch = sizeRe.match(content);
    if (sizeMatch.hasMatch()) {
        m_displaySize = QSize(sizeMatch.captured(1).toInt(), sizeMatch.captured(2).toInt());
    }

    QRegularExpression elemBlockRe("display\\.elements\\[\"([^\"]+)\"\\]\\s*=\\s*\\{([^}]+)\\}", QRegularExpression::DotMatchesEverythingOption);
    auto it = elemBlockRe.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString elemId = match.captured(1);
        QString elemBody = match.captured(2);

        DisplayElement elem;
        elem.id = elemId;

        QRegularExpression typeRe("type\\s*=\\s*\"?(\\d+)\"?");
        QRegularExpressionMatch typeMatch = typeRe.match(elemBody);
        if (typeMatch.hasMatch()) elem.type = static_cast<ElementType>(typeMatch.captured(1).toInt());

        QRegularExpression posRe("position\\s*=\\s*\\{(-?\\d+)\\s*,\\s*(-?\\d+)\\}");
        QRegularExpressionMatch posMatch = posRe.match(elemBody);
        if (posMatch.hasMatch()) {
            elem.position = QPoint(posMatch.captured(1).toInt(), posMatch.captured(2).toInt());
        }

        QRegularExpression sizeElemRe("size\\s*=\\s*\\{(\\d+)\\s*,\\s*(\\d+)\\}");
        QRegularExpressionMatch sizeElemMatch = sizeElemRe.match(elemBody);
        if (sizeElemMatch.hasMatch()) {
            elem.size = QSize(sizeElemMatch.captured(1).toInt(), sizeElemMatch.captured(2).toInt());
        }

        QRegularExpression colorRe("color\\s*=\\s*\\{(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\}");
        QRegularExpressionMatch colorMatch = colorRe.match(elemBody);
        if (colorMatch.hasMatch()) {
            elem.color = QColor(colorMatch.captured(1).toInt(),
                                colorMatch.captured(2).toInt(),
                                colorMatch.captured(3).toInt());
        }

        QRegularExpression fontRe("fontSize\\s*=\\s*(\\d+)");
        QRegularExpressionMatch fontMatch = fontRe.match(elemBody);
        if (fontMatch.hasMatch()) elem.fontSize = fontMatch.captured(1).toInt();

        QRegularExpression fontFamRe("fontFamily\\s*=\\s*\"([^\"]*)\"");
        QRegularExpressionMatch fontFamMatch = fontFamRe.match(elemBody);
        if (fontFamMatch.hasMatch()) elem.fontFamily = fontFamMatch.captured(1);

        m_elements.append(elem);
    }

    emit displayChanged();
    return true;
}

bool ksDisplayEditor::loadFromJson(const QJsonObject& json)
{
    m_elements.clear();

    if (json.contains("name")) m_displayName = json["name"].toString();

    if (json.contains("size")) {
        QJsonObject sizeObj = json["size"].toObject();
        m_displaySize = QSize(sizeObj["width"].toInt(), sizeObj["height"].toInt());
    }

    if (json.contains("backgroundColor")) {
        m_backgroundColor = hexToColor(json["backgroundColor"].toString());
    }
    if (json.contains("backgroundImage")) {
        m_backgroundImage = json["backgroundImage"].toString();
    }

    if (json.contains("elements")) {
        QJsonArray elemArray = json["elements"].toArray();
        for (const QJsonValue& val : elemArray) {
            QJsonObject elemObj = val.toObject();
            DisplayElement elem;
            elem.id = elemObj["id"].toString();
            elem.type = stringToElementType(elemObj["type"].toString());
            elem.source = stringToDataSource(elemObj["source"].toString());

            QJsonObject posObj = elemObj["position"].toObject();
            elem.position = QPoint(posObj["x"].toInt(), posObj["y"].toInt());

            QJsonObject sizeObj = elemObj["size"].toObject();
            elem.size = QSize(sizeObj["width"].toInt(), sizeObj["height"].toInt());

            elem.color = hexToColor(elemObj["color"].toString());
            elem.fontSize = elemObj["fontSize"].toInt(24);
            elem.fontFamily = elemObj["fontFamily"].toString("Arial");
            elem.decimalPlaces = elemObj["decimalPlaces"].toInt(0);
            elem.visible = elemObj["visible"].toBool(true);
            elem.updateInterval = elemObj["updateInterval"].toInt(100);
            elem.minValue = elemObj["minValue"].toInt(0);
            elem.maxValue = elemObj["maxValue"].toInt(100);
            elem.imagePath = elemObj["imagePath"].toString();

            m_elements.append(elem);
        }
    }

    emit displayChanged();
    return true;
}

QString ksDisplayEditor::generateLuaScript() const
{
    QString lua;
    QTextStream out(&lua);
    out << "-- ksEditor Display Configuration\n";
    out << "-- Auto-generated Lua script\n\n";
    out << "local display = {}\n\n";
    out << "display.name = \"" << m_displayName << "\"\n";
    out << "display.size = {" << m_displaySize.width() << ", " << m_displaySize.height() << "}\n";

    if (m_backgroundColor.isValid()) {
        out << "display.backgroundColor = {" << m_backgroundColor.red()
            << ", " << m_backgroundColor.green()
            << ", " << m_backgroundColor.blue() << "}\n";
    }
    if (!m_backgroundImage.isEmpty()) {
        out << "display.backgroundImage = \"" << m_backgroundImage << "\"\n";
    }

    out << "display.elements = {}\n\n";

    for (const auto& elem : m_elements) {
        out << "display.elements[\"" << elem.id << "\"] = {\n";
        out << "    type = \"" << elem.id << "\",\n";
        out << "    source = \"" << dataSourceToString(elem.source) << "\",\n";
        out << "    position = {" << elem.position.x() << ", " << elem.position.y() << "},\n";
        out << "    size = {" << elem.size.width() << ", " << elem.size.height() << "},\n";
        out << "    color = {" << elem.color.red() << ", " << elem.color.green() << ", " << elem.color.blue() << "},\n";
        out << "    fontSize = " << elem.fontSize << ",\n";
        out << "    fontFamily = \"" << elem.fontFamily << "\",\n";
        out << "    decimalPlaces = " << elem.decimalPlaces << ",\n";
        out << "    visible = " << (elem.visible ? "true" : "false") << ",\n";
        out << "    updateInterval = " << elem.updateInterval << ",\n";
        if (!elem.imagePath.isEmpty()) {
            out << "    imagePath = \"" << elem.imagePath << "\",\n";
        }
        out << "}\n\n";
    }

    out << "return display\n";
    return lua;
}

QString ksDisplayEditor::generateIniContent() const
{
    QString ini;
    QTextStream out(&ini);

    out << "[Display]\n";
    out << "Name=" << m_displayName << "\n";
    out << "Width=" << m_displaySize.width() << "\n";
    out << "Height=" << m_displaySize.height() << "\n";
    out << "BackgroundColor=" << colorToHex(m_backgroundColor) << "\n";
    if (!m_backgroundImage.isEmpty()) {
        out << "BackgroundImage=" << m_backgroundImage << "\n";
    }
    out << "\n";

    for (const auto& elem : m_elements) {
        out << "[Element_" << elem.id << "]\n";
        out << "Type=" << elementTypeToString(elem.type) << "\n";
        out << "Source=" << dataSourceToString(elem.source) << "\n";
        out << "X=" << elem.position.x() << "\n";
        out << "Y=" << elem.position.y() << "\n";
        out << "Width=" << elem.size.width() << "\n";
        out << "Height=" << elem.size.height() << "\n";
        out << "Color=" << colorToHex(elem.color) << "\n";
        out << "FontSize=" << elem.fontSize << "\n";
        out << "FontFamily=" << elem.fontFamily << "\n";
        out << "DecimalPlaces=" << elem.decimalPlaces << "\n";
        out << "Visible=" << (elem.visible ? "1" : "0") << "\n";
        out << "UpdateInterval=" << elem.updateInterval << "\n";
        out << "MinValue=" << elem.minValue << "\n";
        out << "MaxValue=" << elem.maxValue << "\n";
        if (!elem.imagePath.isEmpty()) {
            out << "ImagePath=" << elem.imagePath << "\n";
        }
        out << "\n";
    }

    return ini;
}

QString ksDisplayEditor::generateJsonContent() const
{
    QJsonObject json;
    json["name"] = m_displayName;

    QJsonObject sizeObj;
    sizeObj["width"] = m_displaySize.width();
    sizeObj["height"] = m_displaySize.height();
    json["size"] = sizeObj;

    json["backgroundColor"] = colorToHex(m_backgroundColor);
    if (!m_backgroundImage.isEmpty()) {
        json["backgroundImage"] = m_backgroundImage;
    }

    QJsonArray elemArray;
    for (const auto& elem : m_elements) {
        QJsonObject elemObj;
        elemObj["id"] = elem.id;
        elemObj["type"] = elementTypeToString(elem.type);
        elemObj["source"] = dataSourceToString(elem.source);

        QJsonObject posObj;
        posObj["x"] = elem.position.x();
        posObj["y"] = elem.position.y();
        elemObj["position"] = posObj;

        QJsonObject sizeElemObj;
        sizeElemObj["width"] = elem.size.width();
        sizeElemObj["height"] = elem.size.height();
        elemObj["size"] = sizeElemObj;

        elemObj["color"] = colorToHex(elem.color);
        elemObj["fontSize"] = elem.fontSize;
        elemObj["fontFamily"] = elem.fontFamily;
        elemObj["decimalPlaces"] = elem.decimalPlaces;
        elemObj["visible"] = elem.visible;
        elemObj["updateInterval"] = elem.updateInterval;
        elemObj["minValue"] = elem.minValue;
        elemObj["maxValue"] = elem.maxValue;
        if (!elem.imagePath.isEmpty()) {
            elemObj["imagePath"] = elem.imagePath;
        }

        elemArray.append(elemObj);
    }
    json["elements"] = elemArray;

    QJsonDocument doc(json);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

// ============================================================================
// Helper methods
// ============================================================================

QString ksDisplayEditor::elementTypeToString(ElementType type) const
{
    switch (type) {
        case ElementType::TEXT: return "TEXT";
        case ElementType::IMAGE: return "IMAGE";
        case ElementType::BAR: return "BAR";
        case ElementType::CIRCLE: return "CIRCLE";
        case ElementType::DIGIT_GROUP: return "DIGIT_GROUP";
        case ElementType::ANIMATED_TEXT: return "ANIMATED_TEXT";
        case ElementType::PROGRESS_RING: return "PROGRESS_RING";
        default: return "TEXT";
    }
}

ElementType ksDisplayEditor::stringToElementType(const QString& str) const
{
    QString s = str.toUpper();
    if (s == "TEXT") return ElementType::TEXT;
    if (s == "IMAGE") return ElementType::IMAGE;
    if (s == "BAR") return ElementType::BAR;
    if (s == "CIRCLE") return ElementType::CIRCLE;
    if (s == "DIGIT_GROUP") return ElementType::DIGIT_GROUP;
    if (s == "ANIMATED_TEXT") return ElementType::ANIMATED_TEXT;
    if (s == "PROGRESS_RING") return ElementType::PROGRESS_RING;
    return ElementType::TEXT;
}

QString ksDisplayEditor::dataSourceToString(DataSource source) const
{
    switch (source) {
        case DataSource::SPEED: return "SPEED";
        case DataSource::RPM: return "RPM";
        case DataSource::GEAR: return "GEAR";
        case DataSource::LAP_TIME: return "LAP_TIME";
        case DataSource::BEST_LAP: return "BEST_LAP";
        case DataSource::LAST_LAP: return "LAST_LAP";
        case DataSource::DELTA: return "DELTA";
        case DataSource::FUEL: return "FUEL";
        case DataSource::TIRE_TEMP: return "TIRE_TEMP";
        case DataSource::WATER_TEMP: return "WATER_TEMP";
        case DataSource::OIL_TEMP: return "OIL_TEMP";
        case DataSource::TURBO_BOOST: return "TURBO_BOOST";
        case DataSource::SPEED_KMH: return "SPEED_KMH";
        case DataSource::SPEED_MPH: return "SPEED_MPH";
        case DataSource::CURRENT_LAP: return "CURRENT_LAP";
        case DataSource::LAP_COUNT: return "LAP_COUNT";
        case DataSource::POSITION: return "POSITION";
        case DataSource::BRAKE_BIAS: return "BRAKE_BIAS";
        case DataSource::TIRE_PRESSURE: return "TIRE_PRESSURE";
        case DataSource::STEERING_ANGLE: return "STEERING_ANGLE";
        case DataSource::SUSPENSION_TRAVEL: return "SUSPENSION_TRAVEL";
        default: return "SPEED";
    }
}

DataSource ksDisplayEditor::stringToDataSource(const QString& str) const
{
    QString s = str.toUpper();
    if (s == "SPEED") return DataSource::SPEED;
    if (s == "RPM") return DataSource::RPM;
    if (s == "GEAR") return DataSource::GEAR;
    if (s == "LAP_TIME") return DataSource::LAP_TIME;
    if (s == "BEST_LAP") return DataSource::BEST_LAP;
    if (s == "LAST_LAP") return DataSource::LAST_LAP;
    if (s == "DELTA") return DataSource::DELTA;
    if (s == "FUEL") return DataSource::FUEL;
    if (s == "TIRE_TEMP") return DataSource::TIRE_TEMP;
    if (s == "WATER_TEMP") return DataSource::WATER_TEMP;
    if (s == "OIL_TEMP") return DataSource::OIL_TEMP;
    if (s == "TURBO_BOOST") return DataSource::TURBO_BOOST;
    if (s == "SPEED_KMH") return DataSource::SPEED_KMH;
    if (s == "SPEED_MPH") return DataSource::SPEED_MPH;
    if (s == "CURRENT_LAP") return DataSource::CURRENT_LAP;
    if (s == "LAP_COUNT") return DataSource::LAP_COUNT;
    if (s == "POSITION") return DataSource::POSITION;
    if (s == "BRAKE_BIAS") return DataSource::BRAKE_BIAS;
    if (s == "TIRE_PRESSURE") return DataSource::TIRE_PRESSURE;
    if (s == "STEERING_ANGLE") return DataSource::STEERING_ANGLE;
    if (s == "SUSPENSION_TRAVEL") return DataSource::SUSPENSION_TRAVEL;
    return DataSource::SPEED;
}

QString ksDisplayEditor::colorToHex(const QColor& color) const
{
    return color.name(QColor::HexRgb);
}

QColor ksDisplayEditor::hexToColor(const QString& hex) const
{
    return QColor(hex);
}

// ============================================================================
// Animation System
// ============================================================================

void ksDisplayEditor::setAnimation(const QString& elementId, const AnimationConfig& config) {
    DisplayElement* el = getElement(elementId);
    if (!el) return;
    el->animation = config;

    AnimationState state;
    state.config = config;
    state.running = true;
    state.forward = true;
    state.currentScale = config.scaleFrom;
    state.currentRotation = config.rotationFrom;
    state.currentColor = el->color;
    state.currentOpacity = config.maxOpacity;
    m_animations[elementId] = state;

    if (!m_animationTimer->isActive()) {
        m_animationTimer->start();
    }

    emit animationStarted(elementId);
}

void ksDisplayEditor::removeAnimation(const QString& elementId) {
    m_animations.remove(elementId);
    emit animationStopped(elementId);
}

void ksDisplayEditor::setPhysicsValue(const QString& elementId, double value) {
    if (m_animations.contains(elementId)) {
        m_animations[elementId].physicsValue = value;
    }
}

void ksDisplayEditor::pauseAnimations() {
    m_animationsPaused = true;
}

void ksDisplayEditor::resumeAnimations() {
    m_animationsPaused = false;
}

void ksDisplayEditor::stopAllAnimations() {
    m_animations.clear();
    m_animationTimer->stop();
}

AnimationState ksDisplayEditor::getAnimationState(const QString& elementId) const {
    return m_animations.value(elementId);
}

void ksDisplayEditor::updatePhysicsValue(DataSource source, double value) {
    m_physicsValues[source] = value;
}

double ksDisplayEditor::getPhysicsValue(DataSource source) const {
    return m_physicsValues.value(source, 0.0);
}

void ksDisplayEditor::setPhysicsUpdateInterval(int ms) {
    m_physicsUpdateIntervalMs = ms;
}

void ksDisplayEditor::onAnimationTick() {
    if (m_animationsPaused) return;

    double dtMs = 16.0; // ~60 FPS

    for (auto it = m_animations.begin(); it != m_animations.end(); ) {
        AnimationState& state = it.value();
        if (!state.running) {
            ++it;
            continue;
        }

        state.elapsedMs += dtMs;
        state.progress = qBound(0.0, state.elapsedMs / qMax(state.config.durationMs, 1), 1.0);

        // Update animation state
        updateAnimationState(state, dtMs);

        // Check for loop/ping-pong
        if (state.progress >= 1.0) {
            if (state.config.pingPong) {
                state.forward = !state.forward;
                state.elapsedMs = 0;
                state.progress = 0;
                emit animationLooped(it.key());
            } else if (state.config.loop) {
                state.elapsedMs = 0;
                state.progress = 0;
                emit animationLooped(it.key());
            } else {
                state.running = false;
                emit animationStopped(it.key());
            }
        }

        // Apply animation to element properties
        DisplayElement* el = getElement(it.key());
        if (el) {
            el->color = state.currentColor;
        }

        ++it;
    }

    // Stop timer if no animations running
    if (m_animations.isEmpty()) {
        m_animationTimer->stop();
    }
}

void ksDisplayEditor::updateAnimationState(AnimationState& state, double dtMs) {
    const AnimationConfig& cfg = state.config;
    double t = computeEasedValue(cfg, state.progress);

    switch (cfg.type) {
        case AnimationType::FADE_IN:
            state.currentOpacity = cfg.minOpacity + (cfg.maxOpacity - cfg.minOpacity) * t;
            break;

        case AnimationType::FADE_OUT:
            state.currentOpacity = cfg.maxOpacity - (cfg.maxOpacity - cfg.minOpacity) * t;
            break;

        case AnimationType::SLIDE_IN:
            state.currentOffset = computeSlideOffset(cfg, 1.0 - t);
            break;

        case AnimationType::SLIDE_OUT:
            state.currentOffset = computeSlideOffset(cfg, t);
            break;

        case AnimationType::BOUNCE: {
            double bounceT = qSin(t * M_PI * 3.0) * (1.0 - t);
            state.currentScale = 1.0 + bounceT * 0.1;
            break;
        }

        case AnimationType::PULSE: {
            double pulseT = qSin(t * M_PI * 2.0) * 0.5 + 0.5;
            state.currentScale = cfg.pulseMinScale + (cfg.pulseMaxScale - cfg.pulseMinScale) * pulseT;
            break;
        }

        case AnimationType::ROTATE:
            state.currentRotation = cfg.rotationFrom + (cfg.rotationTo - cfg.rotationFrom) * t;
            break;

        case AnimationType::SCALE:
            state.currentScale = cfg.scaleFrom + (cfg.scaleTo - cfg.scaleFrom) * t;
            break;

        case AnimationType::SHAKE: {
            double shakeT = t;
            double decay = 1.0 - shakeT;
            double offsetX = qSin(shakeT * M_PI * 8.0) * cfg.shakeIntensity * decay;
            double offsetY = qCos(shakeT * M_PI * 6.0) * cfg.shakeIntensity * decay * 0.5;
            state.currentOffset = QPointF(offsetX, offsetY);
            break;
        }

        case AnimationType::TYPEWRITER: {
            double revealT = t;
            state.currentOpacity = revealT;
            break;
        }

        case AnimationType::COLOR_CYCLE:
            state.currentColor = interpolateColor(cfg.colorFrom, cfg.colorTo, t);
            break;

        case AnimationType::PHYSICS_DRIVEN: {
            double physVal = m_physicsValues.value(cfg.physicsSource, 0.0);
            double normVal = qBound(0.0, physVal / 100.0, 1.0);
            state.currentScale = cfg.scaleFrom + (cfg.scaleTo - cfg.scaleFrom) * normVal;
            state.currentColor = interpolateColor(cfg.colorFrom, cfg.colorTo, normVal);
            break;
        }

        default:
            break;
    }
}

QPointF ksDisplayEditor::computeSlideOffset(const AnimationConfig& cfg, double progress) {
    return cfg.slideOffset * (1.0 - progress);
}

double ksDisplayEditor::computeEasedValue(const AnimationConfig& cfg, double progress) {
    QEasingCurve easing(cfg.easing);
    return easing.valueForProgress(progress);
}

QColor ksDisplayEditor::interpolateColor(const QColor& from, const QColor& to, double t) {
    return QColor(
        static_cast<int>(from.red() + (to.red() - from.red()) * t),
        static_cast<int>(from.green() + (to.green() - from.green()) * t),
        static_cast<int>(from.blue() + (to.blue() - from.blue()) * t),
        static_cast<int>(from.alpha() + (to.alpha() - from.alpha()) * t)
    );
}
