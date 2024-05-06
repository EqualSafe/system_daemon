#include "ir_sensor.hpp"

IRSensor::IRSensor() {
    this->__type = "IR_SENSOR";
    this->__prefix = "System/ir_sensor";
    this->pin = DEFAULT_IR_PIN;
    this->info = json::parse(R"({})");
    this->info["state"] = "stopped";
    this->__log = new Log(this->__type);
    if (gpioInitialise() < 0) {
        this->__log->print(LOG_ERROR, "Error initializing gpio");
    }
}

IRSensor::IRSensor(int gpio_pin) {
    this->__type = "IR_SENSOR";
    this->__prefix = "System/ir_sensor";
    this->pin = gpio_pin;
    this->info = json::parse(R"({})");
    this->info["state"] = "stopped";
    this->__log = new Log(this->__type);
    if (gpioInitialise() < 0) {
        this->__log->print(LOG_ERROR, "Error initializing gpio");
    }
}

int IRSensor::start() {
    this->running = true;
    gpioSetMode(this->pin, PI_INPUT);
    gpioSetISRFuncEx(this->pin, EITHER_EDGE, 0, this->__gpio_callback, this);
    this->info = json::parse(R"({})");
    this->info["state"] = "running";
    this->info["detect"] = false;
    this->publish_info();
    return IRSENSOR_SUCCESS;
}

int IRSensor::stop() {
    this->running = false;
    gpioSetISRFuncEx(this->pin, EITHER_EDGE, 0, nullptr, nullptr);
    this->info = json::parse(R"({})");
    this->info["state"] = "stopped";
    this->info["detect"] = false;
    this->publish_info();
    return IRSENSOR_SUCCESS;
}

int IRSensor::publish_info()
{
    if (!this->info.is_object()) {
        return IRSENSOR_COMMAND_ERROR;
    }
    this->client->publish(this->__prefix + "/Info", this->info, 1);
    return IRSENSOR_SUCCESS;
}

void IRSensor::__start_timer()
{
    this->__log->print(LOG_SUCCESS, "Starting 10 seconds timer");
    std::lock_guard<std::mutex> lock(this->timer_lock);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    if (this->timer_reset) {
        this->__log->print(LOG_WARNING, "Exiting timer without publishing");
        return;
    }
    this->__log->print(LOG_SUCCESS, "No movement detected, setting detect to false and publishing");
    this->info = json::parse(R"({})");
    this->info["state"] = "running";
    this->info["detect"] = false;
    this->publish_info();

    this->timer_reset = false;
}

void IRSensor::__gpio_callback(int gpio, int level, uint32_t tick, void *udata) {
    IRSensor *self = static_cast<IRSensor*>(udata);
    self->__log->print(LOG_NORMAL, "ISR function triggered");

    if (level) {
        self->__log->print(LOG_SUCCESS, "IR Sensor pin HIGH");
        if (self->info["detect"].is_boolean() && self->info["detect"] == true) {
            self->__log->print(LOG_WARNING, "detect is already true, will not publish");
            return;
        }
        self->info = json::parse(R"({})");
        self->info["state"] = "running";
        self->info["detect"] = true;
        self->publish_info();
    } else {
        self->__log->print(LOG_SUCCESS, "IR Sensor pin LOW");
        if (self->timer_reset) {
            return;
        }
        if (self->timer_thread) {
            self->timer_reset = true;
            self->__log->print(LOG_SUCCESS, "Joining timer and restarting...");
            self->timer_thread->join();
            delete self->timer_thread;
            self->timer_reset = false;
        }
        // Start timer...
        self->timer_thread = new std::thread(&IRSensor::__start_timer, self);
    }
}

int IRSensor::subscribe()
{
    if (this->__prefix == "" || !this->client)
    {
        return IRSENSOR_COMMAND_ERROR;
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

    callbacks[this->__prefix + "/stop/+"] = [&](const std::string& topic, json payload)
    {
        this->__log->print(LOG_NORMAL, "MQTT stop. topic: %s", topic.c_str());
        if (this->running) {
            this->stop();
            json p = {
                {"msg", "stopping"}
            };
            this->__log->print(LOG_SUCCESS, "MQTT start success");
            client->publish(topic + "/success", p);
        } else {
            json p = {
                {"msg", "already stopped"}
            };
            this->__log->print(LOG_ERROR, "MQTT start error");
            client->publish(topic + "/error", p);
        }
        return 0;
    };

    for (auto &it : callbacks) {
        client->subscribe(it.first, it.second);
    }

    return IRSENSOR_SUCCESS;
}

int IRSensor::unsubscribe()
{
    if (!this->client)
    {
        return IRSENSOR_COMMAND_ERROR;
    }

    for (auto &it : callbacks) {
        client->unsubscribe(it.first, it.second);
    }

    return IRSENSOR_SUCCESS;
}

IRSensor::~IRSensor() {
    stop();
    gpioTerminate();
}
