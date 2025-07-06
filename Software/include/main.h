#ifndef main_h
#define main_h

#include "defines.h"
#include <FastLED.h>

// default colors for the mixers
const CHSV mixerColors[NUM_MIXERS] = {
    CHSV(92, 51, 217),   // Master
    CHSV(166, 163, 242), // Discord
    CHSV(100, 214, 184), // Spotify
    CHSV(31, 186, 255),  // Chrome
    CHSV(92, 51, 217)    // Games
};

// color for muted mixers
const CHSV muteColor = CHSV(0, 255, 255); // Color for muted mixers (red)

// leds array
CRGB leds[NUM_MIXERS * LEDS_PER_MIXER];

// current brightness level for all mixers. Used to fade the LEDs in and out
// 0% to 100%, from MIN_BRIGHTNESS to maximum brightness defined by value of HSV color
uint8_t currentBrightnessLevel = 100;

// Volume levels for each mixer, from 0 to 100%
uint8_t volumeLevels[NUM_MIXERS];

// Mute states for each mixer, true if muted, false if not muted
bool isMuted[NUM_MIXERS];

// timer to track the last activity time. Used to determine if the sound mixer is idle
unsigned long lastActivityTime;

// last idle status of the sound mixer
// This is used to determine if the sound mixer just changed idle state
bool lastIdleStatus = false;

// Flag to indicate if the EEPROM should be updated
// This is set to true when the volume levels or mute states are changed
// This is set to false after the EEPROM is updated after the sound mixer has become idle
bool updateEEPROM = false;

// Encoder A states for each mixer
uint8_t encAStates[NUM_MIXERS];
// Last Encoder A states for each mixer
uint8_t lastEncAStates[NUM_MIXERS];
// Encoder switches for each mixer, false if pressed, true if not pressed
bool lastEncoderSwitchStates[NUM_MIXERS];

// last button states
bool lastButtonStates[NUM_BUTTONS]; 

// encoder pins for each mixer
// Each mixer has 3 pins: A, B, and switch
const uint8_t encoderPins[NUM_MIXERS][3] = {
    {13, 14, 12}, // Master
    {16, 17, 15}, // Discord
    {4, 5, 3},    // Spotify
    {7, 8, 6},    // Chrome
    {10, 11, 9}   // Games
};

// current animation frame for the idle animation
uint8_t currentAnimationFrame = 0;

// currently changed mixer index and last changed mixer index
uint8_t currentMixerIndex = 0; 
uint8_t lastMixerIndex = 255;
uint8_t lastVolumeLevel = 100; // if the last volume level was a character larger than the current, clear the area around the volume level

// order of the animation frames, as indexed in the bitmaps.h file
const uint8_t animationFrames[NUM_IDLE_ANIMATION_FRAMES] = {0, 1, 2, 1};

uint8_t getNumberOfDigits(uint8_t number);

// update the last activity time to the current time
// if updateEEPROM is true, it will also set the updateEEPROM flag to true
void updateLastActivityTime(bool updateEEPROM = true);

// check if the sound mixer is idle
bool isIdle() { return (millis() - lastActivityTime) > IDLE_TIMEOUT; }

// calculate the number of LEDs that should be lit up for a given mixer index
// maps the volume level from 0 to 100% to the number of LEDs per mixer
uint8_t litUpLEDs(uint8_t mixerIndex) { return map(volumeLevels[mixerIndex], 0, 100, 0, LEDS_PER_MIXER); }

// sets the settings for the FastLED library and fetches the data from the EEPROM
// initializes the LEDs with the colors and brightness
// also sets the initial volume levels and mute states for each mixer
// this function should be called once in the setup() function
void initMixers();

// sets the brightness of the LEDs for a given mixer index or all mixers
// uses the currentBrightnessLevel to map the brightness from 0-100% to MIN_BRIGHTNESS-color.v
// sets all LEDs that should not be lit up to black
void setMixerLEDS(uint8_t mixerIndex = ALL_MIXERS);

// initializes the EEPROM with default values if it is empty or the version has changed
void initEEPROM();
// fetches the volume levels and mute states from the EEPROM
// this function is called once in initMixers()
void fetchEEPROMData();

// this function should be called once after the soundmixer has become idle
// it updates the eeprom with the current volume levels and mute states
void updateEEPROMData();

// check if soundmixer is idle and fade the LEDS to black or light
// or fade them back to the current brightness level
// this function is called in the checkIdle() function
void fadeLEDS();

// checks if the sound mixer is idle and takes appropriate actions
void checkIdle();

// initializes the encoders by setting the encoder pins as input
// and setting the initial states for the encoders
void initEncoders();

// checks the encoders for changes and updates the volume levels and mute states accordingly
// this function should be called in the loop() function
void checkEncoders();

// checks the buttons for changes and acts accordingly
// this function should be called in the loop() function
void checkButtons();

// checks if a button is pressed by reading the analog value of the pin
inline bool isButtonPressed(uint8_t pin) { return analogRead(pin) < 512; };

// initializes the buttons
void initButtons();

// shows an idle animation on the oled display
// this function should be called periodically in the loop() function
void showIdleAnimation();

// shows the current mixer volume on the oled display
// this function should be called when the volume level of a mixer changes or the mixer is muted
void showCurrentMixerVolume();

// sends the current volume levels of all mixers to the serial port so deej can read them
// only sends the volume levels in the defined interval
void sendVolumeLevelsToSerial();

void setup();
void loop();

#endif // main_h