#ifndef TRL_IOT_CORE_ADAPTER_HPP
    #define HEA_DER__H_T_RLIotCoreAdapter

    // AWS IOT Core Includes
    #include <aws/crt/Api.h>
    #include <aws/crt/StlAllocator.h>
    #include <aws/crt/Types.h>
    #include <aws/crt/UUID.h>
    #include <aws/crt/auth/Credentials.h>
    #include <aws/crt/io/TlsOptions.h>
    #include <aws/iot/MqttClient.h>

    // TRL Lift Interface
    #include "TRLLiftInterface.hpp"
    // TRL Door Interface
    #include "TRLDoorInterface.hpp"
    // Json helper lib
    #include <nlohmann/json.hpp>

    // Standard includes
    #include <algorithm>
    #include <chrono>
    #include <mutex>
    #include <thread>

    // Logging
    #include <boost/date_time.hpp>
    #include <boost/log/core.hpp>
    #include <boost/log/expressions.hpp>
    #include <boost/log/trivial.hpp>
    #include <boost/log/utility/setup/common_attributes.hpp>
    #include <boost/log/utility/setup/console.hpp>
    #include <boost/log/utility/setup/file.hpp>

class TRLIotCoreAdapter {
public:
    /**
     * @brief TRLIotCoreAdapter simple constructor
     */
    TRLIotCoreAdapter();

    /**
     * @brief TRLIotCoreAdapter simple deconstructor
     */
    ~TRLIotCoreAdapter();

    /**
     * @brief Initializes the TRL IOT Core Adapter
     * @param aws_iot_config_file_path path to the aws iot config file needed
     * @param lift_config_file_path path to the lift config file needed
     * @param doors_config_file_path path to the doors config file needed
     * @return 1 if the initialization is successful
     * @return 0 otherwise
     */
    bool Initialize(
        const std::string &aws_iot_config_file_path,
        const std::string &lift_config_file_path,
        const std::string &doors_config_file_path);

    /**
     * @brief This function publishes a MQTT message consisting of the lift and
     * door status
     */
    void PublishState();

    /**
     * @brief Checks if the MQTT connection is completed
     */
    const bool &ConnectionCompleted() const;

private:
    /**
     * @brief Builds a MQTT Connection to AWS IOT Core
     * @param client The MQTT Client
     * @param cert_path absolute path to certificate
     * @param key_path absolute path to key
     * @param aws_url the url for the aws iot core instance
     * @param ca_file_path absolute path to the ca_file
     */
    std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> BuildDirectMQTTConnection(
        Aws::Iot::MqttClient &client,
        std::string &cert_path,
        std::string &key_path,
        std::string &aws_url,
        std::string &ca_file_path);

    /**
     * @brief Returns a client connection from a given mqtt client and config
     * @param client The MQTT Client
     * @param client_config_builder config builder with all necessarary info
     * neededed to return the client connection
     */
    std::shared_ptr<Aws::Crt::Mqtt::MqttConnection>
    GetClientConnectionForMQTTConnection(
        Aws::Iot::MqttClient &client,
        Aws::Iot::MqttClientConnectionConfigBuilder &client_config_builder);

    /**
     * @brief This function is called whenever a message is heard from the lift
     * command MQTT topic
     */
    void LiftCommandReceiveCallback(
        Aws::Crt::Mqtt::MqttConnection &,
        const Aws::Crt::String &topic,
        const Aws::Crt::ByteBuf &byte_buf,
        bool /*dup*/,
        Aws::Crt::Mqtt::QOS /*qos*/,
        bool /*retain*/);

    /**
     * @brief This function is called whenever a message is heard from door
     * command the MQTT topic
     */
    void DoorCommandReceiveCallback(
        Aws::Crt::Mqtt::MqttConnection &,
        const Aws::Crt::String &topic,
        const Aws::Crt::ByteBuf &byte_buf,
        bool /*dup*/,
        Aws::Crt::Mqtt::QOS /*qos*/,
        bool /*retain*/);

    /**
     * @brief Encompassing function to create subscribers for lift and door
     * commands
     */
    void CreateSubscribers();

private:
    Aws::Crt::ApiHandle m_api_handle;        // mqtt api handle
    Aws::Iot::MqttClient m_internal_client;  // mqtt client
    std::shared_ptr<Aws::Crt::Mqtt::MqttConnection> m_connection;
    std::promise<void> m_closed_promise;  // a promise to wait for the complete
                                          // mqtt disconnection
    std::mutex m_mutex;  // mutex to ensure thread safety for simultaneous
                         // subscribing and publishing
    std::string m_lift_command_topic =
        "";  // the mqtt topic for receiving lift commands
    std::string m_lift_state_topic =
        "";  // the mqtt topic where lift states should be published
    std::string m_door_command_topic =
        "";  // the mqtt topic for receiving door commands
    std::string m_door_state_topic =
        "";  // the mqtt topic where door states should be published
    std::vector<std::shared_ptr<TRLDoorInterface>>
        m_doors;  // vector of door instances
    std::vector<std::shared_ptr<TRLLiftInterface>>
        m_lifts;  // vector of lift instances

    bool m_connected = false;
};

#endif  // TRL_IOT_CORE_ADAPTER_HPP
