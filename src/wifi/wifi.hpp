#ifndef     SYSTEM_WIFI
#define     SYSTEM_WIFI

#define     MAX_INPUT               1024
#define     MAX_COMMAND_BUFFER      8192

extern "C" {
    #include "../utils/utils.h"
}

#include <iostream>
#include <map>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <regex>
#include <vector>
#include <set>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <thread>
#include <unistd.h>
#include <climits>
#include <nlohmann/json.hpp>


#include "../utils/log.hpp"
#include "../utils/mqtt_client.hpp"

typedef enum {
    WIFI_SUCCESS,
    WIFI_COMMAND_ERROR,
    WIFI_THREAD_ERROR,
    WIFI_CONFIG_ERROR,
} WIFI_ERROR_T;

class Wifi {
public:
    Wifi();
    int add_network(const std::string& ssid, const std::string& password);
    int remove_network(const std::string& ssid);
    std::vector<std::string> get_known_wifi();
    std::vector<std::string> get_available_wifi();
    int publish_info();
    int start();
    int stop();
    int restart();
    int connect(const std::string& ssid, const std::string& password);
    int subscribe();
    int unsubscribe();

    ~Wifi();

    std::map<std::string, std::function<int(const std::string&, json)>> callbacks;
    bool running;
    bool scanning_for_networks;
    // std::string url;
    json info;
    int time;

    std::string wpa_conf_path;

    // variables
    MQTTClientServer *client;
    std::string __prefix;
    std::unordered_map<std::string, std::string> __config;
    std::vector<std::string> __config_dep;
    std::thread *__wifi_thread;
    int __wifi_fd;
    std::string __type;
    Log *__log;

    bool __kill_wifi_flag;
private:
    int __start();
};

#endif