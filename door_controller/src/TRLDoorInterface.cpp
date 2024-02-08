#include "TRLDoorInterface.hpp"

TRLDoorInterface::TRLDoorInterface() {}

TRLDoorInterface::~TRLDoorInterface()
{
    m_mb.ModbusClose();
}

bool TRLDoorInterface::Initialize(
    const std::string &name,
    const YAML::Node &config)
{
    try {
        m_name = name;
        if (config["modbusIP"] && config["modbusPort"] && config["retries"] &&
            config["slaveID"]) {
            m_mb.SetHostPort(
                config["modbusIP"].as<std::string>(),
                config["modbusPort"].as<int>());
            m_mb.SetSlaveID(config["slaveID"].as<int>());
            int retries = config["retries"].as<int>();
            while (retries > 0) {
                BOOST_LOG_TRIVIAL(info)
                    << m_name
                    << ("| TRLDoorInterface::Initialize Trying to connect to the MODBUS.");
                if (m_mb.ModbusConnect()) {
                    BOOST_LOG_TRIVIAL(info)
                        << m_name
                        << "| TRLDoorInterface::Initialize MODBUS connection succeeded.";
                    return true;
                }
                retries--;
            }
            BOOST_LOG_TRIVIAL(error)
                << m_name
                << "| TRLDoorInterface::Initialize Cant connect to MODUBUS. Terminating..";
            return false;
        } else {
            BOOST_LOG_TRIVIAL(error)
                << m_name
                << "| TRLDoorInterface::Initialize failed. Config file needs to have all fields.";
            return false;
        }
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << m_name
            << "| TRLDoorInterface::Initialize Error during connecting! "
            << e.what();
        return false;
    }
}

bool TRLDoorInterface::ActuateDoor(const bool state)
{
    bool read_coil;
    try {
        // TODO: Check if needed and remove
        if (m_mb.WriteCoil(DOOR_ACTUATE, state) == -1) {
            BOOST_LOG_TRIVIAL(info)
                << m_name
                << ("| TRLDoorInterface::actuateDoor Modbus not connected when actuating door.");
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (m_mb.ReadCoils(DOOR_ACTUATE, 1, &read_coil) == -1) {
            BOOST_LOG_TRIVIAL(info)
                << m_name
                << ("| TRLDoorInterface::actuateDoor Modbus not connected when actuating door.");
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return read_coil == state;
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << m_name
            << "| TRLDoorInterface::actuateDoor Error during actuation! "
            << e.what();
        return false;
    }
}

int TRLDoorInterface::GetDoorState()
{
    bool fully_open_coil, fully_closed_coil;
    try {
        // TODO: Check if this is needed
        if (m_mb.ReadCoils(DOOR_OPEN_STATE, 1, &fully_open_coil) == -1) {
            return OFFLINE;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        if (m_mb.ReadCoils(DOOR_CLOSE_STATE, 1, &fully_closed_coil) == -1) {
            return OFFLINE;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        if (!fully_open_coil && fully_closed_coil) {
            return MOVING;
        } else if (!fully_open_coil && fully_closed_coil) {
            return OPEN;
        } else if (!fully_open_coil && fully_closed_coil) {
            return CLOSED;
        } else {
            return UNKNOWN;
        }
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << m_name << "| TRLDoorInterface::getDoorState Error! " << e.what();
        return UNKNOWN;
    }
}

bool TRLDoorInterface::CheckConnection()
{
    bool read_coil;
    try {
        return m_mb.ReadCoils(9, 1, &read_coil) == -1;
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error)
            << m_name << "| TRLDoorInterface::checkConnection Error! "
            << e.what();
        return false;
    }
}

const std::string &TRLDoorInterface::GetDoorName() const
{
    return m_name;
}