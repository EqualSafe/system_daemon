#include "bluetooth.hpp"

Bluetooth::Bluetooth()
{
    this->__type = "BLUETOOTH";
    this->running = false;
    json p = {{"running",this->running}};
    this->info = &p;
    this->__log = new Log();
}

Bluetooth::Bluetooth(std::unordered_map<std::string, std::string> config)
{
}

int Bluetooth::set_config(std::unordered_map<std::string, std::string> config)
{
    return 0;
}

int Bluetooth::publish_info()
{
    return 0;
}

int Bluetooth::start()
{
    int success;
    popen2_t *childp = new popen2_t;
    success = popen2("python3 /etc/scripts/system_daemon/bluetooth_gatt/bluetooth_gatt.py", childp);

    if (success != 0)
    {
        this->__log->print(LOG_ERROR, "FFmpeg popen error");
        return BLUETOOTH_COMMAND_ERROR;
    }

    this->running = true;
    this->time = 0;

    while (!this->__kill_bluetooth_flag) {
        // read from the pipe here
        // ...
        json p = {
            {"running", this->running},
            {"running_time", this->time}
        };
        this->info = &p;
        this->time += 1;
        this->publish_info();

        this->__log->print(LOG_NORMAL, "Thread is running");
        this->__log->print(LOG_NORMAL, "pid: %d", childp->child_pid);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    kill(childp->child_pid, SIGINT);

    this->running = false;
    json p = {
        {"running", this->running},
    };
    this->info = &p;

    return BLUETOOTH_SUCCESS;
}

int Bluetooth::stop()
{
    return 0;
}

int Bluetooth::subscribe()
{
    return 0;
}

int Bluetooth::unsubscribe()
{
    return 0;
}

int Bluetooth::accept_commands()
{
    return 0;
}

int Bluetooth::register_device()
{
    return 0;
}

Bluetooth::~Bluetooth()
{
}
