#include <ZW101_Fingerprint.h>

#if defined(ARDUINO_ARCH_AVR) && !defined(__AVR_ATmega2560__)
#include <SoftwareSerial.h>
SoftwareSerial zwSerial(2, 3); // RX, TX
ZW101_Fingerprint finger(&zwSerial);
#elif defined(ARDUINO_ARCH_ESP32)
HardwareSerial zwSerial(2);
ZW101_Fingerprint finger(&zwSerial);
#else
#define zwSerial Serial1
ZW101_Fingerprint finger(&zwSerial);
#endif

static uint16_t gEnrollId = 1;
static uint8_t gEnrollTimes = 3;

static void waitSerial()
{
    while (!Serial)
    {
        ;
    }
}

static void initSensor()
{
#if defined(ARDUINO_ARCH_ESP32)
    zwSerial.begin(57600, SERIAL_8N1, 16, 17);
#else
    zwSerial.begin(57600);
#endif
    finger.begin(57600);
    finger.setTimeout(3000);

    if (finger.handshake() != ZW101_OK)
    {
        Serial.println("ZW101 not found");
        while (1)
        {
            delay(10);
        }
    }
}

static uint8_t captureToSlot(uint8_t slot)
{
    waitForFingerPress();

    uint8_t p = finger.getImage();
    if (p != ZW101_OK)
    {
        waitForFingerRelease();
        return p;
    }
    p = finger.image2Tz(slot);
    waitForFingerRelease();
    return p;
}

void setup()
{
    Serial.begin(115200);
    waitSerial();
    initSensor();

    Serial.println();
    Serial.println("== ZW101 enroll example ==");
    Serial.println("Input ID (1..65535), end with newline:");

    while (Serial.available() == 0)
    {
        delay(10);
    }
    gEnrollId = (uint16_t)Serial.parseInt();
    if (gEnrollId == 0)
    {
        gEnrollId = 1;
    }

    Serial.print("Enroll ID: ");
    Serial.println(gEnrollId);
    Serial.println("Press finger to capture first image...");

    uint8_t p = captureToSlot(1);
    if (p != ZW101_OK)
    {
        Serial.print("First capture failed: 0x");
        Serial.println(p, HEX);
        return;
    }

    Serial.println("Press same finger again to capture second image...");
    p = captureToSlot(2);
    if (p != ZW101_OK)
    {
        Serial.print("Second capture failed: 0x");
        Serial.println(p, HEX);
        return;
    }

    p = finger.createModel();
    if (p != ZW101_OK)
    {
        Serial.print("createModel failed: 0x");
        Serial.println(p, HEX);
        return;
    }

    p = finger.storeModel(gEnrollId, 1);
    if (p != ZW101_OK)
    {
        Serial.print("storeModel failed: 0x");
        Serial.println(p, HEX);
        return;
    }

    Serial.println("Enroll success");
    finger.getTemplateCount();
    Serial.print("Template count: ");
    Serial.println(finger.templateCount);
}

void loop()
{
    delay(1000);
}
