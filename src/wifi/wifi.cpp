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
    this->__log->print(LOG_SUCCESS, "Restarting WIFI");
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

    this->__log->print(LOG_SUCCESS, "Added SSID %s", ssid.c_str());

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
        this->__log->print(LOG_ERROR, "Couldn't find in retained networks, didn't remove SSID %s", ssid.c_str());
        return WIFI_COMMAND_ERROR;
    }


    std::vector<std::string> new_lines;
    bool in_block = false;
    bool remove_block = false;
    std::vector<std::string> current_block_lines; // Use a vector to hold lines for the current block

    for (const auto& line : lines) {
        if (line.find("network={") != std::string::npos) {
            in_block = true;
            remove_block = false; // Reset at the start of each new block
            current_block_lines.clear(); // Clear previous block content
            current_block_lines.push_back(line); // Start capturing the new block
            continue; // Move to the next line
        }

        if (in_block) {
            current_block_lines.push_back(line); // Continue capturing lines for the current block
            if (line.find("ssid=\"" + ssid + "\"") != std::string::npos) {
                remove_block = true; // Mark for removal if the SSID matches
            }
            if (line.find("}") != std::string::npos) {
                in_block = false; // We've reached the end of a block
                if (!remove_block) {
                    // If the block is not marked for removal, add the entire block to new_lines
                    for (const auto& block_line : current_block_lines) {
                        new_lines.push_back(block_line);
                    }
                    // Add a newline after each block, except for the last one
                    new_lines.push_back("");
                }
                continue;
            }
        } else {
            // Directly add lines that are outside of network blocks, checking for redundant newlines
            if (!line.empty() || (!new_lines.empty() && !new_lines.back().empty())) {
                new_lines.push_back(line);
            }
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
