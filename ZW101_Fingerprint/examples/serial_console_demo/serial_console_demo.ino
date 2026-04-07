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

static char lineBuf[96];
static uint8_t lineLen = 0;

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

static int parseIntToken(char *&p, int fallback)
{
    while (*p == ' ')
    {
        ++p;
    }
    if (*p == '\0')
    {
        return fallback;
    }
    char *endPtr = nullptr;
    long v = strtol(p, &endPtr, 0);
    if (endPtr == p)
    {
        return fallback;
    }
    p = endPtr;
    return (int)v;
}

static void printHelp()
{
    Serial.println();
    Serial.println("Commands:");
    Serial.println("  help");
    Serial.println("  info");
    Serial.println("  count");
    Serial.println("  search");
    Serial.println("  enroll <id> [times]");
    Serial.println("  autoid [id|-1]");
    Serial.println("  delete <id> [count]");
    Serial.println("  clear");
    Serial.println("  ledmode <0|255>");
    Serial.println("  led <func> <start> <end> <cycles> [duty] [period]");
    Serial.println("  cancel");
    Serial.println();
}

static bool captureToSlot(uint8_t slot)
{
    waitForFingerPress();

    uint8_t p = finger.getImage();
    if (p != ZW101_OK)
    {
        Serial.print("getImage err=0x");
        Serial.println(p, HEX);
        waitForFingerRelease();
        return false;
    }
    p = finger.image2Tz(slot);
    if (p != ZW101_OK)
    {
        Serial.print("image2Tz err=0x");
        Serial.println(p, HEX);
        waitForFingerRelease();
        return false;
    }
    waitForFingerRelease();
    return true;
}

static void doSearch()
{
    if (!captureToSlot(1))
    {
        return;
    }
    uint8_t p = finger.fingerSearch(1, 0, finger.capacity > 0 ? finger.capacity : 3000);
    if (p == ZW101_OK && finger.confidence > 0)
    {
        Serial.print("MATCH id=");
        Serial.print(finger.fingerID);
        Serial.print(" score=");
        Serial.println(finger.confidence);
    }
    else if (p == ZW101_OK)
    {
        Serial.println("NO VALID MATCH");
    }
    else if (p == ZW101_NOTFOUND)
    {
        Serial.println("NO MATCH");
    }
    else
    {
        Serial.print("search err=0x");
        Serial.println(p, HEX);
    }
}

static void doEnroll(uint16_t id, uint8_t times)
{
    waitForFingerPress();
    uint8_t step = 0;
    uint8_t stepValue = 0;
    uint8_t p = finger.autoEnroll(id, times, 0x0000, &step, &stepValue);
    Serial.print("autoEnroll rc=0x");
    Serial.print(p, HEX);
    Serial.print(" step=0x");
    Serial.print(step, HEX);
    Serial.print(" val=0x");
    Serial.println(stepValue, HEX);
    waitForFingerRelease();
}

static void doAutoId(uint16_t id)
{
    waitForFingerPress();
    uint16_t matchedId = 0xFFFF;
    uint16_t score = 0;
    uint8_t stage = 0;
    uint8_t p = finger.autoIdentify(3, id, 0x0000, &matchedId, &score, &stage);

    if (p == ZW101_OK && stage == 0x05 && score > 0)
    {
        Serial.print("MATCH id=");
        Serial.print(matchedId);
        Serial.print(" score=");
        Serial.println(score);
        waitForFingerRelease();
        return;
    }

    waitForFingerRelease();
    if (p == ZW101_OK && stage == 0x05)
    {
        Serial.println("NO VALID MATCH");
        waitForFingerRelease();
        return;
    }

    Serial.print("autoIdentify rc=0x");
    Serial.print(p, HEX);
    Serial.print(" stage=0x");
    Serial.print(stage, HEX);
    Serial.print(" id=");
    Serial.print(matchedId);
    Serial.print(" score=");
    Serial.println(score);
}

static void executeCommand(char *line)
{
    while (*line == ' ')
    {
        ++line;
    }
    if (*line == '\0')
    {
        return;
    }

    char *args = strchr(line, ' ');
    if (args)
    {
        *args = '\0';
        ++args;
    }

    if (strcmp(line, "help") == 0)
    {
        printHelp();
    }
    else if (strcmp(line, "info") == 0)
    {
        uint8_t p = finger.getParameters();
        Serial.print("getParameters=0x");
        Serial.println(p, HEX);
        if (p == ZW101_OK)
        {
            Serial.print("capacity=");
            Serial.println(finger.capacity);
            Serial.print("security=");
            Serial.println(finger.securityLevel);
            Serial.print("packetLength=");
            Serial.println(finger.packetLength);
            Serial.print("baudRate=");
            Serial.println(finger.baudRate);
        }
    }
    else if (strcmp(line, "count") == 0)
    {
        uint8_t p = finger.getTemplateCount();
        Serial.print("getTemplateCount=0x");
        Serial.print(p, HEX);
        Serial.print(" count=");
        Serial.println(finger.templateCount);
    }
    else if (strcmp(line, "search") == 0)
    {
        doSearch();
    }
    else if (strcmp(line, "enroll") == 0)
    {
        if (!args)
        {
            Serial.println("usage: enroll <id> [times]");
            return;
        }
        char *p = args;
        int id = parseIntToken(p, 1);
        int times = parseIntToken(p, 3);
        if (id < 1 || id > 65535 || times < 1 || times > 10)
        {
            Serial.println("invalid args");
            return;
        }
        doEnroll((uint16_t)id, (uint8_t)times);
    }
    else if (strcmp(line, "autoid") == 0)
    {
        uint16_t id = 0xFFFF;
        if (args)
        {
            char *p = args;
            int v = parseIntToken(p, -1);
            if (v >= 0 && v <= 65535)
            {
                id = (uint16_t)v;
            }
        }
        doAutoId(id);
    }
    else if (strcmp(line, "delete") == 0)
    {
        if (!args)
        {
            Serial.println("usage: delete <id> [count]");
            return;
        }
        char *p = args;
        int id = parseIntToken(p, -1);
        int count = parseIntToken(p, 1);
        if (id < 0 || id > 65535 || count < 1 || count > 65535)
        {
            Serial.println("invalid args");
            return;
        }
        uint8_t rc = finger.deleteModel((uint16_t)id, (uint16_t)count);
        Serial.print("delete rc=0x");
        Serial.println(rc, HEX);
    }
    else if (strcmp(line, "clear") == 0)
    {
        uint8_t rc = finger.emptyDatabase();
        Serial.print("empty rc=0x");
        Serial.println(rc, HEX);
    }
    else if (strcmp(line, "ledmode") == 0)
    {
        if (!args)
        {
            Serial.println("usage: ledmode <0|255>");
            return;
        }
        char *p = args;
        int mode = parseIntToken(p, 255);
        uint8_t rc = finger.setLedMode((uint8_t)mode);
        Serial.print("ledmode rc=0x");
        Serial.println(rc, HEX);
    }
    else if (strcmp(line, "led") == 0)
    {
        if (!args)
        {
            Serial.println("usage: led <func> <start> <end> <cycles> [duty] [period]");
            return;
        }
        char *p = args;
        int func = parseIntToken(p, 3);
        int start = parseIntToken(p, 0x01);
        int end = parseIntToken(p, start);
        int cycles = parseIntToken(p, 0);
        int duty = parseIntToken(p, 0);
        int period = parseIntToken(p, 0);
        uint8_t rc = finger.controlLed((uint8_t)func, (uint8_t)start, (uint8_t)end,
                                       (uint8_t)cycles, (uint8_t)duty, (uint8_t)period);
        Serial.print("led rc=0x");
        Serial.println(rc, HEX);
    }
    else if (strcmp(line, "cancel") == 0)
    {
        uint8_t rc = finger.cancel();
        Serial.print("cancel rc=0x");
        Serial.println(rc, HEX);
    }
    else
    {
        Serial.println("unknown command, input 'help'");
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

    uint8_t hs = finger.handshake();
    Serial.print("handshake=0x");
    Serial.println(hs, HEX);
    printHelp();
}

void loop()
{
    while (Serial.available() > 0)
    {
        char c = (char)Serial.read();
        if (c == '\r')
        {
            continue;
        }
        if (c == '\n')
        {
            lineBuf[lineLen] = '\0';
            executeCommand(lineBuf);
            lineLen = 0;
            continue;
        }
        if (lineLen < sizeof(lineBuf) - 1)
        {
            lineBuf[lineLen++] = c;
        }
    }
}
