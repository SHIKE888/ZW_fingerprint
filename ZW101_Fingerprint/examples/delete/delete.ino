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

#if defined(ARDUINO_ARCH_ESP32)
#define TOUCH_PIN 4
#else
#define TOUCH_PIN 4
#endif

// Default is HIGH-active detect pin. Change to LOW if your module is active-low.
#define TOUCH_ACTIVE_LEVEL HIGH

static bool isTouchPressed()
{
    return digitalRead(TOUCH_PIN) == TOUCH_ACTIVE_LEVEL;
}

static void waitForFingerPress()
{
    while (!isTouchPressed())
    {
        delay(20);
    }
}

static void waitForFingerRelease()
{
    while (isTouchPressed())
    {
        delay(20);
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

#if defined(ARDUINO_ARCH_ESP32)
    zwSerial.begin(57600, SERIAL_8N1, 16, 17);
#else
    zwSerial.begin(57600);
#endif

    finger.begin(57600);
    finger.setTimeout(1500);

    pinMode(TOUCH_PIN, INPUT_PULLUP);

    if (finger.handshake() != ZW101_OK)
    {
        Serial.println("ZW101 not found");
        while (1)
        {
            delay(10);
        }
    }

    Serial.println("Input ID to delete:");
    while (Serial.available() == 0)
    {
        delay(10);
    }
    uint16_t id = (uint16_t)Serial.parseInt();
    if (id == 0)
    {
        id = 1;
    }

    Serial.print("Touch pin: ");
    Serial.println(TOUCH_PIN);
    Serial.println("Press finger to start delete.");

    waitForFingerPress();

    uint8_t p = finger.deleteModel(id, 1);
    if (p == ZW101_OK)
    {
        Serial.print("Deleted ID: ");
        Serial.println(id);
    }
    else
    {
        Serial.print("deleteModel failed: 0x");
        Serial.println(p, HEX);
    }

    finger.getTemplateCount();
    Serial.print("Template count: ");
    Serial.println(finger.templateCount);

    waitForFingerRelease();
}

void loop()
{
    delay(1000);
}
