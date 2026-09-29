#pragma once

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../FileFormat/Json.h"

namespace ks::chat {

// ---------------------------------------------------------------------------
// Small colour value type (replaces QColor). Only the operations the chat
// protocol actually uses are provided: #rrggbb serialisation and HSV
// generation for random per-user colours.
// ---------------------------------------------------------------------------
struct Color {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 255;

    Color() = default;
    Color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}

    // QColor::fromHsv(h, s, v) - alpha is opaque.
    static Color fromHsv(int h, int s, int v);
    // QColor(text): accepts "#rgb", "#rrggbb" and "#aarrggbb".
    static Color fromHex(const std::string& text);
    // QColor::name(): "#rrggbb"
    std::string name() const;
};

// Builds a JSON object from a brace list, replacing QJsonObject's
// {{"key", value}, ...} initialiser-list syntax.
inline ks::Json makeObject(std::initializer_list<std::pair<std::string, ks::Json>> fields) {
    ks::Json obj = ks::Json::object();
    for (const auto& field : fields) obj[field.first] = field.second;
    return obj;
}

// Builds a JSON array, replacing QJsonArray{a, b, c}.
inline ks::Json makeArray(std::initializer_list<ks::Json> values) {
    ks::Json arr = ks::Json::array();
    for (const auto& value : values) arr.append(value);
    return arr;
}

// QVector<QString> helpers (contains / removeOne) for std::vector<std::string>.
inline bool vectorContains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

inline void vectorRemove(std::vector<std::string>& values, const std::string& value) {
    values.erase(std::remove(values.begin(), values.end(), value), values.end());
}

enum class MessageType {
    Text,
    Code,
    File,
    Image,
    System,
    Edit,
    Delete,
    Reaction,
    Typing,
    ReadReceipt
};

enum class ChannelType {
    Direct,
    Group,
    Project
};

enum class UserStatus {
    Online,
    Idle,
    Dnd,
    Offline
};

enum class Permission {
    Read,
    Write,
    Invite,
    Kick,
    Ban,
    ManageChannel,
    Admin
};

struct ChatUser {
    std::string id;
    std::string name;
    std::string avatar;
    Color color;
    UserStatus status = UserStatus::Offline;
    std::string statusText;
    // Milliseconds since the Unix epoch (QDateTime was stored as ms as well).
    std::int64_t lastSeen = 0;
    bool isBot = false;

    ks::Json toJson() const;
    static ChatUser fromJson(const ks::Json& obj);
};

struct ChatMessage {
    std::string id;
    std::string channelId;
    std::string authorId;
    std::string authorName;
    MessageType type = MessageType::Text;
    std::string content;
    std::int64_t timestamp = 0;
    std::int64_t editedAt = 0;
    std::vector<std::string> attachments;
    std::map<std::string, std::vector<std::string>> reactions;
    std::string replyToId;
    bool isDeleted = false;

    ks::Json toJson() const;
    static ChatMessage fromJson(const ks::Json& obj);
};

struct ChatChannel {
    std::string id;
    std::string name;
    std::string description;
    ChannelType type = ChannelType::Group;
    std::vector<std::string> members;
    std::string owner;
    std::int64_t createdAt = 0;
    std::int64_t lastMessageAt = 0;
    int unreadCount = 0;
    bool isPinned = false;

    ks::Json toJson() const;
    static ChatChannel fromJson(const ks::Json& obj);
};

struct ChatServerInfo {
    std::string id;
    std::string name;
    std::string icon;
    std::string ownerId;
    std::vector<ChatChannel> channels;
    std::vector<std::string> members;
    std::int64_t createdAt = 0;

    ks::Json toJson() const;
    static ChatServerInfo fromJson(const ks::Json& obj);
};

struct ProtocolMessage {
    std::string type;
    ks::Json payload;
    std::int64_t timestamp = 0;

    ks::Json toJson() const;
    static ProtocolMessage fromJson(const ks::Json& obj);
    static ProtocolMessage make(const std::string& type, const ks::Json& payload);
};

} // namespace ks::chat

