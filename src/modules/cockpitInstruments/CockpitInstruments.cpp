#include "CockpitInstruments.h"
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

ksCockpitInstruments::ksCockpitInstruments(QObject *parent)
    : QObject(parent), m_displaySize(800, 480), m_backgroundColor(0, 0, 0)
{
    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(16); // ~60 FPS
    connect(m_animationTimer, &QTimer::timeout, this, &ksCockpitInstruments::onAnimationTick);

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

ksCockpitInstruments::~ksCockpitInstruments()
{
}

// ============================================================================
// File operations
// ============================================================================

bool ksCockpitInstruments::loadFromFile(const QString& filePath)
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

bool ksCockpitInstruments::saveToFile(const QString& filePath)
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

bool ksCockpitInstruments::loadFromLua(const QString& luaFilePath)
{
    QFile file(luaFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QString content = file.readAll();
    file.close();

    return parseLuaFile(content);
}

bool ksCockpitInstruments::saveToLua(const QString& luaFilePath)
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

void ksCockpitInstruments::addElement(const DisplayElement& element)
{
    DisplayElement elem = element;
    if (elem.id.isEmpty()) {
        elem.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    }
    m_elements.append(elem);
    emit elementAdded(elem.id);
    emit displayChanged();
}

void ksCockpitInstruments::removeElement(const QString& elementId)
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

void ksCockpitInstruments::updateElement(const QString& elementId, const DisplayElement& element)
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

DisplayElement* ksCockpitInstruments::getElement(const QString& elementId)
{
    for (auto& elem : m_elements) {
        if (elem.id == elementId) return &elem;
    }
    return nullptr;
}

QVector<DisplayElement>& ksCockpitInstruments::getAllElements()
{
    return m_elements;
}

void ksCockpitInstruments::clearElements()
{
    m_elements.clear();
    emit displayChanged();
}

// ============================================================================
// Display settings
// ============================================================================

void ksCockpitInstruments::setDisplayName(const QString& name)
{
    m_displayName = name;
    emit displayChanged();
}

QString ksCockpitInstruments::getDisplayName() const
{
    return m_displayName;
}

void ksCockpitInstruments::setDisplaySize(const QSize& size)
{
    m_displaySize = size;
    emit displayChanged();
}

QSize ksCockpitInstruments::getDisplaySize() const
{
    return m_displaySize;
}

void ksCockpitInstruments::setBackgroundImage(const QString& imagePath)
{
    m_backgroundImage = imagePath;
    emit displayChanged();
}

QString ksCockpitInstruments::getBackgroundImage() const
{
    return m_backgroundImage;
}

void ksCockpitInstruments::setBackgroundColor(const QColor& color)
{
    m_backgroundColor = color;
    emit displayChanged();
}

QColor ksCockpitInstruments::getBackgroundColor() const
{
    return m_backgroundColor;
}

// ============================================================================
// Validation
// ============================================================================

bool ksCockpitInstruments::validateConfig()
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

QStringList ksCockpitInstruments::getErrors() const
{
    return m_errors;
}

// ============================================================================
// Export
// ============================================================================

bool ksCockpitInstruments::exportToIni(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << generateIniContent();
    file.close();
    return true;
}

bool ksCockpitInstruments::exportToJson(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << generateJsonContent();
    file.close();
    return true;
}

bool ksCockpitInstruments::exportAsImage(const QString& filePath, int width, int height)
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

bool ksCockpitInstruments::parseIniFile(const QString& content)
{
    m_elements.clear();

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

void ksCockpitInstruments::parseIniSection(const QString& section, const QMap<QString, QString>& values)
{
    // AC CSP format: [ITEM_N]
    if (section.startsWith("ITEM_")) {
        parseAcItem(section, values);
        return;
    }
    // AC CSP format: [LED_N] or [LED_RPM_N]
    if (section.startsWith("LED_")) {
        parseAcLed(section, values);
        return;
    }
    // AC CSP format: [TYRE_LOCK_SLIP_N]
    if (section.startsWith("TYRE_LOCK_SLIP_")) {
        parseAcTyreSlip(section, values);
        return;
    }
    // AC CSP format: [KERS_CHARGE_SERIE_N]
    if (section.startsWith("KERS_CHARGE_SERIE_")) {
        AcKersChargeSerie serie;
        serie.index = section.mid(QString("KERS_CHARGE_SERIE_").length()).toInt();
        if (values.contains("PREFIX")) serie.prefix = values["PREFIX"];
        if (values.contains("START_INDEX")) serie.startIndex = values["START_INDEX"].toInt();
        if (values.contains("END_INDEX")) serie.endIndex = values["END_INDEX"].toInt();
        m_acKersSeries.append(serie);
        return;
    }
    // AC CSP format: [KERS_ENABLED_N]
    if (section.startsWith("KERS_ENABLED_")) {
        AcLed led;
        led.index = section.mid(QString("KERS_ENABLED_").length()).toInt();
        led.objectName = values.value("OBJECT_NAME", "LED_RPM_1");
        led.emissive = rgbStringToColor(values.value("EMISSIVE", "200,10,0"));
        led.diffuse = values.value("DIFFUSE", "0.2").toDouble();
        m_acLeds.append(led);
        return;
    }

    // Legacy format: [Display]
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

    // Legacy format: [Element_<id>]
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

void ksCockpitInstruments::parseAcItem(const QString& section, const QMap<QString, QString>& values)
{
    AcDisplayItem item;
    item.index = section.mid(5).toInt(); // "ITEM_".length() == 5

    if (values.contains("TYPE")) item.type = stringToAcItemType(values["TYPE"]);
    if (values.contains("PARENT")) {
        QString p = values["PARENT"].toUpper();
        if (p == "DISPLAY_WHEEL") item.parent = AcDisplayParent::DISPLAY_WHEEL;
        else if (p == "DISPLAY_MONITOR") item.parent = AcDisplayParent::DISPLAY_MONITOR;
        else item.parent = AcDisplayParent::UNKNOWN;
    }
    if (values.contains("OBJECT_NAME")) item.objectName = values["OBJECT_NAME"];
    if (values.contains("POSITION")) {
        QString posStr = values["POSITION"];
        posStr.remove('(').remove(')');
        QStringList parts = posStr.split(',');
        if (parts.size() >= 2) {
            item.position = QPoint(parts[0].trimmed().toDouble(), parts[1].trimmed().toDouble());
        }
    }
    if (values.contains("SIZE")) {
        QString sizeStr = values["SIZE"];
        sizeStr.remove('(').remove(')');
        QStringList parts = sizeStr.split(',');
        if (parts.size() >= 2) {
            item.size = QSize(parts[0].trimmed().toDouble(), parts[1].trimmed().toDouble());
        }
    }
    if (values.contains("COLOR")) item.color = rgbStringToColor(values["COLOR"]);
    if (values.contains("COLOR_2")) item.color2 = rgbStringToColor(values["COLOR_2"]);
    if (values.contains("INTENSITY")) item.intensity = values["INTENSITY"].toInt();
    if (values.contains("INTENSITY_2")) item.intensity2 = values["INTENSITY_2"].toInt();
    if (values.contains("FONT")) item.font = values["FONT"];
    if (values.contains("VERSION")) item.version = values["VERSION"].toInt();
    if (values.contains("ALIGN")) item.align = values["ALIGN"].toUpper();
    if (values.contains("DECIMALS")) item.decimals = values["DECIMALS"].toInt();
    if (values.contains("PREFIX")) item.prefix = values["PREFIX"];
    if (values.contains("POSTFIX")) item.postfix = values["POSTFIX"];
    if (values.contains("UNITS")) item.units = values["UNITS"];
    if (values.contains("TYRE_NUMBER")) item.tyreNumber = values["TYRE_NUMBER"].toInt();
    if (values.contains("N_TIME")) item.nTime = values["N_TIME"].toDouble();
    if (values.contains("RPM_TRIGGER")) item.rpmTrigger = values["RPM_TRIGGER"].toInt();

    m_acItems.append(item);
}

void ksCockpitInstruments::parseAcLed(const QString& section, const QMap<QString, QString>& values)
{
    AcLed led;
    if (section.startsWith("LED_RPM_")) {
        led.index = section.mid(8).toInt();
    } else {
        led.index = section.mid(4).toInt(); // "LED_".length() == 4
    }

    if (values.contains("OBJECT_NAME")) led.objectName = values["OBJECT_NAME"];
    if (values.contains("RPM_SWITCH")) led.rpmSwitch = values["RPM_SWITCH"].toInt();
    if (values.contains("EMISSIVE")) led.emissive = rgbStringToColor(values["EMISSIVE"]);
    if (values.contains("DIFFUSE")) led.diffuse = values["DIFFUSE"].toDouble();
    if (values.contains("BLINK_SWITCH")) led.blinkSwitch = values["BLINK_SWITCH"].toInt();
    if (values.contains("BLINK_HZ")) led.blinkHz = values["BLINK_HZ"].toDouble();

    m_acLeds.append(led);
}

void ksCockpitInstruments::parseAcTyreSlip(const QString& section, const QMap<QString, QString>& values)
{
    AcTyreLockSlip slip;
    slip.index = section.mid(16).toInt(); // "TYRE_LOCK_SLIP_".length() == 16

    if (values.contains("OBJECT_NAME")) slip.objectName = values["OBJECT_NAME"];
    if (values.contains("TYRE_INDEX")) slip.tyreIndex = values["TYRE_INDEX"].toInt();
    if (values.contains("EMISSIVE")) slip.emissive = rgbStringToColor(values["EMISSIVE"]);
    if (values.contains("EMISSIVE_LOCK")) slip.emissiveLock = rgbStringToColor(values["EMISSIVE_LOCK"]);
    if (values.contains("DIFFUSE")) slip.diffuse = values["DIFFUSE"].toDouble();
    if (values.contains("SLIP_SWITCH")) slip.slipSwitch = values["SLIP_SWITCH"].toDouble();
    if (values.contains("SHOW_LOCK")) slip.showLock = (values["SHOW_LOCK"] == "1");
    if (values.contains("WHEEL_SPEED_MULT")) slip.wheelSpeedMult = values["WHEEL_SPEED_MULT"].toDouble();

    m_acTyreSlips.append(slip);
}

bool ksCockpitInstruments::parseLuaFile(const QString& content)
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

bool ksCockpitInstruments::loadFromJson(const QJsonObject& json)
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

QString ksCockpitInstruments::generateLuaScript() const
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

QString ksCockpitInstruments::generateIniContent() const
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

QString ksCockpitInstruments::generateJsonContent() const
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

QString ksCockpitInstruments::elementTypeToString(ElementType type) const
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

ElementType ksCockpitInstruments::stringToElementType(const QString& str) const
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

QString ksCockpitInstruments::dataSourceToString(DataSource source) const
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

DataSource ksCockpitInstruments::stringToDataSource(const QString& str) const
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

QString ksCockpitInstruments::colorToHex(const QColor& color) const
{
    return color.name(QColor::HexRgb);
}

QColor ksCockpitInstruments::hexToColor(const QString& hex) const
{
    return QColor(hex);
}

QColor ksCockpitInstruments::rgbStringToColor(const QString& rgb) const
{
    QStringList parts = rgb.split(',');
    if (parts.size() >= 3) {
        return QColor(parts[0].trimmed().toInt(),
                      parts[1].trimmed().toInt(),
                      parts[2].trimmed().toInt());
    }
    return QColor(rgb);
}

QString ksCockpitInstruments::acItemTypeToString(AcItemType type) const
{
    switch (type) {
        case AcItemType::GEAR: return "GEAR";
        case AcItemType::SPEED: return "SPEED";
        case AcItemType::RPM: return "RPM";
        case AcItemType::LAPTIME: return "LAPTIME";
        case AcItemType::FUEL: return "FUEL";
        case AcItemType::PERF: return "PERF";
        case AcItemType::PRESSURE: return "PRESSURE";
        case AcItemType::TC_LEVEL: return "TC_LEVEL";
        case AcItemType::WATER_TEMP: return "WATER_TEMP";
        case AcItemType::LAST_LAP: return "LAST_LAP";
        case AcItemType::FUEL_CONS: return "FUEL_CONS";
        case AcItemType::KERS_CHARGE: return "KERS_CHARGE";
        case AcItemType::CURRENT_LAP: return "CURRENT_LAP";
        case AcItemType::POSITION_CAR: return "POSITION_CAR";
        case AcItemType::GEAR_COLOR: return "GEAR_COLOR";
        case AcItemType::BEST_LAP: return "BEST_LAP";
        case AcItemType::PLACE_HOLDER: return "PLACE_HOLDER";
        default: return "UNKNOWN";
    }
}

AcItemType ksCockpitInstruments::stringToAcItemType(const QString& str) const
{
    QString s = str.toUpper();
    if (s == "GEAR") return AcItemType::GEAR;
    if (s == "SPEED") return AcItemType::SPEED;
    if (s == "RPM") return AcItemType::RPM;
    if (s == "LAPTIME") return AcItemType::LAPTIME;
    if (s == "FUEL") return AcItemType::FUEL;
    if (s == "PERF") return AcItemType::PERF;
    if (s == "PRESSURE") return AcItemType::PRESSURE;
    if (s == "TC_LEVEL") return AcItemType::TC_LEVEL;
    if (s == "WATER_TEMP") return AcItemType::WATER_TEMP;
    if (s == "LAST_LAP") return AcItemType::LAST_LAP;
    if (s == "FUEL_CONS") return AcItemType::FUEL_CONS;
    if (s == "KERS_CHARGE") return AcItemType::KERS_CHARGE;
    if (s == "CURRENT_LAP") return AcItemType::CURRENT_LAP;
    if (s == "POSITION_CAR") return AcItemType::POSITION_CAR;
    if (s == "GEAR_COLOR") return AcItemType::GEAR_COLOR;
    if (s == "BEST_LAP") return AcItemType::BEST_LAP;
    if (s == "PLACE_HOLDER") return AcItemType::PLACE_HOLDER;
    return AcItemType::UNKNOWN;
}

// ============================================================================
// AC CSP format accessors
// ============================================================================

QVector<AcDisplayItem>& ksCockpitInstruments::getAcItems() { return m_acItems; }
QVector<AcLed>& ksCockpitInstruments::getAcLeds() { return m_acLeds; }
QVector<AcTyreLockSlip>& ksCockpitInstruments::getAcTyreSlips() { return m_acTyreSlips; }
QVector<AcKersChargeSerie>& ksCockpitInstruments::getAcKersSeries() { return m_acKersSeries; }

void ksCockpitInstruments::addAcItem(const AcDisplayItem& item) {
    m_acItems.append(item);
    emit displayChanged();
}

void ksCockpitInstruments::addAcLed(const AcLed& led) {
    m_acLeds.append(led);
    emit displayChanged();
}

void ksCockpitInstruments::addAcTyreSlip(const AcTyreLockSlip& slip) {
    m_acTyreSlips.append(slip);
    emit displayChanged();
}

void ksCockpitInstruments::removeAcItem(int index) {
    if (index >= 0 && index < m_acItems.size()) {
        m_acItems.removeAt(index);
        emit displayChanged();
    }
}

void ksCockpitInstruments::removeAcLed(int index) {
    if (index >= 0 && index < m_acLeds.size()) {
        m_acLeds.removeAt(index);
        emit displayChanged();
    }
}

void ksCockpitInstruments::removeAcTyreSlip(int index) {
    if (index >= 0 && index < m_acTyreSlips.size()) {
        m_acTyreSlips.removeAt(index);
        emit displayChanged();
    }
}

AcDisplayItem* ksCockpitInstruments::getAcItem(int index) {
    if (index >= 0 && index < m_acItems.size()) return &m_acItems[index];
    return nullptr;
}

AcLed* ksCockpitInstruments::getAcLed(int index) {
    if (index >= 0 && index < m_acLeds.size()) return &m_acLeds[index];
    return nullptr;
}

void ksCockpitInstruments::clearAcItems() { m_acItems.clear(); emit displayChanged(); }
void ksCockpitInstruments::clearAcLeds() { m_acLeds.clear(); emit displayChanged(); }
void ksCockpitInstruments::clearAcTyreSlips() { m_acTyreSlips.clear(); emit displayChanged(); }

QString ksCockpitInstruments::generateAcIniContent() const
{
    QString ini;
    QTextStream out(&ini);

    for (const auto& serie : m_acKersSeries) {
        out << "[KERS_CHARGE_SERIE_" << serie.index << "]\n";
        out << "PREFIX=" << serie.prefix << "\n";
        out << "START_INDEX=" << serie.startIndex << "\n";
        out << "END_INDEX=" << serie.endIndex << "\n\n";
    }

    for (const auto& item : m_acItems) {
        out << "[ITEM_" << item.index << "]\n";
        out << "TYPE=" << acItemTypeToString(item.type) << "\n";
        out << "PARENT=" << (item.parent == AcDisplayParent::DISPLAY_WHEEL ? "DISPLAY_WHEEL" : "DISPLAY_MONITOR") << "\n";
        if (!item.objectName.isEmpty()) out << "OBJECT_NAME=" << item.objectName << "\n";
        out << "POSITION=(" << item.position.x() << "," << item.position.y() << ",0)\n";
        out << "SIZE=(" << item.size.width() << "," << item.size.height() << ")\n";
        out << "COLOR=" << item.color.red() << "," << item.color.green() << "," << item.color.blue() << "\n";
        out << "INTENSITY=" << item.intensity << "\n";
        out << "FONT=" << item.font << "\n";
        out << "VERSION=" << item.version << "\n";
        out << "ALIGN=" << item.align << "\n";
        out << "DECIMALS=" << item.decimals << "\n";
        if (!item.prefix.isEmpty()) out << "PREFIX=" << item.prefix << "\n";
        if (!item.postfix.isEmpty()) out << "POSTFIX=" << item.postfix << "\n";
        if (!item.units.isEmpty()) out << "UNITS=" << item.units << "\n";
        if (item.tyreNumber >= 0) out << "TYRE_NUMBER=" << item.tyreNumber << "\n";
        out << "N_TIME=" << item.nTime << "\n";
        if (item.rpmTrigger > 0) {
            out << "COLOR_2=" << item.color2.red() << "," << item.color2.green() << "," << item.color2.blue() << "\n";
            out << "INTENSITY_2=" << item.intensity2 << "\n";
            out << "RPM_TRIGGER=" << item.rpmTrigger << "\n";
        }
        out << "\n";
    }

    for (const auto& led : m_acLeds) {
        out << "[LED_" << led.index << "]\n";
        out << "OBJECT_NAME=" << led.objectName << "\n";
        out << "RPM_SWITCH=" << led.rpmSwitch << "\n";
        out << "EMISSIVE=" << led.emissive.red() << "," << led.emissive.green() << "," << led.emissive.blue() << "\n";
        out << "DIFFUSE=" << led.diffuse << "\n";
        if (led.blinkSwitch > 0) {
            out << "BLINK_SWITCH=" << led.blinkSwitch << "\n";
            out << "BLINK_HZ=" << led.blinkHz << "\n";
        }
        out << "\n";
    }

    for (const auto& slip : m_acTyreSlips) {
        out << "[TYRE_LOCK_SLIP_" << slip.index << "]\n";
        out << "OBJECT_NAME=" << slip.objectName << "\n";
        out << "TYRE_INDEX=" << slip.tyreIndex << "\n";
        out << "EMISSIVE=" << slip.emissive.red() << "," << slip.emissive.green() << "," << slip.emissive.blue() << "\n";
        out << "EMISSIVE_LOCK=" << slip.emissiveLock.red() << "," << slip.emissiveLock.green() << "," << slip.emissiveLock.blue() << "\n";
        out << "DIFFUSE=" << slip.diffuse << "\n";
        out << "SLIP_SWITCH=" << slip.slipSwitch << "\n";
        out << "SHOW_LOCK=" << (slip.showLock ? "1" : "0") << "\n";
        out << "WHEEL_SPEED_MULT=" << slip.wheelSpeedMult << "\n\n";
    }

    return ini;
}

// ============================================================================
// Animation System
// ============================================================================

void ksCockpitInstruments::setAnimation(const QString& elementId, const AnimationConfig& config) {
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

void ksCockpitInstruments::removeAnimation(const QString& elementId) {
    m_animations.remove(elementId);
    emit animationStopped(elementId);
}

void ksCockpitInstruments::setPhysicsValue(const QString& elementId, double value) {
    if (m_animations.contains(elementId)) {
        m_animations[elementId].physicsValue = value;
    }
}

void ksCockpitInstruments::pauseAnimations() {
    m_animationsPaused = true;
}

void ksCockpitInstruments::resumeAnimations() {
    m_animationsPaused = false;
}

void ksCockpitInstruments::stopAllAnimations() {
    m_animations.clear();
    m_animationTimer->stop();
}

AnimationState ksCockpitInstruments::getAnimationState(const QString& elementId) const {
    return m_animations.value(elementId);
}

void ksCockpitInstruments::updatePhysicsValue(DataSource source, double value) {
    m_physicsValues[source] = value;
}

double ksCockpitInstruments::getPhysicsValue(DataSource source) const {
    return m_physicsValues.value(source, 0.0);
}

void ksCockpitInstruments::setPhysicsUpdateInterval(int ms) {
    m_physicsUpdateIntervalMs = ms;
}

void ksCockpitInstruments::onAnimationTick() {
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

void ksCockpitInstruments::updateAnimationState(AnimationState& state, double dtMs) {
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

QPointF ksCockpitInstruments::computeSlideOffset(const AnimationConfig& cfg, double progress) {
    return cfg.slideOffset * (1.0 - progress);
}

double ksCockpitInstruments::computeEasedValue(const AnimationConfig& cfg, double progress) {
    QEasingCurve easing(cfg.easing);
    return easing.valueForProgress(progress);
}

QColor ksCockpitInstruments::interpolateColor(const QColor& from, const QColor& to, double t) {
    return QColor(
        static_cast<int>(from.red() + (to.red() - from.red()) * t),
        static_cast<int>(from.green() + (to.green() - from.green()) * t),
        static_cast<int>(from.blue() + (to.blue() - from.blue()) * t),
        static_cast<int>(from.alpha() + (to.alpha() - from.alpha()) * t)
    );
}
