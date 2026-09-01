/*
 * Solid color on all four WS2812 strips.
 * Board : Arduino Nano
 * Libs  : Adafruit NeoPixel
 *
 *   Strip DIN -> D9, D10, D5, D6
 *   Common GND, 5 V BEC for the strips (not the Nano 5V pin).
 */

#include <Adafruit_NeoPixel.h>

#define NUM_LEDS    28
#define LED_TYPE    (NEO_GRB + NEO_KHZ800)

#define COLOR_R     255
#define COLOR_G     255
#define COLOR_B     255

Adafruit_NeoPixel strips[] = {
    Adafruit_NeoPixel(NUM_LEDS, 9,  LED_TYPE),
    Adafruit_NeoPixel(NUM_LEDS, 10, LED_TYPE),
    Adafruit_NeoPixel(NUM_LEDS, 5,  LED_TYPE),
    Adafruit_NeoPixel(NUM_LEDS, 6,  LED_TYPE),
};
#define NUM_STRIPS  (sizeof(strips) / sizeof(strips[0]))

void setup()
{
    const uint32_t c = Adafruit_NeoPixel::Color(COLOR_R, COLOR_G, COLOR_B);

    for (uint8_t s = 0; s < NUM_STRIPS; s++) {
        strips[s].begin();
        strips[s].fill(c);
        strips[s].show();
    }
}

void loop()
{
}
