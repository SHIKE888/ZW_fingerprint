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
    zwSerial.begin(57600, SERIAL_8N1, 16, 17); // RX, TX pins can be changed
#else
    zwSerial.begin(57600);
#endif

    finger.begin(57600);
    finger.setTimeout(1200);

    pinMode(TOUCH_PIN, INPUT_PULLUP);

    if (finger.handshake() != ZW101_OK)
    {
        Serial.println("ZW101 not found");
        while (1)
        {
            delay(10);
        }
    }

    Serial.println("ZW101 ready");
    finger.getParameters();
    finger.getTemplateCount();

    Serial.print("Capacity: ");
    Serial.println(finger.capacity);
    Serial.print("Template count: ");
    Serial.println(finger.templateCount);
    Serial.print("Touch pin: ");
    Serial.println(TOUCH_PIN);
    Serial.println("One press prints one result.");
}

void loop()
{
    waitForFingerPress();

    uint8_t p = finger.getImage();
    if (p == ZW101_NOFINGER)
    {
        waitForFingerRelease();
        return;
    }
    if (p != ZW101_OK)
    {
        Serial.print("getImage error: 0x");
        Serial.println(p, HEX);
        waitForFingerRelease();
        return;
    }

    p = finger.image2Tz(1);
    if (p != ZW101_OK)
    {
        Serial.print("image2Tz error: 0x");
        Serial.println(p, HEX);
        waitForFingerRelease();
        return;
    }

    p = finger.fingerSearch(1, 0, finger.capacity > 0 ? finger.capacity : 3000);
    if (p == ZW101_OK && finger.confidence > 0)
    {
        Serial.print("Match ID: ");
        Serial.print(finger.fingerID);
        Serial.print(" score: ");
        Serial.println(finger.confidence);
    }
    else if (p == ZW101_OK)
    {
        Serial.println("No valid match");
    }
    else if (p == ZW101_NOTFOUND)
    {
        Serial.println("No match");
    }
    else
    {
        Serial.print("Search error: 0x");
        Serial.println(p, HEX);
    }

    waitForFingerRelease();
}
