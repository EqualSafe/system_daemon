#include <stdio.h>
#include <unistd.h>
#include <map>
#include <string>
#include <csignal>
#include "utils/mqtt_client.hpp"
#include "utils/log.hpp"
#include "wifi/wifi.hpp"
#include "bluetooth/bluetooth.hpp"

Log global_log = Log("GLOBAL");
MQTTClientServer *mqtt_client;
Bluetooth *bluetooth_instance;
Wifi *wifi_instance;

void handle_signal(int signal) {
    global_log.print(LOG_NORMAL, "Received signal %d, cleaning up and exitng!", signal);
    if (mqtt_client)
        delete mqtt_client;
    if (bluetooth_instance) {
        delete bluetooth_instance;
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
    /**
     * The following should be changed to only allow bluetooth
     * to start when an external signal (mqtt) is issued...
     *
     * If the network is up and running, we should route all
     * of our communication with the device through AWS.
     *
     * On connection drop, we revert to bluetooth, so that we
     * can communicate (unlocking/locking/setup_wifi)
    */
    bluetooth_instance->subscribe();
    bluetooth_instance->start();

    // wifi
    wifi_instance = new Wifi();
    wifi_instance->client = mqtt_client;
    wifi_instance->subscribe();
    wifi_instance->start();

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