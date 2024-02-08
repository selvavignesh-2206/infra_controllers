#ifndef ADS_INTERFACE_HPP
#define ADS_INTERFACE_HPP

#include <time.h>
#include <yaml-cpp/yaml.h>

#include <boost/thread/thread.hpp>
#include <cstdlib>
#include <mutex>
#include <variant>

#include "../lib/ADS/AdsLib/AdsLib.h"
#include "../lib/ADS/AdsLib/AdsVariable.h"
#include "../lib/ADS/AdsLib/standalone/AdsDef.h"

using namespace std;

class AdsInterface {
    enum
    {
        BOOL,
        UINT8_T,
        INT8_T,
        UINT16_T,
        INT16_T,
        UINT32_T,
        INT32_T,
        INT64_T,
        FLOAT,
        DOUBLE,
        DATE,
    };

public:
    using variant_t = variant<
        bool,
        uint8_t,
        int8_t,
        uint16_t,
        int16_t,
        uint32_t,
        int32_t,
        int64_t,
        float,
        double,
        tm>;

    /**
     * @brief AdsInterface simple constructor
     */
    AdsInterface();

    ~AdsInterface();

    /**
     * @brief adsWriteValue writes on a single variable
     * @param name name of the variable to write on
     * @param value value of the variable to write on
     * @return true if the writing succeeded
     * @return false otherwise
     */
    bool AdsWriteValue(
        const std::string& name,
        const AdsInterface::variant_t& value);

    /**
     * @brief adsReadValue reads a single variable's value
     * @param var_name name of the variable to read
     * @return variant_t value of the variable
     */
    AdsInterface::variant_t AdsReadValue(const std::string& var_name);

    /**
     * @brief adsReadVariables reads multiple variables' value
     * @param var_names names of the variables to read
     * @return vector<variant_t> values of the variables
     */
    std::vector<AdsInterface::variant_t> AdsReadVariables(
        const std::vector<std::string>& var_names);

    /**
     * @brief factory (re)-create an IADS variable
     * @param var_name the alias of the variable to (re)-create
     * @return true if the creation succeeded
     */
    bool Factory(const std::string& var_name);

    /**
     * @brief convert_type_from_string gets the type from a string
     * @param type the type as string
     * @return the type as int
     * @return -1 if not a type
     */
    int ConvertTypeFromString(const std::string& type);

    /**
     * @brief setRemoteNetID set the remote NetID
     * @param netId netID to set as string of type: xxx.xxx.xxx.xxx.xxx.xxx
     */
    void SetRemoteNetID(const std::string& net_id)
    {
        m_remote_net_id = net_id;
    }

    /**
     * @brief getRemoteNetID get the remote NetID
     * @return netId netID parameter
     */
    const std::string& GetRemoteNetID() const
    {
        return m_remote_net_id;
    }

    /**
     * @brief setRemoteIPV4 set the remoteIPV4
     * @param address IPV4 to set as string of type: xxx.xxx.xxx.xxx
     */
    void SetRemoteIPV4(const std::string& address)
    {
        m_remote_ip_v4 = address;
    }

    /**
     * @brief getRemoteIPV4 get the remote IPV4
     * @return IPV4 IPV4 parameter
     */
    const std::string& GetRemoteIPV4() const
    {
        return m_remote_ip_v4;
    }

    /**
     * @brief setLocalNetID set the local NetID
     * @param netId netID to set as string of type: xxx.xxx.xxx.xxx.xxx.xxx
     */
    void SetLocalNetID(const std::string& net_id)
    {
        m_local_net_id_param = net_id;
        AdsSetLocalAddress(AmsNetId(m_local_net_id_param));
    }

    /**
     * @brief getLocalNetID get the local NetID
     * @return netId netID parameter
     */
    const std::string& GetLocalNetID() const
    {
        return m_local_net_id_param;
    }

    /**
     * @brief getVariablesMap
     * @return a reference to the variables memory
     */
    const std::
        map<std::string, std::pair<std::string, AdsInterface::variant_t>>&
        GetVariablesMap() const
    {
        return m_variables_map;
    }

    /**
     * @brief updateMemory update the variables memory
     */
    void UpdateMemory();

    /**
     * @brief initRoute initialize a connection to the ADS device
     */
    void InitRoute();

    /**
     * @brief ConnectionCheck
     *
     * Tries to get ADS state
     * If the ADS is not running setsm_device state to false, to true otherwise
     * If the ADS is not running or connection failed, tries to establish a new
     * connection If connection was down and is up again, recreates all IADS
     * variables (get the new handles)
     *
     * @return ads state
     */
    int ConnectionCheck();

    /**
     * @brief getState
     * @return true if data published are valid
     * @return false otherwise
     */
    const bool& GetState() const
    {
        return (m_device_state);
    }

    /**
     * @brief getADSState
     * @return return current ADS state
     */
    const int& GetADSState() const
    {
        return (m_ads_state);
    }

    /**
     * @brief bindPLcVar creates IADS variables for aliased variables given in
     * configuration file
     * @return true if aliasing succeeded
     * @return false otherwise
     */
    bool BindPLCVar();

    /**
     * @brief checkVariableType
     * @param var_name the name of the variable to check the type of
     * @return the type of the variable as int
     * @return -1 if variable does not exist
     */
    int CheckVariableType(const std::string& var_name);

    /**
     * @brief acquireVariables get all ADS variables from the device
     */
    void AcquireVariables()
    {
        if (m_route && m_device_state) {
            m_variable_ads = m_route->GetDeviceAdsVariables();
        }
    }

    /**
     * @brief setName set the name of the device for configuration
     * @param name the name of the device
     */
    void SetName(const std::string& name)
    {
        m_name = name;
    }

    /**
     * @brief setFile set the configuration file the device is configured from
     * @param config_file the full path to the YAML configuration file
     */
    void SetFile(const YAML::Node& config)
    {
        m_config = config;
    }

private:
    string m_remote_net_id;      /*!< the NetID of the ADS device*/
    string m_remote_ip_v4;       /*!< the IPV4 of the ADS device*/
    string m_local_net_id_param; /*!< the local net ID */
    uint64_t m_temp;     /*!< a temp value for reading a variable value*/
    string m_name;       /*!< the name of the device for configuration*/
    YAML::Node m_config; /*!< the configuration file to use*/
    int m_ads_state;     /*!< the last known state of ADS*/

    AmsNetId* m_ams_net_id_remote_net_id; /*!< the NetID
                                              structure of the ADS device */

    AdsDevice* m_route; /*!< the ADS device route*/
    bool m_device_state{
        false};             /*!< the last known validity state of the values*/
    std::mutex m_com_mutex; /*!< Communication mutex */
    std::mutex m_mem_mutex; /*!< memory mutex */

    std::map<std::string, std::string>
        m_variable_ads; /*!< a map with ADS name as key and the
           variable type as value */
    std::map<std::string, std::string>
        m_alias_map; /*!< a map with alias name as key and ADS names as value */
    std::map<std::string, std::pair<int, std::string>>
        m_variable_mapping; /*!< a map with alias name as key and both
                              representation of it's type a value */

    std::map<std::string, IAdsVariable*> m_route_mapping; /*!< a map with alias
                              name as key and the IADS variable as value */
    std::map<std::string, std::pair<std::string, AdsInterface::variant_t>>
        m_variables_map; /*!< a map with alias name as key and a pair with the
                            type of the variable as string and it's value in
                            it's own type */
};
#endif  // ADS_INTERFACE_HPP