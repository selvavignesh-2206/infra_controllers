/* modbus.h
 *
 * Copyright (C) 20017-2021 Fanzhe Lyu <lvfanzhe@hotmail.com>, all rights
 * reserved.
 *
 * modbuspp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Changes made:
// By Chris Lau
// 29 Oct 2022
// 1. Make variable & function conventions consistent
// 2. Make new function called SetHostPort instead of passing variables via
// constructor

#ifndef MODBUSPP_MODBUS_H
#define MODBUSPP_MODBUS_H

#include <stdint.h>

#include <cstring>
#include <string>

#ifdef ENABLE_MODBUSPP_LOGGING
    #include <cstdio>
    #define LOG(fmt, ...) printf("[ modbuspp ]" fmt, ##__VA_ARGS__)
#else
    #define LOG(...) (void)0
#endif

#ifdef _WIN32
    // WINDOWS socket
    #define _WINSOCK_DEPRECATED_NO_WARNINGS

    #include <winsock2.h>
    #pragma comment(lib, "Ws2_32.lib")
using X_SOCKET = SOCKET;
using ssize_t = int;

    #define X_ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
    #define X_CLOSE_SOCKET(s) closesocket(s)
    #define X_ISCONNECTSUCCEED(s) ((s) != SOCKET_ERROR)

#else
    // Berkeley socket
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
using X_SOCKET = int;

    #define X_ISVALIDSOCKET(s) ((s) >= 0)
    #define X_CLOSE_SOCKET(s) close(s)
    #define X_ISCONNECTSUCCEED(s) ((s) >= 0)
#endif

using SOCKADDR = struct sockaddr;
using SOCKADDR_IN = struct sockaddr_in;

#define MAX_MSG_LENGTH 260

/// Function Code
#define READ_COILS 0x01
#define READ_INPUT_BITS 0x02
#define READ_REGS 0x03
#define READ_INPUT_REGS 0x04
#define WRITE_COIL 0x05
#define WRITE_REG 0x06
#define WRITE_COILS 0x0F
#define WRITE_REGS 0x10

/// Exception Codes

#define EX_ILLEGAL_FUNCTION 0x01  // Function Code not Supported
#define EX_ILLEGAL_ADDRESS 0x02   // Output Address not exists
#define EX_ILLEGAL_VALUE 0x03     // Output Value not in Range
#define EX_SERVER_FAILURE 0x04    // Slave Deive Fails to process request
#define EX_ACKNOWLEDGE 0x05       // Service Need Long Time to Execute
#define EX_SERVER_BUSY 0x06       // Server Was Unable to Accept MB Request PDU
#define EX_NEGATIVE_ACK 0x07
#define EX_MEM_PARITY_PROB 0x08
#define EX_GATEWAY_PROBLEMP 0x0A  // Gateway Path not Available
#define EX_GATEWYA_PROBLEMF 0x0B  // Target Device Failed to Response
#define EX_BAD_DATA 0XFF          // Bad Data lenght or Address

#define BAD_CON -1

/// Modbus Operator Class
/**
 * Modbus Operator Class
 * Providing networking support and mobus operation support.
 */
class modbus {
public:
    bool err{};
    int err_no{};
    std::string error_msg;

    modbus();
    ~modbus();

    bool ModbusConnect();
    void ModbusClose() const;

    bool IsConnected() const
    {
        return m_connected;
    }

    void SetHostPort(std::string host, uint16_t port);
    void SetSlaveID(int id);

    int ReadCoils(uint16_t address, uint16_t amount, bool *buffer);
    int ReadInputBits(uint16_t address, uint16_t amount, bool *buffer);
    int
    ReadHoldingRegisters(uint16_t address, uint16_t amount, uint16_t *buffer);
    int ReadInputRegisters(uint16_t address, uint16_t amount, uint16_t *buffer);

    int WriteCoil(uint16_t address, const bool &to_write);
    int WriteRegisters(uint16_t address, const uint16_t &value);
    int WriteCoils(uint16_t address, uint16_t amount, const bool *value);
    int
    WriteRegisters(uint16_t address, uint16_t amount, const uint16_t *value);

private:
    bool m_connected{};
    uint16_t m_port{};
    uint32_t m_msg_id{};
    int m_slaveid{};
    std::string m_host;

    X_SOCKET m_socket{};
    SOCKADDR_IN m_server{};

#ifdef _WIN32
    WSADATA wsadata;
#endif

    void ModbusBuildRequest(uint8_t *to_send, uint16_t address, int func) const;

    int ModbusRead(uint16_t address, uint16_t amount, int func);
    int ModbusWrite(
        uint16_t address,
        uint16_t amount,
        int func,
        const uint16_t *value);

    ssize_t ModbusSend(uint8_t *to_send, size_t length);
    ssize_t ModbusReceive(uint8_t *buffer) const;

    void ModbuserrorHandle(const uint8_t *msg, int func);

    void SetBadCon();
    void SetBadInput();
};

/**
 * Main Constructor of Modbus Connector Object
 */
inline modbus::modbus()
{
    m_host = "";
    m_port = 0;
    m_slaveid = 1;
    m_msg_id = 1;
    m_connected = false;
    err = false;
    err_no = 0;
    error_msg = "";
}

/**
 * Destructor of Modbus Connector Object
 */
inline modbus::~modbus(void) = default;

/**
 * Modbus HOST & IP Setter
 * @param host IP Address of Host
 * @param port Port for the TCP Connection
 */
inline void modbus::SetHostPort(std::string host, uint16_t port)
{
    m_host = host;
    m_port = port;
}

/**
 * Modbus Slave ID Setter
 * @param id  ID of the Modbus Server Slave
 */
inline void modbus::SetSlaveID(int id)
{
    m_slaveid = id;
}

/**
 * Build up a Modbus/TCP Connection
 * @return   If A Connection Is Successfully Built
 */
inline bool modbus::ModbusConnect()
{
    if (m_host.empty() || m_port == 0) {
        LOG("Missing Host and Port");
        return false;
    } else {
        LOG("Found Proper Host %s and Port %d", m_host.c_str(), m_port);
    }

#ifdef _WIN32
    if (WSAStartup(0x0202, &wsadata)) {
        return false;
    }
#endif

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (!X_ISVALIDSOCKET(m_socket)) {
        LOG("Error Opening Socket");
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    } else {
        LOG("Socket Opened Successfully");
    }

#ifdef WIN32
    const DWORD timeout = 20;
#else
    struct timeval timeout {};
    timeout.tv_sec = 20;  // after 20 seconds connect() will timeout
    timeout.tv_usec = 0;
#endif

    setsockopt(
        m_socket,
        SOL_SOCKET,
        SO_SNDTIMEO,
        (const char *)&timeout,
        sizeof(timeout));
    setsockopt(
        m_socket,
        SOL_SOCKET,
        SO_RCVTIMEO,
        (const char *)&timeout,
        sizeof(timeout));
    m_server.sin_family = AF_INET;
    m_server.sin_addr.s_addr = inet_addr(m_host.c_str());
    m_server.sin_port = htons(m_port);

    if (!X_ISCONNECTSUCCEED(
            connect(m_socket, (SOCKADDR *)&m_server, sizeof(m_server)))) {
        LOG("Connection Error");
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    LOG("Connected");
    m_connected = true;
    return true;
}

/**
 * Close the Modbus/TCP Connection
 */
inline void modbus::ModbusClose() const
{
    X_CLOSE_SOCKET(m_socket);
#ifdef _WIN32
    WSACleanup();
#endif
    LOG("Socket Closed");
}

/**
 * Modbus Request Builder
 * @param to_send   Message Buffer to Be Sent
 * @param address   Reference Address
 * @param func      Modbus Functional Code
 */
inline void
modbus::ModbusBuildRequest(uint8_t *to_send, uint16_t address, int func) const
{
    to_send[0] = (uint8_t)(m_msg_id >> 8u);
    to_send[1] = (uint8_t)(m_msg_id & 0x00FFu);
    to_send[2] = 0;
    to_send[3] = 0;
    to_send[4] = 0;
    to_send[6] = (uint8_t)m_slaveid;
    to_send[7] = (uint8_t)func;
    to_send[8] = (uint8_t)(address >> 8u);
    to_send[9] = (uint8_t)(address & 0x00FFu);
}

/**
 * Write Request Builder and Sender
 * @param address   Reference Address
 * @param amount    Amount of data to be Written
 * @param func      Modbus Functional Code
 * @param value     Data to Be Written
 */
inline int modbus::ModbusWrite(
    uint16_t address,
    uint16_t amount,
    int func,
    const uint16_t *value)
{
    int status = 0;
    uint8_t *to_send;
    if (func == WRITE_COIL || func == WRITE_REG) {
        to_send = new uint8_t[12];
        ModbusBuildRequest(to_send, address, func);
        to_send[5] = 6;
        to_send[10] = (uint8_t)(value[0] >> 8u);
        to_send[11] = (uint8_t)(value[0] & 0x00FFu);
        status = ModbusSend(to_send, 12);
    } else if (func == WRITE_REGS) {
        to_send = new uint8_t[13 + 2 * amount];
        ModbusBuildRequest(to_send, address, func);
        to_send[5] = (uint8_t)(7 + 2 * amount);
        to_send[10] = (uint8_t)(amount >> 8u);
        to_send[11] = (uint8_t)(amount & 0x00FFu);
        to_send[12] = (uint8_t)(2 * amount);
        for (int i = 0; i < amount; i++) {
            to_send[13 + 2 * i] = (uint8_t)(value[i] >> 8u);
            to_send[14 + 2 * i] = (uint8_t)(value[i] & 0x00FFu);
        }
        status = ModbusSend(to_send, 13 + 2 * amount);
    } else if (func == WRITE_COILS) {
        to_send = new uint8_t[14 + (amount - 1) / 8];
        ModbusBuildRequest(to_send, address, func);
        to_send[5] = (uint8_t)(7 + (amount + 7) / 8);
        to_send[10] = (uint8_t)(amount >> 8u);
        to_send[11] = (uint8_t)(amount & 0x00FFu);
        to_send[12] = (uint8_t)((amount + 7) / 8);
        for (int i = 0; i < (amount + 7) / 8; i++)
            to_send[13 + i] = 0;  // init needed before summing!
        for (int i = 0; i < amount; i++) {
            to_send[13 + i / 8] += (uint8_t)(value[i] << (i % 8u));
        }
        status = ModbusSend(to_send, 14 + (amount - 1) / 8);
    }
    delete[] to_send;
    return status;
}

/**
 * Read Request Builder and Sender
 * @param address   Reference Address
 * @param amount    Amount of Data to Read
 * @param func      Modbus Functional Code
 */
inline int modbus::ModbusRead(uint16_t address, uint16_t amount, int func)
{
    uint8_t to_send[12];
    ModbusBuildRequest(to_send, address, func);
    to_send[5] = 6;
    to_send[10] = (uint8_t)(amount >> 8u);
    to_send[11] = (uint8_t)(amount & 0x00FFu);
    return ModbusSend(to_send, 12);
}

/**
 * Read Holding Registers
 * MODBUS FUNCTION 0x03
 * @param address    Reference Address
 * @param amount     Amount of Registers to Read
 * @param buffer     Buffer to Store Data Read from Registers
 */
inline int modbus::ReadHoldingRegisters(
    uint16_t address,
    uint16_t amount,
    uint16_t *buffer)
{
    if (m_connected) {
        ModbusRead(address, amount, READ_REGS);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = ModbusReceive(to_rec);
        if (k == -1) {
            SetBadCon();
            return BAD_CON;
        }
        ModbuserrorHandle(to_rec, READ_REGS);
        if (err)
            return err_no;
        for (auto i = 0; i < amount; i++) {
            buffer[i] = ((uint16_t)to_rec[9u + 2u * i]) << 8u;
            buffer[i] += (uint16_t)to_rec[10u + 2u * i];
        }
        return 0;
    } else {
        SetBadCon();
        return BAD_CON;
    }
}

/**
 * Read Input Registers
 * MODBUS FUNCTION 0x04
 * @param address     Reference Address
 * @param amount      Amount of Registers to Read
 * @param buffer      Buffer to Store Data Read from Registers
 */
inline int
modbus::ReadInputRegisters(uint16_t address, uint16_t amount, uint16_t *buffer)
{
    if (m_connected) {
        ModbusRead(address, amount, READ_INPUT_REGS);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = ModbusReceive(to_rec);
        if (k == -1) {
            SetBadCon();
            return BAD_CON;
        }
        ModbuserrorHandle(to_rec, READ_INPUT_REGS);
        if (err)
            return err_no;
        for (auto i = 0; i < amount; i++) {
            buffer[i] = ((uint16_t)to_rec[9u + 2u * i]) << 8u;
            buffer[i] += (uint16_t)to_rec[10u + 2u * i];
        }
        return 0;
    } else {
        SetBadCon();
        return BAD_CON;
    }
}

/**
 * Read Coils
 * MODBUS FUNCTION 0x01
 * @param address     Reference Address
 * @param amount      Amount of Coils to Read
 * @param buffer      Buffer to Store Data Read from Coils
 */
inline int modbus::ReadCoils(uint16_t address, uint16_t amount, bool *buffer)
{
    if (m_connected) {
        if (amount > 2040) {
            SetBadInput();
            return EX_BAD_DATA;
        }
        ModbusRead(address, amount, READ_COILS);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = ModbusReceive(to_rec);
        if (k == -1) {
            SetBadCon();
            return BAD_CON;
        }
        ModbuserrorHandle(to_rec, READ_COILS);
        if (err)
            return err_no;
        for (auto i = 0; i < amount; i++) {
            buffer[i] = (bool)((to_rec[9u + i / 8u] >> (i % 8u)) & 1u);
        }
        return 0;
    } else {
        SetBadCon();
        return BAD_CON;
    }
}

/**
 * Read Input Bits(Discrete Data)
 * MODBUS FUNCITON 0x02
 * @param address   Reference Address
 * @param amount    Amount of Bits to Read
 * @param buffer    Buffer to store Data Read from Input Bits
 */
inline int
modbus::ReadInputBits(uint16_t address, uint16_t amount, bool *buffer)
{
    if (m_connected) {
        if (amount > 2040) {
            SetBadInput();
            return EX_BAD_DATA;
        }
        ModbusRead(address, amount, READ_INPUT_BITS);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = ModbusReceive(to_rec);
        if (k == -1) {
            SetBadCon();
            return BAD_CON;
        }
        if (err)
            return err_no;
        for (auto i = 0; i < amount; i++) {
            buffer[i] = (bool)((to_rec[9u + i / 8u] >> (i % 8u)) & 1u);
        }
        ModbuserrorHandle(to_rec, READ_INPUT_BITS);
        return 0;
    } else {
        return BAD_CON;
    }
}

/**
 * Write Single Coils
 * MODBUS FUNCTION 0x05
 * @param address    Reference Address
 * @param to_write   Value to be Written to Coil
 */
inline int modbus::WriteCoil(uint16_t address, const bool &to_write)
{
    if (m_connected) {
        int value = to_write * 0xFF00;
        ModbusWrite(address, 1, WRITE_COIL, (uint16_t *)&value);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = ModbusReceive(to_rec);
        if (k == -1) {
            SetBadCon();
            return BAD_CON;
        }
        ModbuserrorHandle(to_rec, WRITE_COIL);
        if (err)
            return err_no;
        return 0;
    } else {
        SetBadCon();
        return BAD_CON;
    }
}

/**
 * Write Single Register
 * FUCTION 0x06
 * @param address   Reference Address
 * @param value     Value to Be Written to Register
 */
inline int modbus::WriteRegisters(uint16_t address, const uint16_t &value)
{
    if (m_connected) {
        ModbusWrite(address, 1, WRITE_REG, &value);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = ModbusReceive(to_rec);
        if (k == -1) {
            SetBadCon();
            return BAD_CON;
        }
        ModbuserrorHandle(to_rec, WRITE_COIL);
        if (err)
            return err_no;
        return 0;
    } else {
        SetBadCon();
        return BAD_CON;
    }
}

/**
 * Write Multiple Coils
 * MODBUS FUNCTION 0x0F
 * @param address  Reference Address
 * @param amount   Amount of Coils to Write
 * @param value    Values to Be Written to Coils
 */
inline int
modbus::WriteCoils(uint16_t address, uint16_t amount, const bool *value)
{
    if (m_connected) {
        uint16_t *temp = new uint16_t[amount];
        for (int i = 0; i < amount; i++) {
            temp[i] = (uint16_t)value[i];
        }
        ModbusWrite(address, amount, WRITE_COILS, temp);
        delete[] temp;
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = ModbusReceive(to_rec);
        if (k == -1) {
            SetBadCon();
            return BAD_CON;
        }
        ModbuserrorHandle(to_rec, WRITE_COILS);
        if (err)
            return err_no;
        return 0;
    } else {
        SetBadCon();
        return BAD_CON;
    }
}

/**
 * Write Multiple Registers
 * MODBUS FUNCION 0x10
 * @param address Reference Address
 * @param amount  Amount of Value to Write
 * @param value   Values to Be Written to the Registers
 */
inline int
modbus::WriteRegisters(uint16_t address, uint16_t amount, const uint16_t *value)
{
    if (m_connected) {
        ModbusWrite(address, amount, WRITE_REGS, value);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = ModbusReceive(to_rec);
        if (k == -1) {
            SetBadCon();
            return BAD_CON;
        }
        ModbuserrorHandle(to_rec, WRITE_REGS);
        if (err)
            return err_no;
        return 0;
    } else {
        SetBadCon();
        return BAD_CON;
    }
}

/**
 * Data Sender
 * @param to_send Request to Be Sent to Server
 * @param length  Length of the Request
 * @return        Size of the request
 */
inline ssize_t modbus::ModbusSend(uint8_t *to_send, size_t length)
{
    m_msg_id++;
    return send(m_socket, (const char *)to_send, (size_t)length, 0);
}

/**
 * Data Receiver
 * @param buffer Buffer to Store the Data Retrieved
 * @return       Size of Incoming Data
 */
inline ssize_t modbus::ModbusReceive(uint8_t *buffer) const
{
    return recv(m_socket, (char *)buffer, MAX_MSG_LENGTH, 0);
}

inline void modbus::SetBadCon()
{
    err = true;
    error_msg = "BAD CONNECTION";
}

inline void modbus::SetBadInput()
{
    err = true;
    error_msg = "BAD FUNCTION INPUT";
}

/**
 * Error Code Handler
 * @param msg   Message Received from the Server
 * @param func  Modbus Functional Code
 */
inline void modbus::ModbuserrorHandle(const uint8_t *msg, int func)
{
    err = false;
    error_msg = "NO ERR";
    if (msg[7] == func + 0x80) {
        err = true;
        switch (msg[8]) {
            case EX_ILLEGAL_FUNCTION:
                error_msg = "1 Illegal Function";
                break;
            case EX_ILLEGAL_ADDRESS:
                error_msg = "2 Illegal Address";
                break;
            case EX_ILLEGAL_VALUE:
                error_msg = "3 Illegal Value";
                break;
            case EX_SERVER_FAILURE:
                error_msg = "4 Server Failure";
                break;
            case EX_ACKNOWLEDGE:
                error_msg = "5 Acknowledge";
                break;
            case EX_SERVER_BUSY:
                error_msg = "6 Server Busy";
                break;
            case EX_NEGATIVE_ACK:
                error_msg = "7 Negative Acknowledge";
                break;
            case EX_MEM_PARITY_PROB:
                error_msg = "8 Memory Parity Problem";
                break;
            case EX_GATEWAY_PROBLEMP:
                error_msg = "10 Gateway Path Unavailable";
                break;
            case EX_GATEWYA_PROBLEMF:
                error_msg = "11 Gateway Target Device Failed to Respond";
                break;
            default:
                error_msg = "UNK";
                break;
        }
    }
}

#endif  // MODBUSPP_MODBUS_H
