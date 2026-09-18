#include "MqttClient.h"

#include <cstring>
#include <iostream>
#include <thread>
#include <atomic>

// Platform-specific MQTT includes and setup
#if defined(_WIN32) || defined(__WIN32__)
    #include <mosquitto.h>
    #define MOSQPP_EXPORT __declspec(dllexport)
#else
    #include <mosquitto.h>
    #define MOSQPP_EXPORT
#endif

namespace ks::sim {

MqttClient::MqttClient()
    : m_returnCode(0)
    , m_connected(false)
    , m_handle(nullptr)
{
    // Initialize mosquitto library
#if defined(_WIN32) || defined(__WIN32)
    mosquitto_lib_init();
#endif
}

MqttClient::~MqttClient() {
    disconnect();
#if defined(_WIN32) || defined(__WIN32__)
    mosquitto_lib_cleanup();
#endif
}

bool MqttClient::connect(const MqttConnectionOptions& options) {
    // Create new mosquitto instance
    m_handle = mosquitto_new(options.clientId.c_str(), options.cleanSession ? true : false, nullptr);
    if (!m_handle) {
        std::cerr << "Failed to create mosquitto instance" << std::endl;
        return false;
    }

    // Set username/password if provided
    if (!options.username.empty()) {
        mosquitto_username_pw_set(m_handle, options.username.c_str(), 
                                  options.password.empty() ? nullptr : options.password.c_str());
    }

    // Connect to broker
    int rc = mosquitto_connect(m_handle, options.serverUri.c_str(), 
                               options.keepAliveInterval, 1);
    m_returnCode = rc;

    if (rc == MOSQ_ERR_SUCCESS) {
        m_connected = true;
        std::cout << "MQTT connected to " << options.serverUri << std::endl;
    } else {
        std::cerr << "MQTT connection failed with code " << rc << std::endl;
        mosquitto_destroy(m_handle);
        m_handle = nullptr;
    }

    return m_connected;
}

void MqttClient::disconnect() {
    if (m_handle) {
        mosquitto_disconnect(m_handle);
        mosquitto_destroy(m_handle);
        m_handle = nullptr;
    }
    m_connected = false;
}

bool MqttClient::isConnected() const {
    return m_connected;
}

bool MqttClient::publish(const MqttMessage& message) {
    if (!m_handle || !m_connected) {
        std::cerr << "MQTT not connected" << std::endl;
        return false;
    }

    int rc = mosquitto_publish(m_handle, nullptr, 
                              message.topic.c_str(), 
                              static_cast<int>(message.payload.size()),
                              message.payload.c_str(), 
                              message.qos, message.retained ? 1 : 0);
    return rc == MOSQ_ERR_SUCCESS;
}

bool MqttClient::subscribe(const std::string& topic, int qos) {
    if (!m_handle || !m_connected) {
        std::cerr << "MQTT not connected" << std::endl;
        return false;
    }

    int rc = mosquitto_subscribe(m_handle, nullptr, topic.c_str(), qos);
    return rc == MOSQ_ERR_SUCCESS;
}

bool MqttClient::unsubscribe(const std::string& topic) {
    if (!m_handle || !m_connected) {
        std::cerr << "MQTT not connected" << std::endl;
        return false;
    }

    int rc = mosquitto_unsubscribe(m_handle, nullptr, topic.c_str());
    return rc == MOSQ_ERR_SUCCESS;
}

void MqttClient::setMessageCallback(MessageCallback cb) {
    m_callback = std::move(cb);
    // Note: mosquitto loop would need to be called to trigger callbacks
    // This is a simplified interface
}

int MqttClient::getReturnCode() const {
    return m_returnCode;
}

// Convenience functions for common telemetry messages

inline bool publishCarState(MqttClient& client, const std::string& topic, const ks::sim::AcTelemetryData& data) {
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