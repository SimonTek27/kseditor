#include "PhysicsMessage.h"

namespace ks {
namespace physics {

PhysicsMessageBus& PhysicsMessageBus::instance() {
    static PhysicsMessageBus instance;
    return instance;
}

void PhysicsMessageBus::subscribe(const QString& messageType, MessageHandler handler) {
    m_handlersByName[messageType].push_back(handler);
}

void PhysicsMessageBus::subscribe(MessageType type, MessageHandler handler) {
    m_handlersByType[type].push_back(handler);
}

void PhysicsMessageBus::publish(const PhysicsMessage& message) {
    // Call handlers by message type name
    QString typeName = message.messageType();
    if (m_handlersByName.contains(typeName)) {
        for (auto& handler : m_handlersByName[typeName]) {
            handler(message);
        }
    }
    
    // Call handlers by message type enum
    if (m_handlersByType.contains(message.type)) {
        for (auto& handler : m_handlersByType[message.type]) {
            handler(message);
        }
    }
}

void PhysicsMessageBus::clear() {
    m_handlersByName.clear();
    m_handlersByType.clear();
}

} // namespace physics
} // namespace ks