#ifndef     BLUETOOTH
#define     BLUETOOTH

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
    BLUETOOTH_SUCCESS,
    BLUETOOTH_COMMAND_ERROR,
    BLUETOOTH_THREAD_ERROR,
    BLUETOOTH_CONFIG_ERROR,
} BLUETOOTH_ERROR_T;

class Bluetooth {
public:
    Bluetooth();
    Bluetooth(std::unordered_map<std::string, std::string> config);
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

    ~Bluetooth();

    std::map<std::string, std::function<int(const std::string&, json)>> callbacks;
    bool running;
    json info;
    int time;

    // variables
    MQTTClientServer *client;
    std::string __prefix;
    std::thread *__bluetooth_thread;
    int __bluetooth_fd;
    std::string __type;
    Log *__log;

    /**
     * TODO:
     *      MAKE THE UUIDS CONFIGIRABLE
     *      INSIDE OF THE SYSTEM DAEMON
     *      NOT THE SCRIPT
    */
    std::string service_uuid;
    std::string char_tx_uuid;
    std::string char_rx_uuid;

    bool __kill_bluetooth_flag;
private:
    int __start();
};


#endif