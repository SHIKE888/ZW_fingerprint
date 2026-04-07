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

// Set to LOW when your module's detect pin is active-low, HIGH for active-high.
#define TOUCH_ACTIVE_LEVEL LOW

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
    finger.setTimeout(5000);

    pinMode(TOUCH_PIN, INPUT_PULLUP);

    if (finger.handshake() != ZW101_OK)
    {
        Serial.println("ZW101 not found");
        while (1)
        {
            delay(10);
        }
    }

    Serial.println("Auto identify started (1:N)");
    Serial.print("Touch pin: ");
    Serial.println(TOUCH_PIN);
    Serial.println("One press prints one result.");
}

void loop()
{
    waitForFingerPress();

    uint16_t matchedId = 0xFFFF;
    uint16_t score = 0;
    uint8_t stage = 0;

    // scoreLevel=3, id=0xFFFF means 1:N search, flags=0 for step feedback
    uint8_t p = finger.autoIdentify(3, 0xFFFF, 0x0000, &matchedId, &score, &stage);

    if (p == ZW101_OK && stage == 0x05 && score > 0)
    {
        Serial.print("Matched ID: ");
        Serial.print(matchedId);
        Serial.print(" score: ");
        Serial.println(score);
    }
    else if (p == ZW101_OK && stage == 0x05)
    {
        Serial.println("No valid match");
    }
    else if (p == ZW101_NOTFOUND)
    {
        Serial.println("No match");
    }
    else if (p == ZW101_TIMEOUT || p == ZW101_NOFINGER || (p == ZW101_OK && stage != 0x05))
    {
        // keep silent for common idle states
    }
    else
    {
        Serial.print("autoIdentify code: 0x");
        Serial.print(p, HEX);
        Serial.print(" stage: 0x");
        Serial.println(stage, HEX);
    }

    waitForFingerRelease();
}
