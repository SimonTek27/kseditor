#pragma once

#include <QByteArray>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QString>
#include <cstdint>

namespace ks {

// Protobuf wire types
enum ProtobufWireType : uint32_t {
    PB_VARINT     = 0,
    PB_FIXED64    = 1,
    PB_LENGTH_DELIMITED = 2,
    PB_START_GROUP = 3,
    PB_END_GROUP   = 4,
    PB_FIXED32    = 5,
};

struct ProtobufField {
    uint32_t fieldNumber = 0;
    ProtobufWireType wireType = PB_VARINT;
    QVariant value;
    QByteArray rawBytes;
    bool valid = false;
};

class ProtobufDecoder {
public:
    // Decode a protobuf message from raw bytes (schema-less)
    static QVariantMap decodeMessage(const QByteArray& data);

    // Decode a single varint
    static bool decodeVarint(const char* data, qint64 size, quint64& value, qint64& bytesRead);

    // Decode a protobuf field
    static bool decodeField(const char* data, qint64 size, ProtobufField& field, qint64& bytesRead);

    // Pretty-print a decoded message
    static QString printMessage(const QVariantMap& message, int indent = 0);

    // Get wire type name
    static QString wireTypeName(ProtobufWireType wireType);

    // Try to extract all strings from binary data (for debugging)
    static QStringList extractStrings(const QByteArray& data, int minLength = 4);

private:
    static QVariant decodeVarintField(const char* data, qint64 size, qint64& bytesRead);
    static QVariant decodeFixed32Field(const char* data, qint64 size, qint64& bytesRead);
    static QVariant decodeFixed64Field(const char* data, qint64 size, qint64& bytesRead);
    static QVariant decodeLengthDelimited(const char* data, qint64 size, qint64& bytesRead);
};

} // namespace ks
