#include "TRLIotCoreAdapter.hpp"

TRLIotCoreAdapter::TRLIotCoreAdapter() {}

TRLIotCoreAdapter::~TRLIotCoreAdapter()
{
    if (m_connection->Disconnect()) {
        m_closed_promise.get_future().wait();
    }
}

bool TRLIotCoreAdapter::Initialize(
    const std::string &aws_iot_config_file_path,
    const std::string &lift_config_file_path,
    const std::string &doors_config_file_path)
{
    try {
        BOOST_LOG_TRIVIAL(info) << "TRLIotCoreAdapter::Initializing";

        YAML::Node door_config = YAML::LoadFile(doors_config_file_path);
        for (YAML::const_iterator it = door_config.begin();
             it != door_config.end();
             ++it) {
            std::string name = it->first.as<std::string>();
            std::shared_ptr<TRLDoorInterface> door =
                std::make_shared<TRLDoorInterface>();
            door->Initialize(name, door_config[name]);
            m_doors.push_back(door);
        }

        YAML::Node lift_config = YAML::LoadFile(lift_config_file_path);
        for (YAML::const_iterator it = lift_config.begin();
             it != lift_config.end();
             ++it) {
            std::string name = it->first.as<std::string>();
            std::shared_ptr<TRLLiftInterface> lift =
                std::make_shared<TRLLiftInterface>();
            lift->Initialize(name, lift_config[name]);
            m_lifts.push_back(lift);
        }
        // load yaml
        YAML::Node config = YAML::LoadFile(aws_iot_config_file_path);
        BOOST_LOG_TRIVIAL(info) << "TRLIotCoreAdapter::Yaml Loaded";
        if (config["ca_file_path"] && config["cert_path"] &&
            config["key_path"] && config["client_id"] && config["aws_url"] &&
            config["lift_state_topic"] && config["lift_command_topic"] &&
            config["door_state_topic"] && config["door_command_topic"]) {
            m_lift_state_topic = config["lift_state_topic"].as<std::string>();
            m_lift_command_topic =
                config["lift_command_topic"].as<std::string>();
            m_door_state_topic = config["door_state_topic"].as<std::string>();
            m_door_command_topic =
                config["door_command_topic"].as<std::string>();
            std::string cert_path = config["cert_path"].as<std::string>();
            std::string key_path = config["key_path"].as<std::string>();
            std::string aws_url = config["aws_url"].as<std::string>();
            std::string ca_file_path = config["ca_file_path"].as<std::string>();
            m_internal_client = Aws::Iot::MqttClient();

            if (!m_internal_client) {
                BOOST_LOG_TRIVIAL(error)
                    << ("TRLIotCoreAdapter::initialize MQTT Client Creation failed with error %s",
                        Aws::Crt::ErrorDebugString(
                            m_internal_client.LastError()));
                return false;
            }

            m_connection = BuildDirectMQTTConnection(
                m_internal_client,
                cert_path,
                key_path,
                aws_url,
                ca_file_path);

            auto onConnectionCompleted = [&](Aws::Crt::Mqtt::MqttConnection &,
                                             int error_code,
                                             Aws::Crt::Mqtt::ReturnCode
                                                 return_code,
                                             bool) {
                if (error_code) {
                    BOOST_LOG_TRIVIAL(error)
                        << "TRLIotCoreAdapter::initialize MQTT Client Connection failed with error code: "
                        << error_code;
                    return false;
                } else {
                    BOOST_LOG_TRIVIAL(info)
                        << "TRLIotCoreAdapter::initialize Connection completed with return code "
                        << return_code;
                }

                CreateSubscribers();
                m_connected = true;
                return true;
            };

            auto onInterrupted = [&](Aws::Crt::Mqtt::MqttConnection &,
                                     int error) {
                BOOST_LOG_TRIVIAL(error)
                    << "m_connection::onInterrupted interrupted with error: "
                    << error;
            };

            auto onResumed = [&](Aws::Crt::Mqtt::MqttConnection &,
                                 Aws::Crt::Mqtt::ReturnCode,
                                 bool) {
                BOOST_LOG_TRIVIAL(info)
                    << ("m_connection::onResumedConnection resumed");
            };

            auto onDisconnect = [&](Aws::Crt::Mqtt::MqttConnection &) {
                {
                    BOOST_LOG_TRIVIAL(info)
                        << ("m_connection::onDisconnect Disconnection completed.");
                    m_closed_promise.set_value();
                }
            };
            m_connection->OnConnectionCompleted =
                std::move(onConnectionCompleted);
            m_connection->OnDisconnect = std::move(onDisconnect);
            m_connection->OnConnectionInterrupted = std::move(onInterrupted);
            m_connection->OnConnectionResumed = std::move(onResumed);

            // connecting
            BOOST_LOG_TRIVIAL(info) << ("TRLIotCoreAdapter::MQTT Connecting..");

            if (!m_connection->Connect(
                    config["client_id"].as<string>().c_str(),
                    false /*cleanSession*/,
                    1000 /*keepAliveTimeSecs*/)) {
                BOOST_LOG_TRIVIAL(error)
                    << "TRLIotCoreAdapter::initialize MQTT Connecting Failed.";
                return false;
            }

            BOOST_LOG_TRIVIAL(info) << "TRLIotCoreAdapter::Initialize done!!";
            return true;
        } else {
            BOOST_LOG_TRIVIAL(error)
                << "TRLIotCoreAdapter:initialize failed. Config file needs to have ca_file_path, cert_path, key_path, client_id, lift_command_topic, lift_state_topic, door_command_topic, door_state_topic, and aws_url.";
            return false;
        }
    } catch (const YAML::ParserException &ex) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLIotCoreAdapter::initialize failed YAML error.";
        return false;
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLIotCoreAdapter::initialize failed. " << e.what();
        return false;
    }
}

std::shared_ptr<Aws::Crt::Mqtt::MqttConnection>
TRLIotCoreAdapter::BuildDirectMQTTConnection(
    Aws::Iot::MqttClient &client,
    std::string &cert_path,
    std::string &key_path,
    std::string &aws_url,
    std::string &ca_file_path)
{
    auto client_config_builder = Aws::Iot::MqttClientConnectionConfigBuilder(
        cert_path.c_str(),
        key_path.c_str());
    client_config_builder.WithEndpoint(aws_url.c_str());
    client_config_builder.WithCertificateAuthority(ca_file_path.c_str());
    return GetClientConnectionForMQTTConnection(client, client_config_builder);
}

std::shared_ptr<Aws::Crt::Mqtt::MqttConnection>
TRLIotCoreAdapter::GetClientConnectionForMQTTConnection(
    Aws::Iot::MqttClient &client,
    Aws::Iot::MqttClientConnectionConfigBuilder &client_config_builder)
{
    auto clientConfig = client_config_builder.Build();
    if (!clientConfig) {
        BOOST_LOG_TRIVIAL(fatal)
            << ("TRLIotCoreAdapter::GetClientConnectionForMQTTConnection Client Configuration initialization failed with error %s\n",
                Aws::Crt::ErrorDebugString(clientConfig.LastError()));
        exit(-1);
    }

    auto connection = client.NewConnection(clientConfig);
    if (!*connection) {
        BOOST_LOG_TRIVIAL(fatal)
            << ("TRLIotCoreAdapter::GetClientConnectionForMQTTConnection MQTT Connection Creation failed with error %s\n",
                Aws::Crt::ErrorDebugString(connection->LastError()));
        exit(-1);
    }
    return connection;
}

void TRLIotCoreAdapter::CreateSubscribers()
{
    auto onMultiSubAck = [&](Aws::Crt::Mqtt::MqttConnection &,
                             uint16_t packet_id,
                             const Aws::Crt::Vector<Aws::Crt::String> &topics,
                             Aws::Crt::Mqtt::QOS qos,
                             int error_code) {
        if (error_code) {
            BOOST_LOG_TRIVIAL(error)
                << "m_connection::Subscribe::onSubAck MQTT Subscribe Connection failed with error code: "
                << error_code;
            return false;
        } else {
            if (!packet_id || qos == AWS_MQTT_QOS_FAILURE) {
                BOOST_LOG_TRIVIAL(error)
                    << "m_connection::Subscribe::onSubAck Subscriber rejected by the broker.";
                return false;
            } else {
                for (auto x : topics) {
                    BOOST_LOG_TRIVIAL(info)
                        << "m_connection::Subscribe::onSubAck Subscribe on topic "
                        << x.c_str() << " on packet id " << packet_id
                        << " succeeded";
                }
                return true;
            }
        }
    };

    std::pair<const char *, Aws::Crt::Mqtt::OnMessageReceivedHandler>
        lift_subscription, door_subscription;

    Aws::Crt::Vector<
        std::pair<const char *, Aws::Crt::Mqtt::OnMessageReceivedHandler>>
        subscriptions;

    lift_subscription.first = m_lift_command_topic.c_str();
    lift_subscription.second = std::bind(
        &TRLIotCoreAdapter::LiftCommandReceiveCallback,
        this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3,
        std::placeholders::_4,
        std::placeholders::_5,
        std::placeholders::_6);

    door_subscription.first = m_door_command_topic.c_str();
    door_subscription.second = std::bind(
        &TRLIotCoreAdapter::DoorCommandReceiveCallback,
        this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3,
        std::placeholders::_4,
        std::placeholders::_5,
        std::placeholders::_6);

    subscriptions.push_back(lift_subscription);
    subscriptions.push_back(door_subscription);

    m_connection->Subscribe(
        subscriptions,
        AWS_MQTT_QOS_AT_LEAST_ONCE,
        onMultiSubAck);
}

void TRLIotCoreAdapter::PublishState()
{
    std::scoped_lock lock(m_mutex);
    try {
        auto onPublishComplete =
            [](Aws::Crt::Mqtt::MqttConnection &, uint16_t, int) {};
        // publish state for all the doors
        for (auto x : m_doors) {
            nlohmann::json door_state;
            const auto p1 = std::chrono::system_clock::now();
            int time = std::chrono::duration_cast<std::chrono::seconds>(
                           p1.time_since_epoch())
                           .count();
            door_state["door_time"] = time;
            door_state["door_name"] = x->GetDoorName();
            door_state["current_mode"] = x->GetDoorState();
            Aws::Crt::String message(
                door_state.dump().c_str(),
                door_state.dump().size());
            Aws::Crt::ByteBuf payload = Aws::Crt::ByteBufFromArray(
                (const uint8_t *)message.data(),
                message.length());
            m_connection->Publish(
                m_door_state_topic.c_str(),
                AWS_MQTT_QOS_AT_LEAST_ONCE,
                false,
                payload,
                onPublishComplete);
        }
        // publish state for all the lifts
        for (auto x : m_lifts) {
            nlohmann::json lift_state;
            const auto p1 = std::chrono::system_clock::now();
            int time = std::chrono::duration_cast<std::chrono::seconds>(
                           p1.time_since_epoch())
                           .count();
            lift_state["lift_time"] = time;
            lift_state["lift_name"] = x->GetName();
            lift_state["available_floors"] = x->AvailableFloors();
            ;
            lift_state["currrent_floor"] = x->CurrentFloor().value_or("0");
            lift_state["destination_floor"] =
                x->DestinationFloor().value_or("0");
            lift_state["door_state"] = x->LiftDoorState();
            lift_state["motion_state"] = x->LiftMotionState();
            lift_state["available_modes"] = x->AvailableModes();
            lift_state["lift_mode"] = x->CurrentMode();
            lift_state["session_id"] = x->GetSessionID();
            Aws::Crt::String message(
                lift_state.dump().c_str(),
                lift_state.dump().size());
            Aws::Crt::ByteBuf payload = Aws::Crt::ByteBufFromArray(
                (const uint8_t *)message.data(),
                message.length());
            m_connection->Publish(
                m_lift_state_topic.c_str(),
                AWS_MQTT_QOS_AT_LEAST_ONCE,
                false,
                payload,
                onPublishComplete);
        }
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLIotCoreAdapter::PublishState failed. " << e.what();
    }
}

void TRLIotCoreAdapter::LiftCommandReceiveCallback(
    Aws::Crt::Mqtt::MqttConnection &,
    const Aws::Crt::String &topic,
    const Aws::Crt::ByteBuf &byte_buf,
    bool /*dup*/,
    Aws::Crt::Mqtt::QOS /*qos*/,
    bool /*retain*/)
{
    std::scoped_lock lock(m_mutex);
    BOOST_LOG_TRIVIAL(info) << "Received lift command topic\n";
    try {
        std::string s(byte_buf.buffer, byte_buf.buffer + byte_buf.len);
        nlohmann::json received_data = nlohmann::json::parse(s);

        // check if all fields are present
        if (received_data["lift_name"].empty() ||
            received_data["request_time"].empty() ||
            received_data["session_id"].empty() ||
            received_data["request_type"].empty() ||
            received_data["destination_floor"].empty() ||
            received_data["door_state"].empty()) {
            return;
        }

        int time = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

        for (auto x : m_lifts) {
            if (received_data["lift_name"] == x->GetName() &&
                received_data["request_time"] <= time) {
                x->SetSessionID(received_data["session_id"]);
                if (received_data["request_type"] == 0 ||
                    received_data["request_type"] == 2) {
                    x->EndLift();
                } else if (received_data["request_type"] == 1) {
                    x->CommandLift(
                        to_string(received_data["destination_floor"]));
                }
            }
        }

    } catch (nlohmann::detail::parse_error ex) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLIotCoreAdapter::LiftCommandReceiveCallback parsing error.";
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLIotCoreAdapter::LiftCommandReceiveCallback error. "
            << e.what();
    }
}

void TRLIotCoreAdapter::DoorCommandReceiveCallback(
    Aws::Crt::Mqtt::MqttConnection &,
    const Aws::Crt::String &topic,
    const Aws::Crt::ByteBuf &byte_buf,
    bool /*dup*/,
    Aws::Crt::Mqtt::QOS /*qos*/,
    bool /*retain*/)
{
    std::scoped_lock lock(m_mutex);
    BOOST_LOG_TRIVIAL(info) << "Received door command topic\n";

    try {
        std::string s(byte_buf.buffer, byte_buf.buffer + byte_buf.len);
        nlohmann::json received_data = nlohmann::json::parse(s);

        // check if all fields are present
        if (received_data["door_name"].empty() ||
            received_data["request_time"].empty() ||
            received_data["requester_id"].empty() ||
            received_data["requested_mode"].empty()) {
            return;
        }

        int time = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

        for (auto x : m_doors) {
            if (received_data["door_name"] == x->GetDoorName() &&
                received_data["request_time"] <= time) {
                if (received_data["requested_mode"] == 0) {
                    x->ActuateDoor(false);
                } else if (received_data["requested_mode"] == 2) {
                    x->ActuateDoor(true);
                }
            }
        }

    } catch (nlohmann::detail::parse_error ex) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLIotCoreAdapter::DoorCommandReceiveCallback parsing error.";
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLIotCoreAdapter::DoorCommandReceiveCallback error. "
            << e.what();
    }
}

const bool &TRLIotCoreAdapter::ConnectionCompleted() const
{
    return m_connected;
}
