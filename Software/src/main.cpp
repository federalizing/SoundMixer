/* SoundMixer
 * A simple sketch for controlling a deej-powered SoundMixer with LED rings
 * used to visualize volume levels of different sound sources.
 * The volume levels can be set via 5pin rotary encoders.
 * Also includes support for an i2c oled display with 128x64 resolution,
 * as well as two buttons for controlling different stuff.
 * The volume levels are stored in eeprom, so they persist across reboots.
 */

#include <Arduino.h>
#include <FastLED.h>
#include "main.h"
#include "defines.h"
#include <EEPROM.h>
#include "bitmaps.h"
#include "ssd1306.h"

void updateLastActivityTime(bool alsoUpdateEEPROM)
{
  lastActivityTime = millis();
  if (alsoUpdateEEPROM)
  {
    // If the sound mixer is active, we set the updateEEPROM flag to true
    // This will trigger an update of the eeprom after the sound mixer has become idle
    updateEEPROM = true;
  }
}

void initEEPROM()
{
  // the eeprom is used to store the volume levels of the mixers, as well as the mute states
  // this function checks if the eeprom is empty and initializes it with default values
  // The layout in the eeprom is as follows:
  // 0: amount of mixers (should be NUM_MIXERS), 0xFF if not set
  // 1: version of the eeprom data structure (should be EEPROM_VERSION), 0xFF if not set
  // 1, 2: volume level and mute state for mixer 0
  // 3, 4: volume level and mute state for mixer 1
  // and so on...
  // The volume level is stored as a byte, from 0 to 100, and the mute state is stored as a boolean (0 or 1).
  // If the eeprom is empty, it initializes it with default values.
  if (EEPROM.read(0) != NUM_MIXERS || EEPROM.read(1) != EEPROM_VERSION)
  {
    // Initialize the eeprom with default values
    EEPROM.put(0, NUM_MIXERS);
    EEPROM.put(1, EEPROM_VERSION);
    for (uint8_t i = 0; i < NUM_MIXERS; i++)
    {
      EEPROM.put(2 + i * 2, 100); // Default volume level to 100%
      EEPROM.put(3 + i * 2, 0);   // Default mute state to false (0)
    }
  }
}

void fetchEEPROMData()
{
  // Fetch the volume levels and mute states from the eeprom
  for (uint8_t i = 0; i < NUM_MIXERS; i++)
  {
    EEPROM.get(2 + i * 2, volumeLevels[i]); // Read volume level from eeprom
    EEPROM.get(3 + i * 2, isMuted[i]);      // Read mute state from eeprom
  }
}

void updateEEPROMData()
{
  // update volume levels and mute states in the eeprom with eeprom.put()
  // eeprom.put uses eeprom.update internally, so it only writes to the eeprom if the value has changed
  for (uint8_t i = 0; i < NUM_MIXERS; i++)
  {
    EEPROM.put(2 + i * 2, volumeLevels[i]); // Update volume level in eeprom
    EEPROM.put(3 + i * 2, isMuted[i]);      // Update mute state in eeprom
  }
  updateEEPROM = false; // Reset the update flag
}

void initMixers()
{
  // Initialize the LED strip
  FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_CURRENT);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_MIXERS * LEDS_PER_MIXER);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(GLOBAL_BRIGHTNESS);
  FastLED.clear();

  // Initialize the volume levels and mute states
  fetchEEPROMData(); // Fetch the data from eeprom

  // Set the initial LED colors for each mixer
  setMixerLEDS();
  FastLED.show(); // Show the initial state of the LEDs
}

void setMixerLEDS(uint8_t mixerIndex)
{
  // Set the brightness of the specified mixer or all mixers
  uint8_t startIndex = (mixerIndex == ALL_MIXERS) ? 0 : mixerIndex;
  uint8_t endIndex = (mixerIndex == ALL_MIXERS) ? NUM_MIXERS : mixerIndex + 1;
  for (uint8_t i = startIndex; i < endIndex; i++)
  {
    uint8_t offset = i * LEDS_PER_MIXER;
    CHSV color = isMuted[i] ? muteColor : mixerColors[i];
    color.v = map(currentBrightnessLevel, 0, 100, MIN_BRIGHTNESS, color.v); // Map brightness from 0-100% to MIN_BRIGHTNESS-color.v
    for (uint8_t j = 0; j < litUpLEDs(i); j++)
    {
      leds[offset + j] = color;
    }
    for (uint8_t j = litUpLEDs(i); j < LEDS_PER_MIXER; j++)
    {
      leds[offset + j] = CHSV(0, 0, 0); // Set the remaining LEDs to black
    }
  }
}

void fadeLEDS()
{
  if (isIdle() && currentBrightnessLevel > 0)
  {
    EVERY_N_MILLISECONDS(FADE_BLACK_TIME / FADE_BLACK_STEPS)
    {
      uint8_t stepAmount = (100 / FADE_BLACK_STEPS);
      currentBrightnessLevel = currentBrightnessLevel > stepAmount ? currentBrightnessLevel - stepAmount : 0;
      setMixerLEDS(ALL_MIXERS);
      FastLED.show();
    }
  }
  else if (!isIdle() && currentBrightnessLevel < 100)
  {
    EVERY_N_MILLISECONDS(FADE_LIGHT_TIME / FADE_LIGHT_STEPS)
    {
      uint8_t stepAmount = (100 / FADE_LIGHT_STEPS);
      currentBrightnessLevel = (100 - currentBrightnessLevel) > stepAmount ? currentBrightnessLevel + stepAmount : 100;
      setMixerLEDS(ALL_MIXERS);
      FastLED.show();
    }
  }
}

void checkIdle()
{
  if (!lastIdleStatus && isIdle()) // just changed to idle
  {
    if (updateEEPROM)
      updateEEPROMData(); // If the sound mixer is idle and the eeprom should be updated, we update the eeprom

    // Restart the idle animation on the OLED display
    currentAnimationFrame = 0; // Reset the animation frame to the first frame
    currentMixerIndex = 255;   // Reset the current mixer index to an invalid value
    lastMixerIndex = 255;      // Reset the last mixer index to an invalid value
    ssd1306_clearScreen();     // Clear the OLED display
    // show the mixer icons below the animation
    for (uint8_t i = 0; i < NUM_MIXERS; i++)
    {
      uint8_t xPosition = i * (24 + 2); // size of the icon + 3px padding
      const unsigned char *icon = smallIcons[i];
      ssd1306_drawBitmap(xPosition, 5, 24, 24, icon); // Draw the mixer icon at the bottom of the display
    }
  }
  else if (lastIdleStatus && !isIdle()) // just changed to active
  {
    // nothing yet here
  }

  fadeLEDS(); // Check if the sound mixer is idle and fade the LEDs accordingly

  if (isIdle())
  {
    // show idle animation on the OLED display
    showIdleAnimation();
  }

  lastIdleStatus = isIdle(); // Update the last idle status
}

void initEncoders()
{
  for (uint8_t i = 0; i < NUM_MIXERS; i++)
  {
    pinMode(encoderPins[i][0], INPUT);
    pinMode(encoderPins[i][1], INPUT);
    pinMode(encoderPins[i][2], INPUT);
    encAStates[i] = digitalRead(encoderPins[i][0]); // Read the initial state of the encoder A pin
    lastEncAStates[i] = encAStates[i];              // Store the initial state of the encoder A pin
    lastEncoderSwitchStates[i] = true;              // Initialize the encoder switch states to true
  }
}

void initButtons()
{
  pinMode(BUTTON_PIN_1, INPUT);
  pinMode(BUTTON_PIN_2, INPUT);
  for (bool &state : lastButtonStates)
  {
    state = false; // Initialize button states to false (not pressed)
  }
}

void checkEncoders()
{
  // Check the encoders for changes and update the volume levels and mute states accordingly
  for (uint8_t i = 0; i < NUM_MIXERS; i++)
  {
    encAStates[i] = digitalRead(encoderPins[i][0]); // Read the current state of the encoder A pin
    if (encAStates[i] != lastEncAStates[i])         // If the state has changed
    {
      if (digitalRead(encoderPins[i][1]) != encAStates[i]) // If the encoder A pin is high and the B pin is low, we are rotating clockwise
      {
        if (volumeLevels[i] < 100) // Increase volume level if not at maximum
        {
          // Increase the volume level by a defined step, ensuring it does not exceed 100
          volumeLevels[i] = volumeLevels[i] > 100 - VOLUME_STEP ? 100 : volumeLevels[i] + VOLUME_STEP;
        }
      }
      else // If the encoder A pin is low and the B pin is high, we are rotating counter-clockwise
      {
        if (volumeLevels[i] > 0) // Decrease volume level if not at minimum
        {
          // Decrease the volume level by a defined step, ensuring it does not go below 0
          volumeLevels[i] = volumeLevels[i] < VOLUME_STEP ? 0 : volumeLevels[i] - VOLUME_STEP;
        }
      }
      lastEncAStates[i] = encAStates[i];  // Update the last state of the encoder A pin
      lastMixerIndex = currentMixerIndex; // Store the last mixer index before changing it
      currentMixerIndex = i;              // Set the current mixer index to the one being adjusted
      updateLastActivityTime(true);       // Update the last activity time and set updateEEPROM to true
      setMixerLEDS(i);                    // Update the LEDs for this mixer
      FastLED.show();                     // Show the updated state of the LEDs
      showCurrentMixerVolume();           // Show the current mixer volume on the OLED display
    }
    // Check the encoder switch state
    bool currentSwitchState = digitalRead(encoderPins[i][2]);
    if (currentSwitchState != lastEncoderSwitchStates[i]) // If the switch state has changed
    {
      lastEncoderSwitchStates[i] = currentSwitchState; // Update the switch state
      lastMixerIndex = currentMixerIndex; // Store the last mixer index before changing it
      currentMixerIndex = i;                           // Set the current mixer index to the one being adjusted
      if (!currentSwitchState)                         // If the switch is pressed (active low)
      {
        isMuted[i] = !isMuted[i];
        updateLastActivityTime(true);
        setMixerLEDS(i);
        FastLED.show();
        showCurrentMixerVolume();           // Show the current mixer volume on the OLED display
      }
    }
  }
}

void checkButtons()
{
  bool button1Pressed = isButtonPressed(BUTTON_PIN_1);
  bool button2Pressed = isButtonPressed(BUTTON_PIN_2);
  if (button1Pressed) // If button 1 is pressed
  {
    Serial.println("Button 1 pressed");
    updateLastActivityTime(false);
  }
  if (button2Pressed) // If button 2 is pressed
  {
    Serial.println("Button 2 pressed");
    updateLastActivityTime(false);
  }
}

void showIdleAnimation()
{
  EVERY_N_MILLISECONDS(IDLE_ANIMATION_FRAME_TIME)
  {
    const unsigned char *frame = animationFrames_128x40[animationFrames[currentAnimationFrame]];
    ssd1306_drawBitmap(0, 0, 128, 40, frame);                                        // Draw the current animation frame on the OLED display
    currentAnimationFrame = (currentAnimationFrame + 1) % NUM_IDLE_ANIMATION_FRAMES; // Cycle through the animation frames
  }
}

void showCurrentMixerVolume()
{
  // show the current mixer icon at the
  // show the other mixer icons at the left and right side of the display
  // show the current volume levels of the mixer at the bottom of the display

  uint8_t centerIcon = currentMixerIndex == 255 ? 0 : currentMixerIndex; // If no mixer is selected, show the first mixer icon

  // if no mixer icon was recently drawn, we need to clear the display
  if (lastMixerIndex == 255)
    ssd1306_clearScreen();

  // if no mixer icon is currently drawn or the mixer index changed, we need to redraw the icons
  if (currentMixerIndex != lastMixerIndex)
  {
    uint8_t otherIcons[4] = {0, 1, 2, 3}; // Array to hold the other mixer icons
    for (uint8_t i = 0; i < 4; i++)
    {
      if (otherIcons[i] >= centerIcon) // If the current icon is greater than or equal to the center icon, increment it
      {
        otherIcons[i]++;
      }
    }
    ssd1306_drawBitmap(0, 0, 24, 24, smallIcons[otherIcons[0]]);
    ssd1306_drawBitmap(0, 5, 24, 24, smallIcons[otherIcons[1]]);
    ssd1306_drawBitmap(103, 0, 24, 24, smallIcons[otherIcons[2]]);
    ssd1306_drawBitmap(103, 5, 24, 24, smallIcons[otherIcons[3]]);
    // Show the current mixer icon in the center of the display
    ssd1306_drawBitmap(39, 0, 48, 48, largeIcons[centerIcon]);
  }
  // update the shown volume
  uint8_t volume = volumeLevels[centerIcon]; // Get the current volume level of the selected mixer
  uint8_t volumexPos = 63 - getNumberOfDigits(volume) * 6;
  char volumeStr[4];
  itoa(volume, volumeStr, 10);                                        // Convert the volume level to a string
  if (getNumberOfDigits(lastVolumeLevel) > getNumberOfDigits(volume)) // If the last volume level was a character larger than the current, clear the area around the volume level
  {
    ssd1306_clearBlock(45, 6, 36, 16);
  }
  ssd1306_printFixedN(volumexPos, 55, volumeStr, EFontStyle::STYLE_NORMAL, 1);
  lastVolumeLevel = volume; // Update the last volume level to the current one
}

uint8_t getNumberOfDigits(uint8_t number)
{
  // Calculate the number of digits in a number
  uint8_t digits = 0;
  do
  {
    digits++;
    number /= 10; // Divide the number by 10 to remove the last digit
  } while (number > 0);
  return digits;
}

void sendVolumeLevelsToSerial()
{
  // Create a string with the volume levels of all mixers
  // It consists of the volume levels (from 0 to 1023), separated by a pipe character 

  EVERY_N_MILLISECONDS(DEEJ_UPDATE_INTERVAL)
  {
    String volumeString = "";
    for (uint8_t i = 0; i < NUM_MIXERS; i++)
    {
      int volume = isMuted[i] ? 0 : volumeLevels[i]; // If the mixer is muted, set the volume to 0
      volume = map(volume, 0, 100, 0, 1023); // Map the volume level from 0-100% to 0-1023
      volumeString += String(volume); 
      if (i < NUM_MIXERS - 1)
      {
        volumeString += "|"; // Add a pipe character between the volume levels
      }
    }
    Serial.println(volumeString); // Send the volume levels to the serial port
  }
}

void setup()
{
  updateLastActivityTime(false); // Initialize the last activity time

  pinMode(LED_PIN, OUTPUT); // Set the LED pin as output

  initButtons();  // Initialize the buttons
  initEncoders(); // Initialize the encoders
  initEEPROM();   // Initialize the eeprom
  initMixers();   // Initialize the mixers and LEDs

  sh1106_128x64_i2c_init(); // Initialize the OLED display
  ssd1306_setFixedFont(ssd1306xled_font6x8);
  ssd1306_clearScreen(); // Clear the OLED display

  Serial.begin(9600); // Initialize serial communication for debugging
}

void loop()
{
  checkEncoders(); // Check the encoders for changes and update the volume levels and mute states accordingly
  checkButtons();  // Check the buttons for changes and act accordingly
  checkIdle();     // Check if the sound mixer is idle and take appropriate actions
  sendVolumeLevelsToSerial(); // Send the current volume levels of all mixers to the serial port for deej to read
}
