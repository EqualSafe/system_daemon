// MqttWrapper.cpp
#include "MqttWrapper.hpp"

// Constructor
MqttWrapper::MqttWrapper(const std::string &endpoint, const std::string &certPath, const std::string &keyPath, const std::string &clientId)
    : endpoint_(endpoint), certPath_(certPath), keyPath_(keyPath), clientId_(clientId)
{
    /************************ Setup ****************************/
    int test_argc = 7;
    char *test_argv[] = {
        strdup(clientId_.c_str()),
        strdup("--endpoint"), strdup(endpoint_.c_str()),
        strdup("--cert"), strdup(certPath_.c_str()),
        strdup("--key"), strdup(keyPath_.c_str()),
        nullptr};

    /**
     * cmdData is the arguments/input from the command line placed into a single struct for
     * use in this sample. This handles all of the command line parsing, validating, etc.
     * See the Utils/CommandLineUtils for more information.
     */
    Utils::cmdData cmdData = Utils::parseSampleInputPubSub(test_argc, test_argv, &apiHandle, "mqtt5-pubsub");

    // Create the MQTT5 builder and populate it with data from cmdData.
    builder = Aws::Iot::Mqtt5ClientBuilder::NewMqtt5ClientBuilderWithMtlsFromPath(
        cmdData.input_endpoint, cmdData.input_cert.c_str(), cmdData.input_key.c_str());

    // Check if the builder setup correctly.
    if (builder == nullptr)
    {
        printf(
            "Failed to setup mqtt5 client builder with error code %d: %s", LastError(), ErrorDebugString(LastError()));
    }

    // Setup connection options
    std::shared_ptr<Mqtt5::ConnectPacket> connectOptions = std::make_shared<Mqtt5::ConnectPacket>();
    connectOptions->WithClientId(cmdData.input_clientId);
    builder->WithConnectOptions(connectOptions);
    if (cmdData.input_port != 0)
    {
        builder->WithPort(static_cast<uint32_t>(cmdData.input_port));
    }

    // Setup lifecycle callbacks
    builder->WithClientConnectionSuccessCallback(
        [](const Mqtt5::OnConnectionSuccessEventData &eventData)
        {
            fprintf(
                stdout,
                "Mqtt5 Client connection succeed, clientid: %s.\n",
                eventData.negotiatedSettings->getClientId().c_str());
        });
    builder->WithClientConnectionFailureCallback([](
                                                     const Mqtt5::OnConnectionFailureEventData &eventData)
                                                 { fprintf(stdout, "Mqtt5 Client connection failed with error: %s.\n", aws_error_debug_str(eventData.errorCode)); });
    builder->WithClientStoppedCallback([](const Mqtt5::OnStoppedEventData &)
                                       { fprintf(stdout, "Mqtt5 Client stopped.\n"); });
    builder->WithClientAttemptingConnectCallback([](const Mqtt5::OnAttemptingConnectEventData &)
                                                 { fprintf(stdout, "Mqtt5 Client attempting connection...\n"); });
    builder->WithClientDisconnectionCallback([](const Mqtt5::OnDisconnectionEventData &eventData)
                                             { fprintf(stdout, "Mqtt5 Client disconnection with reason: %s.\n", aws_error_debug_str(eventData.errorCode)); });
}

// Destructor
MqttWrapper::~MqttWrapper(){};

// Connect
int MqttWrapper::connect()
{
    // This is invoked upon the receipt of a Publish on a subscribed topic.
    builder->WithPublishReceivedCallback([this](const Mqtt5::PublishReceivedEventData &eventData)
                                         { this->__message_arrived(eventData); });
    // Create Mqtt5Client
    client = builder->Build();

    if (client == nullptr)
    {
        fprintf(stdout, "Failed to Init Mqtt5Client with error code %d: %s", LastError(), ErrorDebugString(LastError()));
        return -1;
    }

    if (!(client->Start()))
    {
        printf("Faild to start client.");
        return -1;
    }
    return 0;
}

// Disconnect
int MqttWrapper::disconnect()
{
    if (!client->Stop())
    {
        fprintf(stdout, "Failed to disconnect from the mqtt connection. Exiting..\n");
        return -1;
    }
    return 0;
}

int MqttWrapper::subscribe(const std::string &topic, callback_t callback)
{
    auto onSubAck = [](int error_code, std::shared_ptr<Mqtt5::SubAckPacket> suback)
    {
        if (error_code != 0)
        {
            fprintf(
                stdout,
                "MQTT5 Client Subscription failed with error code: (%d)%s\n",
                error_code,
                aws_error_debug_str(error_code));
        }
        if (suback != nullptr)
        {
            for (Mqtt5::SubAckReasonCode reasonCode : suback->getReasonCodes())
            {
                if (reasonCode > Mqtt5::SubAckReasonCode::AWS_MQTT5_SARC_UNSPECIFIED_ERROR)
                {
                    fprintf(
                        stdout,
                        "MQTT5 Client Subscription failed with server error code: (%d)%s\n",
                        reasonCode,
                        suback->getReasonString()->c_str());
                    return;
                }
            }
        }
    };

    Mqtt5::Subscription sub1((Aws::Crt::String)topic.c_str(), Mqtt5::QOS::AWS_MQTT5_QOS_AT_LEAST_ONCE);
    sub1.WithNoLocal(false);
    std::shared_ptr<Mqtt5::SubscribePacket> subPacket = std::make_shared<Mqtt5::SubscribePacket>();
    subPacket->WithSubscription(std::move(sub1));

    if (callbacks.find(topic) == callbacks.end())
    {
        client->Subscribe(subPacket, onSubAck);
    }

    callbacks[topic].push_back(callback);
    return 0;
};

// Unsubscribe
int MqttWrapper::unsubscribe(const std::string &topic, callback_t callback)
{
    auto &cbVector = callbacks[topic];
    // Remove the specified callback
    cbVector.erase(
        std::remove_if(cbVector.begin(), cbVector.end(),
                       [&callback](const callback_t &cb)
                       {
                           return cb.target_type().name() == callback.target_type().name();
                       }),
        cbVector.end());

    // If no callbacks left for the topic, unsubscribe from the MQTT client and remove the topic
    if (cbVector.empty())
    {
        std::shared_ptr<Mqtt5::UnsubscribePacket> unsub = std::make_shared<Mqtt5::UnsubscribePacket>();
        unsub->WithTopicFilter((Aws::Crt::String)topic.c_str());
    }

    return 0; // Assuming success, handle errors as needed
}

// Publish
int MqttWrapper::publish(const std::string &topic, const std::string &payload, bool retain)
{
    auto onPublishComplete = [](int, std::shared_ptr<Aws::Crt::Mqtt5::PublishResult> result)
    {
        if (!result->wasSuccessful())
        {
            fprintf(stdout, "Publish failed with error_code: %d", result->getErrorCode());
        }
        else if (result != nullptr)
        {
            std::shared_ptr<Mqtt5::PubAckPacket> puback =
                std::dynamic_pointer_cast<Mqtt5::PubAckPacket>(result->getAck());
            if (puback->getReasonCode() == 0)
            {
                fprintf(stdout, "Publish Succeed.\n");
            }
            else
            {
                fprintf(
                    stdout,
                    "PubACK reason code: %d : %s\n",
                    puback->getReasonCode(),
                    puback->getReasonString()->c_str());
            }
        };
    };
    ByteCursor message = ByteCursorFromCString(payload.c_str());
    std::shared_ptr<Mqtt5::PublishPacket> publish = std::make_shared<Mqtt5::PublishPacket>(
        (Aws::Crt::String)topic.c_str(), message, Mqtt5::QOS::AWS_MQTT5_QOS_AT_LEAST_ONCE);

    // retain fleg is set here
    publish->WithRetain(retain);
    // publish
    client->Publish(publish, onPublishComplete);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return 0;
};

bool MqttWrapper::__topic_matches_subscription(const std::string &sub, const std::string &topic)
{
    std::istringstream subStream(sub);
    std::istringstream topicStream(topic);
    std::string subPart, topicPart;

    while (std::getline(subStream, subPart, '/') && std::getline(topicStream, topicPart, '/'))
    {
        if (subPart == "#")
            return true; // Multi-level wildcard matches all remaining levels
        if (subPart != "+" && subPart != topicPart)
            return false; // Direct mismatch
    }

    // Handle case where subscription or topic is longer than the other
    return true;
}

int MqttWrapper::__message_arrived(const Mqtt5::PublishReceivedEventData &eventData)
{
    // Extract topic and payload from eventData.
    std::string topic(eventData.publishPacket->getTopic().c_str());
    std::string payload(reinterpret_cast<const char *>(eventData.publishPacket->getPayload().ptr), eventData.publishPacket->getPayload().len);

    // Iterate through the registered callbacks to see if any match the topic of the received message.
    for (const auto &sub : callbacks)
    {
        if (__topic_matches_subscription(sub.first, topic))
        {
            for (auto &cb : sub.second)
            {
                new std::thread([&cb, topic, payload]
                                { return cb(topic, payload); });
            }
        }
    }
    return 0;
}