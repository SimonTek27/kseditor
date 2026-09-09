#pragma once

/**
 * @file PhysicsMessage.h
 * @brief Message system for inter-component communication
 * @copyright KS Physics Engine
 */

#include "PhysicsCoreTypes.h"
#include <QObject>
#include <QMap>
#include <QString>
#include <functional>
#include <memory>

namespace ks {
namespace physics {

// ============================================================================
// Message Types
// ============================================================================

enum class MessageType {
    // System messages
    Initialize,
    Shutdown,
    Reset,
    
    // Physics update messages
    UpdateForces,
    UpdateTireState,
    UpdateWeather,
    UpdateDamage,
    
    // State query messages
    GetState,
    SetState,
    
    // Component-specific messages
    EngineUpdate,
    TireUpdate,
    AeroUpdate,
    ChassisUpdate,
    
    // Event messages
    Collision,
    PitStop,
    LapComplete,
    
    // Custom messages
    Custom
};

// ============================================================================
// Message Base Class
// ============================================================================

struct PhysicsMessage {
    MessageType type;
    QString sender;
    QString recipient;
    double timestamp;
    
    PhysicsMessage() : type(MessageType::Custom), timestamp(0.0) {}
    virtual ~PhysicsMessage() = default;
    
    virtual QString messageType() const { return "PhysicsMessage"; }
};

// ============================================================================
// Specific Message Types
// ============================================================================

struct UpdateForcesMessage : public PhysicsMessage {
    std::array<double, 4> tireForces;
    double aeroDownforce;
    double dragForce;
    
    UpdateForcesMessage() {
        type = MessageType::UpdateForces;
        tireForces = {0, 0, 0, 0};
        aeroDownforce = 0.0;
        dragForce = 0.0;
    }
    
    QString messageType() const override { return "UpdateForcesMessage"; }
};

struct UpdateTireStateMessage : public PhysicsMessage {
    int wheel;
    double slipAngle;
    double slipRatio;
    double normalLoad;
    double temperature;
    
    UpdateTireStateMessage() : wheel(0), slipAngle(0.0), slipRatio(0.0),
        normalLoad(0.0), temperature(30.0) {
        type = MessageType::UpdateTireState;
    }
    
    QString messageType() const override { return "UpdateTireStateMessage"; }
};

struct UpdateWeatherMessage : public PhysicsMessage {
    WeatherState weather;
    
    UpdateWeatherMessage() {
        type = MessageType::UpdateWeather;
        weather = WeatherState();
    }
    
    QString messageType() const override { return "UpdateWeatherMessage"; }
};

struct CollisionMessage : public PhysicsMessage {
    double impactForce;
    QVector3D collisionPoint;
    int otherObjectIndex;
    
    CollisionMessage() : impactForce(0.0), otherObjectIndex(-1) {
        type = MessageType::Collision;
    }
    
    QString messageType() const override { return "CollisionMessage"; }
};

// ============================================================================
// Message Handler
// ============================================================================

using MessageHandler = std::function<void(const PhysicsMessage&)>;

// ============================================================================
// Message Bus
// ============================================================================

class PhysicsMessageBus {
public:
    static PhysicsMessageBus& instance();
    
    // Subscribe to messages
    void subscribe(const QString& messageType, MessageHandler handler);
    void subscribe(MessageType type, MessageHandler handler);
    
    // Publish messages
    void publish(const PhysicsMessage& message);
    
    // Clear all subscriptions
    void clear();

private:
    PhysicsMessageBus() = default;
    
    QMap<QString, std::vector<MessageHandler>> m_handlersByName;
    QMap<MessageType, std::vector<MessageHandler>> m_handlersByType;
};

// ============================================================================
// Message Sender/Receiver Interfaces
// ============================================================================

class PhysicsMessageSender {
public:
    virtual ~PhysicsMessageSender() = default;
    
    void sendMessage(const PhysicsMessage& message) {
        PhysicsMessageBus::instance().publish(message);
    }
};

class PhysicsMessageReceiver {
public:
    virtual ~PhysicsMessageReceiver() = default;
    
    void registerMessageHandler(const QString& messageType, MessageHandler handler) {
        PhysicsMessageBus::instance().subscribe(messageType, handler);
    }
    
    void registerMessageHandler(MessageType type, MessageHandler handler) {
        PhysicsMessageBus::instance().subscribe(type, handler);
    }
};

} // namespace physics
} // namespace ks