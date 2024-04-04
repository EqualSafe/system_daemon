#include "mqtt_client.hpp"
#include <cstring>
#include <iostream>

// Constructor
MQTTClientServer::MQTTClientServer()
{
    this->__log = new Log("MQTT");
    MQTTClient_create(&client, "tcp://0.0.0.0:1883", "mqtt_client",
                      MQTTCLIENT_PERSISTENCE_NONE, nullptr);
}

// Destructor
MQTTClientServer::~MQTTClientServer()
{
    MQTTClient_destroy(&client);
}

// Connect
int MQTTClientServer::connect(const std::string& address, const std::string& clientId)
{
    this->__log->print(LOG_NORMAL, "connect");
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    MQTTClient_create(&client, address.c_str(), clientId.c_str(),
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, this, nullptr, __message_arrived, nullptr);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        this->__log->print(LOG_ERROR, "Failed to connect, return code %d\n", rc);
    }
    return rc;
}

// Disconnect
int MQTTClientServer::disconnect()
{
    this->__log->print(LOG_NORMAL, "disconnect");
    MQTTClient_disconnect(client, 10000); // Timeout set to 10000 milliseconds
    return 0;
}

int MQTTClientServer::subscribe(const std::string& topic, callback_t callback)
{
    this->__log->print(LOG_NORMAL, "subscribe %s", topic.c_str());
    if (callbacks.find(topic) == callbacks.end()) {
        MQTTClient_subscribe(client, topic.c_str(), 0);
    }

    callbacks[topic].push_back(callback);
    return 0;
}

// Unsubscribe
int MQTTClientServer::unsubscribe(const std::string& topic, callback_t callback)
{
    this->__log->print(LOG_NORMAL, "unsubscribe %s", topic.c_str());
    auto& cbVector = callbacks[topic];
    // Remove the specified callback
    cbVector.erase(
        std::remove_if(cbVector.begin(), cbVector.end(),
                       [&callback](const callback_t& cb) {
                           return cb.target_type().name() == callback.target_type().name();
                       }),
        cbVector.end());

    // If no callbacks left for the topic, unsubscribe from the MQTT client and remove the topic
    if (cbVector.empty()) {
        MQTTClient_unsubscribe(client, topic.c_str()); // Unsubscribe
        callbacks.erase(topic); // Remove the topic entry entirely
    }

    return 0; // Assuming success, handle errors as needed
}


// Publish
int MQTTClientServer::publish(const std::string& topic, const json& payload)
{
    return this->publish(topic, payload, 0);
}

// Publish
int MQTTClientServer::publish(const std::string& topic, const json& payload, int retain)
{
    this->__log->print(LOG_NORMAL, "publish %s retain: %d", topic.c_str(), retain);
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    std::string payloadStr = payload.dump(); // Serialize JSON to string
    pubmsg.payload = (void*)payloadStr.c_str();
    pubmsg.payloadlen = payloadStr.length();
    pubmsg.qos = 1;
    pubmsg.retained = retain;
    MQTTClient_deliveryToken token;
    int rc = MQTTClient_publishMessage(client, topic.c_str(), &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, 10000L); // Wait for up to 10000 milliseconds for publication to complete
    return rc;
}

bool MQTTClientServer::__topic_matches_subscription(const std::string& sub, const std::string& topic) {
    std::istringstream subStream(sub);
    std::istringstream topicStream(topic);
    std::string subPart, topicPart;

    while (std::getline(subStream, subPart, '/') && std::getline(topicStream, topicPart, '/')) {
        if (subPart == "#") return true; // Multi-level wildcard matches all remaining levels
        if (subPart != "+" && subPart != topicPart) return false; // Direct mismatch
    }

    // Handle case where subscription or topic is longer than the other
    return true;
}

// Static callback function for when a message arrives
int MQTTClientServer::__message_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* message)
{
    auto* self = static_cast<MQTTClientServer*>(context);
    std::string topic(topicName);
    std::string payloadStr((char*)message->payload, message->payloadlen);
    json payload = json::parse(payloadStr, nullptr, false); // Assuming json is defined elsewhere

    // we will not allow for non object payloads
    if (!payload.is_object()) {
        self->__log->print(LOG_ERROR, "%s : invalid payload, not an object. got: %s", topic.c_str(), payloadStr.c_str());
        return 1;
    }

    for (const auto& sub : self->callbacks) {
        if (self->__topic_matches_subscription(sub.first, topic)) {
            for (auto& cb : sub.second) {
                new std::thread([&cb, topic, payload] {
                    return cb(topic, payload);
                });
            }
        }
    }

    MQTTClient_free(topicName);
    MQTTClient_freeMessage(&message);

    // REALLY IMPORTANT FOR MQTT NOT TO REINVOKE __message_arrived
    return 1;
}