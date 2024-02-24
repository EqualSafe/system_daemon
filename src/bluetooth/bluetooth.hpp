#ifndef     BASE_FFMPEG
#define     BASE_FFMPEG

#define     MAX_INPUT       1024

extern "C" {
    #include "../utils/utils.h"
}

#include <iostream>
#include <map>
#include <unordered_map>
#include <functional>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <thread>
#include <unistd.h>
#include <climits>
#include <nlohmann/json.hpp>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>


#include "../utils/log.hpp"
#include "../utils/mqtt_client.hpp"

typedef enum {
    BLUETOOTH_SUCCESS,
    BLUETOOTH_COMMAND_ERROR,
    BLUETOOTH_THREAD_ERROR,
    BLUETOOTH_CONFIG_ERROR,
} BLUETOOTH_ERROR_T;

class Bluetooth {
public:
    Bluetooth();
    Bluetooth(std::unordered_map<std::string, std::string> config);
    int set_config(std::unordered_map<std::string, std::string> config);
    int publish_info();
    /**
     * Start the bluetooth and make it discoverable
    */
    int start();
    /**
     * Stop the bluetooth and make it undiscoverable
    */
    int stop();
    int subscribe();
    int unsubscribe();
    /**
     * this will be our main entry point for device config
    */
    int accept_commands();
    /**
     * save and send all of the recived configuration to device and aws
    */
    int register_device();

    ~Bluetooth();

    std::map<std::string, std::function<int(const std::string&, json)>> callbacks;
    bool running;
    // std::string url;
    json *info;
    int time;

    // variables
    MQTTClientServer *client;
    std::string __prefix;
    std::unordered_map<std::string, std::string> __config;
    std::vector<std::string> __config_dep;
    std::thread *__bluetooth_thread;
    int __bluetooth_fd;
    std::string __type;
    Log *__log;

    bool __kill_bluetooth_flag;
private:
    int __start();
};


#endif