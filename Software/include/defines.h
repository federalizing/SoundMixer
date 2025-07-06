#ifndef DEFINES_H
#define DEFINES_H


// Soundmixer settings
#define NUM_MIXERS 5
#define LEDS_PER_MIXER 25

// button settings
#define NUM_BUTTONS 2
#define BUTTON_PIN_1 20 // analog pin A6, cannot be used as digital pin
#define BUTTON_PIN_2 21 // analog pin A7, cannot be used as digital pin

// FastLED settings
#define VOLTS 5
#define MAX_CURRENT 500 // in milliamps
#define LED_PIN 2
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define GLOBAL_BRIGHTNESS 90
#define MIN_BRIGHTNESS 60    // from 0 to 255, value of HSV
#define FADE_BLACK_TIME 2000 // in milliseconds
#define FADE_LIGHT_TIME 500  // in milliseconds
#define FADE_BLACK_STEPS 20  // steps to fade to black
#define FADE_LIGHT_STEPS 5   // steps to fade in or out
#define IDLE_TIMEOUT 5000    // in milliseconds

// encoder settings
#define VOLUME_STEP 3 // step to increase or decrease the volume level per encoder step

// oled settings
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define IDLE_ANIMATION_FRAME_TIME 240 // in milliseconds
#define NUM_IDLE_ANIMATION_FRAMES 4 // number of frames in the idle animation

// deej settings
#define DEEJ_UPDATE_INTERVAL 100 // in milliseconds

// Version of the EEPROM data structure
// This version is used to check if the EEPROM data structure has changed
// If the version is changed, the EEPROM data will be reset
#define EEPROM_VERSION 1

// Mixer indices
#define ALL_MIXERS 255



#endif // DEFINES_H