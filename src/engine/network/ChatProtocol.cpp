#include "ChatProtocol.h"

#include <cmath>
#include <cstdio>

#include "NetUtil.h"

namespace ks::chat {

using net::nowMs;

namespace {

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // namespace

// --- Color (QColor replacement) ---

Color Color::fromHsv(int h, int s, int v) {
    // Same parameter ranges as QColor::fromHsv: h in [0,360), s/v in [0,255].
    h = ((h % 360) + 360) % 360;
    s = s < 0 ? 0 : (s > 255 ? 255 : s);
    v = v < 0 ? 0 : (v > 255 ? 255 : v);

    const float c = (static_cast<float>(v) / 255.0f) * (static_cast<float>(s) / 255.0f);
    const float x = c * (1.0f - std::fabs(static_cast<float>(h) / 60.0f - 1.0f));
    const float m = (static_cast<float>(v) / 255.0f) - c;

    float rf = 0.0f, gf = 0.0f, bf = 0.0f;
    if (h < 60)      { rf = c; gf = x; bf = 0.0f; }
    else if (h < 120) { rf = x; gf = c; bf = 0.0f; }
    else if (h < 180) { rf = 0.0f; gf = c; bf = x; }
    else if (h < 240) { rf = 0.0f; gf = x; bf = c; }
    else if (h < 300) { rf = x; gf = 0.0f; bf = c; }
    else              { rf = c; gf = 0.0f; bf = x; }

    auto toByte = [m](float component) {
        const float value = (component + m) * 255.0f;
        const int rounded = static_cast<int>(value + 0.5f);
        return static_cast<unsigned char>(rounded < 0 ? 0 : (rounded > 255 ? 255 : rounded));
    };
    return Color(toByte(rf), toByte(gf), toByte(bf), 255);
}

Color Color::fromHex(const std::string& text) {
    // Mirrors QColor(QString): "#rgb", "#rrggbb", "#aarrggbb".
    if (text.empty() || text[0] != '#') return Color(0, 0, 0, 255);

    const std::string digits = text.substr(1);
    for (char c : digits) {
        if (hexValue(c) < 0) return Color(0, 0, 0, 255);
    }

    auto byteAt = [&digits](std::size_t index) -> unsigned char {
        const int hi = hexValue(digits[index]);
        const int lo = hexValue(digits[index + 1]);
        return static_cast<unsigned char>((hi << 4) | lo);
    };
    auto nibble = [&digits](std::size_t index) -> unsigned char {
        const int value = hexValue(digits[index]);
        return static_cast<unsigned char>(value * 17);
    };

    switch (digits.size()) {
    case 3:
        return Color(nibble(0), nibble(1), nibble(2), 255);
    case 6:
        return Color(byteAt(0), byteAt(2), byteAt(4), 255);
    case 8:
        return Color(byteAt(2), byteAt(4), byteAt(6), byteAt(0));
    default:
        return Color(0, 0, 0, 255);
    }
}

std::string Color::name() const {
    char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "#%02x%02x%02x", r, g, b);
    return std::string(buffer);
}

// --- ChatUser ---

ks::Json ChatUser::toJson() const {
    ks::Json obj = ks::Json::object();
    obj["id"] = id;
    obj["name"] = name;
    obj["avatar"] = avatar;
    obj["color"] = color.name();
    obj["status"] = static_cast<int>(status);
    obj["statusText"] = statusText;
    obj["lastSeen"] = lastSeen;
    obj["isBot"] = isBot;
    return obj;
}

ChatUser ChatUser::fromJson(const ks::Json& obj) {
    ChatUser u;
    u.id = obj["id"].toString();
    u.name = obj["name"].toString();
    u.avatar = obj["avatar"].toString();
    u.color = Color::fromHex(obj["color"].toString());
    u.status = static_cast<UserStatus>(obj["status"].toInt());
    u.statusText = obj["statusText"].toString();
    u.lastSeen = obj["lastSeen"].toInt64();
    u.isBot = obj["isBot"].toBool();
    return u;
}

// --- ChatMessage ---

ks::Json ChatMessage::toJson() const {
    ks::Json obj = ks::Json::object();
    obj["id"] = id;
    obj["channelId"] = channelId;
    obj["authorId"] = authorId;
    obj["authorName"] = authorName;
    obj["type"] = static_cast<int>(type);
    obj["content"] = content;
    obj["timestamp"] = timestamp;
    obj["editedAt"] = editedAt;
    obj["replyToId"] = replyToId;
    obj["isDeleted"] = isDeleted;

    ks::Json attArr = ks::Json::array();
    for (const auto& a : attachments) attArr.append(a);
    obj["attachments"] = attArr;

    ks::Json reactObj = ks::Json::object();
    for (const auto& entry : reactions) {
        ks::Json arr = ks::Json::array();
        for (const auto& uid : entry.second) arr.append(uid);
        reactObj[entry.first] = arr;
    }
    obj["reactions"] = reactObj;

    return obj;
}

ChatMessage ChatMessage::fromJson(const ks::Json& obj) {
    ChatMessage m;
    m.id = obj["id"].toString();
    m.channelId = obj["channelId"].toString();
    m.authorId = obj["authorId"].toString();
    m.authorName = obj["authorName"].toString();
    m.type = static_cast<MessageType>(obj["type"].toInt());
    m.content = obj["content"].toString();
    m.timestamp = obj["timestamp"].toInt64();
    m.editedAt = obj["editedAt"].toInt64();
    m.replyToId = obj["replyToId"].toString();
    m.isDeleted = obj["isDeleted"].toBool();

    const ks::Json& attachmentsJson = obj["attachments"];
    for (int i = 0; i < attachmentsJson.size(); ++i)
        m.attachments.push_back(attachmentsJson.at(i).toString());

    const ks::Json& reactObj = obj["reactions"];
    if (reactObj.isObject()) {
        for (const std::string& key : reactObj.keys()) {
            std::vector<std::string> uids;
            const ks::Json& values = reactObj[key];
            for (int i = 0; i < values.size(); ++i)
                uids.push_back(values.at(i).toString());
            m.reactions[key] = uids;
        }
    }

    return m;
}

// --- ChatChannel ---

ks::Json ChatChannel::toJson() const {
    ks::Json obj = ks::Json::object();
    obj["id"] = id;
    obj["name"] = name;
    obj["description"] = description;
    obj["type"] = static_cast<int>(type);
    obj["owner"] = owner;
    obj["createdAt"] = createdAt;
    obj["lastMessageAt"] = lastMessageAt;
    obj["unreadCount"] = unreadCount;
    obj["isPinned"] = isPinned;

    ks::Json memArr = ks::Json::array();
    for (const auto& m : members) memArr.append(m);
    obj["members"] = memArr;

    return obj;
}

ChatChannel ChatChannel::fromJson(const ks::Json& obj) {
    ChatChannel c;
    c.id = obj["id"].toString();
    c.name = obj["name"].toString();
    c.description = obj["description"].toString();
    c.type = static_cast<ChannelType>(obj["type"].toInt());
    c.owner = obj["owner"].toString();
    c.createdAt = obj["createdAt"].toInt64();
    c.lastMessageAt = obj["lastMessageAt"].toInt64();
    c.unreadCount = obj["unreadCount"].toInt();
    c.isPinned = obj["isPinned"].toBool();

    const ks::Json& membersJson = obj["members"];
    for (int i = 0; i < membersJson.size(); ++i)
        c.members.push_back(membersJson.at(i).toString());

    return c;
}

// --- ChatServerInfo ---

ks::Json ChatServerInfo::toJson() const {
    ks::Json obj = ks::Json::object();
    obj["id"] = id;
    obj["name"] = name;
    obj["icon"] = icon;
    obj["ownerId"] = ownerId;
    obj["createdAt"] = createdAt;

    ks::Json chArr = ks::Json::array();
    for (const auto& ch : channels) chArr.append(ch.toJson());
    obj["channels"] = chArr;

    ks::Json memArr = ks::Json::array();
    for (const auto& m : members) memArr.append(m);
    obj["members"] = memArr;

    return obj;
}

ChatServerInfo ChatServerInfo::fromJson(const ks::Json& obj) {
    ChatServerInfo s;
    s.id = obj["id"].toString();
    s.name = obj["name"].toString();
    s.icon = obj["icon"].toString();
    s.ownerId = obj["ownerId"].toString();
    s.createdAt = obj["createdAt"].toInt64();

    const ks::Json& channelsJson = obj["channels"];
    for (int i = 0; i < channelsJson.size(); ++i)
        s.channels.push_back(ChatChannel::fromJson(channelsJson.at(i)));

    const ks::Json& membersJson = obj["members"];
    for (int i = 0; i < membersJson.size(); ++i)
        s.members.push_back(membersJson.at(i).toString());

    return s;
}

// --- ProtocolMessage ---

ks::Json ProtocolMessage::toJson() const {
    ks::Json obj = ks::Json::object();
    obj["type"] = type;
    obj["payload"] = payload;
    obj["timestamp"] = timestamp;
    return obj;
}

ProtocolMessage ProtocolMessage::fromJson(const ks::Json& obj) {
    ProtocolMessage pm;
    pm.type = obj["type"].toString();
    pm.payload = obj["payload"];
    pm.timestamp = obj["timestamp"].toInt64();
    return pm;
}

ProtocolMessage ProtocolMessage::make(const std::string& type, const ks::Json& payload) {
    ProtocolMessage pm;
    pm.type = type;
    pm.payload = payload;
    pm.timestamp = nowMs();
    return pm;
}

} // namespace ks::chat
