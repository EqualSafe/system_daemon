#include  "wifi.hpp"


Wifi::Wifi()
{
    this->__type = "WIFI";
    this->__prefix = "System/wifi";
    this->wpa_conf_path = "/etc/wpa_supplicant/wpa_supplicant.conf";
    this->__log = new Log(this->__type);
}

int Wifi::restart()
{
    this->stop();
    this->start();
    system("sudo wpa_cli -i wlan0 reconfigure");
    return WIFI_SUCCESS;
}

int Wifi::start()
{
    system("sudo ifconfig wlan0 up");
    system("sudo systemctl enable wpa_supplicant");
    system("sudo systemctl start wpa_supplicant");
    return WIFI_SUCCESS;
}

int Wifi::stop()
{
    system("sudo systemctl stop wpa_supplicant");
    system("sudo ifconfig wlan0 down");
    return WIFI_SUCCESS;
}

int Wifi::add_network(const std::string& ssid, const std::string& password)
{
    // First, remove the network if it already exists
    this->remove_network(ssid);

    // Append the new network block
    std::ofstream out_file(wpa_conf_path, std::ios::app);
    out_file << "\nnetwork={\n";
    out_file << "    ssid=\"" << ssid << "\"\n";
    if (!password.empty()) {
        out_file << "    psk=\"" << password << "\"\n";
    } else {
        out_file << "    key_mgmt=NONE\n";
    }
    out_file << "}\n";
    out_file.close();

    return WIFI_SUCCESS;
}

int Wifi::remove_network(const std::string& ssid) {
    std::ifstream file(this->wpa_conf_path);
    std::string line;
    std::vector<std::string> lines;
    bool ssid_exists = false;

    if (!file.is_open()) {
        this->__log->print(LOG_ERROR, "Couldn't open %s", this->wpa_conf_path.c_str());
        return WIFI_COMMAND_ERROR;
    }

    // Read the current configuration into memory
    while (getline(file, line)) {
        lines.push_back(line);
        if (line.find("ssid=\"" + ssid + "\"") != std::string::npos) {
            ssid_exists = true;
        }
    }
    file.close();

    if (!ssid_exists) {
        this->__log->print(LOG_ERROR, "Couldn't find SSID %s", ssid.c_str());
        return WIFI_COMMAND_ERROR;
    }

    // Remove the existing network block for the specified SSID
    std::vector<std::string> new_lines;
    bool in_block = false;
    for (const auto& l : lines) {
        if (l.find("network={") != std::string::npos) {
            in_block = true;
        }
        if (!in_block) {
            new_lines.push_back(l);
        }
        if (l.find("ssid=\"" + ssid + "\"") != std::string::npos) {
            in_block = false; // Start skipping lines
        }
        if (l.find("}") != std::string::npos && in_block) {
            in_block = false; // Stop skipping lines after the block ends
            continue;
        }
    }

    // Write the updated configuration
    std::ofstream out_file(wpa_conf_path, std::ios::trunc);
    for (const auto& l : new_lines) {
        out_file << l << std::endl;
    }
    out_file.close();

    this->__log->print(LOG_SUCCESS, "Removed SSID %s", ssid.c_str());
    return WIFI_SUCCESS;
}

int Wifi::connect(const std::string &ssid, const std::string &password)
{
    this->add_network(ssid, password);
    this->restart();

    return WIFI_SUCCESS;
}

int Wifi::subscribe()
{
    if (this->__prefix == "" || !this->client)
    {
        return WIFI_COMMAND_ERROR;
    }

    callbacks[this->__prefix + "/start/+"] = [&](const std::string& topic, json payload)
    {
        this->__log->print(LOG_NORMAL, "MQTT start. topic: %s", topic.c_str());
        if (!this->running) {
            this->start();
            json p = {
                {"msg", "starting"}
            };
            this->__log->print(LOG_SUCCESS, "MQTT start success");
            client->publish(topic + "/success", p);
        } else {
            json p = {
                {"msg", "already running"}
            };
            this->__log->print(LOG_ERROR, "MQTT start error");
            client->publish(topic + "/error", p);
        }
        return 0;
    };

    callbacks["sys/ble_serial/rx/+"] = [&](const std::string& topic, json payload)
    {
        if (payload["command"] == "add_wifi") {
            this->__log->print(LOG_NORMAL, "Got ble command add_wifi. topic: %s", topic.c_str());
            if (!payload["ssid"].is_string() || payload["ssid"].empty()) {
                json p = {
                    {"ts", "UNKNOWN"},
                    {"status", "error"},
                    {"msg", "no ssid provided"}
                };
                this->__log->get_time();
                this->client->publish("sys/ble_serial/tx/" + this->__log->get_time(), p);
                return 0;
            }

            std::string password = payload["password"].is_string() ? payload["password"] : "";

            this->connect(payload["ssid"], password);
            json p = {
                {"ts", "UNKNOWN"},
                {"status", "success"},
            };
            this->client->publish("sys/ble_serial/tx/" + this->__log->get_time(), p);
        }
        return 0;
    };

    for (auto &it : callbacks) {
        client->subscribe(it.first, it.second);
    }

    return WIFI_SUCCESS;
}


int Wifi::unsubscribe()
{
    if (!this->client)
    {
        return WIFI_COMMAND_ERROR;
    }

    for (auto &it : callbacks) {
        client->unsubscribe(it.first, it.second);
    }

    return WIFI_SUCCESS;
}
