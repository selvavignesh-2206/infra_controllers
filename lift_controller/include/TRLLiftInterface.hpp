#ifndef TRL_LIFT_INTERFACE_HPP
#define TRL_LIFT_INTERFACE_HPP

// YAML include
#include <yaml-cpp/yaml.h>

// Header file
#include "AdsInterface.hpp"

// Standard includes
#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

// Logger
#include <boost/log/trivial.hpp>

class TRLLiftInterface {
public:
    /**
     * @brief TRLLiftInterface simple constructor
     */
    TRLLiftInterface();

    /**
     * @brief TRLLiftInterface simple deconstructor
     */
    ~TRLLiftInterface();

    /**
     * @brief Initializes the TRL lift interface
     * @param name name of the lift
     * @param config Yaml::Node of the lift
     * @return 1 if the initialization is successful
     * @return 0 otherwise
     */
    bool Initialize(const std::string &name, const YAML::Node &config);

    /**
     * @brief Checks if the connection is alive, serves as a heartbeat checker
     * @return 1 if connection is alive
     * @return 0 otherwise
     */
    bool CheckConnection();

    /**
     * @brief Checks if the connection is alive, serves as a heartbeat checker
     * using shared variable with PLC
     * @return 1 if connection is alive
     * @return 0 otherwise
     */
    bool HeartbeatConnection();

    /**
     * @brief returns the available floors for this lift
     * @return returns a vector of strings describing available floors
     * @return returns an empty vector if no available floors
     */
    const std::vector<std::string> &AvailableFloors() const;

    /**
     * @brief returns the available modes for this lift
     * @return returns a vector of int describing available modes
     * @return returns an empty vector if no available modes
     */
    const std::vector<int> &AvailableModes() const;

    /**
     * @brief retrieves the current_mode of the lift
     * @return an int corresponding to the current_mode of the lift
     */
    int CurrentMode();

    /**
     * @brief returns the current floor of this lift
     * @return a string describing the current floor it is at right now
     * @return an empty string if the query fails
     */
    std::optional<std::string> CurrentFloor();

    /**
     * @brief returns the destination floor of this lift
     * @return a string describing the destination floor it is serving right now
     * @return an empty string if the query fails
     */
    std::optional<std::string> DestinationFloor();

    /**
     * @brief retrieves the door state of the lift
     * @return an int corresponding to the door state of the lift, based on the
     * static enum DoorState
     */
    int LiftDoorState();

    /**
     * @brief retrieves the motion state of the lift
     * @return an int corresponding to the motion state of the lift, based on
     * the static enum MotionState
     */
    int LiftMotionState();

    /**
     * @brief Sends the lift cabin to a specific floor and opens all available
     * doors for that floor
     * @return 1 if request was sent out successfully
     * @return 0 otherwise
     */
    bool CommandLift(const std::string &floor);

    /**
     * @brief Returns control to lift and goes from AGV mode to Normal mode
     * @return 1 if request was sent out successfully
     * @return 0 otherwise
     */
    bool EndLift();

    /**
     * @brief Returns the name of the lift
     * @return name of the lift
     */
    const std::string &GetName() const;

    /**
     * @brief Returns the session id of the lift
     * @return session of the lift
     */
    const std::string &GetSessionID() const;

    /**
     * @brief Sets the session id of the lift
     * @param session_id session id string
     */
    void SetSessionID(const std::string &session_id);

private:
    AdsInterface m_adsinterface;
    std::vector<std::string> m_available_floors;
    std::vector<int> m_available_modes;
    std::string m_name = "";
    std::string m_session_id = "";
};
#endif  // TRL_LIFT_INTERFACE_HPP
