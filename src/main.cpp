#include <stdio.h>
#include <unistd.h>
#include <map>
#include <string>
#include <csignal>
#include "utils/mqtt_client.hpp"
#include "utils/log.hpp"
#include "wifi/wifi.hpp"
#include "bluetooth/bluetooth.hpp"
#include "ir_sensor/ir_sensor.hpp"

Log global_log = Log("GLOBAL");
MQTTClientServer *mqtt_client;
Bluetooth *bluetooth_instance;
Wifi *wifi_instance;
IRSensor *ir_sensor_instance;

void handle_signal(int signal) {
    global_log.print(LOG_NORMAL, "Received signal %d, cleaning up and exitng!", signal);
    if (mqtt_client)
        delete mqtt_client;
    if (bluetooth_instance) {
        delete bluetooth_instance;
    }
    if (wifi_instance) {
        delete wifi_instance;
    }

    exit(signal);
}

void register_signal_handlers() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGQUIT, handle_signal);
    signal(SIGHUP, handle_signal);
}

int main(int argc, char **argv)
{
    global_log.print(LOG_SUCCESS, "DAEMON STARTED");
    global_log.print(LOG_NORMAL, "main");
    // EXIT SIGNALS
    register_signal_handlers();

    // mqtt
    mqtt_client = new MQTTClientServer();
    mqtt_client->connect("tcp://127.0.0.1:1883", "system_daemon");

    global_log.print(LOG_NORMAL, "WAITING 3 SECOUNDS FOR THINGS TO SETTLE...");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // bluetooth
    bluetooth_instance = new Bluetooth();
    bluetooth_instance->client = mqtt_client;
    bluetooth_instance->subscribe();

    // wifi
    wifi_instance = new Wifi();
    wifi_instance->client = mqtt_client;
    wifi_instance->subscribe();
    wifi_instance->start();

    // ir_sensor
    ir_sensor_instance = new IRSensor();
    ir_sensor_instance->client = mqtt_client;
    ir_sensor_instance->subscribe();
    // ir_sensor_instance->start();

    global_log.print(LOG_NORMAL, "PUBLISHING INFO TOPICS...");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    bluetooth_instance->publish_info();
    wifi_instance->publish_info();
    ir_sensor_instance->publish_info();

    int time = 0;
    while (1) {
        json status_p = {
            {"running", true},
            {"time", time}
        };
        mqtt_client->publish("System/Info", status_p, 1);
        global_log.print(LOG_NORMAL, "running : %d", time);
        time++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}