/**
 * IR Sensor
 *
 * This module will use the pin XX to trigger a callback
 * that will publish a status of if someone is at the door
*/

#ifndef     IRSENSOR
#define     IRSENSOR

#ifndef     MAX_INPUT
#define     MAX_INPUT       1024
#endif

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

#include "../utils/log.hpp"
#include "../utils/mqtt_client.hpp"

typedef enum {
    IRSENSOR_SUCCESS,
    IRSENSOR_COMMAND_ERROR,
    IRSENSOR_THREAD_ERROR,
    IRSENSOR_CONFIG_ERROR,
} IRSENSOR_ERROR_T;

class IRSensor {
public:
    IRSensor();
    IRSensor(std::unordered_map<std::string, std::string> config);
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

    ~IRSensor();

    std::map<std::string, std::function<int(const std::string&, json)>> callbacks;
    bool running;
    json *info;
    int time;

    // variables
    MQTTClientServer *client;
    std::string __prefix;
    // std::thread *__bluetooth_thread;
    // int __bluetooth_fd;
    std::string __type;
    Log *__log;

    bool __kill_bluetooth_flag;
private:
    int __start();
};

#endif