#include "ACEProtobufDecoder.h"
#include <QDataStream>
#include <QDebug>
#include <cstring>

namespace ks {

QVariantMap ProtobufDecoder::decodeMessage(const QByteArray& data) {
    QVariantMap message;
    const char* ptr = data.constData();
    const char* end = data.constData() + data.size();

    while (ptr < end) {
        ProtobufField field;
        qint64 bytesRead = 0;

        if (!decodeField(ptr, end - ptr, field, bytesRead)) {
            break;
        }

        ptr += bytesRead;

        if (!field.valid) continue;

        QString key = QString("field_%1").arg(field.fieldNumber);

        // Handle repeated fields (same field number appears multiple times)
        if (message.contains(key)) {
            QVariant existing = message.value(key);
            QVariantList list;
            if (existing.canConvert<QVariantList>()) {
                list = existing.toList();
            } else {
                list.append(existing);
            }
            list.append(field.value);
            message.insert(key, list);
        } else {
            message.insert(key, field.value);
        }
    }

    return message;
}

bool ProtobufDecoder::decodeVarint(const char* data, qint64 size, quint64& value, qint64& bytesRead) {
    value = 0;
    bytesRead = 0;
    int shift = 0;

    for (qint64 i = 0; i < size && i < 10; ++i) {
        quint8 byte = static_cast<quint8>(data[i]);
        value |= static_cast<quint64>(byte & 0x7F) << shift;
        bytesRead++;
        if ((byte & 0x80) == 0) {
            return true;
        }
        shift += 7;
    }

    return false; // Too long or truncated
}

bool ProtobufDecoder::decodeField(const char* data, qint64 size, ProtobufField& field, qint64& bytesRead) {
    if (size < 1) return false;

    // Decode tag (field number + wire type)
    quint64 tag = 0;
    qint64 tagBytes = 0;
    if (!decodeVarint(data, size, tag, tagBytes)) {
        return false;
    }

    field.fieldNumber = static_cast<uint32_t>(tag >> 3);
    field.wireType = static_cast<ProtobufWireType>(tag & 0x07);
    bytesRead = tagBytes;

    const char* fieldData = data + tagBytes;
    qint64 fieldSize = size - tagBytes;

    switch (field.wireType) {
    case PB_VARINT: {
        quint64 varintValue = 0;
        qint64 varintBytes = 0;
        if (!decodeVarint(fieldData, fieldSize, varintValue, varintBytes)) {
            return false;
        }
        field.value = QVariant::fromValue(varintValue);
        field.rawBytes = QByteArray(fieldData, varintBytes);
        bytesRead += varintBytes;
        field.valid = true;
        break;
    }
    case PB_FIXED64: {
        if (fieldSize < 8) return false;
        quint64 fixedValue = 0;
        memcpy(&fixedValue, fieldData, 8);
        field.value = QVariant::fromValue(fixedValue);
        field.rawBytes = QByteArray(fieldData, 8);
        bytesRead += 8;
        field.valid = true;
        break;
    }
    case PB_LENGTH_DELIMITED: {
        quint64 length = 0;
        qint64 lengthBytes = 0;
        if (!decodeVarint(fieldData, fieldSize, length, lengthBytes)) {
            return false;
        }
        if (static_cast<quint64>(fieldSize - lengthBytes) < length) {
            return false;
        }
        QByteArray bytes(fieldData + lengthBytes, length);

        // Try to decode as UTF-8 string first
        QString str = QString::fromUtf8(bytes);
        bool isString = true;
        for (const QChar& c : str) {
            if (c.unicode() < 0x20 && c != '\n' && c != '\r' && c != '\t') {
                isString = false;
                break;
            }
        }

        if (isString && !str.isEmpty()) {
            field.value = str;
        } else {
            // Try to decode as nested message
            QVariantMap nested = decodeMessage(bytes);
            if (!nested.isEmpty()) {
                field.value = nested;
            } else {
                field.value = bytes;
            }
        }
        field.rawBytes = bytes;
        bytesRead += lengthBytes + length;
        field.valid = true;
        break;
    }
    case PB_FIXED32: {
        if (fieldSize < 4) return false;
        quint32 fixedValue = 0;
        memcpy(&fixedValue, fieldData, 4);
        field.value = QVariant::fromValue(fixedValue);
        field.rawBytes = QByteArray(fieldData, 4);
        bytesRead += 4;
        field.valid = true;
        break;
    }
    case PB_START_GROUP:
    case PB_END_GROUP:
        // Groups are deprecated, skip
        return false;
    default:
        return false;
    }

    return true;
}

QVariant ProtobufDecoder::decodeVarintField(const char* data, qint64 size, qint64& bytesRead) {
    quint64 value = 0;
    if (decodeVarint(data, size, value, bytesRead)) {
        return QVariant::fromValue(value);
    }
    return QVariant();
}

QVariant ProtobufDecoder::decodeFixed32Field(const char* data, qint64 size, qint64& bytesRead) {
    if (size < 4) return QVariant();
    bytesRead = 4;
    quint32 value = 0;
    memcpy(&value, data, 4);
    return QVariant::fromValue(value);
}

QVariant ProtobufDecoder::decodeFixed64Field(const char* data, qint64 size, qint64& bytesRead) {
    if (size < 8) return QVariant();
    bytesRead = 8;
    quint64 value = 0;
    memcpy(&value, data, 8);
    return QVariant::fromValue(value);
}

QVariant ProtobufDecoder::decodeLengthDelimited(const char* data, qint64 size, qint64& bytesRead) {
    quint64 length = 0;
    qint64 lengthBytes = 0;
    if (!decodeVarint(data, size, length, lengthBytes)) {
        return QVariant();
    }
    if (static_cast<quint64>(size - lengthBytes) < length) {
        return QVariant();
    }
    bytesRead = lengthBytes + length;
    return QByteArray(data + lengthBytes, length);
}

QString ProtobufDecoder::printMessage(const QVariantMap& message, int indent) {
    QString result;
    QString prefix(indent * 2, ' ');

    for (auto it = message.constBegin(); it != message.constEnd(); ++it) {
        const QString& key = it.key();
        const QVariant& value = it.value();

        if (value.canConvert<QVariantMap>()) {
            result += prefix + key + " {\n";
            result += printMessage(value.toMap(), indent + 1);
            result += prefix + "}\n";
        } else if (value.canConvert<QVariantList>()) {
            QVariantList list = value.toList();
            result += prefix + key + " [\n";
            for (const QVariant& item : list) {
                if (item.canConvert<QVariantMap>()) {
                    result += prefix + "  {\n";
                    result += printMessage(item.toMap(), indent + 2);
                    result += prefix + "  }\n";
                } else {
                    result += prefix + "  " + item.toString() + "\n";
                }
            }
            result += prefix + "]\n";
        } else if (value.typeId() == QMetaType::QString) {
            result += prefix + key + ": \"" + value.toString() + "\"\n";
        } else if (value.typeId() == QMetaType::QByteArray) {
            QByteArray bytes = value.toByteArray();
            result += prefix + key + ": [" + QString::number(bytes.size()) + " bytes]\n";
        } else {
            result += prefix + key + ": " + value.toString() + "\n";
        }
    }

    return result;
}

QString ProtobufDecoder::wireTypeName(ProtobufWireType wireType) {
    switch (wireType) {
    case PB_VARINT:          return "varint";
    case PB_FIXED64:         return "fixed64";
    case PB_LENGTH_DELIMITED: return "length-delimited";
    case PB_START_GROUP:     return "start-group";
    case PB_END_GROUP:       return "end-group";
    case PB_FIXED32:         return "fixed32";
    default:                 return "unknown";
    }
}

QStringList ProtobufDecoder::extractStrings(const QByteArray& data, int minLength) {
    QStringList strings;
    QByteArray current;

    for (int i = 0; i < data.size(); ++i) {
        char c = data[i];
        if (c >= 0x20 && c < 0x7F) {
            current.append(c);
        } else {
            if (current.size() >= minLength) {
                strings.append(QString::fromLatin1(current));
            }
            current.clear();
        }
    }

    if (current.size() >= minLength) {
        strings.append(QString::fromLatin1(current));
    }

    return strings;
}

} // namespace ks
