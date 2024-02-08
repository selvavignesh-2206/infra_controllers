#include "TRLLiftInterface.hpp"

TRLLiftInterface::TRLLiftInterface() {}

TRLLiftInterface::~TRLLiftInterface() {}

bool TRLLiftInterface::Initialize(
    const std::string &name,
    const YAML::Node &config)
{
    BOOST_LOG_TRIVIAL(info) << "TRLLiftInterface::Initialize initializing";
    // set remoteip, remotenetid and localnetid
    try {
        m_name = name;
        if (config["remoteIP"] && config["remoteNetID"] &&
            config["localNetID"] && config["available_floors"] &&
            config["available_modes"]) {
            m_adsinterface.SetRemoteIPV4(config["remoteIP"].as<std::string>());
            m_adsinterface.SetLocalNetID(
                config["localNetID"].as<std::string>());
            m_adsinterface.SetRemoteNetID(
                config["remoteNetID"].as<std::string>());
            m_adsinterface.SetName(name);
            m_adsinterface.SetFile(config);
            m_available_floors =
                config["available_floors"].as<std::vector<std::string>>();
            m_available_modes =
                config["available_modes"].as<std::vector<int>>();
        } else {
            BOOST_LOG_TRIVIAL(error)
                << "TRLLiftInterface::initialize failed. Config file needs to have remoteIP, localNetID, remoteNetID , m_available_floors, available_modes fields.";
            return false;
        }
    } catch (const YAML::ParserException &ex) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::initialize failed YAML error.";
        return false;
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::initialize failed." << e.what();
        return false;
    }

    // init route
    try {
        BOOST_LOG_TRIVIAL(info)
            << "TRLLiftInterface::initialize Init ADS Route";
        m_adsinterface.InitRoute();
        BOOST_LOG_TRIVIAL(info)
            << "TRLLiftInterface::initialize Init ADS Route Done.";
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::initialize ADS route init failed."
            << e.what();
        return false;
    }

    // acquire variables
    try {
        m_adsinterface.AcquireVariables();
        m_adsinterface.BindPLCVar();
        BOOST_LOG_TRIVIAL(info)
            << "TRLLiftInterface::initialize Ready to communicate with the remote PLC via ADS.";
    } catch (AdsException ex) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::initialize ERROR in mapping alias with ADS.";
        return false;
    }

    return true;
}

bool TRLLiftInterface::CheckConnection()
{
    try {
        return m_adsinterface.ConnectionCheck();
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::CheckConnection failed" << e.what();
        return false;
    }
}

bool TRLLiftInterface::HeartbeatConnection()
{
    /// TODO!!: redo this
    time_t current_time;
    try {
        if (std::get<int16_t>(m_adsinterface.AdsReadValue("randomCount")) <=
            0) {
            m_adsinterface.AdsWriteValue("randomCount", (int16_t)1);
            BOOST_LOG_TRIVIAL(debug)
                << "Reading randomCount: "
                << std::get<int16_t>(
                       m_adsinterface.AdsReadValue("randomCount"));
            current_time = time(0);
        } else if ((time(0) - current_time) > 10) {
            return false;
        }
        return true;
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::HeartbeatConnection Error. " << e.what();
        return false;
    }
}

const std::vector<std::string> &TRLLiftInterface::AvailableFloors() const
{
    return m_available_floors;
}

const std::vector<int> &TRLLiftInterface::AvailableModes() const
{
    return m_available_modes;
}

int TRLLiftInterface::CurrentMode()
{
    try {
        // BOOST_LOG_TRIVIAL(debug) << "---current mode---";
        // BOOST_LOG_TRIVIAL(debug)
        //     << "State: "
        //     << std::get<int16_t>(m_adsinterface.AdsReadValue("state"));
        // BOOST_LOG_TRIVIAL(debug)
        //     << "lifttask: "
        //     << std::get<bool>(m_adsinterface.AdsReadValue("liftTask"));
        // BOOST_LOG_TRIVIAL(debug)
        //     << "EndliftTask: "
        //     << std::get<bool>(m_adsinterface.AdsReadValue("endLiftTask"));
        // BOOST_LOG_TRIVIAL(debug)
        //     << "fireAlarm: "
        //     << std::get<bool>(m_adsinterface.AdsReadValue("fireAlarm"));
        // BOOST_LOG_TRIVIAL(debug)
        //     << "turnKeyToManual: "
        //     <<
        //     std::get<bool>(m_adsinterface.AdsReadValue("turnKeyToManual"));
        // BOOST_LOG_TRIVIAL(debug) << "----------------";

        if (std::get<bool>(m_adsinterface.AdsReadValue("fireAlarm"))) {
            return 3;
        } else if (std::get<bool>(
                       m_adsinterface.AdsReadValue("turnKeyToManual"))) {
            return 4;
        } else if (std::get<bool>(m_adsinterface.AdsReadValue("agvMode"))) {
            return 2;
        } else if (!std::get<bool>(m_adsinterface.AdsReadValue("agvMode"))) {
            return 1;
        } else {
            BOOST_LOG_TRIVIAL(error)
                << "TRLLiftInterface::currentMode Unknown mode. This should not happen.";
            return 0;
        }
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::currentMode Error. " << e.what();
        return 0;
    }
}

std::optional<std::string> TRLLiftInterface::CurrentFloor()
{
    try {
        std::string current_floor{std::to_string(
            std::get<int8_t>(m_adsinterface.AdsReadValue("liftCurrentFloor")))};
        if (current_floor.empty() || current_floor == "0") {
            BOOST_LOG_TRIVIAL(error)
                << "TRLLiftInterface::currentFloor Couldn't get liftCurrentFloor.";
            return std::nullopt;
        }
        return current_floor;
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::currentFloor Error. " << e.what();
        return std::nullopt;
    }
}

std::optional<std::string> TRLLiftInterface::DestinationFloor()
{
    // not string& as taking value and using it to write in another fn
    try {
        std::string destination_floor{std::to_string(std::get<int8_t>(
            m_adsinterface.AdsReadValue("liftDestinationFloor")))};
        if (destination_floor.empty() || destination_floor == "0") {
            BOOST_LOG_TRIVIAL(error)
                << "TRLLiftInterface::destinationFloor Couldnt get liftDestinationFloor.";
            return std::nullopt;
        }
        return destination_floor;

    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::destinationFloor Error. " << e.what();
        return std::nullopt;
    }
}

int TRLLiftInterface::LiftDoorState()
{
    try {
        return std::get<int16_t>(m_adsinterface.AdsReadValue("liftDoorState"));
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::liftDoorState Error. " << e.what();
        return 0;
    }
}

int TRLLiftInterface::LiftMotionState()
{
    try {
        return std::get<int16_t>(
            m_adsinterface.AdsReadValue("liftMotionState"));
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::liftMotionState Error. " << e.what();
        return 0;
    }
}

bool TRLLiftInterface::CommandLift(const std::string &floor)
{
    try {
        m_adsinterface.AdsWriteValue("liftTask", true);
        m_adsinterface.AdsWriteValue("endLiftTask", false);

        m_adsinterface.AdsWriteValue(
            "robotDestinationFloor",
            (int8_t)std::stoi(floor));
        if (!(std::stoi(floor) == std::get<int8_t>(m_adsinterface.AdsReadValue(
                                      "robotDestinationFloor")))) {
            BOOST_LOG_TRIVIAL(error)
                << "TRLLiftInterface::commandLift in writing variable with ADS.";
            return false;
        }

        return std::get<bool>(m_adsinterface.AdsReadValue("liftTask")) &&
               !std::get<bool>(m_adsinterface.AdsReadValue("endLiftTask")) &&
               DestinationFloor();

    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::commandLift Error. " << e.what();
        return false;
    }
}

bool TRLLiftInterface::EndLift()
{
    try {
        m_adsinterface.AdsWriteValue("liftTask", false);
        m_adsinterface.AdsWriteValue("endLiftTask", true);

        return !std::get<bool>(m_adsinterface.AdsReadValue("liftTask")) &&
               std::get<bool>(m_adsinterface.AdsReadValue("endLiftTask"));
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << "TRLLiftInterface::endLift Error. " << e.what();
        return false;
    }
}

const std::string &TRLLiftInterface::GetName() const
{
    return m_name;
}

const std::string &TRLLiftInterface::GetSessionID() const
{
    return m_session_id;
}

void TRLLiftInterface::SetSessionID(const std::string &session_id)
{
    m_session_id = session_id;
}