#include "ZW101_Fingerprint.h"

#if defined(__AVR__) || defined(ESP8266)
ZW101_Fingerprint::ZW101_Fingerprint(SoftwareSerial *ss, uint32_t password)
    : _serial(ss), _swSerial(ss), _hwSerial(nullptr), _password(password), _timeoutMs(ZW101_READ_TIMEOUT) {}
#endif

ZW101_Fingerprint::ZW101_Fingerprint(HardwareSerial *hs, uint32_t password)
    : _serial(hs), _hwSerial(hs), _password(password), _timeoutMs(ZW101_READ_TIMEOUT)
{
#if defined(__AVR__) || defined(ESP8266)
    _swSerial = nullptr;
#endif
}

ZW101_Fingerprint::ZW101_Fingerprint(Stream *serial, uint32_t password)
    : _serial(serial), _hwSerial(nullptr), _password(password), _timeoutMs(ZW101_READ_TIMEOUT)
{
#if defined(__AVR__) || defined(ESP8266)
    _swSerial = nullptr;
#endif
}

void ZW101_Fingerprint::begin(uint32_t baudrate)
{
    delay(100);
    if (_hwSerial)
    {
        _hwSerial->begin(baudrate);
    }
#if defined(__AVR__) || defined(ESP8266)
    if (_swSerial)
    {
        _swSerial->begin(baudrate);
    }
#endif
}

uint16_t ZW101_Fingerprint::readU16(const uint8_t *buf) const
{
    return (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
}

uint32_t ZW101_Fingerprint::readU32(const uint8_t *buf) const
{
    return (static_cast<uint32_t>(buf[0]) << 24) |
           (static_cast<uint32_t>(buf[1]) << 16) |
           (static_cast<uint32_t>(buf[2]) << 8) | buf[3];
}

uint8_t ZW101_Fingerprint::sendCommand(const uint8_t *payload, uint16_t payloadLen)
{
    if (!_serial || !payload || payloadLen == 0)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    const uint16_t packetLen = payloadLen + 2;
    uint16_t checksum = ZW101_PACKET_COMMAND + (packetLen >> 8) + (packetLen & 0xFF);

    _serial->write(static_cast<uint8_t>(ZW101_START_CODE >> 8));
    _serial->write(static_cast<uint8_t>(ZW101_START_CODE & 0xFF));

    _serial->write(static_cast<uint8_t>(deviceAddress >> 24));
    _serial->write(static_cast<uint8_t>(deviceAddress >> 16));
    _serial->write(static_cast<uint8_t>(deviceAddress >> 8));
    _serial->write(static_cast<uint8_t>(deviceAddress & 0xFF));

    _serial->write(ZW101_PACKET_COMMAND);
    _serial->write(static_cast<uint8_t>(packetLen >> 8));
    _serial->write(static_cast<uint8_t>(packetLen & 0xFF));

    for (uint16_t i = 0; i < payloadLen; ++i)
    {
        _serial->write(payload[i]);
        checksum += payload[i];
    }

    _serial->write(static_cast<uint8_t>(checksum >> 8));
    _serial->write(static_cast<uint8_t>(checksum & 0xFF));

    return ZW101_OK;
}

uint8_t ZW101_Fingerprint::readPacket(uint8_t &packetType, uint8_t *payload,
                                      uint16_t &payloadLen, uint16_t maxPayload,
                                      uint16_t timeoutMs)
{
    if (!_serial || !payload)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    unsigned long start = millis();
    auto readByte = [&](uint8_t &out) -> bool
    {
        while (!_serial->available())
        {
            if (millis() - start >= timeoutMs)
            {
                return false;
            }
            delay(1);
        }
        out = static_cast<uint8_t>(_serial->read());
        return true;
    };

    uint8_t b0 = 0, b1 = 0;
    do
    {
        if (!readByte(b0))
        {
            return ZW101_TIMEOUT;
        }
        if (b0 != static_cast<uint8_t>(ZW101_START_CODE >> 8))
        {
            continue;
        }
        if (!readByte(b1))
        {
            return ZW101_TIMEOUT;
        }
    } while (b0 != static_cast<uint8_t>(ZW101_START_CODE >> 8) ||
             b1 != static_cast<uint8_t>(ZW101_START_CODE & 0xFF));

    uint8_t addr[4];
    for (uint8_t i = 0; i < 4; ++i)
    {
        if (!readByte(addr[i]))
        {
            return ZW101_TIMEOUT;
        }
    }

    if (!readByte(packetType))
    {
        return ZW101_TIMEOUT;
    }

    uint8_t lenHi = 0, lenLo = 0;
    if (!readByte(lenHi) || !readByte(lenLo))
    {
        return ZW101_TIMEOUT;
    }
    const uint16_t totalLen = (static_cast<uint16_t>(lenHi) << 8) | lenLo;
    if (totalLen < 2)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    payloadLen = totalLen - 2;
    if (payloadLen > maxPayload)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint16_t checksum = packetType + lenHi + lenLo;
    for (uint16_t i = 0; i < payloadLen; ++i)
    {
        if (!readByte(payload[i]))
        {
            return ZW101_TIMEOUT;
        }
        checksum += payload[i];
    }

    uint8_t sumHi = 0, sumLo = 0;
    if (!readByte(sumHi) || !readByte(sumLo))
    {
        return ZW101_TIMEOUT;
    }

    const uint16_t received = (static_cast<uint16_t>(sumHi) << 8) | sumLo;
    if (received != checksum)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    return ZW101_OK;
}

bool ZW101_Fingerprint::verifyPassword()
{
    uint8_t cmd[5] = {
        ZW101_CMD_VERIFY_PASSWORD,
        static_cast<uint8_t>(_password >> 24),
        static_cast<uint8_t>(_password >> 16),
        static_cast<uint8_t>(_password >> 8),
        static_cast<uint8_t>(_password)};

    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return false;
    }

    uint8_t packetType = 0;
    uint8_t payload[16];
    uint16_t len = 0;
    if (readPacket(packetType, payload, len, sizeof(payload), _timeoutMs) != ZW101_OK)
    {
        return false;
    }

    return packetType == ZW101_PACKET_ACK && len >= 1 && payload[0] == ZW101_OK;
}

uint8_t ZW101_Fingerprint::setPassword(uint32_t password)
{
    uint8_t cmd[5] = {
        ZW101_CMD_SET_PASSWORD,
        static_cast<uint8_t>(password >> 24),
        static_cast<uint8_t>(password >> 16),
        static_cast<uint8_t>(password >> 8),
        static_cast<uint8_t>(password)};

    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[16];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    if (payload[0] == ZW101_OK)
    {
        _password = password;
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::handshake()
{
    uint8_t cmd[] = {ZW101_CMD_HANDSHAKE};
    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[8];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::getParameters()
{
    uint8_t cmd[] = {ZW101_CMD_READ_SYSPARAM};
    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[32];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 17)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    if (payload[0] != ZW101_OK)
    {
        return payload[0];
    }

    statusReg = readU16(payload + 1);
    systemId = readU16(payload + 3);
    capacity = readU16(payload + 5);
    securityLevel = readU16(payload + 7);
    deviceAddress = readU32(payload + 9);

    uint16_t packetLenCode = readU16(payload + 13);
    if (packetLenCode == 0)
    {
        packetLength = 32;
    }
    else if (packetLenCode == 1)
    {
        packetLength = 64;
    }
    else if (packetLenCode == 2)
    {
        packetLength = 128;
    }
    else if (packetLenCode == 3)
    {
        packetLength = 256;
    }
    else
    {
        packetLength = packetLenCode;
    }

    baudRate = readU16(payload + 15) * 9600;
    return payload[0];
}

uint8_t ZW101_Fingerprint::getTemplateCount()
{
    uint8_t cmd[] = {ZW101_CMD_TEMPLATE_COUNT};
    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[16];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 3)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    if (payload[0] == ZW101_OK)
    {
        templateCount = readU16(payload + 1);
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::getImage()
{
    uint8_t cmd[] = {ZW101_CMD_GET_IMAGE};
    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[8];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::image2Tz(uint8_t slot)
{
    uint8_t cmd[] = {ZW101_CMD_GEN_CHAR, slot};
    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[8];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::createModel()
{
    uint8_t cmd[] = {ZW101_CMD_REG_MODEL};
    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[8];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::storeModel(uint16_t id, uint8_t slot)
{
    uint8_t cmd[] = {ZW101_CMD_STORE, slot, static_cast<uint8_t>(id >> 8), static_cast<uint8_t>(id)};
    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[8];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::loadModel(uint16_t id, uint8_t slot)
{
    uint8_t cmd[] = {ZW101_CMD_LOAD, slot, static_cast<uint8_t>(id >> 8), static_cast<uint8_t>(id)};
    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[8];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::deleteModel(uint16_t id, uint16_t count)
{
    uint8_t cmd[] = {
        ZW101_CMD_DELETE,
        static_cast<uint8_t>(id >> 8),
        static_cast<uint8_t>(id),
        static_cast<uint8_t>(count >> 8),
        static_cast<uint8_t>(count)};

    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[8];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::emptyDatabase()
{
    uint8_t cmd[] = {ZW101_CMD_EMPTY};
    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[8];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::fingerSearch(uint8_t slot, uint16_t startPage, uint16_t pageNum)
{
    uint8_t cmd[] = {
        ZW101_CMD_SEARCH,
        slot,
        static_cast<uint8_t>(startPage >> 8),
        static_cast<uint8_t>(startPage),
        static_cast<uint8_t>(pageNum >> 8),
        static_cast<uint8_t>(pageNum)};

    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[16];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    if (payload[0] == ZW101_OK && len >= 5)
    {
        fingerID = readU16(payload + 1);
        confidence = readU16(payload + 3);
    }
    else
    {
        fingerID = 0xFFFF;
        confidence = 0;
    }

    return payload[0];
}

uint8_t ZW101_Fingerprint::fingerFastSearch()
{
    uint16_t pages = capacity > 0 ? capacity : 3000;
    return fingerSearch(1, 0, pages);
}

uint8_t ZW101_Fingerprint::autoEnroll(uint16_t id, uint8_t enrollCount, uint16_t flags,
                                      uint8_t *step, uint8_t *stepValue)
{
    uint8_t cmd[] = {
        ZW101_CMD_AUTO_ENROLL,
        static_cast<uint8_t>(id >> 8),
        static_cast<uint8_t>(id),
        enrollCount,
        static_cast<uint8_t>(flags >> 8),
        static_cast<uint8_t>(flags)};

    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[16];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    if (step && len >= 2)
    {
        *step = payload[1];
    }
    if (stepValue && len >= 3)
    {
        *stepValue = payload[2];
    }

    return payload[0];
}

uint8_t ZW101_Fingerprint::autoIdentify(uint8_t scoreLevel, uint16_t id, uint16_t flags,
                                        uint16_t *matchedId, uint16_t *score,
                                        uint8_t *stage)
{
    uint8_t cmd[] = {
        ZW101_CMD_AUTO_IDENTIFY,
        scoreLevel,
        static_cast<uint8_t>(id >> 8),
        static_cast<uint8_t>(id),
        static_cast<uint8_t>(flags >> 8),
        static_cast<uint8_t>(flags)};

    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[16];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    if (stage && len >= 2)
    {
        *stage = payload[1];
    }
    if (matchedId && len >= 4)
    {
        *matchedId = readU16(payload + 2);
    }
    if (score && len >= 6)
    {
        *score = readU16(payload + 4);
    }

    if (payload[0] == ZW101_OK && len >= 6)
    {
        fingerID = readU16(payload + 2);
        confidence = readU16(payload + 4);
    }

    return payload[0];
}

uint8_t ZW101_Fingerprint::cancel()
{
    uint8_t cmd[] = {ZW101_CMD_CANCEL};
    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[8];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::setLedMode(uint8_t mode)
{
    uint8_t cmd[] = {ZW101_CMD_BLN_MODE_SWITCH, mode};
    if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
    {
        return ZW101_PACKETRECIEVEERR;
    }

    uint8_t packetType = 0;
    uint8_t payload[8];
    uint16_t len = 0;
    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }
    return payload[0];
}

uint8_t ZW101_Fingerprint::controlLed(uint8_t func, uint8_t startColor, uint8_t endColor,
                                      uint8_t cycles, uint8_t duty, uint8_t period)
{
    uint8_t packetType = 0;
    uint8_t payload[8];
    uint16_t len = 0;

    if (duty == 0 && period == 0)
    {
        uint8_t cmd[] = {ZW101_CMD_CONTROL_BLN, func, startColor, endColor, cycles};
        if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
        {
            return ZW101_PACKETRECIEVEERR;
        }
    }
    else
    {
        uint8_t cmd[] = {ZW101_CMD_CONTROL_BLN, func, startColor, endColor, duty, cycles, period};
        if (sendCommand(cmd, sizeof(cmd)) != ZW101_OK)
        {
            return ZW101_PACKETRECIEVEERR;
        }
    }

    uint8_t rc = readPacket(packetType, payload, len, sizeof(payload), _timeoutMs);
    if (rc != ZW101_OK || packetType != ZW101_PACKET_ACK || len < 1)
    {
        return ZW101_PACKETRECIEVEERR;
    }
    return payload[0];
}
