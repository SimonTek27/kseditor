#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>

namespace ks::sim {

// ============================================================================
// Simple MQTT Client - Optional integration for telemetry messaging
// Uses Paho MQTT C library if available, otherwise provides a basic interface
// ============================================================================

struct MqttConnectionOptions {
    std::string clientId = "kseditor";
    std::string serverUri = "tcp://localhost:1883";
    std::string username;
    std::string password;
    int keepAliveInterval = 60;
    bool cleanSession = true;
};

struct MqttMessage {
    std::string topic;
    std::string payload;
    int qos = 1;
    bool retained = false;
};

class MqttClient {
public:
    MqttClient();
    ~MqttClient();

    // Initialize MQTT client
    bool connect(const MqttConnectionOptions& options);

    // Disconnect from broker
    void disconnect();

    // Check if connected
    bool isConnected() const;

    // Publish a message
    bool publish(const MqttMessage& message);

    // Subscribe to a topic
    bool subscribe(const std::string& topic, int qos = 1);

    // Unsubscribe from a topic
    bool unsubscribe(const std::string& topic);

    // Set callback for received messages
    using MessageCallback = std::function<void(const MqttMessage&)>;
    void setMessageCallback(MessageCallback cb);

    // Get last connection return code
    int getReturnCode() const;

private:
    int m_returnCode = 0;
    MessageCallback m_callback;
    bool m_connected = false;
    // Platform-specific handle (opaque pointer)
    void* m_handle = nullptr;
};

// Convenience functions for common telemetry messages

// Publish car state telemetry
inline bool publishCarState(MqttClient& client, const std::string& topic, const ks::sim::AcTelemetryData& data) {
    // Simple JSON serialization - in production would use proper JSON library
    std::string payload = "{";
    payload += "\"speed\":" + std::to_string(data.speed) + ",";
    payload += "\"rpm\":" + std::to_string(data.rpm) + ",";
    payload += "\"throttle\":" + std::to_string(data.throttle) + ",";
    payload += "\"brake\":" + std::to_string(data.brake) + ",";
    payload += "\"steering\":" + std::to_string(data.steering) + ",";
    payload += "\"fuel\":" + std::to_string(data.fuel) + ",";
    payload += "\"gear\":" + std::to_string(data.gear) + "";
    payload += "}";
    MqttMessage msg{topic, payload};
    return client.publish(msg);
}

// Publish session state
inline bool publishSessionState(MqttClient& client, const std::string& topic, 
                                uint8_t sessionType, uint8_t phase, int currentLap, int totalLaps, double timeRemaining) {
    std::string payload = "{";
    payload += "\"sessionType\":" + std::to_string(sessionType) + ",";
    payload += "\"phase\":" + std::to_string(phase) + ",";
    payload += "\"currentLap\":" + std::to_string(currentLap) + ",";
    payload += "\"totalLaps\":" + std::to_string(totalLaps) + ",";
    payload += "\"timeRemaining\":" + std::to_string(timeRemaining);
    payload += "}";
    MqttMessage msg{topic, payload};
    return client.publish(msg);
}

} // namespace ks::sim