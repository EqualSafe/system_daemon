#include "bluetooth.hpp"

Bluetooth::Bluetooth()
{
    this->__type = "BLUETOOTH";
    this->running = false;
    this->__prefix = "System/bluetooth";
    json p = {{"running",this->running}};
    this->info = &p;
    this->__log = new Log(this->__type);
}

int Bluetooth::__start()
{
    this->__log->print(LOG_NORMAL, "start");
    int success;
    popen2_t *gatt_p = new popen2_t;
    popen2_t *advertise_p = new popen2_t;

    popen2("python3 /etc/system_daemon/bluetooth_gatt/bluetooth_gatt.py", gatt_p);
    popen2("bluetoothctl advertise on", advertise_p);

    this->running = true;
    this->time = 0;

    while (!this->__kill_bluetooth_flag) {
        json p = {
            {"running", this->running},
            {"running_time", this->time},
        };
        this->info = &p;
        this->time += 1;
        this->publish_info();

        this->__log->print(LOG_NORMAL, "Thread is running");
        this->__log->print(LOG_NORMAL, "GATT pid: %d", gatt_p->child_pid);
        this->__log->print(LOG_NORMAL, "ADVERTISE pid: %d", advertise_p->child_pid);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    kill(gatt_p->child_pid, SIGKILL);
    kill(advertise_p->child_pid, SIGKILL);

    system("sudo pkill -f bluetooth_gatt.py");
    system("sudo pkill -f bluetoothctl");

    this->running = false;
    json p = {
        {"running", this->running},
    };
    this->info = &p;

    return BLUETOOTH_SUCCESS;
}

int Bluetooth::start()
{
    this->__kill_bluetooth_flag = false;
    this->__bluetooth_thread = new std::thread([this] {
        this->__start();
    });
    return BLUETOOTH_SUCCESS;
}

int Bluetooth::stop()
{
    if (this->running && this->__bluetooth_thread) {
        this->__kill_bluetooth_flag = true;
        this->running = false;
        this->__bluetooth_thread->join();
        delete this->__bluetooth_thread;
    }
    return BLUETOOTH_SUCCESS;
}

int Bluetooth::publish_info()
{
    if (!this->info) {
        return BLUETOOTH_COMMAND_ERROR;
    }
    client->publish(this->__prefix + "/Info", *this->info);
    return BLUETOOTH_SUCCESS;
}

int Bluetooth::subscribe()
{
    if (this->__prefix == "" || !this->client) {
        return BLUETOOTH_COMMAND_ERROR;
    }

    callbacks[this->__prefix + "/start/+"] = [&](const std::string& topic, json payload) {
        this->__log->print(LOG_NORMAL, "MQTT start. topic: %s", topic.c_str());
        if (!this->running) {
            this->start();
            json p = {
                {"msg", "bluetooth started successfully"}
            };
            this->__log->print(LOG_SUCCESS, "MQTT start success");
            client->publish(topic + "/success", p);
        } else {
            json p = {
                {"msg", "bluetooth is already running"}
            };
            this->__log->print(LOG_ERROR, "MQTT start error");
            client->publish(topic + "/error", p);
        }
        return 0;
    };

    callbacks[this->__prefix + "/stop/+"] =  [&](const std::string& topic, json payload) {
        this->__log->print(LOG_NORMAL, "MQTT stop. topic: %s", topic.c_str());
        if (this->running) {
            this->stop();
            json p = {
                {"msg", "bluetooth stopped"}
            };
            this->__log->print(LOG_NORMAL, "MQTT stop success");
            client->publish(topic + "/success", p);
        } else {
            json p = {
                {"msg", "bluetooth is not running"}
            };
            this->__log->print(LOG_NORMAL, "MQTT stop error");
            client->publish(topic + "/error", p);
        }
        return 0;
    };

    for (auto &it : callbacks) {
        client->subscribe(it.first, it.second);
    }

    return BLUETOOTH_SUCCESS;
}

int Bluetooth::unsubscribe()
{
    if (!this->client)
    {
        return BLUETOOTH_COMMAND_ERROR;
    }

    for (auto &it : callbacks) {
        client->unsubscribe(it.first, it.second);
    }

    return BLUETOOTH_SUCCESS;
}

Bluetooth::~Bluetooth()
{
    this->stop();
}
