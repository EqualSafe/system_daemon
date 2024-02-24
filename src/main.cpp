
#include <stdio.h>
#include <unistd.h>
#include <map>
#include <string>
#include <csignal>
#include "utils/mqtt_client.hpp"
#include "utils/log.hpp"
//#include "bluetooth/bluetooth.hpp"

Log global_log = Log("GLOBAL");
MQTTClientServer *mqtt_client;

void handle_signal(int signal) {
    global_log.print(LOG_NORMAL, "Received signal %d, cleaning up and exitng!", signal);
    if (mqtt_client)
        delete mqtt_client;

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
    // bluetooth

    // wifi

    // lock

    global_log.print(LOG_NORMAL, "WAITING 3 SECOUNDS FOR THINGS TO SETTLE...");
    std::this_thread::sleep_for(std::chrono::seconds(3));

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