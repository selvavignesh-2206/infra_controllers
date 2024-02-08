#include "AdsInterface.hpp"

using namespace std;

AdsInterface::AdsInterface() {}

AdsInterface::~AdsInterface()
{
    if (m_route) {
        delete m_route;
    }
    if (m_ams_net_id_remote_net_id) {
        delete m_ams_net_id_remote_net_id;
    }

    for (map<string, IAdsVariable *>::iterator it = m_route_mapping.begin();
         it != m_route_mapping.end();
         ++it) {
        delete it->second;
    }
}

bool AdsInterface::AdsWriteValue(
    const std::string &name,
    const variant_t &value)
{
    int var_type = CheckVariableType(name);
    bool data_correct = true;
    bool bresult = true;
    bool no_issue = true;
    if (m_variable_mapping.find(name) == m_variable_mapping.end()) {
        data_correct = false;
        bresult = false;
    } else if (m_variable_mapping[name].first == var_type && m_device_state) {
        std::scoped_lock lock(m_com_mutex, m_mem_mutex);
        try {
            switch (m_variable_mapping[name].first) {
                case BOOL: {
                    *m_route_mapping[name] = get<bool>(value);
                    break;
                }
                case UINT8_T: {
                    *m_route_mapping[name] = get<uint8_t>(value);
                    break;
                }
                case INT8_T: {
                    *m_route_mapping[name] = get<int8_t>(value);
                    break;
                }
                case UINT16_T: {
                    *m_route_mapping[name] = get<uint16_t>(value);
                    break;
                }
                case INT16_T: {
                    *m_route_mapping[name] = get<int16_t>(value);
                    break;
                }
                case UINT32_T: {
                    *m_route_mapping[name] = get<uint32_t>(value);
                    break;
                }
                case INT32_T: {
                    *m_route_mapping[name] = get<int32_t>(value);
                    break;
                }
                case INT64_T: {
                    *m_route_mapping[name] = get<int64_t>(value);
                    break;
                }
                case FLOAT: {
                    *m_route_mapping[name] = get<float>(value);
                    break;
                }
                case DOUBLE: {
                    *m_route_mapping[name] = get<double>(value);
                    break;
                }
                case DATE: {
                    tm temp = get<tm>(value);
                    *m_route_mapping[name] = mktime(&temp);
                    break;
                }
                default: {
                    no_issue = false;
                }
            }
        } catch (AdsException e) {
            no_issue = false;
        }
    } else {
        Factory(name);
    }
    if (!no_issue) {
        Factory(name);
    }

    if (!data_correct) {
        bresult = false;
    }
    return bresult;
}

AdsInterface::variant_t AdsInterface::AdsReadValue(const std::string &var_name)
{
    AdsInterface::variant_t result;

    if (m_route_mapping.find(var_name) != m_route_mapping.end()) {
        auto no_issue = true;

        std::scoped_lock lock(m_com_mutex, m_mem_mutex);
        if (m_device_state) {
            try {
                if (m_route_mapping[var_name]) {
                    m_route_mapping[var_name]->ReadValue(&m_temp);

                    switch (m_variable_mapping[var_name].first) {
                        case BOOL: {
                            result = (bool)m_temp;
                            break;
                        }
                        case UINT8_T: {
                            result = (uint8_t)m_temp;
                            break;
                        }
                        case INT8_T: {
                            result = (int8_t)m_temp;
                            break;
                        }
                        case UINT16_T: {
                            result = (uint16_t)m_temp;
                            break;
                        }
                        case INT16_T: {
                            result = (int16_t)m_temp;
                            break;
                        }
                        case UINT32_T: {
                            result = (uint32_t)m_temp;
                            break;
                        }
                        case INT32_T: {
                            result = (int32_t)m_temp;
                            break;
                        }
                        case INT64_T: {
                            result = (int64_t)m_temp;
                            break;
                        }
                        case FLOAT: {
                            result = (float)m_temp;
                            break;
                        }
                        case DOUBLE: {
                            result = (double)m_temp;
                            break;
                        }
                        case DATE: {
                            result = (uint32_t)m_temp;
                            break;
                        }
                        default: {
                        }
                    }
                } else {
                    no_issue = false;
                }
            } catch (const std::exception &e) {
                no_issue = false;
            }
        }

        if (!no_issue) {
            Factory(var_name);
        }
    }
    return result;
}

/**
 * @brief AdsReadVariables reads multiple variables' value
 * @param var_names names of the variables to read
 * @return vector<variant_t> values of the variables
 */
std::vector<AdsInterface::variant_t> AdsInterface::AdsReadVariables(
    const std::vector<std::string> &var_names)
{
    std::vector<AdsInterface::variant_t> result;

    for (auto &name : var_names) {
        result.push_back(AdsReadValue(name));
    }

    return result;
}

/**
 * @brief Factory (re)-create an IADS variable
 * @param var_name the alias of the variable to (re)-create
 * @return true if the creation succeeded
 */
bool AdsInterface::Factory(const std::string &var_name)
{
    bool result = false;
    bool no_issue = true;

    std::scoped_lock lock(m_mem_mutex);
    try {
        string type = m_variable_ads[m_alias_map[var_name]];
        do {
            if (m_route_mapping[var_name]) {
                delete m_route_mapping[var_name];
            }
            if (type == "BOOL") {
                m_route_mapping[var_name] =
                    new AdsVariable<bool>(*m_route, m_alias_map[var_name]);
                result = true;
                break;
            }
            if (type == "BYTE" || type == "USINT") {
                m_route_mapping[var_name] =
                    new AdsVariable<uint8_t>(*m_route, m_alias_map[var_name]);
                result = true;
                break;
            }
            if (type == "SINT") {
                m_route_mapping[var_name] =
                    new AdsVariable<int8_t>(*m_route, m_alias_map[var_name]);
                result = true;
                break;
            }
            if (type == "WORD" || type == "UINT") {
                m_route_mapping[var_name] =
                    new AdsVariable<uint16_t>(*m_route, m_alias_map[var_name]);
                result = true;
                break;
            }
            if (type == "INT") {
                m_route_mapping[var_name] =
                    new AdsVariable<int16_t>(*m_route, m_alias_map[var_name]);
                result = true;
                break;
            }
            if (type == "DWORD" || type == "UDINT" || type == "DATE" ||
                type == "TIME" || type == "TIME_OF_DAY" || type == "LTIME") {
                m_route_mapping[var_name] =
                    new AdsVariable<uint32_t>(*m_route, m_alias_map[var_name]);
                result = true;
                break;
            }
            if (type == "DINT") {
                m_route_mapping[var_name] =
                    new AdsVariable<int32_t>(*m_route, m_alias_map[var_name]);
                result = true;
                break;
            }
            if (type == "LINT") {
                m_route_mapping[var_name] =
                    new AdsVariable<int64_t>(*m_route, m_alias_map[var_name]);
                result = true;
                break;
            }
            if (type == "REAL") {
                m_route_mapping[var_name] =
                    new AdsVariable<float>(*m_route, m_alias_map[var_name]);
                result = true;
                break;
            }
            if (type == "LREAL") {
                m_route_mapping[var_name] =
                    new AdsVariable<double>(*m_route, m_alias_map[var_name]);
                result = true;
                break;
            }
        } while (false);
    } catch (const std::exception &e) {
        no_issue = false;
    }

    if (!no_issue) {
        m_ads_state = false;
        ConnectionCheck();
    }

    return result;
}

/**
 * @brief gets the type from a string
 * @param type the type as string
 * @return the type as int
 * @return -1 if not a type
 */
int AdsInterface::ConvertTypeFromString(const std::string &type)
{
    int result = -1;
    do {
        if (type == "BOOL") {
            result = BOOL;
            break;
        }
        if (type == "BYTE" || type == "USINT") {
            result = UINT8_T;
            break;
        }
        if (type == "SINT") {
            result = INT8_T;
            break;
        }
        if (type == "WORD" || type == "UINT") {
            result = UINT16_T;
            break;
        }
        if (type == "INT") {
            result = INT16_T;
            break;
        }
        if (type == "DWORD" || type == "UDINT") {
            result = UINT32_T;
            break;
        }
        if (type == "DINT") {
            result = INT32_T;
            break;
        }
        if (type == "LINT") {
            result = INT64_T;
            break;
        }
        if (type == "REAL") {
            result = FLOAT;
            break;
        }
        if (type == "LREAL") {
            result = DOUBLE;
            break;
        }
        if (type == "DATE") {
            result = DATE;
            break;
        }
    } while (false);
    return result;
}

/**
 * @brief updateMemory update the variables memory
 */
void AdsInterface::UpdateMemory()
{
    for (auto &[name, pair] : m_variables_map) {
        pair.second = AdsReadValue(name);
    }
}

/**
 * @brief initRoute initialize a connection to the ADS device
 */
void AdsInterface::InitRoute()
{
    m_ams_net_id_remote_net_id = new AmsNetId(m_remote_net_id);
    m_route = new AdsDevice(
        m_remote_ip_v4.c_str(),
        *m_ams_net_id_remote_net_id,
        AMSPORT_R0_PLC_TC3);
}

/**
 * @brief ConnectionCheck
 *
 * Tries to get ADS state
 * If the ADS is not running sets m_device state to false, to true otherwise
 * If the ADS is not running or connection failed, tries to establish a new
 * connection If connection was down and is up again, recreates all IADS
 * variables (get the new handles)
 *
 * @return ads state
 */
int AdsInterface::ConnectionCheck()
{
    bool result = false;
    int ads;
    bool temp_state = m_device_state;
    std::scoped_lock lock(m_com_mutex);
    try {
        ads = m_route->GetState().ads;
        result = (ads == ADSSTATE_RUN);
        m_ads_state = (uint16_t)ads;
    } catch (const std::exception &e) {
        m_ads_state = ADSSTATE_INVALID;
    }

    if (!result)  // recovery
    {
        if (m_route) {
            delete m_route;
        }
        if (m_ams_net_id_remote_net_id) {
            delete m_ams_net_id_remote_net_id;
        }
        InitRoute();
    }

    /// TODO!!: `temp_state` is only read and never modified, why not just use
    /// `m_device_state`?
    if (result &&
        !temp_state)  // recreate ADSVariables if connexion is re-established
    {
        AcquireVariables();
        for (auto &[name, alias] : m_variable_mapping) {
            Factory(name);
        }
    }

    m_device_state = result;

    return (int)ads;
}

/**
 * @brief BindPLCVar creates IADS variables for aliased variables given in
 * configuration file
 * @return true if aliasing succeeded
 * @return false otherwise
 */
bool AdsInterface::BindPLCVar()
{
    if (m_config) {
        // Read each alias with corresponding ADS name
        for (YAML::const_iterator element = m_config["variables"].begin();
             element != m_config["variables"].end();
             ++element) {
            std::string ads_name = element->first.as<std::string>();
            std::string alias = element->second.as<std::string>();
            // Check if ADS name is part of downloaded PLC ADS list
            if (m_variable_ads.find(ads_name) == m_variable_ads.end()) {
                continue;
            }

            std::string type = m_variable_ads[ads_name];
            m_variable_mapping[alias] =
                std::pair<int, std::string>(ConvertTypeFromString(type), type);
            m_variables_map[alias] =
                std::pair<std::string, AdsInterface::variant_t>(
                    type,
                    AdsInterface::variant_t());
            m_alias_map[alias] = ads_name;
            Factory(alias);
        }
        return true;
    }
    return false;
}

/**
 * @brief CheckVariableType
 * @param var_name the name of the variable to check the type of
 * @return the type of the variable as int
 * @return -1 if variable does not exist
 */
int AdsInterface::CheckVariableType(const std::string &var_name)
{
    std::map<std::string, std::pair<int, std::string>>::iterator it =
        m_variable_mapping.find(var_name);
    if (it != m_variable_mapping.end()) {
        return it->second.first;
    }
    return -1;
}
