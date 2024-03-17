# System Daemon

## Responsible for:

1. Bluetooth
2. Wifi
3. IR Sensor

## IMPORTANT NOTES:

please run `requirments.sh` before running on system


## Links

* [Documentation](https://github.com/404) ...*to be added*


## Build and Install

```
. requiremnets.sh
. setup_env.sh
make clean && make all
sudo make install
```

## Systemd Commands

* Start
    ```
    sudo systemctl daemon-reload
    sudo systemctl start system_daemon.service
    ```

* Start on bootup
    ```
    sudo systemctl enable system_daemon.service
    ```

* Get status
    ```
    sudo systemctl status system_daemon.service
    ```

**NOTE: You will need the device password to install the requiremnets.**

## Run in foreground

```
killall system_daemon; system_daemon
```

**NOTE: If the daemon is not installed for systemd and added to path use the following command:**

```
sudo ./build/bin/system_daemon
```

## MQTT

The daemon reserves the topic `System` and all of the subtopics underneith it. To check for the daemon info, subscribe to `System/Info`. For other topics and actions, please check the following documentation: [DOCUMENTATION](https://github.com/404)