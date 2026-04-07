#ifndef ZW101_FINGERPRINT_H
#define ZW101_FINGERPRINT_H

#include <Arduino.h>

#if defined(__AVR__) || defined(ESP8266)
#include <SoftwareSerial.h>
#endif

#define ZW101_OK 0x00
#define ZW101_PACKETRECIEVEERR 0x01
#define ZW101_NOFINGER 0x02
#define ZW101_IMAGEFAIL 0x03
#define ZW101_IMAGEMESS 0x06
#define ZW101_FEATUREFAIL 0x07
#define ZW101_NOMATCH 0x08
#define ZW101_NOTFOUND 0x09
#define ZW101_ENROLLMISMATCH 0x0A
#define ZW101_BADLOCATION 0x0B
#define ZW101_DBRANGEFAIL 0x0C
#define ZW101_UPLOADFEATUREFAIL 0x0D
#define ZW101_PACKETRESPONSEFAIL 0x0E
#define ZW101_UPLOADFAIL 0x0F
#define ZW101_DELETEFAIL 0x10
#define ZW101_DBCLEARFAIL 0x11
#define ZW101_PASSFAIL 0x13
#define ZW101_INVALIDIMAGE 0x15
#define ZW101_FLASHERR 0x18
#define ZW101_INVALIDREG 0x1A
#define ZW101_TIMEOUT 0x26
#define ZW101_ALREADY_EXISTS 0x27
#define ZW101_SENSOR_ERROR 0x29

#define ZW101_DEFAULT_ADDRESS 0xFFFFFFFF
#define ZW101_START_CODE 0xEF01

#define ZW101_PACKET_COMMAND 0x01
#define ZW101_PACKET_DATA 0x02
#define ZW101_PACKET_ACK 0x07
#define ZW101_PACKET_ENDDATA 0x08

#define ZW101_CMD_GET_IMAGE 0x01
#define ZW101_CMD_GEN_CHAR 0x02
#define ZW101_CMD_MATCH 0x03
#define ZW101_CMD_SEARCH 0x04
#define ZW101_CMD_REG_MODEL 0x05
#define ZW101_CMD_STORE 0x06
#define ZW101_CMD_LOAD 0x07
#define ZW101_CMD_UP_CHAR 0x08
#define ZW101_CMD_DOWN_CHAR 0x09
#define ZW101_CMD_DELETE 0x0C
#define ZW101_CMD_EMPTY 0x0D
#define ZW101_CMD_WRITE_REG 0x0E
#define ZW101_CMD_READ_SYSPARAM 0x0F
#define ZW101_CMD_SET_PASSWORD 0x12
#define ZW101_CMD_VERIFY_PASSWORD 0x13
#define ZW101_CMD_TEMPLATE_COUNT 0x1D
#define ZW101_CMD_CANCEL 0x30
#define ZW101_CMD_AUTO_ENROLL 0x31
#define ZW101_CMD_AUTO_IDENTIFY 0x32
#define ZW101_CMD_SLEEP 0x33
#define ZW101_CMD_GET_CHIP_SN 0x34
#define ZW101_CMD_HANDSHAKE 0x35
#define ZW101_CMD_CHECK_SENSOR 0x36
#define ZW101_CMD_CONTROL_BLN 0x3C
#define ZW101_CMD_GET_IMAGE_INFO 0x3D
#define ZW101_CMD_SEARCH_NOW 0x3E
#define ZW101_CMD_READ_EXTRA_PARAM 0x62
#define ZW101_CMD_WRITE_EM_PARA 0x63
#define ZW101_CMD_BLN_MODE_SWITCH 0x60

#define ZW101_READ_TIMEOUT 1000

class ZW101_Fingerprint
{
public:
#if defined(__AVR__) || defined(ESP8266)
    explicit ZW101_Fingerprint(SoftwareSerial *ss, uint32_t password = 0x00000000);
#endif
    explicit ZW101_Fingerprint(HardwareSerial *hs, uint32_t password = 0x00000000);
    explicit ZW101_Fingerprint(Stream *serial, uint32_t password = 0x00000000);

    void begin(uint32_t baudrate);

    bool verifyPassword();
    uint8_t setPassword(uint32_t password);
    uint8_t handshake();

    uint8_t getParameters();
    uint8_t getTemplateCount();

    uint8_t getImage();
    uint8_t image2Tz(uint8_t slot = 1);
    uint8_t createModel();
    uint8_t storeModel(uint16_t id, uint8_t slot = 1);
    uint8_t loadModel(uint16_t id, uint8_t slot = 1);
    uint8_t deleteModel(uint16_t id, uint16_t count = 1);
    uint8_t emptyDatabase();

    uint8_t fingerSearch(uint8_t slot = 1, uint16_t startPage = 0, uint16_t pageNum = 0x03E8);
    uint8_t fingerFastSearch();

    uint8_t autoEnroll(uint16_t id, uint8_t enrollCount, uint16_t flags,
                       uint8_t *step = nullptr, uint8_t *stepValue = nullptr);
    uint8_t autoIdentify(uint8_t scoreLevel, uint16_t id, uint16_t flags,
                         uint16_t *matchedId = nullptr, uint16_t *score = nullptr,
                         uint8_t *stage = nullptr);
    uint8_t cancel();

    uint8_t setLedMode(uint8_t mode);
    uint8_t controlLed(uint8_t func, uint8_t startColor, uint8_t endColor,
                       uint8_t cycles, uint8_t duty = 0, uint8_t period = 0);

    void setTimeout(uint16_t timeoutMs) { _timeoutMs = timeoutMs; }

    uint16_t fingerID = 0xFFFF;
    uint16_t confidence = 0;
    uint16_t templateCount = 0;

    uint16_t statusReg = 0;
    uint16_t systemId = 0;
    uint16_t capacity = 0;
    uint16_t securityLevel = 0;
    uint32_t deviceAddress = ZW101_DEFAULT_ADDRESS;
    uint16_t packetLength = 0;
    uint16_t baudRate = 0;

private:
    Stream *_serial;
#if defined(__AVR__) || defined(ESP8266)
    SoftwareSerial *_swSerial;
#endif
    HardwareSerial *_hwSerial;

    uint32_t _password;
    uint16_t _timeoutMs;

    uint8_t sendCommand(const uint8_t *payload, uint16_t payloadLen);
    uint8_t readPacket(uint8_t &packetType, uint8_t *payload, uint16_t &payloadLen,
                       uint16_t maxPayload, uint16_t timeoutMs);

    uint16_t readU16(const uint8_t *buf) const;
    uint32_t readU32(const uint8_t *buf) const;
};

#endif
