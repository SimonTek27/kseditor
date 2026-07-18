#include "LicensePlateGenerator.h"
#include <QPainter>
#include <QImage>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QUuid>
#include <algorithm>

namespace ks {
namespace license {

LicensePlateGenerator::LicensePlateGenerator(QObject* parent)
    : QObject(parent)
{
    initializeTemplates();
    initializeCountryData();
}

LicensePlateGenerator::~LicensePlateGenerator() = default;

void LicensePlateGenerator::initializeTemplates() {
    // Germany (DE)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Germany;
        tmpl.name = "Germany";
        tmpl.pattern = "^[A-Z]{1,3}-[A-Z]{1,2}\\d{1,4}$";
        tmpl.formatTemplate = "{region}-{letters}{numbers}";
        tmpl.regionCode = "DE";
        tmpl.regionCodes = {"B", "M", "HH", "HB", "S", "BY", "BW", "BE", "BB", "RP", "SL", "SN", "ST", "SH", "TH", "MV", "NI", "NW"};
        tmpl.fontFamily = "FE-Schrift";
        tmpl.fontSize = 72;
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "D";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "German license plate (DIN 1451 FE-Schrift)";
        m_templates[CountryCode::Germany] = tmpl;
    }
    
    // France (FR)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::France;
        tmpl.name = "France";
        tmpl.pattern = "^[A-Z]{2}-\\d{3}-[A-Z]{2}$";
        tmpl.formatTemplate = "{letters}-\\d{3}-{letters}";
        tmpl.regionCode = "FR";
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "F";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.fontFamily = "FE-Schrift";
        tmpl.description = "French license plate (SIV format)";
        m_templates[CountryCode::France] = tmpl;
    }
    
    // Italy (IT)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Italy;
        tmpl.name = "Italy";
        tmpl.pattern = "^[A-Z]{2}\\d{3}[A-Z]{2}$";
        tmpl.formatTemplate = "{letters}\\d{3}{letters}";
        tmpl.regionCode = "IT";
        tmpl.regionCodes = {"MI", "RM", "NA", "TO", "PD", "BO", "FI", "VE", "GE", "BA"};
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "I";
        tmpl.plateWidth = 360;
        tmpl.plateHeight = 110;
        tmpl.description = "Italian license plate (current format)";
        m_templates[CountryCode::Italy] = tmpl;
    }
    
    // Spain (ES)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Spain;
        tmpl.name = "Spain";
        tmpl.pattern = "^\\d{4}[A-Z]{3}$";
        tmpl.formatTemplate = "\\d{4}{letters}";
        tmpl.regionCode = "ES";
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "E";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Spanish license plate (current format)";
        m_templates[CountryCode::Spain] = tmpl;
    }
    
    // Netherlands (NL)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Netherlands;
        tmpl.name = "Netherlands";
        tmpl.pattern = "^\\d{2}-[A-Z]{2}-\\d{2}$|^[A-Z]{2}-\\d{2}-[A-Z]{2}$|^\\d{2}-[A-Z]{3}-\\d{1}$";
        tmpl.formatTemplate = "\\d{2}-{letters}-\\d{2}";
        tmpl.regionCode = "NL";
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "NL";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Dutch license plate (side code format)";
        m_templates[CountryCode::Netherlands] = tmpl;
    }
    
    // Belgium (BE)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Belgium;
        tmpl.name = "Belgium";
        tmpl.pattern = "^[A-Z]{3}-\\d{3}$";
        tmpl.formatTemplate = "{letters}-\\d{3}";
        tmpl.regionCode = "BE";
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "B";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Belgian license plate (current format)";
        m_templates[CountryCode::Belgium] = tmpl;
    }
    
    // Austria (AT)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Austria;
        tmpl.name = "Austria";
        tmpl.pattern = "^[A-Z]{1,2}\\s?[A-Z]{1,2}\\s?\\d{1,4}$";
        tmpl.formatTemplate = "{region} {letters}{numbers}";
        tmpl.regionCode = "AT";
        tmpl.regionCodes = {"W", "G", "S", "T", "K", "O", "V", "B", "E"};
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "A";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Austrian license plate";
        m_templates[CountryCode::Austria] = tmpl;
    }
    
    // Poland (PL)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Poland;
        tmpl.name = "Poland";
        tmpl.pattern = "^[A-Z]{2,3}\\s?\\d{1,4}[A-Z]?$";
        tmpl.formatTemplate = "{region} {numbers}{letter}";
        tmpl.regionCode = "PL";
        tmpl.regionCodes = {"WA", "KR", "PO", "GD", "LU", "BI", "BY", "CZ", "EL", "KI", "KL", "OP", "PK", "SL", "SZ", "WB", "WR"};
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "PL";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Polish license plate";
        m_templates[CountryCode::Poland] = tmpl;
    }
    
    // Sweden (SE)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Sweden;
        tmpl.name = "Sweden";
        tmpl.pattern = "^[A-Z]{3}\\s?\\d{3}$";
        tmpl.formatTemplate = "{letters} {numbers}";
        tmpl.regionCode = "SE";
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "S";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Swedish license plate";
        m_templates[CountryCode::Sweden] = tmpl;
    }
    
    // Finland (FI)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Finland;
        tmpl.name = "Finland";
        tmpl.pattern = "^[A-Z]{2,3}-\\d{3}$";
        tmpl.formatTemplate = "{letters}-\\d{3}";
        tmpl.regionCode = "FI";
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "FIN";
        tmpl.plateWidth = 440;
        tmpl.plateHeight = 110;
        tmpl.description = "Finnish license plate";
        m_templates[CountryCode::Finland] = tmpl;
    }
    
    // Denmark (DK)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Denmark;
        tmpl.name = "Denmark";
        tmpl.pattern = "^[A-Z]{2}\\s?\\d{5}$";
        tmpl.formatTemplate = "{letters} {numbers}";
        tmpl.regionCode = "DK";
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "DK";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Danish license plate";
        m_templates[CountryCode::Denmark] = tmpl;
    }
    
    // Ireland (IE)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Ireland;
        tmpl.name = "Ireland";
        tmpl.pattern = "^\\d{2}-[A-Z]{1,2}-\\d{1,6}$";
        tmpl.formatTemplate = "\\d{2}-{region}-\\d{1,6}";
        tmpl.regionCode = "IE";
        tmpl.regionCodes = {"D", "C", "G", "L", "LK", "LS", "MH", "MN", "MO", "OY", "RN", "SO", "TA", "WD", "WH", "WW", "WX"};
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "IRL";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Irish license plate";
        m_templates[CountryCode::Ireland] = tmpl;
    }
    
    // Portugal (PT)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Portugal;
        tmpl.name = "Portugal";
        tmpl.pattern = "^[A-Z]{2}-\\d{2}-[A-Z]{2}$";
        tmpl.formatTemplate = "{letters}-\\d{2}-{letters}";
        tmpl.regionCode = "PT";
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "P";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Portuguese license plate";
        m_templates[CountryCode::Portugal] = tmpl;
    }
    
    // Greece (GR)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Greece;
        tmpl.name = "Greece";
        tmpl.pattern = "^[A-Z]{3}-\\d{4}$";
        tmpl.formatTemplate = "{letters}-\\d{4}";
        tmpl.regionCode = "GR";
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "GR";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Greek license plate";
        m_templates[CountryCode::Greece] = tmpl;
    }
    
    // Czech Republic (CZ)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::CzechRepublic;
        tmpl.name = "Czech Republic";
        tmpl.pattern = "^[A-Z]{1}[A-Z]{2}\\s?\\d{3,4}$";
        tmpl.formatTemplate = "{region}{letters} {numbers}";
        tmpl.regionCode = "CZ";
        tmpl.regionCodes = {"A", "B", "C", "E", "H", "J", "K", "L", "M", "P", "S", "T", "U", "Z"};
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "CZ";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Czech license plate";
        m_templates[CountryCode::CzechRepublic] = tmpl;
    }
    
    // Hungary (HU)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Hungary;
        tmpl.name = "Hungary";
        tmpl.pattern = "^[A-Z]{3}-\\d{3}$";
        tmpl.formatTemplate = "{letters}-\\d{3}";
        tmpl.regionCode = "HU";
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "H";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Hungarian license plate";
        m_templates[CountryCode::Hungary] = tmpl;
    }
    
    // Romania (RO)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Romania;
        tmpl.name = "Romania";
        tmpl.pattern = "^[A-Z]{1,2}\\s?\\d{2,3}[A-Z]{3}$";
        tmpl.formatTemplate = "{region} {numbers}{letters}";
        tmpl.regionCode = "RO";
        tmpl.regionCodes = {"B", "AB", "AR", "AG", "BC", "BH", "BN", "BR", "BT", "BV", "BZ", "CJ", "CL", "CS", "CT", "CV", "DB", "DJ", "GJ", "GL", "GR", "HD", "HR", "IF", "IL", "IS", "MH", "MM", "MS", "NT", "OT", "PH", "SB", "SJ", "SM", "SV", "TL", "TM", "TR", "VL", "VN", "VS"};
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "RO";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Romanian license plate";
        m_templates[CountryCode::Romania] = tmpl;
    }
    
    // Slovakia (SK)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Slovakia;
        tmpl.name = "Slovakia";
        tmpl.pattern = "^[A-Z]{2}\\s?\\d{3}[A-Z]{2}$";
        tmpl.formatTemplate = "{region} {numbers}{letters}";
        tmpl.regionCode = "SK";
        tmpl.regionCodes = {"BA", "BB", "BY", "CA", "DN", "GA", "HC", "HE", "IL", "KE", "KK", "KN", "LC", "LE", "LM", "LV", "MA", "MI", "ML", "MT", "NI", "NM", "NO", "NR", "NV", "OK", "PE", "PK", "PN", "PO", "PP", "PV", "RA", "RK", "RN", "RV", "SA", "SE", "SI", "SK", "SP", "SV", "TN", "TO", "TR", "TV", "TT", "TV", "ZA", "ZM", "ZV"};
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "SK";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Slovak license plate";
        m_templates[CountryCode::Slovakia] = tmpl;
    }
    
    // Slovenia (SI)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Slovenia;
        tmpl.name = "Slovenia";
        tmpl.pattern = "^[A-Z]{2}\\s?\\d{3,4}[A-Z]?$";
        tmpl.formatTemplate = "{region} {numbers}{letter}";
        tmpl.regionCode = "SI";
        tmpl.regionCodes = {"LJ", "MB", "KP", "KR", "MS", "NM", "SG", "MS"};
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "SLO";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Slovenian license plate";
        m_templates[CountryCode::Slovenia] = tmpl;
    }
    
    // Croatia (HR)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Croatia;
        tmpl.name = "Croatia";
        tmpl.pattern = "^[A-Z]{2}\\s?\\d{3,4}[A-Z]?$";
        tmpl.formatTemplate = "{region} {numbers}{letter}";
        tmpl.regionCode = "HR";
        tmpl.regionCodes = {"ZG", "ST", "RI", "PU", "OS", "SK", "ZD", "DU", "VK", "GS", "IM", "KA", "KS", "VU", "VT", "ZD"};
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "HR";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Croatian license plate";
        m_templates[CountryCode::Croatia] = tmpl;
    }
    
    // Bulgaria (BG)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Bulgaria;
        tmpl.name = "Bulgaria";
        tmpl.pattern = "^[A-Z]{1,2}\\s?\\d{4}[A-Z]{2}$";
        tmpl.formatTemplate = "{region} {numbers}{letters}";
        tmpl.regionCode = "BG";
        tmpl.regionCodes = {"A", "B", "V", "VT", "BP", "VL", "GR", "DO", "KN", "KH", "KZ", "LO", "LOV", "MO", "PZ", "PA", "PP", "RA", "RZ", "RR", "SI", "SM", "SN", "SO", "SS", "ST", "T", "TX", "HA", "HK", "HS", "Y", "YA", "E", "EB", "EH", "OB", "OH", "OK", "CH", "C", "CC", "CA", "CB", "E", "EN", "RK", "P", "PB", "PK", "RN", "S", "SA", "SL", "SO", "SM", "ST", "T", "H", "HA", "X", "K"};
        tmpl.useEUStrip = true;
        tmpl.euCountryCode = "BG";
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Bulgarian license plate";
        m_templates[CountryCode::Bulgaria] = tmpl;
    }
    
    // UK
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::UK;
        tmpl.name = "United Kingdom";
        tmpl.pattern = "^[A-Z]{2}\\d{2}\\s?[A-Z]{3}$";
        tmpl.formatTemplate = "{region}{numbers} {letters}";
        tmpl.regionCode = "GB";
        tmpl.useEUStrip = false;
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 111;
        tmpl.backgroundColor = Qt::white;
        tmpl.textColor = Qt::black;
        tmpl.description = "UK license plate (current format)";
        m_templates[CountryCode::UK] = tmpl;
    }
    
    // Switzerland
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Switzerland;
        tmpl.name = "Switzerland";
        tmpl.pattern = "^[A-Z]{2}\\s?\\d{1,6}$";
        tmpl.formatTemplate = "{region} {numbers}";
        tmpl.regionCode = "CH";
        tmpl.regionCodes = {"ZH", "BE", "LU", "UR", "SZ", "OW", "NW", "GL", "ZG", "FR", "SO", "BS", "BL", "SH", "AR", "AI", "SG", "GR", "AG", "TG", "TI", "VD", "VS", "NE", "GE", "JU"};
        tmpl.useEUStrip = false;
        tmpl.plateWidth = 300;
        tmpl.plateHeight = 110;
        tmpl.description = "Swiss license plate";
        m_templates[CountryCode::Switzerland] = tmpl;
    }
    
    // Norway
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Norway;
        tmpl.name = "Norway";
        tmpl.pattern = "^[A-Z]{2}\\s?\\d{5}$";
        tmpl.formatTemplate = "{letters} {numbers}";
        tmpl.regionCode = "NO";
        tmpl.useEUStrip = false;
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Norwegian license plate";
        m_templates[CountryCode::Norway] = tmpl;
    }
    
    // USA (simplified - varies by state)
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::USA;
        tmpl.name = "USA (Standard)";
        tmpl.pattern = "^[A-Z]{3}\\d{4}$";
        tmpl.formatTemplate = "{letters}{numbers}";
        tmpl.regionCode = "US";
        tmpl.useEUStrip = false;
        tmpl.plateWidth = 305;
        tmpl.plateHeight = 152;
        tmpl.description = "US license plate (standard format)";
        m_templates[CountryCode::USA] = tmpl;
    }
    
    // Japan
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Japan;
        tmpl.name = "Japan";
        tmpl.pattern = "^[A-Z]{1}\\d{2,3}[A-Z]{1}\\d{2,4}$";
        tmpl.formatTemplate = "{prefecture}{class}{hiragana}{numbers}";
        tmpl.regionCode = "JP";
        tmpl.useEUStrip = false;
        tmpl.plateWidth = 330;
        tmpl.plateHeight = 165;
        tmpl.description = "Japanese license plate";
        m_templates[CountryCode::Japan] = tmpl;
    }
    
    // Custom template
    {
        LicensePlateTemplate tmpl;
        tmpl.country = CountryCode::Custom;
        tmpl.name = "Custom Template";
        tmpl.pattern = ".*";
        tmpl.formatTemplate = "{custom}";
        tmpl.regionCode = "XX";
        tmpl.useEUStrip = false;
        tmpl.plateWidth = 520;
        tmpl.plateHeight = 110;
        tmpl.description = "Custom license plate template";
        m_templates[CountryCode::Custom] = tmpl;
    }
}

void LicensePlateGenerator::initializeCountryData() {
    m_countryData[CountryCode::Germany] = {"Germany", "DE", "D", {"B", "M", "HH", "HB", "S", "BY", "BW", "BE", "BB", "RP", "SL", "SN", "ST", "SH", "TH", "MV", "NI", "NW"}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::France] = {"France", "FR", "F", {}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Italy] = {"Italy", "IT", "I", {"MI", "RM", "NA", "TO", "PD", "BO", "FI", "VE", "GE", "BA"}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Spain] = {"Spain", "ES", "E", {}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Netherlands] = {"Netherlands", "NL", "NL", {}, "FE-Schrift", QColor(Qt::yellow).darker(120), Qt::black, true};
    m_countryData[CountryCode::Belgium] = {"Belgium", "BE", "B", {}, "FE-Schrift", Qt::red, Qt::white, true};
    m_countryData[CountryCode::Austria] = {"Austria", "AT", "A", {"W", "G", "S", "T", "K", "O", "V", "B", "E"}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Poland] = {"Poland", "PL", "PL", {"WA", "KR", "PO", "GD", "LU", "BI", "BY", "CZ", "EL", "KI", "KL", "OP", "PK", "SL", "SZ", "WB", "WR"}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Sweden] = {"Sweden", "SE", "S", {}, "FE-Schrift", Qt::blue, Qt::yellow, true};
    m_countryData[CountryCode::Finland] = {"Finland", "FI", "FIN", {}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Denmark] = {"Denmark", "DK", "DK", {}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Ireland] = {"Ireland", "IE", "IRL", {"D", "C", "G", "L", "LK", "LS", "MH", "MN", "MO", "OY", "RN", "SO", "TA", "WD", "WH", "WW", "WX"}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Portugal] = {"Portugal", "PT", "P", {}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Greece] = {"Greece", "GR", "GR", {}, "FE-Schrift", Qt::white, Qt::blue, true};
    m_countryData[CountryCode::CzechRepublic] = {"Czech Republic", "CZ", "CZ", {"A", "B", "C", "E", "H", "J", "K", "L", "M", "P", "S", "T", "U", "Z"}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Hungary] = {"Hungary", "HU", "H", {}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Romania] = {"Romania", "RO", "RO", {"B", "AB", "AR", "AG", "BC", "BH", "BN", "BR", "BT", "BV", "BZ", "CJ", "CL", "CS", "CT", "CV", "DB", "DJ", "GJ", "GL", "GR", "HD", "HR", "IF", "IL", "IS", "MH", "MM", "MS", "NT", "OT", "PH", "SB", "SJ", "SM", "SV", "TL", "TM", "TR", "VL", "VN", "VS"}, "FE-Schrift", Qt::blue, Qt::yellow, true};
    m_countryData[CountryCode::Slovakia] = {"Slovakia", "SK", "SK", {"BA", "BB", "BY", "CA", "DN", "GA", "HC", "HE", "IL", "KE", "KK", "KN", "LC", "LE", "LM", "LV", "MA", "MI", "ML", "MT", "NI", "NM", "NO", "NR", "NV", "OK", "PE", "PK", "PN", "PO", "PP", "PV", "RA", "RK", "RN", "RV", "SA", "SE", "SI", "SK", "SP", "SV", "TN", "TO", "TR", "TV", "TT", "TV", "ZA", "ZM", "ZV"}, "FE-Schrift", Qt::white, Qt::red, true};
    m_countryData[CountryCode::Slovenia] = {"Slovenia", "SI", "SLO", {"LJ", "MB", "KP", "KR", "MS", "NM", "SG"}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Croatia] = {"Croatia", "HR", "HR", {"ZG", "ST", "RI", "PU", "OS", "SK", "ZD", "DU", "VK", "GS", "IM", "KA", "KS", "VU", "VT", "ZD"}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::Bulgaria] = {"Bulgaria", "BG", "BG", {"A", "B", "V", "VT", "BP", "VL", "GR", "DO", "KN", "KH", "KZ", "LO", "LOV", "MO", "PZ", "PA", "PP", "RA", "RZ", "RR", "SI", "SM", "SN", "SO", "SS", "ST", "T", "TX", "HA", "HK", "HS", "Y", "YA", "E", "EB", "EH", "OB", "OH", "OK", "CH", "C", "CC", "CA", "CB", "E", "EN", "RK", "P", "PB", "PK", "RN", "S", "SA", "SL", "SO", "SM", "ST", "T", "H", "HA", "X", "K"}, "FE-Schrift", Qt::white, Qt::black, true};
    m_countryData[CountryCode::UK] = {"United Kingdom", "GB", "GB", {}, "Charles Wright", Qt::white, Qt::black, false};
    m_countryData[CountryCode::Switzerland] = {"Switzerland", "CH", "CH", {"ZH", "BE", "LU", "UR", "SZ", "OW", "NW", "GL", "ZG", "FR", "SO", "BS", "BL", "SH", "AR", "AI", "SG", "GR", "AG", "TG", "TI", "VD", "VS", "NE", "GE", "JU"}, "FE-Schrift", Qt::white, Qt::red, false};
    m_countryData[CountryCode::Norway] = {"Norway", "NO", "NO", {}, "FE-Schrift", Qt::white, Qt::black, false};
    m_countryData[CountryCode::USA] = {"USA", "US", "US", {}, "Standard", Qt::white, Qt::black, false};
    m_countryData[CountryCode::Japan] = {"Japan", "JP", "JP", {}, "Japanese", Qt::white, Qt::green, false};
    m_countryData[CountryCode::Custom] = {"Custom", "XX", "XX", {}, "Arial", Qt::white, Qt::black, false};
}

void LicensePlateGenerator::registerTemplate(const LicensePlateTemplate& template_) {
    m_templates[template_.country] = template_;
}

void LicensePlateGenerator::unregisterTemplate(CountryCode country) {
    m_templates.remove(country);
}

LicensePlateTemplate LicensePlateGenerator::getTemplate(CountryCode country) const {
    return m_templates.value(country, LicensePlateTemplate());
}

QVector<CountryCode> LicensePlateGenerator::availableCountries() const {
    return m_templates.keys();
}

LicensePlate LicensePlateGenerator::generatePlate(CountryCode country, const QString& region, const QString& seed) {
    auto tmpl = getTemplate(country);
    if (tmpl.country == CountryCode::Custom && tmpl.pattern.isEmpty()) {
        LicensePlate empty;
        empty.country = country;
        return empty;
    }
    return generateFromTemplate(tmpl, region, seed);
}

LicensePlate LicensePlateGenerator::generatePlate(const LicensePlateTemplate& template_, const QString& region, const QString& seed) {
    return generateFromTemplate(template_, region, seed);
}

QVector<LicensePlate> LicensePlateGenerator::generateBatch(const BatchGenerationOptions& options) {
    QVector<LicensePlate> plates;
    plates.reserve(options.count);
    
    emit batchProgress(0, options.count);
    
    for (int i = 0; i < options.count; ++i) {
        QString currentRegion = options.region;
        if (currentRegion.isEmpty() && !m_templates[options.country].regionCodes.isEmpty()) {
            const auto& regions = m_templates[options.country].regionCodes;
            currentRegion = regions[QRandomGenerator::global()->bounded(regions.size())];
        }
        
        QString seed = QString::number(options.seed);
        if (options.sequential) {
            seed = QString::number(options.startNumber + i);
        }
        
        LicensePlate plate = generatePlate(options.country, options.region.isEmpty() ? QString() : options.region, seed);
        
        if (!options.prefix.isEmpty()) {
            plate.plateText = options.prefix + plate.plateText;
        }
        if (!options.suffix.isEmpty()) {
            plate.plateText = plate.plateText + options.suffix;
        }
        
        plate.generatedAt = QDateTime::currentDateTime();
        plate.metadata = options.customMetadata;
        
        plates.append(plate);
        emit plateGenerated(plate);
        emit batchProgress(i + 1, options.count);
    }
    
    emit batchCompleted(plates);
    return plates;
}

bool LicensePlateGenerator::generateBatchToFiles(const BatchGenerationOptions& options) {
    QVector<LicensePlate> plates = generateBatch(options);
    
    QDir dir(options.outputDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit generationError("Failed to create output directory: " + options.outputDir);
            return false;
        }
    }
    
    for (int i = 0; i < plates.size(); ++i) {
        QString fileName = QString("%1.%2").arg(plates[i].plateText).arg(options.outputFormat);
        QString filePath = QDir(options.outputDir).filePath(fileName);
        
        QImage img = renderPlate(plates[i], 520, 110, options.dpi);
        
        if (options.addWeathering) {
            img = applyWeathering(img, options.weatheringIntensity);
        }
        if (options.addReflection) {
            img = applyReflection(img);
        }
        
        if (!img.save(filePath, options.outputFormat.toUpper().toLatin1().constData())) {
            emit generationError("Failed to save: " + filePath);
            return false;
        }
        
        emit plateSaved(filePath);
    }
    
    return true;
}

bool LicensePlateGenerator::validatePlate(const QString& plateText, CountryCode country, QString* error) const {
    auto tmpl = getTemplate(country);
    if (tmpl.country == CountryCode::Custom && tmpl.pattern.isEmpty()) {
        if (error) *error = "No template for country";
        return false;
    }
    
    QRegularExpression re(tmpl.pattern);
    QRegularExpressionMatch match = re.match(plateText);
    
    if (!match.hasMatch()) {
        if (error) *error = QString("Plate '%1' does not match pattern for %2").arg(plateText).arg(countryCodeToString(country));
        return false;
    }
    return true;
}

bool LicensePlateGenerator::validateRegion(const QString& region, CountryCode country) const {
    auto tmpl = getTemplate(country);
    return tmpl.regionCodes.contains(region);
}

QString LicensePlateGenerator::normalizePlate(const QString& plateText, CountryCode country) const {
    QString normalized = plateText.toUpper().remove(QRegularExpression("[^A-Z0-9]"));
    
    auto tmpl = getTemplate(country);
    if (tmpl.country == CountryCode::Custom) return normalized;
    
    // Try to format according to template
    QRegularExpression re(tmpl.pattern);
    QRegularExpressionMatch match = re.match(normalized);
    
    if (match.hasMatch()) {
        // Already matches pattern
        return normalized;
    }
    
    // Try to extract parts and reformat
    QString lettersOnly = normalized.remove(QRegularExpression("[^A-Z]"));
    QString numbersOnly = normalized.remove(QRegularExpression("[^0-9]"));
    
    // Simple reformat attempt
    if (country == CountryCode::Germany && lettersOnly.length() >= 1 && numbersOnly.length() >= 1) {
        return "B-" + lettersOnly.left(2) + numbersOnly.left(4);
    }
    
    return normalized;
}

QImage LicensePlateGenerator::renderPlate(const LicensePlate& plate, int width, int height, int dpi) const {
    auto tmpl = getTemplate(plate.country);
    
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(tmpl.backgroundColor);
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    // Draw EU strip if enabled
    if (tmpl.useEUStrip && !tmpl.euCountryCode.isEmpty()) {
        QImage euStrip = renderEUStrip(tmpl.euCountryCode, width, height / 5);
        painter.drawImage(0, 0, euStrip);
    }
    
    // Draw plate border
    QPen borderPen(tmpl.borderColor, 3);
    painter.setPen(borderPen);
    painter.drawRect(2, tmpl.useEUStrip ? height / 5 : 2, width - 4, height - 4 - (tmpl.useEUStrip ? height / 5 : 0));
    
    // Render text
    QFont font(tmpl.fontFamily, tmpl.fontSize, QFont::Bold);
    if (!QFontDatabase::hasFamily(tmpl.fontFamily)) {
        font = QFont("Arial", tmpl.fontSize, QFont::Bold);
    }
    painter.setFont(font);
    painter.setPen(tmpl.textColor);
    
    QRect textRect(10, tmpl.useEUStrip ? height / 5 + 10 : 10, width - 20, height - 20 - (tmpl.useEUStrip ? height / 5 : 0));
    painter.drawText(textRect, Qt::AlignCenter, plate.formattedText.isEmpty() ? plate.plateText : plate.formattedText);
    
    // Apply weathering if metadata indicates
    if (plate.metadata.value("weathering", false).toBool()) {
        float intensity = plate.metadata.value("weathering_intensity", 0.3).toFloat();
        return applyWeathering(image, intensity);
    }
    
    return image;
}

bool LicensePlateGenerator::savePlateImage(const LicensePlate& plate, const QString& filePath, int dpi) const {
    QImage image = renderPlate(plate, 520, 110, dpi);
    return image.save(filePath);
}

QImage LicensePlateGenerator::applyWeathering(const QImage& plate, float intensity) const {
    return applyDirt(applyScratches(plate, intensity * 0.5f), intensity);
}

QImage LicensePlateGenerator::applyReflection(const QImage& plate) const {
    QImage result = plate;
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Add subtle reflection gradient at top
    QLinearGradient gradient(0, 0, 0, plate.height() / 3);
    gradient.setColorAt(0, QColor(255, 255, 255, 30));
    gradient.setColorAt(1, QColor(255, 255, 255, 0));
    painter.fillRect(0, 0, plate.width(), plate.height() / 3, gradient);
    
    return result;
}

QImage LicensePlateGenerator::renderEUStrip(const QString& countryCode, int width, int height) const {
    QImage strip(width, height, QImage::Format_ARGB32);
    strip.fill(Qt::transparent);
    
    QPainter painter(&strip);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // EU blue background
    QColor euBlue(0, 51, 153);
    painter.fillRect(0, 0, width, height, euBlue);
    
    // 12 stars in a circle
    QFont starFont("Arial", height * 0.4, QFont::Bold);
    painter.setFont(starFont);
    painter.setPen(Qt::white);
    
    int centerX = width - height / 2;
    int centerY = height / 2;
    int radius = height * 0.35;
    
    for (int i = 0; i < 12; ++i) {
        double angle = i * 30.0 * M_PI / 180.0 - M_PI / 2;  // Start at top
        int x = centerX + radius * cos(angle);
        int y = centerY + radius * sin(angle);
        
        painter.save();
        painter.translate(x, y);
        painter.rotate(i * 30.0);
        painter.drawText(-height/4, height/4, "★");
        painter.restore();
    }
    
    // Country code
    QFont codeFont("Arial", height * 0.35, QFont::Bold);
    painter.setFont(codeFont);
    painter.setPen(Qt::white);
    painter.drawText(width - height, 0, height, height, Qt::AlignCenter, countryCode);
    
    return strip;
}

LicensePlate LicensePlateGenerator::generateFromTemplate(const LicensePlateTemplate& tmpl, const QString& region, const QString& seed) {
    LicensePlate plate;
    plate.country = tmpl.country;
    plate.region = region.isEmpty() && !tmpl.regionCodes.isEmpty() 
        ? tmpl.regionCodes[QRandomGenerator::global()->bounded(tmpl.regionCodes.size())]
        : region;
    plate.seed = seed.isEmpty() ? QUuid::createUuid().toString() : seed;
    plate.generatedAt = QDateTime::currentDateTime();
    
    // Use seed for reproducible generation
    QRandomGenerator gen(seed.isEmpty() ? QRandomGenerator::global()->generate() : seed.toUInt(nullptr, 16));
    
    plate.plateText = generateRandomPlate(tmpl, plate.region);
    plate.formattedText = applyPattern(tmpl.formatTemplate, {
        {"region", plate.region},
        {"letters", genRandomLetters(2, gen)},
        {"numbers", genRandomNumbers(4, gen)},
        {"number", genRandomNumbers(3, gen)},
        {"prefecture", genRandomPrefecture(gen)},
        {"class", genRandomClass(gen)},
        {"hiragana", genRandomHiragana(gen)}
    });
    
    return plate;
}

QString LicensePlateGenerator::generateRandomPlate(const LicensePlateTemplate& tmpl, const QString& region) const {
    QRandomGenerator* gen = QRandomGenerator::global();
    QMap<QString, QString> replacements;
    replacements["region"] = region;
    replacements["letters"] = genRandomLetters(2, *gen);
    replacements["numbers"] = genRandomNumbers(4, *gen);
    replacements["number"] = genRandomNumbers(3, *gen);
    replacements["prefecture"] = genRandomPrefecture(*gen);
    replacements["class"] = genRandomClass(*gen);
    replacements["hiragana"] = genRandomHiragana(*gen);
    
    return applyPattern(tmpl.formatTemplate, replacements);
}

QString LicensePlateGenerator::genRandomLetters(int count, QRandomGenerator& gen) const {
    QString result;
    for (int i = 0; i < count; ++i) {
        result += QChar('A' + gen.bounded(26));
    }
    return result;
}

QString LicensePlateGenerator::genRandomNumbers(int count, QRandomGenerator& gen) const {
    QString result;
    for (int i = 0; i < count; ++i) {
        result += QString::number(gen.bounded(10));
    }
    return result;
}

QString LicensePlateGenerator::genRandomPrefecture(QRandomGenerator& gen) const {
    static const QStringList prefectures = {
        "TK", "OS", "KN", "FO", "SI", "IS", "KY", "HG", "YM", "NR",
        "TKS", "FUK", "OKI", "KAG", "MIY", "KUM", "OIT", "NGS", "SAG", "KTK",
        "YMT", "FUKU", "TOK", "KANA", "SAI", "CHI", "IBK", "TOC", "GUN", "TOCH"
    };
    return prefectures[gen.bounded(prefectures.size())];
}

QString LicensePlateGenerator::genRandomClass(QRandomGenerator& gen) const {
    static const QStringList classes = {"あ", "い", "う", "え", "か", "き", "く", "け", "こ", "さ", "し", "す", "せ", "そ", "た", "ち", "つ", "て", "と", "な", "に", "ぬ", "ね", "の", "は", "ひ", "ふ", "へ", "ほ", "ま", "み", "む", "め", "も", "や", "ゆ", "よ", "ら", "り", "る", "れ", "ろ", "わ", "れ", "ん"};
    return classes[gen.bounded(classes.size())];
}

QString LicensePlateGenerator::genRandomHiragana(QRandomGenerator& gen) const {
    static const QStringList hiragana = {"あ", "い", "う", "え", "お", "か", "き", "く", "け", "こ", "さ", "し", "す", "せ", "そ", "た", "ち", "つ", "て", "と", "な", "に", "ぬ", "ね", "の", "は", "ひ", "ふ", "へ", "ほ", "ま", "み", "む", "め", "も", "や", "ゆ", "よ", "ら", "り", "る", "れ", "ろ", "わ", "を", "ん"};
    return hiragana[gen.bounded(hiragana.size())];
}

QString LicensePlateGenerator::applyPattern(const QString& pattern, const QMap<QString, QString>& replacements) const {
    QString result = pattern;
    for (auto it = replacements.constBegin(); it != replacements.constEnd(); ++it) {
        result.replace("{" + it.key() + "}", it.value());
    }
    return result;
}

QImage LicensePlateGenerator::renderText(const QString& text, const LicensePlateTemplate& tmpl, int width, int height) const {
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    QFont font(tmpl.fontFamily, tmpl.fontSize, QFont::Bold);
    if (!QFontDatabase::hasFamily(tmpl.fontFamily)) {
        font = QFont("Arial", tmpl.fontSize, QFont::Bold);
    }
    painter.setFont(font);
    painter.setPen(tmpl.textColor);
    
    QRect rect(0, 0, width, height);
    painter.drawText(rect, Qt::AlignCenter, text);
    
    return image;
}

QImage LicensePlateGenerator::applyDirt(const QImage& image, float intensity) const {
    QImage result = image;
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRandomGenerator* gen = QRandomGenerator::global();
    int dirtCount = int(100 * intensity);
    
    for (int i = 0; i < dirtCount; ++i) {
        int x = gen->bounded(result.width());
        int y = gen->bounded(result.height());
        int radius = gen->bounded(3, 8);
        
        QColor dirtColor(45, 40, 35, int(50 * intensity));
        painter.setBrush(dirtColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(x - radius/2, y - radius/2, radius, radius);
    }
    
    return result;
}

QImage LicensePlateGenerator::applyScratches(const QImage& image, float intensity) const {
    QImage result = image;
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRandomGenerator* gen = QRandomGenerator::global();
    int scratchCount = int(20 * intensity);
    
    QPen scratchPen(QColor(80, 75, 70, int(80 * intensity)), 1, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(scratchPen);
    
    for (int i = 0; i < scratchCount; ++i) {
        int x1 = gen->bounded(result.width());
        int y1 = gen->bounded(result.height());
        int x2 = x1 + gen->bounded(-30, 30);
        int y2 = y1 + gen->bounded(-10, 10);
        painter.drawLine(x1, y1, x2, y2);
    }
    
    return result;
}

QImage LicensePlateGenerator::addNoise(const QImage& image, float intensity) const {
    QImage result = image;
    QRandomGenerator* gen = QRandomGenerator::global();
    
    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            if (gen->generateDouble() < intensity * 0.1) {
                QColor c = result.pixelColor(x, y);
                int noise = gen->bounded(-20, 20);
                c.setRed(qBound(0, c.red() + noise, 255));
                c.setGreen(qBound(0, c.green() + noise, 255));
                c.setBlue(qBound(0, c.blue() + noise, 255));
                result.setPixelColor(x, y, c);
            }
        }
    }
    return result;
}

QImage LicensePlateGenerator::applyPerspective(const QImage& image, float angle) const {
    // Simple perspective transform - would use QTransform in real implementation
    return image;
}

QString LicensePlateGenerator::formatWithSeparators(const QString& text, const QString& separator, int groupSize) {
    QString result;
    for (int i = 0; i < text.length(); i += groupSize) {
        if (i > 0) result += separator;
        result += text.mid(i, groupSize);
    }
    return result;
}

bool LicensePlateGenerator::matchesPattern(const QString& text, const QString& pattern) {
    QRegularExpression re(pattern);
    return re.match(text).hasMatch();
}

QString LicensePlateGenerator::replacePlaceholders(const QString& template_, const QMap<QString, QString>& values) {
    QString result = template_;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        result.replace("{" + it.key() + "}", it.value());
    }
    return result;
}

QString LicensePlateGenerator::countryCodeToString(CountryCode code) {
    switch (code) {
        case CountryCode::Germany: return "Germany";
        case CountryCode::France: return "France";
        case CountryCode::Italy: return "Italy";
        case CountryCode::Spain: return "Spain";
        case CountryCode::Netherlands: return "Netherlands";
        case CountryCode::Belgium: return "Belgium";
        case CountryCode::Austria: return "Austria";
        case CountryCode::Poland: return "Poland";
        case CountryCode::Sweden: return "Sweden";
        case CountryCode::Finland: return "Finland";
        case CountryCode::Denmark: return "Denmark";
        case CountryCode::Ireland: return "Ireland";
        case CountryCode::Portugal: return "Portugal";
        case CountryCode::Greece: return "Greece";
        case CountryCode::CzechRepublic: return "Czech Republic";
        case CountryCode::Hungary: return "Hungary";
        case CountryCode::Romania: return "Romania";
        case CountryCode::Slovakia: return "Slovakia";
        case CountryCode::Slovenia: return "Slovenia";
        case CountryCode::Croatia: return "Croatia";
        case CountryCode::Bulgaria: return "Bulgaria";
        case CountryCode::UK: return "United Kingdom";
        case CountryCode::Switzerland: return "Switzerland";
        case CountryCode::Norway: return "Norway";
        case CountryCode::USA: return "USA";
        case CountryCode::Japan: return "Japan";
        case CountryCode::Custom: return "Custom";
        default: return "Unknown";
    }
}

CountryCode LicensePlateGenerator::stringToCountryCode(const QString& code) {
    static QMap<QString, CountryCode> map = {
        {"DE", CountryCode::Germany}, {"DEU", CountryCode::Germany},
        {"FR", CountryCode::France}, {"FRA", CountryCode::France},
        {"IT", CountryCode::Italy}, {"ITA", CountryCode::Italy},
        {"ES", CountryCode::Spain}, {"ESP", CountryCode::Spain},
        {"NL", CountryCode::Netherlands}, {"NLD", CountryCode::Netherlands},
        {"BE", CountryCode::Belgium}, {"BEL", CountryCode::Belgium},
        {"AT", CountryCode::Austria}, {"AUT", CountryCode::Austria},
        {"PL", CountryCode::Poland}, {"POL", CountryCode::Poland},
        {"SE", CountryCode::Sweden}, {"SWE", CountryCode::Sweden},
        {"FI", CountryCode::Finland}, {"FIN", CountryCode::Finland},
        {"DK", CountryCode::Denmark}, {"DNK", CountryCode::Denmark},
        {"IE", CountryCode::Ireland}, {"IRL", CountryCode::Ireland},
        {"PT", CountryCode::Portugal}, {"PRT", CountryCode::Portugal},
        {"GR", CountryCode::Greece}, {"GRC", CountryCode::Greece},
        {"CZ", CountryCode::CzechRepublic}, {"CZE", CountryCode::CzechRepublic},
        {"HU", CountryCode::Hungary}, {"HUN", CountryCode::Hungary},
        {"RO", CountryCode::Romania}, {"ROU", CountryCode::Romania},
        {"SK", CountryCode::Slovakia}, {"SVK", CountryCode::Slovakia},
        {"SI", CountryCode::Slovenia}, {"SVN", CountryCode::Slovenia},
        {"HR", CountryCode::Croatia}, {"HRV", CountryCode::Croatia},
        {"BG", CountryCode::Bulgaria}, {"BGR", CountryCode::Bulgaria},
        {"GB", CountryCode::UK}, {"UK", CountryCode::UK}, {"GBR", CountryCode::UK},
        {"CH", CountryCode::Switzerland}, {"CHE", CountryCode::Switzerland},
        {"NO", CountryCode::Norway}, {"NOR", CountryCode::Norway},
        {"US", CountryCode::USA}, {"USA", CountryCode::USA},
        {"JP", CountryCode::Japan}, {"JPN", CountryCode::Japan},
    };
    return map.value(code.toUpper(), CountryCode::Custom);
}

QStringList LicensePlateGenerator::getRegionsForCountry(CountryCode country) const {
    return m_templates.value(country).regionCodes;
}

QString LicensePlateGenerator::getCountryName(CountryCode country) const {
    return m_templates.value(country).name;
}

void LicensePlateGenerator::loadPresets(const QString& presetName) {
    if (m_presets.contains(presetName)) {
        // Load preset
    }
}

void LicensePlateGenerator::savePresets(const QString& presetName) const {
    // Save current templates as preset
}

QStringList LicensePlateGenerator::availablePresets() const {
    return m_presets.keys();
}

} // namespace license
} // namespace ks