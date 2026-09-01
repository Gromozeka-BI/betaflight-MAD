/*
 * Betaflight-BOOST LEDSTRIP_UART receiver
 * Board : Arduino Nano
 * Libs  : Adafruit NeoPixel
 *
 *   FC LED_STRIP TX -> Nano D4
 *   FC GND          -> Nano GND
 *   Strips DIN      -> D9, D10, D5, D6
 *
 *   set ledstrip_output = UART
 *   save
 */

#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

#define FC_RX_PIN           4
#define FC_TX_UNUSED_PIN    3

#define NUM_LEDS            28
#define LED_TYPE            (NEO_GRB + NEO_KHZ800)

#define UART_BAUD           9600
#define FAILSAFE_MS         200
#define LINE_MAX            48
#define BOOST_BLINK_MS      125
#define SHOW_MIN_MS         15
#define LINE_STALE_MS       30

SoftwareSerial fcSerial(FC_RX_PIN, FC_TX_UNUSED_PIN);

Adafruit_NeoPixel strips[] = {
    Adafruit_NeoPixel(NUM_LEDS, 9,  LED_TYPE),
    Adafruit_NeoPixel(NUM_LEDS, 10, LED_TYPE),
    Adafruit_NeoPixel(NUM_LEDS, 5,  LED_TYPE),
    Adafruit_NeoPixel(NUM_LEDS, 6,  LED_TYPE),
};
#define NUM_STRIPS  (sizeof(strips) / sizeof(strips[0]))

static uint8_t boostOn, bright, colR, colG, colB;
static bool haveFrame = false;
static uint32_t lastRxMs = 0;
static uint32_t lastByteMs = 0;
static uint32_t lastShowMs = 0;

static char lineBuf[LINE_MAX + 1];
static uint8_t lineLen = 0;

static void fillAll(uint8_t r, uint8_t g, uint8_t b)
{
    const uint32_t c = Adafruit_NeoPixel::Color(r, g, b);
    for (uint8_t s = 0; s < NUM_STRIPS; s++) {
        strips[s].fill(c);
        strips[s].show();
    }
}

static bool parseFrame(const char *line)
{
    if (line[0] != 'L' || line[1] != ',') {
        return false;
    }

    unsigned v[9];
    const char *p = line + 2;

    for (uint8_t i = 0; i < 9; i++) {
        if (*p < '0' || *p > '9') {
            return false;
        }
        unsigned acc = 0;
        while (*p >= '0' && *p <= '9') {
            acc = acc * 10u + (unsigned)(*p - '0');
            p++;
        }
        v[i] = acc;
        if (i < 8) {
            if (*p != ',') {
                return false;
            }
            p++;
        }
    }
    if (*p != '\0') {
        return false;
    }

    boostOn = v[1] ? 1 : 0;
    bright  = v[4] > 100 ? 100 : (uint8_t)v[4];
    colR    = v[6] > 255 ? 255 : (uint8_t)v[6];
    colG    = v[7] > 255 ? 255 : (uint8_t)v[7];
    colB    = v[8] > 255 ? 255 : (uint8_t)v[8];
    return true;
}

static void feedByte(char c)
{
    lastByteMs = millis();

    if (c == '\n') {
        lineBuf[lineLen] = '\0';
        if (lineLen > 0 && parseFrame(lineBuf)) {
            haveFrame = true;
            lastRxMs = millis();
        }
        lineLen = 0;
        return;
    }

    if (c == '\r') {
        return;
    }

    if (lineLen >= LINE_MAX) {
        lineLen = 0;
        return;
    }

    lineBuf[lineLen++] = c;
}

static void render(uint32_t now)
{
    if (!haveFrame || (now - lastRxMs) > FAILSAFE_MS) {
        fillAll(0, 0, 0);
        return;
    }

    uint8_t r = (uint16_t)colR * bright / 100;
    uint8_t g = (uint16_t)colG * bright / 100;
    uint8_t b = (uint16_t)colB * bright / 100;

    if (boostOn && ((now / BOOST_BLINK_MS) & 1)) {
        r = g = b = 0;
    }

    fillAll(r, g, b);
}

void setup()
{
    fcSerial.begin(UART_BAUD);
    for (uint8_t s = 0; s < NUM_STRIPS; s++) {
        strips[s].begin();
        strips[s].clear();
        strips[s].show();
    }
}

void loop()
{
    while (fcSerial.available()) {
        feedByte((char)fcSerial.read());
    }

    const uint32_t now = millis();

    if (lineLen > 0) {
        if ((now - lastByteMs) > LINE_STALE_MS) {
            lineLen = 0;
        } else {
            return;
        }
    }

    if ((now - lastShowMs) < SHOW_MIN_MS) {
        return;
    }
    lastShowMs = now;
    render(now);
}
