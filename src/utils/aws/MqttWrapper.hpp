// MqttWrapper.hpp
#ifndef MQTT_WRAPPER_HPP
#define MQTT_WRAPPER_HPP

#include <aws/crt/Api.h>
#include <aws/crt/UUID.h>
#include <aws/crt/mqtt/Mqtt5Packets.h>
#include <aws/iot/Mqtt5Client.h>
#include <unordered_map>

#include <thread>

#include "CommandLineUtils.h"

using namespace Aws::Crt;
using callback_t = std::function<int(const std::string &, std::string)>;

class MqttWrapper
{
public:
    MqttWrapper(const std::string &endpoint, const std::string &certPath, const std::string &keyPath, const std::string &clientId);
    ~MqttWrapper();

    int connect();
    int disconnect();
    int subscribe(const std::string &topic, callback_t callback);
    int unsubscribe(const std::string &topic, callback_t callback);
    int publish(const std::string &topic, const std::string &payload, bool retain);

private:
    std::unordered_map<std::string, std::vector<callback_t>> callbacks; // Store multiple callbacks for each topic
    int __message_arrived(const Mqtt5::PublishReceivedEventData &eventData);
    bool __topic_matches_subscription(const std::string &sub, const std::string &topic);

    Aws::Iot::Mqtt5ClientBuilder *builder;
    Aws::Crt::ApiHandle apiHandle;
    std::shared_ptr<Aws::Crt::Mqtt5::Mqtt5Client> client;
    std::string endpoint_;
    std::string certPath_;
    std::string keyPath_;
    std::string clientId_;
};

#endif // MQTT_WRAPPER_HPP
