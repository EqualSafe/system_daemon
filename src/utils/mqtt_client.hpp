#ifndef MQTTCLIENTSERVER_HPP
#define MQTTCLIENTSERVER_HPP

extern "C" {
#include <MQTTClient.h>
}

#include "log.hpp"
#include <sstream>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
#include <thread>

using json = nlohmann::json;
using callback_t = std::function<int(const std::string&, json)>;

class MQTTClientServer
{
public:
    MQTTClientServer();
    ~MQTTClientServer();
    int connect(const std::string& address, const std::string& clientId);
    int connect(std::unordered_map<std::string,std::string> config, const std::string& clientId);
    int disconnect();
    int subscribe(const std::string& topic, callback_t callback);
    int unsubscribe(const std::string& topic, callback_t callback);
    int publish(const std::string& topic, const json& payload);
    int publish(const std::string& topic, const json& payload, int retain);

private:
    Log *__log;
    MQTTClient client;
    std::unordered_map<std::string, std::vector<callback_t>> callbacks; // Store multiple callbacks for each topic
    std::vector<std::string> __config_dep;
    static int __message_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* message);
    bool __topic_matches_subscription(const std::string& sub, const std::string& topic);
};

#endif // MQTTCLIENTSERVER_HPP
