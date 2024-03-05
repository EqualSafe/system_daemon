# AWS MQTT Wrapper README

Welcome to the AWS MQTT Wrapper, a convenient C++ wrapper for the AWS IoT Device SDK for C++ v2. This wrapper simplifies connecting to, publishing to, and subscribing to topics in AWS IoT Core using MQTT. Before you can use this wrapper, there are a few prerequisites and setup steps you need to follow.

## Prerequisites

To use the AWS MQTT Wrapper, you need to have the AWS IoT Device SDK for C++ v2 installed in your environment. Additionally, ensure you have CMake and a C++ compiler (like GCC or Clang) available for building the SDK and your projects.

## Installation

### Step 1: Install AWS IoT Device SDK for C++ v2

The AWS MQTT Wrapper depends on the AWS IoT Device SDK for C++ v2. Follow the installation instructions provided in the SDK's GitHub repository to install the SDK:

[AWS IoT Device SDK for C++ v2 GitHub Repository](https://github.com/aws/aws-iot-device-sdk-cpp-v2/tree/main)

The installation steps generally involve cloning the SDK repository, installing dependencies, and building the SDK using CMake.

### Step 2: Include the AWS MQTT Wrapper in Your Project

After installing the SDK, you can include the AWS MQTT Wrapper in your project. Make sure to link against the AWS IoT Device SDK for C++ v2 libraries when building your application.

## Usage

To use the AWS MQTT Wrapper in your application, include the wrapper's header file and instantiate the wrapper object with your AWS IoT Core endpoint, certificate path, private key path, and a client ID. You can then connect to AWS IoT Core, subscribe to topics, and publish messages.

Here's a simple example:

```cpp
#include "MqttWrapper.hpp"

int main() {
    MqttWrapper mqttClient("<endpoint>", "<certPath>", "<keyPath>", "clientId");
    
    if (mqttClient.connect() != 0) {
        std::cerr << "Failed to connect to the MQTT broker." << std::endl;
        return 1;
    }
    
    // Subscribe and publish logic here
    
    return 0;
}