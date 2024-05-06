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

#define     DEFAULT_IR_PIN  20

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

#include <mutex>
#include <chrono>
#include <thread>

#include <pigpio.h>

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
    IRSensor(int pin);
    int publish_info();
    int start();
    int stop();
    int subscribe();
    int unsubscribe();

    ~IRSensor();

    std::map<std::string, std::function<int(const std::string&, json)>> callbacks;
    bool running;
    json info;
    int time;

    // variables
    MQTTClientServer *client;
    std::string __prefix;
    std::string __type;
    Log *__log;

private:
    int pin;
    std::thread *timer_thread;
    std::mutex timer_lock;
    bool timer_reset;

    void __start_timer();
    static void __gpio_callback(int gpio, int level, uint32_t tick, void *udata);
};

#endif