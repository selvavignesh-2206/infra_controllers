#ifndef TRL_DOOR_INTERFACE_HPP
#define TRL_DOOR_INTERFACE_HPP

#include <yaml-cpp/yaml.h>

#include "modbus.hpp"

// Standard includes
#include <chrono>
#include <ctime>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

// Logger
#include <boost/log/trivial.hpp>

// pin defs
#define DOOR_OPEN_STATE 1
#define DOOR_CLOSE_STATE 2
#define DOOR_ACTUATE 17

// door mode defs
#define CLOSED 0
#define MOVING 1
#define OPEN 2
#define OFFLINE 3
#define UNKNOWN 4

class TRLDoorInterface {
public:
    /**
     * @brief TRL door interface constructor
     */
    TRLDoorInterface();

    /**
     * @brief Deconstructor, mostly to properly close the connection between the
     * ADAM6052 module
     */
    ~TRLDoorInterface();

    /**
     * @brief Initializes the TRL door interface
     * @param name name of the door
     * @param config Yaml::Node of the lift
     * @return 1 if the initialization is successful
     * @return 0 otherwise
     */
    bool Initialize(const std::string &name, const YAML::Node &config);

    /**
     * @brief Commands the door to either close or open
     * @param state true means open, false means close
     * @return true if command is successfully sent
     * @return 0 otherwise
     */
    bool ActuateDoor(const bool state);

    /**
     * @brief retrieves door state
     * @return an int corresponding to the door state
     * @return 0 = closed, 1 = moving, 2 = open, 3 = offline, 4 = unkown
     */
    int GetDoorState();

    /**
     * @brief Checks if the connection is alive, serves as a heartbeat checker
     * @return 1 if connection is alive
     * @return 0 otherwise
     */
    bool CheckConnection();

    /**
     * @brief return door name
     * @return return door name
     */
    const std::string &GetDoorName() const;

private:
    modbus m_mb;
    std::string m_name = "";
};
#endif  // TRL_DOOR_INTERFACE_HPP